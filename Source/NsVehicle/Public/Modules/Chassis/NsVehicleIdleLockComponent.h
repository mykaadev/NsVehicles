// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleIdleLockComponent.generated.h"

/**
 * Holds the vehicle in place when it stops or moves very slowly, to prevent micro slip
 * The parked hold is a second, stronger hold: it stops the physics of a vehicle that nobody drives, after it lands and settles. Pushes, slopes, and the suspension forces then cannot move it
 */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleIdleLockComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleIdleLockComponent();

    /** Enables or disables the idle lock */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Idle Lock")
    void SetIdleLockEnabled(bool bEnabled);

    /** Returns true when the idle lock holds the vehicle */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Idle Lock")
    bool IsIdleLocked() const;

    /** Starts, holds, or releases the idle lock for one physics step */
    void UpdateIdleLock(float DeltaTime);

    /** Enables or disables the parked hold. Disable gives the physics back to a held vehicle */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Idle Lock")
    void SetParkedHoldEnabled(bool bEnabled);

    /** Returns true while the parked hold stops the physics of the vehicle */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Idle Lock")
    bool IsParkedHeld() const;

    /** Returns true when a player drives the vehicle now, and the vehicle does not count as parked */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Idle Lock")
    bool IsDriven() const;

    /** True makes the vehicle count as parked while a player still controls it, for example during a cutscene. False gives the physics back at once when a player drives */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Idle Lock")
    void SetTreatAsParked(bool bParked);

    /** Gives the physics back to a held vehicle and starts the settle time again. Call it when something changes the stance, for example a lost wheel */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Idle Lock")
    void ReleaseParkedHold();

    /** Releases the hold when a player drives now, else starts the settle time again. The vehicle calls it when its controller changes */
    void RefreshParkedHold();

    /** Counts the settle time of a parked vehicle, and starts the parked hold when it rests on the ground. The vehicle calls it on the game thread each frame */
    void UpdateParkedHold(float DeltaTime);

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

    /** Stops the physics of a vehicle that nobody drives, after the settle time, when it rests on the ground. A player who drives gets the physics back at once */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock|Parked Hold")
    bool bUseParkedHold;

    /** The time that a vehicle nobody drives gets to land and settle before the parked hold */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock|Parked Hold", meta = (ClampMin = "0.01", Units = "s", EditCondition = "bUseParkedHold"))
    float ParkedSettleTime;

    /** The highest speed at which the parked hold can start. A faster vehicle waits one more settle time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Idle Lock|Parked Hold", meta = (ClampMin = "0.0", Units = "cm/s", EditCondition = "bUseParkedHold"))
    float ParkedMaxSpeed;

private:
    /** The time in seconds that the vehicle is below the idle lock speed */
    float IdleLockCount = 0.f;

    /** True when the idle lock also holds the position and rotation */
    bool bIsIdleLockedRotationLocation = false;

    /** The time in seconds before the idle lock holds the position and rotation */
    float IdleLockRotationLocationCount = 0.f;

    /** True while the parked hold stops the physics of the vehicle */
    bool bParkedHeld;

    /** True while the vehicle counts as parked even when a player controls it */
    bool bTreatAsParked;

    /** The time in seconds that the vehicle settles before the next parked hold test */
    float ParkedSettleCount;
};
