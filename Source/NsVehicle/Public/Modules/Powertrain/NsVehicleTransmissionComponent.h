// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleTransmissionComponent.generated.h"

/** Owns the gears, the differential, the drive wheel radius, and the automatic gearbox */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleTransmissionComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Locks or unlocks the gearbox */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetShiftLocked(bool bLocked);

    /** Enables or disables the automatic gearbox */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetAutomaticTransmission(bool bEnabled);

    /** Sets the RPM ratio for an upshift, from 0 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetUpshiftRatio(float NewUpshiftRatio);

    /** Sets the RPM ratio for a downshift, from 0 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetDownshiftRatio(float NewDownshiftRatio);

    /** Sets the transmission efficiency, from 0 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetTransmissionEfficiency(float NewTransmissionEfficiency);

    /** Replaces the gear ratios. The current gear resets to 0 if it is not valid */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetGearRatios(const TArray<float>& NewGearRatios);

    /** Overrides the active gear ratio that the force calculation uses */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetCurrentGearRatio(float NewGearRatio);

    /** Selects a gear index. An index that is not valid has no effect */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetGear(int32 NewGear);

    /** Sets the differential ratio for the standard powertrain and the active differential ratio */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetDifferentialRatio(float NewDifferentialRatio);

    /** Sets the drive wheel radius in meters and compares it with the wheel meshes */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void SetPhysicsWheelRadius(float NewRadius);

    /** Shifts one gear up */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void Upshift();

    /** Shifts one gear down */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Transmission")
    void Downshift();

    /** Shifts one gear up or down if the gearbox can shift */
    void Shift(bool bUpshift);

    /** Returns the RPM ratio for an upshift */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    float GetUpshiftRatio() const;

    /** Returns true when the gearbox is locked */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    bool IsShiftLocked() const;

    /** Returns the active gear ratio that the force calculation uses */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    float GetCurrentGearRatio() const;

    /** Returns the active differential ratio that the arcade force and the wheel torque use */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    float GetDifferentialRatio() const;

    /** Returns the drive wheel radius in meters */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    float GetPhysicsWheelRadius() const;

    /** Returns true while the gearbox upshifts */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    bool IsGearboxUpshifting() const;

    /** Returns true while the gearbox downshifts */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    bool IsGearboxDownshifting() const;

    /** Returns the current gear index */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    int32 GetCurrentGear() const;

    /** Returns the number of gears */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Transmission")
    int32 GetGearCount() const;

    /** Returns the transmission efficiency */
    float GetTransmissionEfficiency() const;

    /** Returns the differential ratio setting that the standard powertrain uses */
    float GetStandardDifferentialRatio() const;

    /** Sets the active gear ratio from the current gear. With no gears, the ratio is 3.5. Returns the new ratio */
    float UpdateActiveGearRatio();

    /** Shifts the automatic gearbox from the RPM or the speed */
    void UpdateAutomaticShift(float RPM, float MaxRPM, float SpeedKPH, bool bInAir, bool bDrivetrainEnabled);

    /** Ends an upshift or a downshift after the shift delay */
    void UpdateShiftTimers(float DeltaTime);

    /** Copies the gear state from the server on a client */
    void SetReplicatedGearState(int32 NewGear, float NewGearRatio, bool bNewIsUpshifting, bool bNewIsDownshifting);

private:
    /** Logs a warning when the drive wheel radius is far from the wheel mesh radius */
    void ValidatePhysicsWheelRadiusSetup() const;

// Variables
protected:
    /** Scales the acceleration in all gears. It does not change the top speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission", meta = (ClampMin = "0", ClampMax = "1"))
    float TransmissionEfficiency = 0.7f;

    /** The duration of a gear shift in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission")
    float ShiftDelay = 0.2f;

    /** The ratio between the engine revolutions and the drive wheel revolutions for the standard powertrain */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission")
    float DifferentialRatio = 3.05f;

    /** One ratio for each gear, between the engine revolutions and the gear revolutions */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission")
    TArray<float> GearRatios;

    /** The wheel radius in meters for the torque calculation. It is separate from the suspension wheel radius */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission")
    float WheelRadius = 0.28f;

    /** Enables the automatic gearbox */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission")
    bool bAutomatic = true;

    /** Uses the vehicle speed instead of the RPM to decide when the gearbox shifts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission")
    bool bUseSpeedBasedShifting = false;

    /** The shift speeds in KPH for each gear. X is the downshift speed. Y is the upshift speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission", meta = (EditCondition = "bUseSpeedBasedShifting", EditConditionHides))
    TArray<FVector2D> SpeedShiftingRatios;

    /** The RPM to MaxRPM ratio for an upshift */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission", meta = (EditCondition = "bAutomatic && !bUseSpeedBasedShifting", EditConditionHides, ClampMin = "0", ClampMax = "1"))
    float UpshiftRatio = 0.85f;

    /** The RPM to MaxRPM ratio for a downshift. Space the gear ratios so that the upshift and downshift ranges overlap */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission", meta = (EditCondition = "bAutomatic && !bUseSpeedBasedShifting", EditConditionHides, ClampMin = "0", ClampMax = "1"))
    float DownshiftRatio = 0.3f;

    /** Stops all gear shifts while true */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Transmission")
    bool bLockShift = false;

private:
    /** The current gear index */
    int32 CurrentGear = 0;

    /** The gear ratio that the force calculation uses */
    float ActiveGearRatio = 3.f;

    /** The differential ratio that the arcade force and the wheel torque use */
    float ActiveDifferentialRatio = 3.15f;

    /** True while the gearbox upshifts */
    bool bIsUpshifting = false;

    /** True while the gearbox downshifts */
    bool bIsDownshifting = false;

    /** The physics steps since the upshift started */
    int32 UpshiftFrameCount = 0;

    /** The physics steps since the downshift started */
    int32 DownshiftFrameCount = 0;
};
