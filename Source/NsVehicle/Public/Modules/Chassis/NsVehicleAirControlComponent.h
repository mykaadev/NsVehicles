// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleAirControlComponent.generated.h"

class UPrimitiveComponent;
class UNsVehicleSuspensionComponent;

/** Turns the vehicle in the air from the air pitch, yaw, and roll inputs */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleAirControlComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Adds the air control torque when enough wheels are off the ground, or when the vehicle has no wheels */
    void ApplyAirControl(UPrimitiveComponent* PhysicsBody, const FVector& Forward, const FVector& Right, const FVector& Up, const TArray<UNsVehicleSuspensionComponent*>& Suspensions, int32 InAirWheelCount, float PitchInput, float YawInput, float RollInput) const;

// Variables
protected:
    /** The air control torque on the pitch axis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Air Control")
    float PitchStrength = 2.f;

    /** The air control torque on the yaw axis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Air Control")
    float YawStrength = 2.f;

    /** The air control torque on the roll axis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Air Control")
    float RollStrength = 2.f;
};
