// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleDrivetrainComponent.generated.h"

class UNsVehicleSuspensionComponent;

/** Sends the engine power to the driven wheels and removes power when the driven wheels slip. Mark the driven wheels on their suspension components */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleDrivetrainComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Enables or disables the drivetrain slip simulation */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Drivetrain")
    void SetDrivetrainEnabled(bool bEnabled);

    /** Sets the slip power reduction, the slip start speed, and the slip restore speed */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Drivetrain")
    void SetDrivetrainSlipSettings(float NewSlipPowerReduction, float NewSlipInitiateSpeed, float NewSlipRestoreSpeed);

    /** Returns true when the drivetrain slip simulation is on */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Drivetrain")
    bool IsDrivetrainEnabled() const;

    /** Returns the average normalized slip of the driven wheels */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Drivetrain")
    float GetAverageDrivetrainSlip() const;

    /** Returns the average throttle slip value of the driven wheels */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Drivetrain")
    float GetAveragedSlipGamma() const;

    /** Returns true when the driven wheels lose traction */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Drivetrain")
    bool IsSlipping() const;

    /** Returns the number of driven wheels from the last physics step */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Drivetrain")
    float GetDrivetrainWheelCount() const;

    /** Returns the part of the engine power that gets to the wheels */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Drivetrain")
    float GetEffectivePower() const;

    /** Returns how fast the slip starts */
    float GetSlipInitiateSpeed() const;

    /** Returns how fast the traction comes back */
    float GetSlipRestoreSpeed() const;

    /** Returns how fast the RPM goes up when the driven wheels are in the air */
    float GetInAirSpeedUp() const;

    /** Returns how fast the RPM goes down when the driven wheels are in the air */
    float GetInAirSlowdownSpeed() const;

    /** Averages the slip of the driven wheels. Returns the number of driven wheels on the ground */
    int32 UpdateWheelSlip(const TArray<UNsVehicleSuspensionComponent*>& Suspensions);

    /** Moves the effective power toward full power, or toward the slip power when the driven wheels slip */
    void UpdateEffectivePower(float Throttle, float DeltaTime);

// Variables
protected:
    /** Enables the drivetrain slip simulation. Also enable the drivetrain on each suspension component. It overrides EffectivePower */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Drivetrain")
    bool bUseDrivetrain = false;

    /** The part of the engine power that gets to the wheels. The slip simulation changes it when it is on */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Drivetrain")
    float EffectivePower = 1.f;

    /** How much power the wheel slip removes. Higher values remove more power */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Drivetrain", meta = (EditCondition = "bUseDrivetrain", EditConditionHides))
    float SlipPowerReduction = 1.f;

    /** How fast the slip starts when the wheels lose traction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Drivetrain", meta = (EditCondition = "bUseDrivetrain", EditConditionHides))
    float SlipInitiateSpeed = 15.f;

    /** How fast the traction comes back after the throttle is released */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Drivetrain", meta = (EditCondition = "bUseDrivetrain", EditConditionHides))
    float SlipRestoreSpeed = 0.5f;

    /** How fast the RPM goes up when the driven wheels are in the air */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Drivetrain", meta = (EditCondition = "bUseDrivetrain", EditConditionHides))
    float InAirSpeedUp = 10.f;

    /** How fast the RPM goes down when the driven wheels are in the air with no throttle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Drivetrain", meta = (EditCondition = "bUseDrivetrain", EditConditionHides))
    float InAirSlowdownSpeed = 2.f;

private:
    /** The average slip ratio of the driven wheels */
    float AveragedSlip = 1.f;

    /** The average throttle slip value of the driven wheels */
    float AveragedSlipGamma = 0.f;

    /** The number of driven wheels from the last physics step */
    float DrivetrainWheelCount = 0.f;
};
