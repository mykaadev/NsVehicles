// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "Vehicle/NsVehicleTypes.h"
#include "NsVehicleEngineComponent.generated.h"

class UCurveFloat;
class UNsVehicleDrivetrainComponent;

/** Makes the engine torque and the engine RPM for the arcade and standard powertrains */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleEngineComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    /** Sets the peak engine torque in Nm */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetEngineTorque(float NewTorque);

    /** Restores the engine torque from Begin Play, or uses a temporary torque */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetEngineTorqueEnabled(bool bEnabled, float TempTorque);

    /** Sets the powertrain model */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetPowertrainType(ENsVehicleBehaviorType NewPowertrainType);

    /** Sets the arcade top speed in KPH */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetArcadeMaxSpeed(float NewMaxSpeed);

    /** Sets the maximum engine RPM. Negative values become 0 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetMaxRPM(float NewMaxRPM);

    /** Sets the RPM scale */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetRPMMultiplier(float NewRPMMultiplier);

    /** Sets the drive torque scale that stays at the RPM limit, from 0 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetRpmLimitTorqueFloor(float NewTorqueFloor);

    /** Sets the engine brake coefficient, from 0 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void SetEngineBrakeCoefficient(float NewEngineBrakeCoefficient);

    /** Overrides the current engine RPM */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Engine")
    void OverrideRPM(float NewRPM);

    /** Returns the powertrain model */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Engine")
    ENsVehicleBehaviorType GetPowertrainType() const;

    /** Returns the peak engine torque in Nm. Use GetEngineCurrentTorque for the torque now */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Engine")
    float GetEngineTorque() const;

    /** Returns the engine torque now in Nm */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Engine")
    float GetEngineCurrentTorque() const;

    /** Returns the arcade top speed in KPH */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Engine")
    float GetArcadeMaxSpeed() const;

    /** Returns the engine RPM */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Engine")
    float GetRPM() const;

    /** Returns the maximum engine RPM */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Engine")
    float GetMaxRPM() const;

    /** Sets the current torque for the standard powertrain from the torque settings. Returns false when the torque curve is missing */
    bool UpdateStandardTorque();

    /** Sets the current torque for the arcade powertrain. The torque fades to 0 near the top speed. Returns the new torque */
    float UpdateArcadeTorque(float SpeedKPH);

    /** Returns the drive force scale near the RPM limit. The upshift ratio sets where the fade starts */
    float GetRpmLimitTorqueScale(float UpshiftRatio) const;

    /** Returns the engine brake force in N with no throttle */
    float GetEngineBrakeForce(float Throttle, float GearRatio, float VelocityDirection) const;

    /** Moves the RPM toward the wheel speed RPM, or toward the throttle RPM when the driven wheels are in the air */
    void UpdateStandardRPM(float DeltaTime, float GearRatio, float DifferentialRatio, float WheelRadius, float LinearVelocity, float Throttle, float Brakes, bool bInAir, const UNsVehicleDrivetrainComponent& Drivetrain);

// Variables
protected:
    /** The powertrain model. Arcade uses constant torque with no gearbox. Standard uses a full powertrain with torque as a function of RPM */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine")
    ENsVehicleBehaviorType PowertrainType = ENsVehicleBehaviorType::Arcade;

    /** The maximum engine torque in Nm. With a torque curve, this is the peak of the curve */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine")
    float EngineTorque = 500.f;

    /** The arcade top speed in KPH. The real top speed is a little lower */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine", meta = (EditCondition = "PowertrainType==ENsVehicleBehaviorType::Arcade", EditConditionHides))
    float MaxSpeed = 318.f;

    /** The maximum engine RPM. Only OverrideRPM can go above it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine", meta = (EditCondition = "PowertrainType==ENsVehicleBehaviorType::Standard", EditConditionHides))
    float MaxRPM = 7000.f;

    /** The drive torque scale that stays at the RPM limit. It stops on and off surges against drag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine", meta = (EditCondition = "PowertrainType==ENsVehicleBehaviorType::Standard", EditConditionHides, ClampMin = "0", ClampMax = "1"))
    float RpmLimitTorqueFloor = 0.06f;

    /** Scales the RPM. Values other than 1 can make the engine unstable */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine", meta = (EditCondition = "PowertrainType==ENsVehicleBehaviorType::Standard", EditConditionHides))
    float RPM_Multiplier = 1.f;

    /** How the standard powertrain gets the torque. Constant keeps the same torque in a gear. Curve reads the torque curve at the current RPM */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine", meta = (EditCondition = "PowertrainType==ENsVehicleBehaviorType::Standard", EditConditionHides))
    ENsVehicleTorqueImplementation TorqueImplementation = ENsVehicleTorqueImplementation::ETI_Constant;

    /** The normalized torque curve from 0 to 1. X is the RPM, Y is the torque multiplier of EngineTorque */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine", meta = (EditCondition = "PowertrainType==ENsVehicleBehaviorType::Standard && TorqueImplementation==ENsVehicleTorqueImplementation::ETI_Curve", EditConditionHides))
    TObjectPtr<UCurveFloat> EngineTorqueCurve;

    /** How fast the RPM falls with no throttle. Higher values give more off-throttle deceleration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Engine", meta = (EditCondition = "PowertrainType==ENsVehicleBehaviorType::Standard", EditConditionHides, ClampMin = "0", ClampMax = "1"))
    float EngineBrakeCoefficient = 0.35f;

private:
    /** The engine RPM */
    float RPM = 0.f;

    /** The engine torque now in Nm */
    float CurrentTorque = 0.f;

    /** The engine torque from Begin Play that SetEngineTorqueEnabled restores */
    float EngineTorqueDuplicate = 500.f;
};
