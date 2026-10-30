// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "Vehicle/NsVehicleTypes.h"
#include "NsVehicleSteeringComponent.generated.h"

class UCurveFloat;
class UPrimitiveComponent;
class UNsVehicleSuspensionComponent;

/** Limits the steering input by speed, sets the steering response, turns the steering wheels, and adds the drift torque */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleSteeringComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Sets the constant steering response speed */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Steering")
    void SetSteeringSensitivity(float NewSteeringSensitivity);

    /** Returns the raw steering input after the speed clamp */
    float ClampSteeringInput(float RawInput, float SpeedKPH) const;

    /** Returns the steering response speed at a speed in KPH */
    float GetSteeringResponse(float SpeedKPH) const;

    /** Sends the steering input and the steering response to the steering wheels */
    void UpdateWheelSteering(const TArray<UNsVehicleSuspensionComponent*>& Suspensions, float SteeringInput, float SpeedKPH) const;

    /** Adds the helper yaw torque that follows the steering input */
    void ApplyDriftTorque(UPrimitiveComponent* PhysicsBody, const FVector& Up, float LinearVelocity, float SteeringInput) const;

// Variables
protected:
    /** A helper yaw torque in Unreal torque units that follows the steering input. It makes the vehicle turn faster */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering")
    float DriftTorque = 0.f;

    /** How the maximum steering input changes with speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering")
    ENsVehicleSpeedClamp SteeringInputClamp = ENsVehicleSpeedClamp::NoClamp;

    /** The maximum steering input at all speeds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "SteeringInputClamp==ENsVehicleSpeedClamp::Constant", EditConditionHides))
    float ConstantSteeringClamp = 1.f;

    /** Maps a speed range in KPH to a maximum steering input range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering", meta = (EditCondition = "SteeringInputClamp==ENsVehicleSpeedClamp::Linear", EditConditionHides))
    FNsVehicleRangedClamp LinearSteeringInputClamp;

    /** The maximum steering input as a curve. X is the speed in KPH. Y is the maximum input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering", meta = (EditCondition = "SteeringInputClamp==ENsVehicleSpeedClamp::Curve", EditConditionHides))
    TObjectPtr<UCurveFloat> CurveSteeringInputClamp;

    /** Makes the steering response slower at higher speeds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering")
    bool bUseSensitivityClamp = false;

    /** The steering response speed at all speeds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering", meta = (EditCondition = "!bUseSensitivityClamp", EditConditionHides))
    float SteeringSensitivity = 5.f;

    /** Maps a speed range in KPH to a steering response speed range */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Steering", meta = (EditCondition = "bUseSensitivityClamp", EditConditionHides))
    FNsVehicleRangedClamp SteeringSensitivityClamp;
};
