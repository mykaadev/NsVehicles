// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NsVehicleAI.generated.h"

class USplineComponent;

/** Makes a steering input that follows a target spline with a PID controller */
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

private:
    /** Returns the PID steering output for a lateral error */
    float CalculatePIDOutput(float Error, float DeltaTime);

// Variables
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
};
