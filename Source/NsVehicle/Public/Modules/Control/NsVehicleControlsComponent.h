// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleControlsComponent.generated.h"

/** Holds the driver input and sends it to the server. Call these functions from the player input or from AI */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleControlsComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleControlsComponent();

    /** Sets the throttle input from 0 to 1 and sends it to the server. The throttle is 0 during an upshift */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetThrottleInput(float Value);

    /** Sets the throttle input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetThrottleInput(float Value);

    /** Sets the brake input from 0 to 1 and sends it to the server. With brake as reverse, this input also drives the vehicle backward */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetBrakeInput(float Value);

    /** Sets the brake input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetBrakeInput(float Value);

    /** Sets the steering input from -1 to 1 and sends it to the server. The steering module limits the value by speed */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetSteeringInput(float Value);

    /** Sets the steering input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetSteeringInput(float Value);

    /** Sets the handbrake input and sends it to the server */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void ApplyHandbrakeInput(bool bValue);

    /** Sets the handbrake input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerApplyHandbrakeInput(bool bValue);

    /** Enables or disables all input. Use it when a controller unpossesses the vehicle, so that old input does not continue */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetControllable(bool bNewInputEnabled);

    /** Enables or disables all input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetControllable(bool bNewInputEnabled);

    /** Sets the air control pitch input from -1 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetAirPitchInput(float Value);

    /** Sets the air control pitch input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetAirPitchInput(float Value);

    /** Sets the air control yaw input from -1 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetAirYawInput(float Value);

    /** Sets the air control yaw input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetAirYawInput(float Value);

    /** Sets the air control roll input from -1 to 1 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetAirRollInput(float Value);

    /** Sets the air control roll input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetAirRollInput(float Value);

    /** Sets a custom replicated input from -1 to 1 for project logic. The index is from 0 to 5 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Controls")
    void SetExtReplicationInput(int32 Index, float Value);

    /** Sets a custom replicated input on the server */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSetExtReplicationInput(int32 Index, float Value);

    /** Returns a custom replicated input. The index is from 0 to 5 */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Controls")
    float GetExtReplicationInput(int32 Index) const;

    /** Returns the throttle input */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Controls")
    float GetThrottleInput() const;

    /** Returns the brake input */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Controls")
    float GetBrakesInput() const;

    /** Returns the steering input after the speed clamp */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Controls")
    float GetSteeringInput() const;

    /** Returns the handbrake input */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Controls")
    bool GetHandbrakeInput() const;

    /** Returns true when the vehicle accepts input */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Controls")
    bool IsControllable() const;

    /** Returns the air control pitch input */
    float GetAirPitchInput() const;

    /** Returns the air control yaw input */
    float GetAirYawInput() const;

    /** Returns the air control roll input */
    float GetAirRollInput() const;

    /** Copies the throttle, brake, and steering inputs from the server on a client */
    void SetReplicatedInputs(float NewThrottle, float NewBrakes, float NewSteeringInput);

private:
    /** Returns 1 when input is enabled, and 0 when it is disabled */
    float GetControllableValue() const;

// Variables
public:
    /** The number of custom replicated inputs */
    static constexpr int32 ExtReplicationInputCount = 6;

private:
    /** The throttle input from 0 to 1 */
    float Throttle = 0.f;

    /** The brake input from 0 to 1 */
    float Brakes = 0.f;

    /** The steering input from -1 to 1, after the speed clamp */
    float SteeringInput = 0.f;

    /** The handbrake input */
    bool bHandbrakeInput = false;

    /** The air control pitch input */
    float PitchInput = 0.f;

    /** The air control roll input */
    float RollInput = 0.f;

    /** The air control yaw input */
    float YawInput = 0.f;

    /** True when the vehicle accepts input */
    bool bInputEnabled = true;

    /** The custom replicated inputs for project logic */
    float ExtReplicationInputs[ExtReplicationInputCount] = {};
};
