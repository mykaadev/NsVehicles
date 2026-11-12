// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "AI/NsVehicleAI.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"    

DEFINE_LOG_CATEGORY_STATIC(LogNsVehicleAI, Log, All);

UNsVehicleAI::UNsVehicleAI()
{
    PrimaryComponentTick.bCanEverTick = true;

    DriveThrottle = 1.f;
    LookAheadDistance = 150.f;
    FullSteerAngle = 30.f;
    ArriveDistance = 120.f;
    StallSpeed = 5.f;
    StallSeconds = 2.5f;
    ReverseSeconds = 1.5f;
    MaxStallRetries = 4;
    NoProgressSeconds = 6.f;
    NoProgressSpeed = 30.f;
    ProgressDistance = 100.f;
}

void UNsVehicleAI::BeginPlay()
{
    Super::BeginPlay();

    OwnerActor = GetOwner();
    World = GetWorld();

}

void UNsVehicleAI::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bDriving)
    {
        TickDrive(DeltaTime);
    }
}

void UNsVehicleAI::StartDrive(USplineComponent* const InSplineComponent)
{
    if (InSplineComponent == nullptr || GetControls() == nullptr)
    {
        UE_LOG(LogNsVehicleAI, Error, TEXT("StartDrive on %s needs a spline and a controls component on the owner"), *GetNameSafe(GetOwner()));
        return;
    }

    SetTargetSpline(InSplineComponent);
    bDriving = true;
    StallTimer = 0.f;
    ReverseTimer = 0.f;
    StallRetries = 0;
    BestDistance = 0.f;
    NoProgressTimer = 0.f;
}

void UNsVehicleAI::StopDrive()
{
    bDriving = false;
    if (UNsVehicleControlsComponent* const Controls = GetControls())
    {
        Controls->SetThrottleInput(0.f);
        Controls->SetBrakeInput(0.f);
        Controls->SetSteeringInput(0.f);
    }
}

void UNsVehicleAI::TickDrive(const float DeltaTime)
{
    UNsVehicleControlsComponent* const Controls = GetControls();
    if (Controls == nullptr || TargetSpline == nullptr || OwnerActor == nullptr)
    {
        UE_LOG(LogNsVehicleAI, Error, TEXT("The drive mode of %s lost its spline or its controls"), *GetNameSafe(GetOwner()));
        StopDrive();
        return;
    }

    const FVector Location = OwnerActor->GetActorLocation();
    const float Length = TargetSpline->GetSplineLength();
    const float Distance = TargetSpline->GetDistanceAlongSplineAtSplineInputKey(TargetSpline->FindInputKeyClosestToWorldLocation(Location));
    const FVector End = TargetSpline->GetLocationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::World);
    if (FVector::Dist2D(Location, End) < ArriveDistance)
    {
        StopDrive();
        UE_LOG(LogNsVehicleAI, Display, TEXT("The drive mode of %s arrived"), *GetNameSafe(GetOwner()));
        ShowDriveState(TEXT("Arrived"), Length, Length, End, FColor::Green);
        OnDriveArrived.Broadcast();
        return;
    }

    // Pure pursuit: steer to a point a fixed distance ahead on the spline. A positive steering input turns to the right, to a larger yaw
    const FVector Target = TargetSpline->GetLocationAtDistanceAlongSpline(FMath::Min(Distance + LookAheadDistance, Length), ESplineCoordinateSpace::World);
    const float YawError = FRotator::NormalizeAxis((Target - Location).GetSafeNormal2D().Rotation().Yaw - OwnerActor->GetActorRotation().Yaw);
    const float Steering = FMath::Clamp(YawError / FullSteerAngle, -1.f, 1.f);

    // Only a slow owner counts time with no progress. A fast one moves freely, for example back to the spline from the side
    const float Speed = OwnerActor->GetVelocity().Size();
    NoProgressTimer = Speed < NoProgressSpeed ? NoProgressTimer + DeltaTime : 0.f;
    if (Distance > BestDistance + ProgressDistance)
    {
        BestDistance = Distance;
        StallRetries = 0;
        NoProgressTimer = 0.f;
    }

    if (ReverseTimer > 0.f)
    {
        // Backward with the wheels turned the other way, so the next try starts at a new angle
        ReverseTimer -= DeltaTime;
        Controls->SetThrottleInput(0.f);
        Controls->SetBrakeInput(1.f);
        Controls->SetSteeringInput(-Steering);
        ShowDriveState(TEXT("Backing off"), Distance, Length, Target, FColor::Orange);
        return;
    }

    // A vehicle that shakes against a wall keeps some speed, so no progress along the spline also counts as a stall
    StallTimer = Speed < StallSpeed ? StallTimer + DeltaTime : 0.f;
    if (StallTimer > StallSeconds || NoProgressTimer > NoProgressSeconds)
    {
        StallTimer = 0.f;
        NoProgressTimer = 0.f;
        if (++StallRetries > MaxStallRetries)
        {
            StopDrive();
            const FString Blocker = DescribeBlocker();
            UE_LOG(LogNsVehicleAI, Warning, TEXT("The drive mode of %s is stuck at %s: %s"), *GetNameSafe(GetOwner()), *Location.ToString(), *Blocker);
            ShowDriveState(FString::Printf(TEXT("Stuck: %s"), *Blocker), Distance, Length, Target, FColor::Red);
            OnDriveStuck.Broadcast();
            return;
        }
        ReverseTimer = ReverseSeconds;
        return;
    }

    Controls->ApplyHandbrakeInput(false);
    Controls->SetBrakeInput(0.f);
    Controls->SetThrottleInput(DriveThrottle);
    Controls->SetSteeringInput(Steering);
    ShowDriveState(TEXT("Driving"), Distance, Length, Target, FColor::Yellow);
}

FString UNsVehicleAI::DescribeBlocker() const
{
    if (OwnerActor == nullptr || World == nullptr)
    {
        return TEXT("no owner");
    }

    // An actor just ahead at body height blocks the way. With nothing ahead, the slope or a step under the wheels holds the vehicle
    const FVector Location = OwnerActor->GetActorLocation();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NsVehicleAIBlocker), false, OwnerActor);
    if (World->LineTraceSingleByChannel(Hit, Location, Location + OwnerActor->GetActorForwardVector() * LookAheadDistance * 0.5f, ECC_WorldStatic, Params) && Hit.GetActor() != nullptr)
    {
        return FString::Printf(TEXT("blocked by %s"), *Hit.GetActor()->GetActorNameOrLabel());
    }
    return FString::Printf(TEXT("cannot climb, pitch %.0f deg"), FMath::Abs(OwnerActor->GetActorRotation().Pitch));
}

void UNsVehicleAI::ShowDriveState(const FString& State, const float Distance, const float Length, const FVector& Target, const FColor& Color) const
{
#if !UE_BUILD_SHIPPING
    if (!bDrawDebug || OwnerActor == nullptr || GEngine == nullptr)
    {
        return;
    }
    const FString Text = FString::Printf(TEXT("%s: %s | %.1f of %.1f m | %.0f cm/s | pitch %.0f deg | stalls %d of %d"),
        *OwnerActor->GetActorNameOrLabel(), *State, Distance / 100.f, Length / 100.f, OwnerActor->GetVelocity().Size(),
        OwnerActor->GetActorRotation().Pitch, StallRetries, MaxStallRetries);
    // One message line for each component: the key is the unique id, and a final state stays on screen longer
    const bool bFinal = State.StartsWith(TEXT("Stuck")) || State == TEXT("Arrived");
    GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), bFinal ? 30.f : 0.5f, Color, Text);
#if ENABLE_DRAW_DEBUG
    DrawDebugLine(World, OwnerActor->GetActorLocation(), Target, Color, false, 0.f, 0, 2.f);
    DrawDebugSphere(World, Target, 10.f, 8, Color, false, 0.f, 0, 1.f);
#endif
#endif
}

UNsVehicleControlsComponent* UNsVehicleAI::GetControls() const
{
    return GetOwner() != nullptr ? GetOwner()->FindComponentByClass<UNsVehicleControlsComponent>() : nullptr;
}

void UNsVehicleAI::SetTargetSpline(USplineComponent* InSplineComponent)
{
    if (InSplineComponent != nullptr)
    {
        TargetSpline = InSplineComponent;
        Refresh();
    }
}

void UNsVehicleAI::Update(float deltaTime, float& OutSteering)
{
    if (OwnerActor == nullptr)
    {
        return;
    }

    FVector location = OwnerActor->GetActorLocation();

    float key = TargetSpline->FindInputKeyClosestToWorldLocation(location);

    FVector approxPoint = TargetSpline->GetLocationAtSplineInputKey(key, ESplineCoordinateSpace::World);
    FVector direction = TargetSpline->GetDirectionAtSplineInputKey(key, ESplineCoordinateSpace::World).GetSafeNormal();
    FVector up = TargetSpline->GetUpVectorAtSplineInputKey(key, ESplineCoordinateSpace::World).GetSafeNormal();

    const FVector directionVector = location - approxPoint;
    FVector splineVector = FVector::CrossProduct(direction, up);
    splineVector.Normalize();

    LastDelta = LateralError;
    LateralError = FVector::DotProduct(directionVector, splineVector);

    BounceVelocity = (LateralError - LastDelta) * (1.f / deltaTime);
    float DampingForce = -BounceVelocity * Kd2;

    OutSteering = CalculatePIDOutput(LateralError - DampingForce, deltaTime);

    if (bDrawDebug)
    {
        DrawDebugSphere(World, location, 20.0f, 12, FColor::Blue, false, 0.0f, 1, 1.0f);
        DrawDebugSphere(World, approxPoint, 20.0f, 12, FColor::Red, false, 0.0f, 1, 1.0f);
        DrawDebugLine(World, location, approxPoint, FColor::Green, false, 0.0f, 1, 2.0f);
    }
}

void UNsVehicleAI::Refresh()
{
    IntegralError = 0.0f;
    PreviousError = 0.0f;
    bIsFirstUpdate = true;
}

float UNsVehicleAI::CalculatePIDOutput(float Error, float DeltaTime)
{

    if (DeltaTime <= MinDeltaTime)
    {
        if (!FMath::IsNearlyZero(Kd) && !bIsFirstUpdate)
        {
            return FMath::Clamp(Kp * Error, -OutputMax, OutputMax);
        }

        return FMath::Clamp(Kp * Error, -OutputMax, OutputMax);
    }

    float P_Term = Kp * Error;

    // Integral Term
    IntegralError += Error * DeltaTime;
    IntegralError = FMath::Clamp(IntegralError, -MaxIntegral, MaxIntegral);
    float I_Term = Ki * IntegralError;


    float D_Term = 0.0f;
    if (!bIsFirstUpdate)
    {
        float DerivativeError = (Error - PreviousError) / DeltaTime;
        D_Term = Kd * DerivativeError;
    }

    PreviousError = Error;
    bIsFirstUpdate = false;

    float Output = P_Term + I_Term + D_Term;
    return FMath::Clamp(Output, -OutputMax, OutputMax);
}

