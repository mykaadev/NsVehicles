// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/NsVehiclePathfinder.h"
#include "Components/ActorComponent.h"
#include "NsVehicleAI.generated.h"

class USplineComponent;

/** An event of the drive mode */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNsVehicleAIDriveEvent);

/**
 * Makes a steering input that follows a target spline with a PID controller. In the drive mode it also drives the owner along the spline
 * Drive To plans a way with A* and drives it: it follows the waypoints, slows in sharp turns, backs off after a stall, and plans again from where it stands
 */
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

    /** Plans a way to the goal with A* for the profile, then drives it. The owner plans again after each stall. Returns false when there is no way */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI|Drive")
    bool StartDriveTo(FVector Goal, const class UNsVehicleReachProfile* Profile);

    /** Sets the grid, the traces, and the costs of the A* plan of Drive To. The owner and the actors on it are added at each plan */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI|Drive")
    void SetPathQuery(const FNsVehiclePathQuery& InPathQuery) { PathQuery = InPathQuery; }

    /** Returns the waypoints of the current way of Drive To */
    UFUNCTION(BlueprintPure, Category = "NsVehicle AI|Drive")
    const TArray<FVector>& GetWaypoints() const { return Waypoints; }

    /** Returns where the drive mode last gave up */
    UFUNCTION(BlueprintPure, Category = "NsVehicle AI|Drive")
    FVector GetLastStuckLocation() const { return LastStuckLocation; }

    /** Returns why the drive mode last gave up: the actor that blocked it, the slope, or no way left */
    UFUNCTION(BlueprintPure, Category = "NsVehicle AI|Drive")
    FString GetLastStuckCause() const { return LastStuckCause; }

    /** Returns the number of new plans in the current Drive To */
    UFUNCTION(BlueprintPure, Category = "NsVehicle AI|Drive")
    int32 GetReplanCount() const { return Replans; }

    /** Turns the debug drawing and the on-screen state of the drive mode on or off */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle AI")
    void SetDrawDebug(bool bInDrawDebug) { bDrawDebug = bInDrawDebug; }

private:
    /** Returns the PID steering output for a lateral error */
    float CalculatePIDOutput(float Error, float DeltaTime);

    /** Sets the inputs of the drive mode for one tick: steer to a point ahead on the spline, back off after a stall, and stop at the end */
    void TickDrive(float DeltaTime);

    /** Sets the inputs of Drive To for one tick: steer to a point ahead on the waypoints, slow in turns, back off and plan again after a stall */
    void TickWaypoints(float DeltaTime);

    /** Plans a way from a point to the goal of Drive To, around the blocked points. Returns false when there is no way */
    bool PlanWaypoints(const FVector& From);

    /** Returns the distance along the waypoints of the point nearest to a location, near the current progress */
    float ProjectOnWaypoints(const FVector& Location) const;

    /** Returns the point at a distance along the waypoints */
    FVector GetWaypointPointAt(float Distance) const;

    /** Returns the turn of the way in the corner look-ahead after a distance, in degrees */
    float GetTurnAhead(float Distance) const;

    /** Stops the drive mode, keeps the place and the cause, and broadcasts On Drive Stuck */
    void GiveUp(const FVector& Location, const FString& Cause, float Distance, float Length, const FVector& Target);

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

    /** The grid, the traces, and the costs of the A* plan of Drive To */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (AllowPrivateAccess = "true"))
    FNsVehiclePathQuery PathQuery;

    /** The distance ahead on the waypoints of the point that Drive To steers to, at rest. A short distance follows the way closely */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "1.0", Units = "cm", AllowPrivateAccess = "true"))
    float WaypointLookAhead;

    /** The time of travel that adds to the steering distance. A fast owner steers to a point farther ahead, so it does not weave */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0.0", Units = "s", AllowPrivateAccess = "true"))
    float LookAheadTime;

    /** The time of the yaw rate that Drive To takes from the heading error. A higher value turns in more calmly */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0.0", Units = "s", AllowPrivateAccess = "true"))
    float SteerDamping;

    /** The largest change of the steering input in one second */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0.1", AllowPrivateAccess = "true"))
    float SteerRate;

    /** The speed of Drive To on a straight way */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "1.0", Units = "cm/s", AllowPrivateAccess = "true"))
    float MaxDriveSpeed;

    /** The speed of Drive To in a sharp turn and at the goal */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "1.0", Units = "cm/s", AllowPrivateAccess = "true"))
    float CornerSpeed;

    /** The distance ahead on the waypoints in which Drive To looks for a turn and slows before it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "1.0", Units = "cm", AllowPrivateAccess = "true"))
    float CornerLookAhead;

    /** The speed error that gives the full throttle or the full brake */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "1.0", Units = "cm/s", AllowPrivateAccess = "true"))
    float SpeedBand;

    /** The throttle at the target speed, so the owner keeps its speed on flat ground */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
    float CruiseThrottle;

    /** The number of grid points that one straight part of the smoothed way can replace */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "2", AllowPrivateAccess = "true"))
    int32 MaxSmoothSkip;

    /** The distance to the goal at which Drive To stops */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "1.0", Units = "cm", AllowPrivateAccess = "true"))
    float WaypointArriveDistance;

    /** The turn angle ahead at which the target speed is the corner speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "1.0", Units = "deg", AllowPrivateAccess = "true"))
    float SharpTurnAngle;

    /** The number of new plans after stalls before Drive To gives up */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 MaxReplans;

    /** The pitch above which a stall counts as a stall on a slope */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0.0", Units = "deg", AllowPrivateAccess = "true"))
    float SlopeStallPitch;

    /** The part of the slope limit that each stall on a slope takes from the plans of the current Drive To */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0.0", ClampMax = "0.5", AllowPrivateAccess = "true"))
    float SlopeMarginStep;

    /** The lowest slope margin that the stalls on slopes can give */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive To", meta = (ClampMin = "0.1", ClampMax = "1.0", AllowPrivateAccess = "true"))
    float MinSlopeMargin;

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

    /** True while the drive mode follows the waypoints of Drive To, and not a spline */
    bool bWaypointMode = false;

    /** The waypoints of the current way of Drive To */
    TArray<FVector> Waypoints;

    /** The distance along the waypoints at each waypoint */
    TArray<float> WaypointDistances;

    /** The goal of Drive To */
    FVector DriveGoal = FVector::ZeroVector;

    /** The profile of Drive To */
    UPROPERTY(Transient)
    TObjectPtr<const class UNsVehicleReachProfile> DriveProfile;

    /** The slope margin of the path query before the stalls on slopes lowered it */
    float BaseSlopeMargin = 1.f;

    /** True while the stalls on slopes have lowered the slope margin of the current Drive To */
    bool bSlopeMarginLowered = false;

    /** The steering input of Drive To from the last tick */
    float SteeringOutput = 0.f;

    /** The distance along the waypoints that the owner reached */
    float WaypointProgress = 0.f;

    /** The new plans in the current Drive To */
    int32 Replans = 0;

    /** True when a back off ends with a new plan */
    bool bReplanAfterReverse = false;

    /** Where the drive mode last gave up */
    FVector LastStuckLocation = FVector::ZeroVector;

    /** Why the drive mode last gave up */
    FString LastStuckCause;
};
