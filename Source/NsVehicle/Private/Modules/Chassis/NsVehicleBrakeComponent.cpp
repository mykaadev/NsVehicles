// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleBrakeComponent.h"
#include "AsyncTickFunctions.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

void UNsVehicleBrakeComponent::BeginPlay()
{
    Super::BeginPlay();

    BrakeForceDuplicate = BrakeForce;
}

void UNsVehicleBrakeComponent::SetBrakeForce(const float NewBrakeForce)
{
    BrakeForce = FMath::Max(0.f, NewBrakeForce);
    BrakeForceDuplicate = BrakeForce;
}

void UNsVehicleBrakeComponent::SetBrakesEnabled(const bool bEnabled)
{
    BrakeForce = bEnabled ? BrakeForceDuplicate : 0.f;
}

void UNsVehicleBrakeComponent::SetMaxSpeedInReverse(const float NewMaxSpeedInReverse)
{
    MaxSpeedInReverse = FMath::Max(0.f, NewMaxSpeedInReverse);
}

void UNsVehicleBrakeComponent::SetHandbrakeStrength(const float NewHandbrakeStrength)
{
    HandbrakeStrength = FMath::Max(0.f, NewHandbrakeStrength);
}

void UNsVehicleBrakeComponent::SetExternalResistanceForce(const float Force)
{
    ExternalResistanceForce = Force;
}

float UNsVehicleBrakeComponent::GetExternalResistanceForce() const
{
    return ExternalResistanceForce;
}

bool UNsVehicleBrakeComponent::GetIsHandbrakeActive() const
{
    return bHandbrakeActive;
}

float UNsVehicleBrakeComponent::GetHandbrakeStrength() const
{
    return HandbrakeForce;
}

float UNsVehicleBrakeComponent::GetStandardBrakeForce(const float RPM, const float MaxRPM, const float SpeedKPH, const float VelocityDirection, float& OutBrakeDirection) const
{
    OutBrakeDirection = bUseBrakeAsReverse ? -1.f : -VelocityDirection;

    if (!bUseBrakeAsReverse)
    {
        return FMath::GetMappedRangeValueClamped(FVector2D(0.f, 5.f), FVector2D(0.f, BrakeForce), FMath::Abs(SpeedKPH));
    }

    if (SpeedKPH < -MaxSpeedInReverse)
    {
        return 0.f;
    }

    return FMath::Abs(RPM) < MaxRPM ? BrakeForce : 0.f;
}

float UNsVehicleBrakeComponent::GetArcadeBrakeForce(const float SpeedKPH) const
{
    return SpeedKPH < -MaxSpeedInReverse ? 0.f : BrakeForce;
}

void UNsVehicleBrakeComponent::ApplyHandbrake(UPrimitiveComponent* PhysicsBody, const TArray<UNsVehicleSuspensionComponent*>& Suspensions, const bool bHandbrakeInput, const bool bInAir, const FVector& WorldVelocity, const float LocalForwardVelocity)
{
    int32 FrontHandbrakeCount = 0;
    int32 RearHandbrakeCount = 0;

    // The force applies one time for each wheel after the first handbrake wheel. This keeps the original tuning
    for (UNsVehicleSuspensionComponent* Suspension : Suspensions)
    {
        if (Suspension != nullptr && Suspension->IsHandbrakeWheel())
        {
            const ENsVehicleHandbrakeWheelType HandbrakeType = Suspension->GetWheelHandbrake();
            FrontHandbrakeCount += HandbrakeType == ENsVehicleHandbrakeWheelType::Front ? 1 : 0;
            RearHandbrakeCount += HandbrakeType == ENsVehicleHandbrakeWheelType::Rear ? 1 : 0;
        }

        bHandbrakeActive = (FrontHandbrakeCount > 0 || RearHandbrakeCount > 0) && bHandbrakeInput;

        if (bInAir || !bHandbrakeActive)
        {
            continue;
        }

        const FVector PlanarDirection = FVector(WorldVelocity.X, WorldVelocity.Y, 0.f).GetSafeNormal();
        const float MaxHandbrakeForce = HandbrakeStrength * (bHandbrakeInput ? 1.f : 0.f);
        HandbrakeForce = FMath::GetMappedRangeValueClamped(FVector2D(0.f, 1.f), FVector2D(0.f, MaxHandbrakeForce), FMath::Abs(LocalForwardVelocity));

        UAsyncTickFunctions::ATP_AddForce(PhysicsBody, HandbrakeForce * 100.f * -PlanarDirection, false);
    }
}
