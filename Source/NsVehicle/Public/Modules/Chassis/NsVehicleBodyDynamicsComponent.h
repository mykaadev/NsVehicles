// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleBodyDynamicsComponent.generated.h"

class UPrimitiveComponent;
class UNsVehicleSuspensionComponent;

/** Adds the body pitch under acceleration and braking, and the bike lean in a turn */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleBodyDynamicsComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Starts the lean steer system. Call it one time in Begin Play. A rear suspension component gives the best result, but any suspension component works */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Body Dynamics")
    void InitializeLeanSteer(UNsVehicleSuspensionComponent* SuspensionComponent);

    /** Adds the pitch torque from a drive force in N */
    void ApplyAccelerationPitch(UPrimitiveComponent* PhysicsBody, const FVector& Right, float DriveForce) const;

    /** Adds the pitch torque from a brake force in N */
    void ApplyBrakePitch(UPrimitiveComponent* PhysicsBody, const FVector& Right, float BrakeForce) const;

    /** Adds the lean torque that makes the vehicle lean into a turn */
    void ApplyLeanSteer(UPrimitiveComponent* PhysicsBody, const FVector& Up, const FVector& Forward, float SteeringInput, float SpeedKPH, bool bInAir) const;

// Variables
protected:
    /** Scales the pitch lean under acceleration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Body Dynamics")
    float DynamicPitchMomentMultiplier = 0.f;

    /** Scales the pitch lean under braking. High values can make the vehicle unstable */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Body Dynamics")
    float DynamicBrakeMomentMultiplier = 0.f;

    /** Makes the vehicle lean into a turn like a bike. Call InitializeLeanSteer in Begin Play to use it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Body Dynamics")
    bool bUseSteerLean = false;

    /** The force that makes the vehicle lean */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Body Dynamics", meta = (EditCondition = "bUseSteerLean", EditConditionHides))
    float SteerLeanCoefficient = 4.85f;

    /** The minimum speed in KPH for lean steer */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Body Dynamics", meta = (EditCondition = "bUseSteerLean", EditConditionHides))
    float SteerLeanMinSpeed = 5.f;

    /** The ground normal dot range that maps to the lean strength. X is the start, Y is the full lean */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Body Dynamics", meta = (EditCondition = "bUseSteerLean", EditConditionHides))
    FVector2D SteerLeanMapping = FVector2D(0.9f, 0.99f);

private:
    /** The suspension component that the lean steer reads the ground from */
    UPROPERTY(Transient)
    TObjectPtr<UNsVehicleSuspensionComponent> LeanSteerSuspension;
};
