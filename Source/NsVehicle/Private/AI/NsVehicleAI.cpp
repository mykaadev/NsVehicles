// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "AI/NsVehicleAI.h"
#include "AI/NsVehicleReachProfile.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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
    WaypointLookAhead = 60.f;
    WaypointArriveDistance = 40.f;
    LookAheadTime = 0.35f;
    SteerDamping = 0.12f;
    SteerRate = 3.f;
    MaxDriveSpeed = 300.f;
    CornerSpeed = 100.f;
    CornerLookAhead = 250.f;
    SpeedBand = 120.f;
    CruiseThrottle = 0.3f;
    MaxSmoothSkip = 12;
    SharpTurnAngle = 60.f;
    MaxReplans = 6;
    SlopeStallPitch = 10.f;
    SlopeMarginStep = 0.05f;
    MinSlopeMargin = 0.7f;
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

    if (bDriving && bWaypointMode)
    {
        TickWaypoints(DeltaTime);
    }
    else if (bDriving)
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
    bWaypointMode = false;
    bDriving = true;
    StallTimer = 0.f;
    ReverseTimer = 0.f;
    StallRetries = 0;
    BestDistance = 0.f;
    NoProgressTimer = 0.f;
}

bool UNsVehicleAI::StartDriveTo(const FVector Goal, const UNsVehicleReachProfile* const Profile)
{
    if (Profile == nullptr || GetControls() == nullptr || GetOwner() == nullptr)
    {
        UE_LOG(LogNsVehicleAI, Error, TEXT("StartDriveTo on %s needs a profile and a controls component on the owner"), *GetNameSafe(GetOwner()));
        return false;
    }

    OwnerActor = GetOwner();
    World = GetWorld();
    DriveGoal = Goal;
    DriveProfile = Profile;
    Replans = 0;
    DriveStalls = 0;
    LastStuckCause.Reset();
    LastDriveResult.Reset();
    PathQuery.BlockedPoints.Reset();
    DriveLog.Reset();
    LastDriveStart = OwnerActor->GetActorLocation();
    DriveStartTime = World != nullptr ? World->GetTimeSeconds() : 0.0;
    AddDriveLog(FString::Printf(TEXT("Start at %s to %s with %s, slope margin %.2f"), *LastDriveStart.ToCompactString(), *Goal.ToCompactString(), *Profile->GetName(), PathQuery.SlopeMargin));
    if (!PlanWaypoints(OwnerActor->GetActorLocation()))
    {
        LastStuckLocation = OwnerActor->GetActorLocation();
        LastStuckCause = TEXT("no way to the goal");
        LastDriveResult = TEXT("no way");
        AddDriveLog(TEXT("No way to the goal"));
        return false;
    }

    bWaypointMode = true;
    bDriving = true;
    StallTimer = 0.f;
    ReverseTimer = 0.f;
    StallRetries = 0;
    NoProgressTimer = 0.f;
    bReplanAfterReverse = false;
    SteeringOutput = 0.f;
    return true;
}

bool UNsVehicleAI::PlanWaypoints(const FVector& From)
{
    // The plan ignores the owner and the actors on it, for example the driver, and its sweeps use the collision of the owner
    PathQuery.Vehicle = OwnerActor;
    TArray<AActor*> Attached;
    OwnerActor->GetAttachedActors(Attached, true, true);
    PathQuery.IgnoredActors.Reset();
    for (AActor* const Actor : Attached)
    {
        PathQuery.IgnoredActors.Add(Actor);
    }

    const TArray<FVector> Path = UNsVehiclePathfinder::FindPath(OwnerActor, From, DriveGoal, DriveProfile, PathQuery);
    if (Path.Num() < 2)
    {
        return false;
    }

    Waypoints = UNsVehiclePathfinder::SmoothPath(OwnerActor, Path, DriveProfile, PathQuery, MaxSmoothSkip);
    AddDriveLog(FString::Printf(TEXT("Plan from %s: %d grid points, %d after smoothing"), *From.ToCompactString(), Path.Num(), Waypoints.Num()));
    WaypointDistances.Reset(Waypoints.Num());
    WaypointDistances.Add(0.f);
    for (int32 Index = 1; Index < Waypoints.Num(); ++Index)
    {
        WaypointDistances.Add(WaypointDistances.Last() + FVector::Dist(Waypoints[Index - 1], Waypoints[Index]));
    }
    WaypointProgress = 0.f;
    BestDistance = 0.f;
    return true;
}

float UNsVehicleAI::ProjectOnWaypoints(const FVector& Location) const
{
    // The search stays near the current progress, so a way that loops back near itself does not make the progress jump
    float BestProgress = WaypointProgress;
    float BestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Index = 1; Index < Waypoints.Num(); ++Index)
    {
        if (WaypointDistances[Index] < WaypointProgress - WaypointLookAhead || WaypointDistances[Index - 1] > WaypointProgress + WaypointLookAhead * 4.f)
        {
            continue;
        }
        const FVector Closest = FMath::ClosestPointOnSegment(Location, Waypoints[Index - 1], Waypoints[Index]);
        const float DistanceSquared = FVector::DistSquared2D(Location, Closest);
        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestProgress = WaypointDistances[Index - 1] + FVector::Dist(Waypoints[Index - 1], Closest);
        }
    }
    return BestProgress;
}

FVector UNsVehicleAI::GetWaypointPointAt(const float Distance) const
{
    for (int32 Index = 1; Index < Waypoints.Num(); ++Index)
    {
        if (WaypointDistances[Index] >= Distance)
        {
            const float Segment = FMath::Max(WaypointDistances[Index] - WaypointDistances[Index - 1], 0.01f);
            return FMath::Lerp(Waypoints[Index - 1], Waypoints[Index], (Distance - WaypointDistances[Index - 1]) / Segment);
        }
    }
    return Waypoints.Last();
}

float UNsVehicleAI::GetTurnAhead(const float Distance) const
{
    // The heading change between the first half and the second half of the look-ahead
    const FVector A = GetWaypointPointAt(Distance);
    const FVector B = GetWaypointPointAt(Distance + CornerLookAhead * 0.5f);
    const FVector C = GetWaypointPointAt(Distance + CornerLookAhead);
    if (FVector::DistSquared2D(A, B) < 1.f || FVector::DistSquared2D(B, C) < 1.f)
    {
        return 0.f;
    }
    return FMath::Abs(FRotator::NormalizeAxis((C - B).GetSafeNormal2D().Rotation().Yaw - (B - A).GetSafeNormal2D().Rotation().Yaw));
}

void UNsVehicleAI::TickWaypoints(const float DeltaTime)
{
    UNsVehicleControlsComponent* const Controls = GetControls();
    if (Controls == nullptr || OwnerActor == nullptr || Waypoints.Num() < 2)
    {
        UE_LOG(LogNsVehicleAI, Error, TEXT("Drive To of %s lost its waypoints or its controls"), *GetNameSafe(GetOwner()));
        StopDrive();
        return;
    }

    const FVector Location = OwnerActor->GetActorLocation();
    const float Length = WaypointDistances.Last();
    if (FVector::Dist2D(Location, Waypoints.Last()) < WaypointArriveDistance)
    {
        LastDriveResult = TEXT("arrived");
        AddDriveLog(FString::Printf(TEXT("Arrived after %d new plans and %d stalls"), Replans, DriveStalls));
        StopDrive();
        UE_LOG(LogNsVehicleAI, Display, TEXT("Drive To of %s arrived after %d new plans"), *GetNameSafe(GetOwner()), Replans);
        ShowDriveState(TEXT("Arrived"), Length, Length, Waypoints.Last(), FColor::Green);
        OnDriveArrived.Broadcast();
        return;
    }

    // The steering point moves ahead with the speed, and the yaw rate damps the steering, so a fast owner does not weave
    const FVector Velocity = OwnerActor->GetVelocity();
    const float ForwardSpeed = FVector::DotProduct(Velocity, OwnerActor->GetActorForwardVector());
    const UPrimitiveComponent* const Body = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent());
    const float YawRate = Body != nullptr && Body->IsSimulatingPhysics() ? Body->GetPhysicsAngularVelocityInDegrees().Z : 0.f;
    WaypointProgress = FMath::Max(WaypointProgress, ProjectOnWaypoints(Location));
    const float LookAhead = WaypointLookAhead + FMath::Max(ForwardSpeed, 0.f) * LookAheadTime;
    const FVector Target = GetWaypointPointAt(FMath::Min(WaypointProgress + LookAhead, Length));
    const float YawError = FRotator::NormalizeAxis((Target - Location).GetSafeNormal2D().Rotation().Yaw - OwnerActor->GetActorRotation().Yaw);
    const float Steering = FMath::Clamp((YawError - YawRate * SteerDamping) / FullSteerAngle, -1.f, 1.f);
    SteeringOutput = FMath::FInterpConstantTo(SteeringOutput, Steering, DeltaTime, SteerRate);

#if ENABLE_DRAW_DEBUG
    if (bDrawDebug)
    {
        for (int32 Index = 1; Index < Waypoints.Num(); ++Index)
        {
            if (WaypointDistances[Index] > WaypointProgress)
            {
                DrawDebugLine(World, Waypoints[Index - 1] + FVector(0.f, 0.f, 5.f), Waypoints[Index] + FVector(0.f, 0.f, 5.f), FColor::Green, false, 0.f, 0, 3.f);
            }
        }
    }
#endif

    const float Speed = Velocity.Size();
    NoProgressTimer = Speed < NoProgressSpeed ? NoProgressTimer + DeltaTime : 0.f;
    if (WaypointProgress > BestDistance + ProgressDistance)
    {
        BestDistance = WaypointProgress;
        StallRetries = 0;
        NoProgressTimer = 0.f;
    }

    if (ReverseTimer > 0.f)
    {
        // Backward with the wheels turned the other way. At the end the owner plans again from where it stands, around the place of the stall
        ReverseTimer -= DeltaTime;
        Controls->SetThrottleInput(0.f);
        Controls->SetBrakeInput(1.f);
        SteeringOutput = -Steering;
        Controls->SetSteeringInput(SteeringOutput);
        ShowDriveState(TEXT("Backing off"), WaypointProgress, Length, Target, FColor::Orange);
        if (ReverseTimer <= 0.f && bReplanAfterReverse)
        {
            bReplanAfterReverse = false;
            if (!PlanWaypoints(Location))
            {
                GiveUp(Location, TEXT("no way left around the stalls"), WaypointProgress, Length, Target);
            }
        }
        return;
    }

    // A vehicle that shakes against a wall keeps some speed, so no progress also counts as a stall
    StallTimer = Speed < StallSpeed ? StallTimer + DeltaTime : 0.f;
    if (StallTimer > StallSeconds || NoProgressTimer > NoProgressSeconds)
    {
        StallTimer = 0.f;
        NoProgressTimer = 0.f;
        ++DriveStalls;
        AddDriveLog(FString::Printf(TEXT("Stall at %s, pitch %.0f deg, speed %.0f cm/s: %s"), *Location.ToCompactString(), FMath::Abs(OwnerActor->GetActorRotation().Pitch), Speed, *DescribeBlocker()));
        if (++Replans > MaxReplans)
        {
            GiveUp(Location, DescribeBlocker(), WaypointProgress, Length, Target);
            return;
        }
        // The next plan avoids the place just ahead of the stall
        PathQuery.BlockedPoints.Add(Location + OwnerActor->GetActorForwardVector() * WaypointLookAhead);

        // A stall on a slope means that the owner cannot climb what the plan allowed here, so the next plans use less of the slope limit
        if (FMath::Abs(OwnerActor->GetActorRotation().Pitch) > SlopeStallPitch && PathQuery.SlopeMargin > MinSlopeMargin)
        {
            if (!bSlopeMarginLowered)
            {
                BaseSlopeMargin = PathQuery.SlopeMargin;
                bSlopeMarginLowered = true;
            }
            PathQuery.SlopeMargin = FMath::Max(PathQuery.SlopeMargin - SlopeMarginStep, MinSlopeMargin);
            UE_LOG(LogNsVehicleAI, Display, TEXT("Drive To of %s stalled on a slope at %s, pitch %.0f deg. The next plans use a slope margin of %.2f"), *GetNameSafe(GetOwner()), *Location.ToString(), FMath::Abs(OwnerActor->GetActorRotation().Pitch), PathQuery.SlopeMargin);
            AddDriveLog(FString::Printf(TEXT("Slope margin lowered to %.2f"), PathQuery.SlopeMargin));
        }
        ReverseTimer = ReverseSeconds;
        bReplanAfterReverse = true;
        return;
    }

    // The target speed drops before a turn and near the goal. Under it the throttle rises, and well over it the brake slows the owner
    const float TurnPart = FMath::Clamp(FMath::Max(GetTurnAhead(WaypointProgress), FMath::Abs(YawError)) / SharpTurnAngle, 0.f, 1.f);
    const float TargetSpeed = FMath::Min(FMath::Lerp(MaxDriveSpeed, CornerSpeed, TurnPart), CornerSpeed + (Length - WaypointProgress));
    const float SpeedError = TargetSpeed - ForwardSpeed;
    const bool bBrake = ForwardSpeed > TargetSpeed + SpeedBand * 0.5f;
    Controls->ApplyHandbrakeInput(false);
    Controls->SetBrakeInput(bBrake ? FMath::Clamp(-SpeedError / SpeedBand, 0.f, 1.f) : 0.f);
    Controls->SetThrottleInput(bBrake ? 0.f : DriveThrottle * FMath::Clamp(CruiseThrottle + SpeedError / SpeedBand, 0.f, 1.f));
    Controls->SetSteeringInput(SteeringOutput);
    ShowDriveState(TEXT("Driving"), WaypointProgress, Length, Target, FColor::Yellow);
}

void UNsVehicleAI::AddDriveLog(const FString& Line)
{
    const double Seconds = World != nullptr ? World->GetTimeSeconds() - DriveStartTime : 0.0;
    DriveLog.Add(FString::Printf(TEXT("%6.1f s  %s"), Seconds, *Line));
}

FString UNsVehicleAI::GetDriveStatus() const
{
    if (DriveLog.IsEmpty())
    {
        return TEXT("No drive yet");
    }
    const FString State = bDriving ? (ReverseTimer > 0.f ? TEXT("Backing off") : TEXT("Driving")) : LastDriveResult;
    const FVector Location = OwnerActor != nullptr ? OwnerActor->GetActorLocation() : LastDriveStart;
    const float Left = FVector::Dist2D(Location, DriveGoal) * 0.01f;
    return FString::Printf(TEXT("%s | %d new plans | %d stalls | slope margin %.2f | %.0f m left%s%s"), *State, Replans, DriveStalls, PathQuery.SlopeMargin, Left,
        LastStuckCause.IsEmpty() ? TEXT("") : TEXT(" | last cause: "), *LastStuckCause);
}

FString UNsVehicleAI::SaveDriveReport() const
{
#if UE_BUILD_SHIPPING
    return FString();
#else
    if (DriveLog.IsEmpty())
    {
        return FString();
    }
    TArray<FString> Lines;
    Lines.Add(FString::Printf(TEXT("# Drive report %s"), *GetNameSafe(GetOwner())));
    Lines.Add(FString());
    Lines.Add(FString::Printf(TEXT("Date: %s. Profile: %s. Result: %s"), *FDateTime::Now().ToString(), *GetNameSafe(DriveProfile), LastDriveResult.IsEmpty() ? TEXT("driving") : *LastDriveResult));
    Lines.Add(FString::Printf(TEXT("Start: %s. Goal: %s"), *LastDriveStart.ToCompactString(), *DriveGoal.ToCompactString()));
    Lines.Add(FString());
    Lines.Add(TEXT("```"));
    Lines.Append(DriveLog);
    Lines.Add(TEXT("```"));
    const FString Folder = FPaths::ProjectSavedDir() / TEXT("QA");
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Path = FPaths::ConvertRelativePathToFull(Folder / FString::Printf(TEXT("Drive_%s.md"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))));
    if (!FFileHelper::SaveStringArrayToFile(Lines, *Path))
    {
        UE_LOG(LogNsVehicleAI, Error, TEXT("SaveDriveReport could not write %s"), *Path);
        return FString();
    }
    UE_LOG(LogNsVehicleAI, Display, TEXT("Drive report: %s"), *Path);
    return Path;
#endif
}

bool UNsVehicleAI::RepeatLastDrive(const bool bFromSameStart)
{
    if (DriveProfile == nullptr || DriveLog.IsEmpty())
    {
        return false;
    }
    if (bFromSameStart && GetOwner() != nullptr)
    {
        StopDrive();
        AActor* const Owner = GetOwner();
        const FVector Toward = (DriveGoal - LastDriveStart).GetSafeNormal2D();
        Owner->SetActorLocationAndRotation(LastDriveStart + FVector(0.f, 0.f, 20.f), FRotator(0.f, Toward.Rotation().Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
        if (UPrimitiveComponent* const Body = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
        {
            Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
            Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        }
    }
    return StartDriveTo(DriveGoal, DriveProfile);
}

void UNsVehicleAI::GiveUp(const FVector& Location, const FString& Cause, const float Distance, const float Length, const FVector& Target)
{
    LastDriveResult = FString::Printf(TEXT("stuck: %s"), *Cause);
    AddDriveLog(FString::Printf(TEXT("Gave up at %s: %s"), *Location.ToCompactString(), *Cause));
    StopDrive();
    LastStuckLocation = Location;
    LastStuckCause = Cause;
    UE_LOG(LogNsVehicleAI, Warning, TEXT("The drive mode of %s is stuck at %s: %s"), *GetNameSafe(GetOwner()), *Location.ToString(), *Cause);
    ShowDriveState(FString::Printf(TEXT("Stuck: %s"), *Cause), Distance, Length, Target, FColor::Red);
    OnDriveStuck.Broadcast();
}

void UNsVehicleAI::StopDrive()
{
    if (bDriving && bWaypointMode && LastDriveResult.IsEmpty())
    {
        LastDriveResult = TEXT("stopped");
        AddDriveLog(TEXT("Stopped from outside"));
    }
    bDriving = false;
    ReverseTimer = 0.f;
    if (bSlopeMarginLowered)
    {
        PathQuery.SlopeMargin = BaseSlopeMargin;
        bSlopeMarginLowered = false;
    }
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
            GiveUp(Location, DescribeBlocker(), Distance, Length, Target);
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
    const FString Tries = bWaypointMode ? FString::Printf(TEXT("plans %d of %d"), Replans, MaxReplans) : FString::Printf(TEXT("stalls %d of %d"), StallRetries, MaxStallRetries);
    const FString Text = FString::Printf(TEXT("%s: %s | %.1f of %.1f m | %.0f cm/s | pitch %.0f deg | %s"),
        *OwnerActor->GetActorNameOrLabel(), *State, Distance / 100.f, Length / 100.f, OwnerActor->GetVelocity().Size(),
        OwnerActor->GetActorRotation().Pitch, *Tries);
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

