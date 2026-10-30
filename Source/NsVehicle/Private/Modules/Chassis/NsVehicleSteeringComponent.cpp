// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleSteeringComponent.h"
#include "AsyncTickFunctions.h"
#include "Curves/CurveFloat.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

void UNsVehicleSteeringComponent::SetSteeringSensitivity(const float NewSteeringSensitivity)
{
    SteeringSensitivity = FMath::Max(0.f, NewSteeringSensitivity);
}

float UNsVehicleSteeringComponent::ClampSteeringInput(const float RawInput, const float SpeedKPH) const
{
    switch (SteeringInputClamp)
    {
    case ENsVehicleSpeedClamp::Constant:
        return RawInput * ConstantSteeringClamp;

    case ENsVehicleSpeedClamp::Linear:
        return RawInput * FMath::GetMappedRangeValueClamped(LinearSteeringInputClamp.InputRange, LinearSteeringInputClamp.OutputRange, SpeedKPH);

    case ENsVehicleSpeedClamp::Curve:
        return CurveSteeringInputClamp != nullptr ? RawInput * CurveSteeringInputClamp->GetFloatValue(SpeedKPH) : RawInput;

    case ENsVehicleSpeedClamp::NoClamp:
    default:
        return RawInput;
    }
}

float UNsVehicleSteeringComponent::GetSteeringResponse(const float SpeedKPH) const
{
    return bUseSensitivityClamp
        ? FMath::GetMappedRangeValueClamped(SteeringSensitivityClamp.InputRange, SteeringSensitivityClamp.OutputRange, SpeedKPH)
        : SteeringSensitivity;
}

void UNsVehicleSteeringComponent::UpdateWheelSteering(const TArray<UNsVehicleSuspensionComponent*>& Suspensions, const float SteeringInput, const float SpeedKPH) const
{
    const float Response = GetSteeringResponse(SpeedKPH);

    for (UNsVehicleSuspensionComponent* Suspension : Suspensions)
    {
        if (Suspension->IsSteeringWheel())
        {
            Suspension->SetSteeringInput(SteeringInput, Response);
        }
    }
}

void UNsVehicleSteeringComponent::ApplyDriftTorque(UPrimitiveComponent* PhysicsBody, const FVector& Up, const float LinearVelocity, const float SteeringInput) const
{
    const float ScaledTorque = FMath::GetMappedRangeValueClamped(FVector2D(3.f, 6.f), FVector2D(0.f, DriftTorque), FMath::Abs(LinearVelocity));
    const float Direction = FMath::Sign(LinearVelocity) > 0 ? 1.f : -1.f;
    UAsyncTickFunctions::ATP_AddTorque(PhysicsBody, Up * ScaledTorque * SteeringInput * Direction, true);
}
