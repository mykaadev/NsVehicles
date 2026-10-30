// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Powertrain/NsVehicleDrivetrainComponent.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

void UNsVehicleDrivetrainComponent::SetDrivetrainEnabled(const bool bEnabled)
{
    bUseDrivetrain = bEnabled;
}

void UNsVehicleDrivetrainComponent::SetDrivetrainSlipSettings(const float NewSlipPowerReduction, const float NewSlipInitiateSpeed, const float NewSlipRestoreSpeed)
{
    SlipPowerReduction = FMath::Max(UE_SMALL_NUMBER, NewSlipPowerReduction);
    SlipInitiateSpeed = FMath::Max(0.f, NewSlipInitiateSpeed);
    SlipRestoreSpeed = FMath::Max(0.f, NewSlipRestoreSpeed);
}

bool UNsVehicleDrivetrainComponent::IsDrivetrainEnabled() const
{
    return bUseDrivetrain;
}

float UNsVehicleDrivetrainComponent::GetAverageDrivetrainSlip() const
{
    return AveragedSlip;
}

float UNsVehicleDrivetrainComponent::GetAveragedSlipGamma() const
{
    return AveragedSlipGamma;
}

bool UNsVehicleDrivetrainComponent::IsSlipping() const
{
    return AveragedSlip > 1.f;
}

float UNsVehicleDrivetrainComponent::GetDrivetrainWheelCount() const
{
    return DrivetrainWheelCount;
}

float UNsVehicleDrivetrainComponent::GetEffectivePower() const
{
    return EffectivePower;
}

float UNsVehicleDrivetrainComponent::GetSlipInitiateSpeed() const
{
    return SlipInitiateSpeed;
}

float UNsVehicleDrivetrainComponent::GetSlipRestoreSpeed() const
{
    return SlipRestoreSpeed;
}

float UNsVehicleDrivetrainComponent::GetInAirSpeedUp() const
{
    return InAirSpeedUp;
}

float UNsVehicleDrivetrainComponent::GetInAirSlowdownSpeed() const
{
    return InAirSlowdownSpeed;
}

int32 UNsVehicleDrivetrainComponent::UpdateWheelSlip(const TArray<UNsVehicleSuspensionComponent*>& Suspensions)
{
    DrivetrainWheelCount = 0.f;
    float TotalSlip = 0.f;
    float TotalSlipGamma = 0.f;
    int32 GroundedDrivenWheels = 0;

    for (UNsVehicleSuspensionComponent* Suspension : Suspensions)
    {
        if (Suspension->IsWheelDetached() || !Suspension->GetIsDrivenWheel())
        {
            continue;
        }

        DrivetrainWheelCount += 1.f;
        TotalSlip += Suspension->GetSlipRatio();
        TotalSlipGamma += Suspension->GetSlipGamma();

        if (!Suspension->IsInAir())
        {
            GroundedDrivenWheels++;
        }
    }

    AveragedSlip = TotalSlip / DrivetrainWheelCount;
    AveragedSlipGamma = TotalSlipGamma / DrivetrainWheelCount;

    return GroundedDrivenWheels;
}

void UNsVehicleDrivetrainComponent::UpdateEffectivePower(const float Throttle, const float DeltaTime)
{
    if (!bUseDrivetrain)
    {
        return;
    }

    const float InterpSpeed = FMath::Lerp(SlipRestoreSpeed, SlipInitiateSpeed, Throttle);
    const float TargetPower = AveragedSlip > 1.f ? 1.f / SlipPowerReduction : 1.f;
    EffectivePower = FMath::FInterpTo(EffectivePower, TargetPower, DeltaTime, InterpSpeed);
}
