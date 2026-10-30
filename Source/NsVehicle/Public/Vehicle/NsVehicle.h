// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AsyncTickPawn.h"
#include "Vehicle/NsVehicleTypes.h"
#include "NsVehicle.generated.h"

class UNsVehicleAerodynamicsComponent;
class UNsVehicleAirControlComponent;
class UNsVehicleAnimationComponent;
class UNsVehicleAntiGravityComponent;
class UNsVehicleArticulatedComponent;
class UNsVehicleBodyDynamicsComponent;
class UNsVehicleBrakeComponent;
class UNsVehicleControlsComponent;
class UNsVehicleDrivetrainComponent;
class UNsVehicleEngineComponent;
class UNsVehicleIdleLockComponent;
class UNsVehicleInteriorComponent;
class UNsVehicleNetworkComponent;
class UNsVehicleSteeringComponent;
class UNsVehicleSuspensionComponent;
class UNsVehicleTransmissionComponent;

/**
 * Physics vehicle pawn. It owns the physics state and runs the vehicle modules in a fixed order.
 * Each system is a module component: Controls, Engine, Transmission, Drivetrain, Brakes, Steering,
 * BodyDynamics, Aerodynamics, AirControl, AntiGravity, IdleLock, Network, and Animation
 */
UCLASS()
class NSVEHICLE_API ANsVehicle : public AAsyncTickPawn
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    ANsVehicle();

    //~ Begin AActor Interface
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    //~ End AActor Interface

    //~ Begin AAsyncTickPawn Interface
    virtual void NativeAsyncTick(float DeltaTime) override;
    //~ End AAsyncTickPawn Interface

    /** Returns the driver input module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleControlsComponent* GetControls() const;

    /** Returns the engine module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleEngineComponent* GetEngine() const;

    /** Returns the transmission module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleTransmissionComponent* GetTransmission() const;

    /** Returns the drivetrain module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleDrivetrainComponent* GetDrivetrain() const;

    /** Returns the brake module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleBrakeComponent* GetBrakes() const;

    /** Returns the steering module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleSteeringComponent* GetSteering() const;

    /** Returns the body dynamics module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleBodyDynamicsComponent* GetBodyDynamics() const;

    /** Returns the aerodynamics module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleAerodynamicsComponent* GetAerodynamics() const;

    /** Returns the air control module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleAirControlComponent* GetAirControl() const;

    /** Returns the anti gravity module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleAntiGravityComponent* GetAntiGravity() const;

    /** Returns the idle lock module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleIdleLockComponent* GetIdleLock() const;

    /** Returns the network module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleNetworkComponent* GetNetwork() const;

    /** Returns the take recorder animation module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Modules")
    UNsVehicleAnimationComponent* GetAnimation() const;

    /** Adds an upward impulse when the vehicle is on the ground. An amount near the vehicle mass works well */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle")
    void Jump(float Amount);

    /** Wakes the rigid bodies of the vehicle. Call it often when Auto Wake Rigid Bodies is off */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle")
    void RTWakeRigidBodies();

    /** Overrides the airborne state until the next physics step */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle")
    void SetInAir(bool bAirborne);

    /** Returns true when the vehicle is airborne */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    bool IsInAir() const;

    /** Returns the forward speed in KPH */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetSpeedKPH() const;

    /** Returns the local forward velocity in m/s from the physics body */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetLinearVelocity() const;

    /** Returns the local velocity vector in m/s from the physics body */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    FVector GetVelocityVector() const;

    /** Returns the physics transform from the last async physics step */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    FTransform GetPhysicsTransform() const;

    /** Returns the angle in degrees between the forward direction and the velocity. Use it in Event Async Tick only */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetSideSlipAngle() const;

    /** Returns the angle in degrees between the forward direction and the velocity. Use it in Event Tick only */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetSideSlipAngleGT() const;

    /** Returns the root physics body of the vehicle */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    UPrimitiveComponent* GetPrimitiveRoot() const;

    /** Returns the forward acceleration in g */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetLongitudinalG() const;

    /** Returns the sideways acceleration in g */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetLateralG() const;

    /** Returns the upward acceleration in g */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetVerticalG() const;

    /** Returns the local yaw angular velocity in rad/s */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetAngularVelocity() const;

    /** Returns the local yaw angular acceleration in rad/s^2 */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetAngularAcceleration() const;

    /** Returns the suspension components that the vehicle updates */
    TArray<UNsVehicleSuspensionComponent*> GetSuspension() const;

    /** Replaces the suspension components that the vehicle updates. The animation module uses it */
    void SetSuspensionComponents(const TArray<UNsVehicleSuspensionComponent*>& NewSuspensions);

    /** Returns the forward speed in m/s from the last physics step */
    float GetLinearSpeed() const;

    /** Copies the forward speed in m/s from the server on a client */
    void SetReplicatedSpeed(float NewSpeed);

private:
    /** Sets the airborne state from the wheel contacts, and averages the drivetrain slip */
    void UpdateGroundContact();

    /** Updates the local yaw angular velocity and acceleration */
    void UpdateAngularMotion();

    /** Applies the powertrain and chassis forces for one physics step */
    void UpdatePhysicsCore(float DeltaTime);

    /** Applies the standard powertrain forces and updates the RPM and the gearbox. Returns false when the engine cannot make torque */
    bool UpdateStandardPowertrain(float DeltaTime, const FVector& Forward, const FVector& Right);

    /** Applies the arcade powertrain forces */
    void UpdateArcadePowertrain(const FVector& Forward, const FVector& Right, float VelocityDirection);

    /** Updates the local velocity, the speed, and the acceleration from the physics body */
    void UpdateMotionState(float DeltaTime);

    /** Sends the drive torque and the inputs to the suspension components */
    void UpdateWheelPhysics(float DeltaTime);

    /** Steers, places, and spins the wheels, and updates the interior and articulated parts on the game thread */
    void UpdateAttachedComponents(float DeltaTime);

// Variables
protected:
    /** Holds the driver input and sends it to the server */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleControlsComponent> Controls;

    /** Makes the engine torque and the RPM */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleEngineComponent> Engine;

    /** Owns the gears and the automatic gearbox */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleTransmissionComponent> Transmission;

    /** Removes power when the driven wheels slip */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleDrivetrainComponent> Drivetrain;

    /** Owns the brakes, the reverse drive, and the handbrake */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleBrakeComponent> Brakes;

    /** Limits the steering input and turns the steering wheels */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleSteeringComponent> Steering;

    /** Adds the body pitch and the bike lean */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleBodyDynamicsComponent> BodyDynamics;

    /** Adds the air drag */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleAerodynamicsComponent> Aerodynamics;

    /** Turns the vehicle in the air */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleAirControlComponent> AirControl;

    /** Pulls the vehicle to the surface under it */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleAntiGravityComponent> AntiGravity;

    /** Holds the vehicle in place when it stops */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleIdleLockComponent> IdleLock;

    /** Replicates the vehicle state */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleNetworkComponent> Network;

    /** Spins the wheels for the take recorder */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NsVehicle|Modules")
    TObjectPtr<UNsVehicleAnimationComponent> Animation;

    /** The number of wheels off the ground that makes the vehicle airborne. An airborne vehicle gets no drive force */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Physics")
    int32 InAirWheelCount = 2;

    /** Wakes the rigid bodies every frame. Turn it off for many replicated vehicles, and call RTWakeRigidBodies instead */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Physics")
    bool bAutoWakeRigidBodies = true;

private:
    /** The suspension components that the vehicle updates. The actor owns them */
    TArray<UNsVehicleSuspensionComponent*> SuspensionComponents;

    /** The interior components that the vehicle updates. The actor owns them */
    TArray<UNsVehicleInteriorComponent*> InteriorComponents;

    /** The articulated components that the vehicle updates. The actor owns them */
    TArray<UNsVehicleArticulatedComponent*> ArticulatedComponents;

    /** The root physics body of the vehicle. The actor owns it */
    UPrimitiveComponent* PrimitiveComponent = nullptr;

    /** The physics transform from the last async physics step */
    FTransform PhysicsTransform;

    /** The local velocity in cm/s from the previous physics step */
    FVector PreviousLocalVelocity = FVector::ZeroVector;

    /** The local velocity in cm/s from the last physics step */
    FVector LocalVelocity = FVector::ZeroVector;

    /** The local acceleration in m/s^2 */
    FVector Acceleration = FVector::ZeroVector;

    /** The local velocity in cm/s on the game thread */
    FVector LocalVelocityGT = FVector::ZeroVector;

    /** The local yaw angular velocity in rad/s */
    float AngularVelocity = 0.f;

    /** The local yaw angular velocity from the previous physics step */
    float LastAngularVelocity = 0.f;

    /** The local yaw angular acceleration in rad/s^2 */
    float AngularAcceleration = 0.f;

    /** The time in seconds of the last async physics step */
    float AsyncDeltaTime = 0.f;

    /** The forward speed in m/s */
    float Speed = 0.f;

    /** True when the vehicle is airborne */
    bool bInAir = false;
};
