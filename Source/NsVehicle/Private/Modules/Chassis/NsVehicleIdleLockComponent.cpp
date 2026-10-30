// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleIdleLockComponent.h"
#include "AsyncTickFunctions.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Modules/Network/NsVehicleNetworkComponent.h"
#include "Vehicle/NsVehicle.h"

void UNsVehicleIdleLockComponent::SetIdleLockEnabled(const bool bEnabled)
{
    bUseIdleLock = bEnabled;
}

bool UNsVehicleIdleLockComponent::IsIdleLocked() const
{
    return bIsIdleLocked;
}

void UNsVehicleIdleLockComponent::UpdateIdleLock(const float DeltaTime)
{
    ANsVehicle* const Vehicle = GetVehicle();
    const UNsVehicleControlsComponent* const Controls = Vehicle->GetControls();

    const bool bStopped = FMath::Abs(Vehicle->GetSpeedKPH()) < IdleLockSpeedThreshold
        && Controls->GetThrottleInput() == 0
        && Controls->GetBrakesInput() == 0
        && !Vehicle->IsInAir()
        && FMath::Abs(Vehicle->GetAngularVelocity()) < 0.15f;

    if (!bStopped)
    {
        IdleLockCount = 0.f;
        IdleLockRotationLocationCount = 0.f;
        bIsIdleLocked = false;
        bIsIdleLockedRotationLocation = false;
        return;
    }

    if (!bIsIdleLocked && bUseIdleLock)
    {
        IdleLockCount += DeltaTime;
    }

    if (IdleLockCount <= IdleLockTime || !bUseIdleLock)
    {
        return;
    }

    bIsIdleLocked = true;
    IdleLockCount = 612.f;

    UPrimitiveComponent* const PhysicsBody = Vehicle->GetPrimitiveRoot();
    const FVector Velocity = UAsyncTickFunctions::ATP_GetLinearVelocity(PhysicsBody);
    constexpr float InterpSpeed = 5.f;

    if (Vehicle->GetNetwork()->GetReplicationMethod() == ENsVehicleReplicationType::ERT_None)
    {
        const FVector SlowedVelocity(
            FMath::FInterpTo(Velocity.X, 0.f, DeltaTime, InterpSpeed),
            FMath::FInterpTo(Velocity.Y, 0.f, DeltaTime, InterpSpeed),
            FMath::FInterpTo(Velocity.Z, 0.f, DeltaTime, InterpSpeed));

        UAsyncTickFunctions::ATP_SetLinearVelocity(PhysicsBody, SlowedVelocity, false);
        UAsyncTickFunctions::ATP_SetAngularVelocityInRadians(PhysicsBody, FVector::ZeroVector, false);
    }

    IdleLockRotationLocationCount += DeltaTime;
    if (IdleLockRotationLocationCount >= IdleLockTime / 2.f)
    {
        bIsIdleLockedRotationLocation = true;
        UAsyncTickFunctions::ATP_AddForce(PhysicsBody, -Velocity.GetSafeNormal() * Velocity.Size() * IdleLockStiffness * 100.f, false);
    }
}
