// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Powertrain/NsVehicleEngineComponent.h"
#include "Curves/CurveFloat.h"
#include "Modules/Powertrain/NsVehicleDrivetrainComponent.h"

void UNsVehicleEngineComponent::BeginPlay()
{
    Super::BeginPlay();

    EngineTorqueDuplicate = EngineTorque;
}

void UNsVehicleEngineComponent::SetEngineTorque(const float NewTorque)
{
    EngineTorque = NewTorque;
}

void UNsVehicleEngineComponent::SetEngineTorqueEnabled(const bool bEnabled, const float TempTorque)
{
    EngineTorque = bEnabled ? EngineTorqueDuplicate : TempTorque;
}

void UNsVehicleEngineComponent::SetPowertrainType(const ENsVehicleBehaviorType NewPowertrainType)
{
    PowertrainType = NewPowertrainType;
}

void UNsVehicleEngineComponent::SetArcadeMaxSpeed(const float NewMaxSpeed)
{
    MaxSpeed = NewMaxSpeed;
}

void UNsVehicleEngineComponent::SetMaxRPM(const float NewMaxRPM)
{
    MaxRPM = FMath::Max(0.f, NewMaxRPM);
}

void UNsVehicleEngineComponent::SetRPMMultiplier(const float NewRPMMultiplier)
{
    RPM_Multiplier = NewRPMMultiplier;
}

void UNsVehicleEngineComponent::SetRpmLimitTorqueFloor(const float NewTorqueFloor)
{
    RpmLimitTorqueFloor = FMath::Clamp(NewTorqueFloor, 0.f, 1.f);
}

void UNsVehicleEngineComponent::SetEngineBrakeCoefficient(const float NewEngineBrakeCoefficient)
{
    EngineBrakeCoefficient = FMath::Clamp(NewEngineBrakeCoefficient, 0.f, 1.f);
}

void UNsVehicleEngineComponent::OverrideRPM(const float NewRPM)
{
    RPM = NewRPM;
}

ENsVehicleBehaviorType UNsVehicleEngineComponent::GetPowertrainType() const
{
    return PowertrainType;
}

float UNsVehicleEngineComponent::GetEngineTorque() const
{
    return EngineTorque;
}

float UNsVehicleEngineComponent::GetEngineCurrentTorque() const
{
    return CurrentTorque;
}

float UNsVehicleEngineComponent::GetArcadeMaxSpeed() const
{
    return MaxSpeed;
}

float UNsVehicleEngineComponent::GetRPM() const
{
    return RPM;
}

float UNsVehicleEngineComponent::GetMaxRPM() const
{
    return MaxRPM;
}

bool UNsVehicleEngineComponent::UpdateStandardTorque()
{
    switch (TorqueImplementation)
    {
    case ENsVehicleTorqueImplementation::ETI_Constant:
        CurrentTorque = EngineTorque;
        break;

    case ENsVehicleTorqueImplementation::ETI_Curve:
        if (EngineTorqueCurve == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("NsVehicle engine '%s' uses a torque curve but has no curve asset"), *GetPathNameSafe(this));
            return false;
        }

        CurrentTorque = EngineTorque * EngineTorqueCurve->GetFloatValue(FMath::Abs(RPM));
        break;
    }

    return true;
}

float UNsVehicleEngineComponent::UpdateArcadeTorque(const float SpeedKPH)
{
    float ClampedTorque = 0.f;
    if (MaxSpeed > UE_SMALL_NUMBER)
    {
        const float SpeedFadeRange = FMath::Clamp(MaxSpeed * 0.35f, 1.f, 20.f);
        const float SpeedFadeStart = FMath::Max(0.f, MaxSpeed - SpeedFadeRange);
        ClampedTorque = FMath::GetMappedRangeValueClamped(FVector2D(SpeedFadeStart, MaxSpeed), FVector2D(EngineTorque, 0.f), FMath::Abs(SpeedKPH));
    }

    CurrentTorque = ClampedTorque;
    return ClampedTorque;
}

float UNsVehicleEngineComponent::GetRpmLimitTorqueScale(const float UpshiftRatio) const
{
    if (MaxRPM <= UE_SMALL_NUMBER)
    {
        return 1.f;
    }

    const float RpmLimitStart = MaxRPM * FMath::Clamp(UpshiftRatio, 0.1f, 0.98f);
    return FMath::GetMappedRangeValueClamped(FVector2D(RpmLimitStart, MaxRPM), FVector2D(1.f, RpmLimitTorqueFloor), FMath::Abs(RPM));
}

float UNsVehicleEngineComponent::GetEngineBrakeForce(const float Throttle, const float GearRatio, const float VelocityDirection) const
{
    return FMath::Abs(RPM) * EngineBrakeCoefficient * (1 - Throttle) * GearRatio * -VelocityDirection;
}

void UNsVehicleEngineComponent::UpdateStandardRPM(const float DeltaTime, const float GearRatio, const float DifferentialRatio, const float WheelRadius, const float LinearVelocity, const float Throttle, const float Brakes, const bool bInAir, const UNsVehicleDrivetrainComponent& Drivetrain)
{
    // The order of operations matches the original tuning, so 60 / 2 * PI stays as written
    const float WheelRPM = GearRatio * DifferentialRatio * LinearVelocity * WheelRadius * (60.f / 2.f * PI) * RPM_Multiplier;
    constexpr float InterpSpeed = 15.f;

    if (Drivetrain.IsDrivetrainEnabled() && bInAir)
    {
        const float AirGamma = FMath::Lerp(Drivetrain.GetInAirSlowdownSpeed(), Drivetrain.GetInAirSpeedUp(), Throttle);
        RPM = FMath::Clamp(FMath::FInterpTo(RPM, (Throttle - Brakes) * MaxRPM, DeltaTime, AirGamma), 0.f, MaxRPM);
        return;
    }

    RPM = FMath::Clamp(FMath::FInterpTo(RPM, WheelRPM, DeltaTime, InterpSpeed), -MaxRPM, MaxRPM);
}
