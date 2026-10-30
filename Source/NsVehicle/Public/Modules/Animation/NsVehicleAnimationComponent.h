// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleAnimationComponent.generated.h"

class UNsVehicleSuspensionComponent;
class UNsVehicleWheel;

/** Spins the wheels from the actor movement for the take recorder and sequencer. Do not enable the animation mode for gameplay */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleAnimationComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Finds the tagged suspension components and animation wheels under the vehicle root */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Animation")
    void InitializeAnimationSystem();

    /** Calculates the actor velocity and spins the animation wheels. Use it for take recorder playback in the editor */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Animation")
    void SimulateInternalAnimation();

    /** Spins the animation wheels from the actor velocity */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Animation")
    void SimulateInternalWheelAnimation();

    /** Returns the local velocity in m/s from the actor movement between frames */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Animation")
    FVector GetLocationVelocity() const;

    /** Returns true when the animation mode is on */
    bool IsAnimationModeEnabled() const;

    /** Returns the wheel speed scale for the animation mode */
    float GetGlobalWheelSpeedMultiplier() const;

    /** Calculates the local actor velocity from the movement since the last call */
    void UpdateLocationVelocity();

    /** Spins the wheel meshes of the suspension components from the actor velocity */
    void SpinSuspensionWheels(const TArray<UNsVehicleSuspensionComponent*>& Suspensions) const;

// Variables
protected:
    /** Enables the take recorder animation system. Do not enable it for gameplay */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NsVehicle|Animation")
    bool bEnableAnimationMode = false;

    /** Scales the speed of all wheels in animation mode. The suspension multipliers do not apply in this mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "NsVehicle|Animation")
    float AnimationGlobalWheelSpeedMultiplier = 1.f;

    /** The fixed frame time in seconds that the animation system uses */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Animation")
    float AnimDeltaTime = 1.f / 60.f;

private:
    /** The animation wheels that InitializeAnimationSystem found */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UNsVehicleWheel>> AnimWheels;

    /** The actor location in this frame */
    FVector CurrentLocation = FVector::ZeroVector;

    /** The actor location in the previous frame */
    FVector PreviousLocation = FVector::ZeroVector;

    /** The local velocity in m/s from the actor movement between frames */
    FVector LocationVelocity = FVector::ZeroVector;
};
