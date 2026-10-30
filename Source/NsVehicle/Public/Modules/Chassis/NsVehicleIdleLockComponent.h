// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleIdleLockComponent.generated.h"

/** Holds the vehicle in place when it stops or moves very slowly, to prevent micro slip */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleIdleLockComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Enables or disables the idle lock */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Idle Lock")
    void SetIdleLockEnabled(bool bEnabled);

    /** Returns true when the idle lock holds the vehicle */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Idle Lock")
    bool IsIdleLocked() const;

    /** Starts, holds, or releases the idle lock for one physics step */
    void UpdateIdleLock(float DeltaTime);

// Variables
protected:
    /** Holds the vehicle in place when it stops or moves very slowly */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock")
    bool bUseIdleLock = true;

    /** The time in seconds before the idle lock holds the vehicle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock")
    float IdleLockTime = 1.5f;

    /** True when the idle lock holds the vehicle. Set it at runtime to control the idle lock directly */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock")
    bool bIsIdleLocked = false;

    /** The speed in KPH below which the idle lock timer starts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock")
    float IdleLockSpeedThreshold = 10.f;

    /** The hold force on slopes. A force that is too high can push the vehicle out of the idle lock */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock")
    float IdleLockStiffness = 3000.f;

private:
    /** The time in seconds that the vehicle is below the idle lock speed */
    float IdleLockCount = 0.f;

    /** True when the idle lock also holds the position and rotation */
    bool bIsIdleLockedRotationLocation = false;

    /** The time in seconds before the idle lock holds the position and rotation */
    float IdleLockRotationLocationCount = 0.f;
};
