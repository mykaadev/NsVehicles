// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleAntiGravityComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

/** Pulls the vehicle to the surface under it, for wall and ceiling driving. Call InitializeAntiGravity in Begin Play to use it */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleAntiGravityComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Starts the anti gravity system. Call it one time in Begin Play */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Anti Gravity")
    void InitializeAntiGravity(USceneComponent* AntiGravityComponent, UStaticMeshComponent* CoreMesh);

    /** Forces the anti gravity system on or off. Initialize the anti gravity system before you use this */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Anti Gravity")
    void OverrideAntiGravityState(bool bOn);

    /** Returns true when the anti gravity system applies force */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Anti Gravity")
    bool IsAntiGravityActive() const;

    /** Reads the core mesh mass and turns the anti gravity component to the vehicle yaw. Call it on the game thread */
    void UpdateGameThread(const UPrimitiveComponent* PhysicsBody);

    /** Adds the anti gravity forces for one physics step */
    void ApplyAntiGravity(UPrimitiveComponent* PhysicsBody, const FTransform& PhysicsTransform, bool bInAir, float DeltaTime);

// Variables
protected:
    /** Enables the anti gravity system */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Anti Gravity")
    bool bAntiGravityEnabled = false;

    /** The gravity magnitude in cm/s^2 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Anti Gravity")
    float GravityStrength = 980.f;

    /** Scales the anti gravity on the Z axis. Higher values pull the vehicle more to the surface. 1 to 2 works well */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Anti Gravity")
    float AntiGravityZStrength = 1.2f;

    /** Scales the anti gravity on the X axis. Above 1 stops rolling back on a slope. 0 allows rolling */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Anti Gravity")
    float AntiGravityXStrength = 1.02f;

    /** Scales the anti gravity on the Y axis. Above 1 stops side slip on a slope. 0 allows side slip */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Anti Gravity")
    float AntiGravityYStrength = 1.02f;

    /** The time in seconds that the anti gravity stays on after the wheels lose contact */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Anti Gravity")
    float AntiGravityOffContactThreshold = 1.f;

    /** Mass in kg that the anti gravity adds to the core mesh mass. Use it for a mass change at runtime. It has no performance cost */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Anti Gravity")
    float AddedMass = 0.f;

private:
    /** The scene component that sets the anti gravity direction */
    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> AntiGravity;

    /** The mesh that gives the anti gravity mass */
    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> AntiGravityCoreMesh;

    /** The mass in kg of the anti gravity core mesh */
    float AntiGravityMass = 0.f;

    /** The time in seconds since the wheels lost contact */
    float AntiGravityOffContactTime = 0.f;

    /** True when the anti gravity system applies force */
    bool bAntiGravityActive = false;

    /** True when OverrideAntiGravityState forces the anti gravity on */
    bool bOverrideAntiGravity = false;
};
