// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Network/NsVehicleNetworkComponent.h"
#include "AsyncTickFunctions.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Modules/Powertrain/NsVehicleEngineComponent.h"
#include "Modules/Powertrain/NsVehicleTransmissionComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleNetworkComponent::UNsVehicleNetworkComponent()
{
    SetIsReplicatedByDefault(true);
}

void UNsVehicleNetworkComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UNsVehicleNetworkComponent, ServerState);
    DOREPLIFETIME(UNsVehicleNetworkComponent, ServerVelocity);
    DOREPLIFETIME(UNsVehicleNetworkComponent, ServerAngularVelocity);
}

void UNsVehicleNetworkComponent::AsyncReplicatedTick()
{
    UPrimitiveComponent* const PhysicsBody = GetVehicle()->GetPrimitiveRoot();
    if (PhysicsBody == nullptr)
    {
        return;
    }

    if (GetOwner()->HasAuthority())
    {
        ServerVelocity = UAsyncTickFunctions::ATP_GetLinearVelocity(PhysicsBody);
        ServerAngularVelocity = UAsyncTickFunctions::ATP_GetAngularVelocity(PhysicsBody);
        return;
    }

    UAsyncTickFunctions::ATP_SetLinearVelocity(PhysicsBody, ServerVelocity, false);
    UAsyncTickFunctions::ATP_SetAngularVelocityInRadians(PhysicsBody, ServerAngularVelocity, false);
}

ENsVehicleReplicationType UNsVehicleNetworkComponent::GetReplicationMethod() const
{
    return ReplicationMethod;
}

void UNsVehicleNetworkComponent::ServerStateSync(const bool bUpdatePhysics)
{
    const ANsVehicle* const Vehicle = GetVehicle();
    UPrimitiveComponent* const PhysicsBody = Vehicle->GetPrimitiveRoot();
    const UNsVehicleControlsComponent* const Controls = Vehicle->GetControls();
    const UNsVehicleTransmissionComponent* const Transmission = Vehicle->GetTransmission();

    if (bUpdatePhysics)
    {
        ServerState.ServerLinearVelocity = UAsyncTickFunctions::ATP_GetLinearVelocity(PhysicsBody);
        ServerState.ServerAngularVelocity = UAsyncTickFunctions::ATP_GetAngularVelocity(PhysicsBody);
        ServerState.ServerTransform = UAsyncTickFunctions::ATP_GetTransform(PhysicsBody);
    }

    ServerState.ServerRPM = Vehicle->GetEngine()->GetRPM();
    ServerState.ServerCurrentGearRatio = Transmission->GetCurrentGearRatio();
    ServerState.ServerCurrentGear = Transmission->GetCurrentGear();
    ServerState.ServerThrottle = Controls->GetThrottleInput();
    ServerState.ServerBrakes = Controls->GetBrakesInput();
    ServerState.ServerSteering = Controls->GetSteeringInput();
    ServerState.ServerSpeed = Vehicle->GetLinearSpeed();
    ServerState.ServerIsUpshifting = Transmission->IsGearboxUpshifting();
    ServerState.ServerIsDownshifting = Transmission->IsGearboxDownshifting();
}

void UNsVehicleNetworkComponent::ClientStateSync(const bool bUpdatePhysics)
{
    ANsVehicle* const Vehicle = GetVehicle();

    if (bUpdatePhysics)
    {
        UPrimitiveComponent* const PhysicsBody = Vehicle->GetPrimitiveRoot();
        UAsyncTickFunctions::ATP_SetLinearVelocity(PhysicsBody, ServerState.ServerLinearVelocity, false);
        UAsyncTickFunctions::ATP_SetAngularVelocityInRadians(PhysicsBody, ServerState.ServerAngularVelocity, false);
    }

    Vehicle->GetEngine()->OverrideRPM(ServerState.ServerRPM);
    Vehicle->GetTransmission()->SetReplicatedGearState(ServerState.ServerCurrentGear, ServerState.ServerCurrentGearRatio, ServerState.ServerIsUpshifting, ServerState.ServerIsDownshifting);
    Vehicle->GetControls()->SetReplicatedInputs(ServerState.ServerThrottle, ServerState.ServerBrakes, ServerState.ServerSteering);
    Vehicle->SetReplicatedSpeed(ServerState.ServerSpeed);
}

void UNsVehicleNetworkComponent::UpdateFastSync()
{
    if (GetOwner()->HasAuthority() || !bUseServerFastSync || ReplicationMethod != ENsVehicleReplicationType::ERT_Full)
    {
        return;
    }

    FastServerSyncCount++;
    if (FastServerSyncCount > ServerFastSyncLatency)
    {
        FastServerSyncCount = 0;
        GetOwner()->SetActorTransform(ServerState.ServerTransform);
    }
}
