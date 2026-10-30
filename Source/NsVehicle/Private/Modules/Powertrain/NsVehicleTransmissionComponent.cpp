// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Powertrain/NsVehicleTransmissionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Vehicle/NsVehicle.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

void UNsVehicleTransmissionComponent::SetShiftLocked(const bool bLocked)
{
    bLockShift = bLocked;
}

void UNsVehicleTransmissionComponent::SetAutomaticTransmission(const bool bEnabled)
{
    bAutomatic = bEnabled;
}

void UNsVehicleTransmissionComponent::SetUpshiftRatio(const float NewUpshiftRatio)
{
    UpshiftRatio = FMath::Clamp(NewUpshiftRatio, 0.f, 1.f);
}

void UNsVehicleTransmissionComponent::SetDownshiftRatio(const float NewDownshiftRatio)
{
    DownshiftRatio = FMath::Clamp(NewDownshiftRatio, 0.f, 1.f);
}

void UNsVehicleTransmissionComponent::SetTransmissionEfficiency(const float NewTransmissionEfficiency)
{
    TransmissionEfficiency = FMath::Clamp(NewTransmissionEfficiency, 0.f, 1.f);
}

void UNsVehicleTransmissionComponent::SetGearRatios(const TArray<float>& NewGearRatios)
{
    GearRatios = NewGearRatios;
    CurrentGear = GearRatios.IsValidIndex(CurrentGear) ? CurrentGear : 0;
}

void UNsVehicleTransmissionComponent::SetCurrentGearRatio(const float NewGearRatio)
{
    ActiveGearRatio = NewGearRatio;
}

void UNsVehicleTransmissionComponent::SetGear(const int32 NewGear)
{
    if (GearRatios.IsValidIndex(NewGear))
    {
        CurrentGear = NewGear;
    }
}

void UNsVehicleTransmissionComponent::SetDifferentialRatio(const float NewDifferentialRatio)
{
    DifferentialRatio = NewDifferentialRatio;
    ActiveDifferentialRatio = NewDifferentialRatio;
}

void UNsVehicleTransmissionComponent::SetPhysicsWheelRadius(const float NewRadius)
{
    WheelRadius = FMath::Max(NewRadius, UE_SMALL_NUMBER);
    ValidatePhysicsWheelRadiusSetup();
}

void UNsVehicleTransmissionComponent::Upshift()
{
    Shift(true);
}

void UNsVehicleTransmissionComponent::Downshift()
{
    Shift(false);
}

void UNsVehicleTransmissionComponent::Shift(const bool bUpshift)
{
    if (bUpshift)
    {
        if (CurrentGear + 1 < GearRatios.Num() && (!bIsUpshifting || bIsDownshifting))
        {
            bIsUpshifting = true;
            CurrentGear++;
        }
    }
    else
    {
        if (CurrentGear - 1 >= 0 && (bIsUpshifting || !bIsDownshifting))
        {
            bIsDownshifting = true;
            CurrentGear--;
        }
    }
}

float UNsVehicleTransmissionComponent::GetUpshiftRatio() const
{
    return UpshiftRatio;
}

bool UNsVehicleTransmissionComponent::IsShiftLocked() const
{
    return bLockShift;
}

float UNsVehicleTransmissionComponent::GetCurrentGearRatio() const
{
    return ActiveGearRatio;
}

float UNsVehicleTransmissionComponent::GetDifferentialRatio() const
{
    return ActiveDifferentialRatio;
}

float UNsVehicleTransmissionComponent::GetPhysicsWheelRadius() const
{
    return WheelRadius;
}

bool UNsVehicleTransmissionComponent::IsGearboxUpshifting() const
{
    return bIsUpshifting;
}

bool UNsVehicleTransmissionComponent::IsGearboxDownshifting() const
{
    return bIsDownshifting;
}

int32 UNsVehicleTransmissionComponent::GetCurrentGear() const
{
    return CurrentGear;
}

int32 UNsVehicleTransmissionComponent::GetGearCount() const
{
    return GearRatios.Num();
}

float UNsVehicleTransmissionComponent::GetTransmissionEfficiency() const
{
    return TransmissionEfficiency;
}

float UNsVehicleTransmissionComponent::GetStandardDifferentialRatio() const
{
    return DifferentialRatio;
}

float UNsVehicleTransmissionComponent::UpdateActiveGearRatio()
{
    ActiveGearRatio = GearRatios.Num() == 0 ? 3.5f : GearRatios[CurrentGear];
    return ActiveGearRatio;
}

void UNsVehicleTransmissionComponent::UpdateAutomaticShift(const float RPM, const float MaxRPM, const float SpeedKPH, const bool bInAir, const bool bDrivetrainEnabled)
{
    bool bUpshiftCondition = !bUseSpeedBasedShifting ? RPM > MaxRPM * UpshiftRatio : false;
    bool bDownshiftCondition = !bUseSpeedBasedShifting ? RPM < MaxRPM * DownshiftRatio : false;

    if (bUseSpeedBasedShifting && SpeedShiftingRatios.IsValidIndex(CurrentGear))
    {
        bUpshiftCondition = SpeedKPH > SpeedShiftingRatios[CurrentGear].Y;
        bDownshiftCondition = SpeedKPH < SpeedShiftingRatios[CurrentGear].X;
    }

    if (!bAutomatic || bLockShift || GearRatios.Num() == 1)
    {
        return;
    }

    // With the drivetrain, the gearbox does not shift while the vehicle is airborne
    const bool bCanShift = !bDrivetrainEnabled || !bInAir;

    if (bUpshiftCondition && bCanShift)
    {
        Shift(true);
    }

    if (bDownshiftCondition && bCanShift)
    {
        Shift(false);
    }
}

void UNsVehicleTransmissionComponent::UpdateShiftTimers(const float DeltaTime)
{
    if (bIsUpshifting)
    {
        UpshiftFrameCount++;

        if (UpshiftFrameCount * DeltaTime > ShiftDelay)
        {
            bIsUpshifting = false;
        }
    }
    else
    {
        UpshiftFrameCount = 0;
    }

    if (bIsDownshifting)
    {
        DownshiftFrameCount++;

        if (DownshiftFrameCount * DeltaTime > ShiftDelay)
        {
            bIsDownshifting = false;
        }
    }
    else
    {
        DownshiftFrameCount = 0;
    }
}

void UNsVehicleTransmissionComponent::SetReplicatedGearState(const int32 NewGear, const float NewGearRatio, const bool bNewIsUpshifting, const bool bNewIsDownshifting)
{
    CurrentGear = NewGear;
    ActiveGearRatio = NewGearRatio;
    bIsUpshifting = bNewIsUpshifting;
    bIsDownshifting = bNewIsDownshifting;
}

void UNsVehicleTransmissionComponent::ValidatePhysicsWheelRadiusSetup() const
{
    const ANsVehicle* const Vehicle = GetVehicle();
    if (Vehicle == nullptr)
    {
        return;
    }

    float VisualRadiusMetersSum = 0.f;
    int32 VisualRadiusCount = 0;

    for (UNsVehicleSuspensionComponent* Suspension : Vehicle->GetSuspension())
    {
        if (Suspension == nullptr)
        {
            continue;
        }

        const UStaticMeshComponent* const WheelMeshComponent = Suspension->GetWheelMeshComponent();
        if (WheelMeshComponent == nullptr || WheelMeshComponent->GetStaticMesh() == nullptr)
        {
            continue;
        }

        const float VisualRadiusMeters = WheelMeshComponent->GetStaticMesh()->GetBoundingBox().GetExtent().Z * WheelMeshComponent->GetRelativeScale3D().GetAbsMax() * 0.01f;
        if (VisualRadiusMeters > UE_SMALL_NUMBER)
        {
            VisualRadiusMetersSum += VisualRadiusMeters;
            ++VisualRadiusCount;
        }
    }

    if (VisualRadiusCount <= 0)
    {
        return;
    }

    const float AverageVisualRadiusMeters = VisualRadiusMetersSum / static_cast<float>(VisualRadiusCount);
    const float SmallerRadius = FMath::Min(WheelRadius, AverageVisualRadiusMeters);
    const float LargerRadius = FMath::Max(WheelRadius, AverageVisualRadiusMeters);
    const float RadiusRatio = SmallerRadius > UE_SMALL_NUMBER ? LargerRadius / SmallerRadius : TNumericLimits<float>::Max();

    if (RadiusRatio > 1.35f)
    {
        UE_LOG(LogTemp, Warning, TEXT("NsVehicle vehicle '%s' has physics wheel radius %.3f m but visual wheel radius averages %.3f m. This can skew torque, RPM, speed feel, and suspension setup; align the drivetrain radius with the wheel mesh unless this is intentional"),
            *GetPathNameSafe(Vehicle),
            WheelRadius,
            AverageVisualRadiusMeters);
    }
}
