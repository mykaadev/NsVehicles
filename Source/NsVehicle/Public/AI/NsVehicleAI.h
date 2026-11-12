// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NsVehicleAI.generated.h"

class USplineComponent;

/** An event of the drive mode */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNsVehicleAIDriveEvent);

/** Makes a steering input that follows a target spline with a PID controller. In the drive mode it also drives the owner along the spline */
UCLASS(ClassGroup = ("NsVehicle"), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleAI : public UActorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleAI();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** Calculates the steering input that moves the owner to the target spline */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI")
    void Update(float deltaTime, float& OutSteering);

    /** Sets the spline that the owner follows */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI")
    void SetTargetSpline(USplineComponent* InSplineComponent);

    /** Resets the controller state. Use it when you start to follow a spline again, or change the spline */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI")
    void Refresh();

    /** Starts the drive mode: each tick the component sets the throttle, the brake, and the steering of the owner, to the end of the spline */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI|Drive")
    void StartDrive(USplineComponent* InSplineComponent);

    /** Stops the drive mode and releases the throttle, the brake, and the steering */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI|Drive")
    void StopDrive();

    /** True while the drive mode moves the owner */
    UFUNCTION(BlueprintPure, Category = "NsVehicle AI|Drive")
    bool IsDriving() const { return bDriving; }

    /** Turns the debug drawing and the on-screen state of the drive mode on or off */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI")
    void SetDrawDebug(bool bInDrawDebug) { bDrawDebug = bInDrawDebug; }

private:
    /** Returns the PID steering output for a lateral error */
    float CalculatePIDOutput(float Error, float DeltaTime);

    /** Sets the inputs of the drive mode for one tick: steer to a point ahead on the spline, back off after a stall, and stop at the end */
    void TickDrive(float DeltaTime);

    /** Returns the controls component of the owner, or null */
    class UNsVehicleControlsComponent* GetControls() const;

    /** Returns what holds the owner back: the actor just ahead of it, or the slope under it */
    FString DescribeBlocker() const;

    /** Shows the state of the drive mode on screen and draws the steering point, when Draw Debug is on. Not in Shipping */
    void ShowDriveState(const FString& State, float Distance, float Length, const FVector& Target, const FColor& Color) const;

// Variables
public:
    /** Broadcasts when the drive mode reaches the end of the spline */
    UPROPERTY(BlueprintAssignable, Category = "NsVehicle AI|Drive")
    FNsVehicleAIDriveEvent OnDriveArrived;

    /** Broadcasts when the drive mode gives up after too many stalls in one place */
    UPROPERTY(BlueprintAssignable, Category = "NsVehicle AI|Drive")
    FNsVehicleAIDriveEvent OnDriveStuck;

private:
    /** The proportional gain. It sets how fast the controller responds. A value that is too high causes overshoot */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (ClampMin = "0.0"), meta = (AllowPrivateAccess="true"))
    float Kp = 0.3f;

    /** The derivative gain. It damps the movement and makes it smoother */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (ClampMin = "0.0"), meta = (AllowPrivateAccess = "true"))
    float Kd = 0.2f;

    /** The second derivative gain. It damps the full system, and smooths better than Kd */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (AllowPrivateAccess = "true"))
    float Kd2 = 0.2f;

    /** The integral gain. It corrects a constant offset from the spline */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (ClampMin = "0.0"), meta = (AllowPrivateAccess = "true"))
    float Ki = 0.5f;

    /** The maximum integral error */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (ClampMin = "0.0"), meta = (AllowPrivateAccess = "true"))
    float MaxIntegral = 20.0f;

    /** The maximum steering output */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (AllowPrivateAccess = "true"))
    float OutputMax = 1.0f;

    /** The minimum time step in seconds for the integral and derivative terms */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (ClampMin = "0.0"), meta = (AllowPrivateAccess = "true"))
    float MinDeltaTime = 0.0001f;

    /** Draws the target point and the error */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PID Settings", meta = (ClampMin = "0.0"), meta = (AllowPrivateAccess = "true"))
    bool bDrawDebug = false;

    /** The throttle input of the drive mode */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
    float DriveThrottle;

    /** The distance ahead on the spline of the point that the drive mode steers to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "1.0", Units = "cm", AllowPrivateAccess = "true"))
    float LookAheadDistance;

    /** The angle to the steering point that gives the full steering input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "1.0", Units = "deg", AllowPrivateAccess = "true"))
    float FullSteerAngle;

    /** The distance to the end of the spline at which the drive mode stops */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "1.0", Units = "cm", AllowPrivateAccess = "true"))
    float ArriveDistance;

    /** The speed under which the owner counts as stalled */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.0", Units = "cm/s", AllowPrivateAccess = "true"))
    float StallSpeed;

    /** The time under the stall speed after which the drive mode backs off */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.1", Units = "s", AllowPrivateAccess = "true"))
    float StallSeconds;

    /** The time that the drive mode drives backward after a stall */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.0", Units = "s", AllowPrivateAccess = "true"))
    float ReverseSeconds;

    /** The number of stalls with no progress after which the drive mode stops and broadcasts On Drive Stuck */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 MaxStallRetries;

    /** The time with no progress along the spline after which a slow owner backs off, for example when it shakes against a wall */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.1", Units = "s", AllowPrivateAccess = "true"))
    float NoProgressSeconds;

    /** The speed under which no progress counts as a stall. A faster owner, for example one that drives back to the spline from the side, does not back off */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.0", Units = "cm/s", AllowPrivateAccess = "true"))
    float NoProgressSpeed;

    /** The gain along the spline that counts as progress and resets the stall count */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "1.0", Units = "cm", AllowPrivateAccess = "true"))
    float ProgressDistance;

    /** The spline that the owner follows */
    USplineComponent* TargetSpline;

    /** The owner actor, cached in Begin Play */
    AActor* OwnerActor;

    /** The world, cached in Begin Play */
    UWorld* World;

    /** True until the first update after a reset */
    bool bIsFirstUpdate = true;

    /** The rate of change of the lateral error */
    float BounceVelocity = 0.f;

    /** The lateral error from the previous update */
    float LastDelta = 0.f;

    /** The distance from the owner to the spline */
    float LateralError = 0.f;

    /** The accumulated error for the integral term */
    float IntegralError = 0.0f;

    /** The error from the previous update */
    float PreviousError = 0.0f;

    /** True while the drive mode moves the owner */
    bool bDriving = false;

    /** The time that the owner is under the stall speed */
    float StallTimer = 0.f;

    /** The time left to drive backward after a stall */
    float ReverseTimer = 0.f;

    /** The stalls since the last progress */
    int32 StallRetries = 0;

    /** The farthest distance along the spline that the owner reached */
    float BestDistance = 0.f;

    /** The time since the last progress along the spline */
    float NoProgressTimer = 0.f;
};
