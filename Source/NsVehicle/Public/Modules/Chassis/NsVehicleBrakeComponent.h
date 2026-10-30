// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleBrakeComponent.generated.h"

class UPrimitiveComponent;
class UNsVehicleSuspensionComponent;

/** Owns the brakes, the reverse drive, the handbrake, and the external resistance force */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleBrakeComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    /** Sets the brake force in N */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Brakes")
    void SetBrakeForce(float NewBrakeForce);

    /** Enables the brakes with the stored brake force, or sets the brake force to 0 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Brakes")
    void SetBrakesEnabled(bool bEnabled);

    /** Sets the maximum reverse speed in KPH */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Brakes")
    void SetMaxSpeedInReverse(float NewMaxSpeedInReverse);

    /** Sets the handbrake force in N */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Brakes")
    void SetHandbrakeStrength(float NewHandbrakeStrength);

    /** Sets a force in N that opposes the arcade movement, for example for a rough surface */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Brakes")
    void SetExternalResistanceForce(float Force);

    /** Returns the external resistance force in N */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Brakes")
    float GetExternalResistanceForce() const;

    /** Returns true when the handbrake applies force */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Brakes")
    bool GetIsHandbrakeActive() const;

    /** Returns the handbrake force now in N */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Brakes")
    float GetHandbrakeStrength() const;

    /** Returns the brake force in N for the standard powertrain and sets the force direction */
    float GetStandardBrakeForce(float RPM, float MaxRPM, float SpeedKPH, float VelocityDirection, float& OutBrakeDirection) const;

    /** Returns the brake force in N for the arcade powertrain. It is 0 above the maximum reverse speed */
    float GetArcadeBrakeForce(float SpeedKPH) const;

    /** Slows the vehicle with the handbrake force when a handbrake wheel exists and the handbrake input is on */
    void ApplyHandbrake(UPrimitiveComponent* PhysicsBody, const TArray<UNsVehicleSuspensionComponent*>& Suspensions, bool bHandbrakeInput, bool bInAir, const FVector& WorldVelocity, float LocalForwardVelocity);

// Variables
protected:
    /** The brake force in N. With brake as reverse, this is also the reverse force */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Brakes")
    float BrakeForce = 15000.f;

    /** Uses the brake input to drive backward in the standard powertrain. The arcade powertrain always does this */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Brakes")
    bool bUseBrakeAsReverse = true;

    /** The maximum reverse speed in KPH with brake as reverse. Without it, the gear ratio sets the reverse speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Brakes")
    float MaxSpeedInReverse = 60.f;

    /** The handbrake force in N */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Handbrake")
    float HandbrakeStrength = 7500.f;

private:
    /** The brake force from Begin Play that SetBrakesEnabled restores */
    float BrakeForceDuplicate = 15000.f;

    /** The arcade force in N that opposes the movement */
    float ExternalResistanceForce = 0.f;

    /** True when the handbrake applies force */
    bool bHandbrakeActive = false;

    /** The handbrake force now in N */
    float HandbrakeForce = 0.f;
};
