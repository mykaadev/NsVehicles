// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleSelfRightComponent.generated.h"

/**
 * Finds an overturned vehicle, and lets the driver roll it back onto its wheels with the steering
 * The overturned test compares the up of the vehicle with the ground under it, so a vehicle on a slope is not overturned
 * Each steering input on an overturned vehicle that rests on the ground adds a small hop and a roll. The tests work also when the roll is off
 */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleSelfRightComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleSelfRightComponent();

    /** Enables or disables the roll from the steering */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Self Right")
    void SetSelfRightEnabled(bool bEnabled);

    /** Returns true when the vehicle lies on its side or on its roof, against the ground under it */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Self Right")
    bool IsOverturned() const;

    /** Returns true when the ground is just below the bounds of the body, so the body rests and does not fly */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Self Right")
    bool IsBodyOnGround() const;

    /** Traces from the center of the body down to just below its bounds. Returns true on a ground hit */
    bool TraceBodyGround(FHitResult& OutHit) const;

    /** Adds one hop and one roll impulse when the vehicle is overturned, rests on the ground, and the pulse interval is over. A negative input rolls to the left. Returns true when it added the impulses */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Self Right")
    bool TryRollFromSteering(float SteeringInput);

    /** Rolls the vehicle from the steering input of the Controls module. The vehicle calls it on the game thread each frame */
    void UpdateSelfRight();

private:
    /** Returns the physics body of the vehicle, or null */
    class UPrimitiveComponent* GetBody() const;

// Variables
protected:
    /** Rolls an overturned vehicle with the steering input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right")
    bool bUseSelfRight;

    /** The highest alignment of the vehicle up with the ground normal under it that counts as overturned */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float OverturnedUpDot;

    /** The shortest time between two roll impulses */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right", meta = (ClampMin = "0.01", Units = "s", EditCondition = "bUseSelfRight"))
    float RollPulseInterval;

    /** The vertical velocity change of one hop */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right", meta = (Units = "cm/s", EditCondition = "bUseSelfRight"))
    float HopVelocity;

    /** The roll velocity change of one impulse at a full steering input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right", meta = (EditCondition = "bUseSelfRight"))
    float RollAngularImpulse;

    /** The highest vertical speed that allows an impulse, so the impulses do not add up while the vehicle flies */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right", meta = (ClampMin = "0.0", Units = "cm/s", EditCondition = "bUseSelfRight"))
    float MaxVerticalSpeed;

    /** The distance below the bounds of the body that still counts as ground */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right", meta = (ClampMin = "0.0", Units = "cm"))
    float GroundReach;

    /** The trace channel of the ground test */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Self Right")
    TEnumAsByte<ECollisionChannel> GroundChannel;

private:
    /** The world time of the last roll impulse */
    double LastRollTime;
};
