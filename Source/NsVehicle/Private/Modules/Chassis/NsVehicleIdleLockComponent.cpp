// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleIdleLockComponent.h"
#include "AsyncTickFunctions.h"
#include "Components/PrimitiveComponent.h"
#include "Modules/Chassis/NsVehicleSelfRightComponent.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Modules/Network/NsVehicleNetworkComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleIdleLockComponent::UNsVehicleIdleLockComponent()
{
    bUseParkedHold = false;
    ParkedSettleTime = 1.f;
    ParkedMaxSpeed = 20.f;
    bParkedHeld = false;
    bTreatAsParked = false;
    ParkedSettleCount = 0.f;
}

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

void UNsVehicleIdleLockComponent::SetParkedHoldEnabled(const bool bEnabled)
{
    bUseParkedHold = bEnabled;
    if (!bEnabled)
    {
        ReleaseParkedHold();
    }
}

bool UNsVehicleIdleLockComponent::IsParkedHeld() const
{
    return bParkedHeld;
}

bool UNsVehicleIdleLockComponent::IsDriven() const
{
    const ANsVehicle* const Vehicle = GetVehicle();
    return Vehicle != nullptr && Vehicle->IsPlayerControlled() && !bTreatAsParked;
}

void UNsVehicleIdleLockComponent::SetTreatAsParked(const bool bParked)
{
    bTreatAsParked = bParked;
    RefreshParkedHold();
}

void UNsVehicleIdleLockComponent::ReleaseParkedHold()
{
    ParkedSettleCount = 0.f;
    if (!bParkedHeld)
    {
        return;
    }

    bParkedHeld = false;
    UPrimitiveComponent* const Body = GetVehicle() != nullptr ? Cast<UPrimitiveComponent>(GetVehicle()->GetRootComponent()) : nullptr;
    if (Body == nullptr)
    {
        return;
    }

    Body->SetSimulatePhysics(true);
    Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

void UNsVehicleIdleLockComponent::RefreshParkedHold()
{
    if (IsDriven())
    {
        ReleaseParkedHold();
        return;
    }

    ParkedSettleCount = 0.f;
}

void UNsVehicleIdleLockComponent::UpdateParkedHold(const float DeltaTime)
{
    if (bParkedHeld)
    {
        // A held vehicle waits for a driver. A turned off hold or a driver gives the physics back
        if (!bUseParkedHold || IsDriven())
        {
            ReleaseParkedHold();
        }
        return;
    }
    if (!bUseParkedHold)
    {
        return;
    }

    ANsVehicle* const Vehicle = GetVehicle();
    UPrimitiveComponent* const Body = Vehicle != nullptr ? Cast<UPrimitiveComponent>(Vehicle->GetRootComponent()) : nullptr;

    // Another system can stop the physics for a while, for example a scripted turn. The settle time starts again when the physics comes back
    if (Body == nullptr || IsDriven() || !Body->IsSimulatingPhysics())
    {
        ParkedSettleCount = 0.f;
        return;
    }

    ParkedSettleCount += DeltaTime;
    if (ParkedSettleCount < FMath::Max(ParkedSettleTime, 0.01f))
    {
        return;
    }
    ParkedSettleCount = 0.f;

    // A vehicle in the air or still rolling lands and slows first, because a held vehicle in the air would float. A vehicle that lost a wheel rests on its body
    const bool bLanded = !Vehicle->IsInAir() || Vehicle->GetSelfRight()->IsBodyOnGround();
    if (!bLanded || Body->GetPhysicsLinearVelocity().Size() > ParkedMaxSpeed)
    {
        return;
    }

    // The suspensions push a parked vehicle each step and wake it, so only a stop of the physics holds it
    Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Body->SetSimulatePhysics(false);
    bParkedHeld = true;
}
