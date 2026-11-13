// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "AI/NsVehiclePathfinder.h"

#include "AI/NsVehicleReachProfile.h"
#include "Algo/Reverse.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogNsVehiclePath, Log, All);

FNsVehiclePathQuery::FNsVehiclePathQuery()
{
    GridSpacing = 50.f;
    MaxCells = 400000;
    TraceHeight = 300.f;
    TraceChannel = ECC_WorldStatic;
    WaterClassNameFilter = TEXT("WaterBody");
    SlopeMargin = 0.9f;
    SlopeCost = 2.f;
    EdgeCost = 4.f;
    BlockedRadius = 60.f;
    Clearance = 10.f;
    WallAngle = 50.f;
}

FNsVehiclePointFilter::FNsVehiclePointFilter()
{
    bLandscapeOnly = false;
    bAllowUnderWater = false;
    LandscapeClassNameFilter = TEXT("Landscape");
    MaxTries = 64;
}

bool UNsVehiclePathfinder::FindRandomPoint(const UObject* const WorldContextObject, const FVector Center, const float Radius, const FNsVehiclePointFilter& Filter, const FNsVehiclePathQuery& Query, FVector& OutPoint)
{
    const UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    if (World == nullptr)
    {
        UE_LOG(LogNsVehiclePath, Error, TEXT("FindRandomPoint needs a world"));
        return false;
    }

    for (int32 Try = 0; Try < Filter.MaxTries; ++Try)
    {
        const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
        const float Distance = Radius * FMath::Sqrt(FMath::FRand());
        const FVector Above(Center.X + Distance * FMath::Cos(Angle), Center.Y + Distance * FMath::Sin(Angle), Center.Z + RandomSearchHeight);

        // A water surface can block the trace. Under water allowed, the trace goes through the water to the bed
        FNsVehiclePathQuery TraceQuery = Query;
        FVector Ground = FVector::ZeroVector;
        bool bFound = false;
        bool bInWater = false;
        for (int32 Layer = 0; Layer < 3 && !bFound; ++Layer)
        {
            if (!FindGround(World, Above, RandomSearchHeight * 2.f, TraceQuery, Ground))
            {
                break;
            }
            AActor* const GroundActor = FindGroundActor(World, Ground, TraceQuery);
            const bool bWaterActor = GroundActor != nullptr && !Query.WaterClassNameFilter.IsEmpty() && GroundActor->GetClass()->GetName().Contains(Query.WaterClassNameFilter);
            if (bWaterActor)
            {
                bInWater = true;
                TraceQuery.IgnoredActors.Add(GroundActor);
                continue;
            }
            bInWater = bInWater || IsWater(World, Ground, Query);
            const bool bLandscape = GroundActor != nullptr && GroundActor->GetClass()->GetName().Contains(Filter.LandscapeClassNameFilter);
            bFound = (!Filter.bLandscapeOnly || bLandscape) && (Filter.bAllowUnderWater || !bInWater);
            if (!bFound)
            {
                break;
            }
        }
        if (bFound)
        {
            OutPoint = Ground;
            return true;
        }
    }
    return false;
}

TArray<FVector> UNsVehiclePathfinder::FindPath(const UObject* const WorldContextObject, const FVector Start, const FVector Goal, const UNsVehicleReachProfile* const Profile, const FNsVehiclePathQuery& Query)
{
    TArray<FVector> Path;
    const UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    if (World == nullptr || Profile == nullptr)
    {
        UE_LOG(LogNsVehiclePath, Error, TEXT("FindPath needs a world and a profile"));
        return Path;
    }

    const float Spacing = Query.GridSpacing;
    FVector StartGround;
    if (!FindGround(World, Start + FVector(0.f, 0.f, Query.TraceHeight), Start.Z + Query.TraceHeight + 1000000.f, Query, StartGround))
    {
        UE_LOG(LogNsVehiclePath, Warning, TEXT("FindPath found no ground under the start %s"), *Start.ToString());
        return Path;
    }

    // The grid aligns to the world, so one place gives one way from any start point near it, in the editor and in a game
    // The start joins the nearest grid point that it can drive to. When it reaches none, the grid starts at the start point
    FVector GridOrigin = StartGround;
    FVector OriginGround = StartGround;
    const float CellX = FMath::FloorToFloat(Start.X / Spacing) * Spacing;
    const float CellY = FMath::FloorToFloat(Start.Y / Spacing) * Spacing;
    TArray<FVector2D> Corners = { { CellX, CellY }, { CellX + Spacing, CellY }, { CellX, CellY + Spacing }, { CellX + Spacing, CellY + Spacing } };
    Corners.Sort([&Start](const FVector2D& A, const FVector2D& B) { return FVector2D::DistSquared(A, FVector2D(Start)) < FVector2D::DistSquared(B, FVector2D(Start)); });
    for (const FVector2D& Corner : Corners)
    {
        FVector CornerGround;
        if (FindGround(World, FVector(Corner.X, Corner.Y, StartGround.Z + Query.TraceHeight), Query.TraceHeight + Profile->MaxDropHeight, Query, CornerGround)
            && CanStep(World, *Profile, StartGround, CornerGround, Query, true))
        {
            GridOrigin = FVector(Corner.X, Corner.Y, CornerGround.Z);
            OriginGround = CornerGround;
            break;
        }
    }

    // Each cell keeps the ground height where the vehicle stood
    const FIntPoint GoalCell(FMath::RoundToInt((Goal.X - GridOrigin.X) / Spacing), FMath::RoundToInt((Goal.Y - GridOrigin.Y) / Spacing));
    const auto Heuristic = [GoalCell, Spacing](const FIntPoint& Cell) { return static_cast<float>(FVector2D(Cell - GoalCell).Size()) * Spacing; };
    struct FOpenCell
    {
        FIntPoint Cell;
        float Cost;
    };
    const auto ByCost = [](const FOpenCell& A, const FOpenCell& B) { return A.Cost < B.Cost; };

    TMap<FIntPoint, float> GroundZ;
    TMap<FIntPoint, float> CostSoFar;
    TMap<FIntPoint, FIntPoint> CameFrom;
    TArray<FOpenCell> Open;
    const FIntPoint Origin(0, 0);
    GroundZ.Add(Origin, OriginGround.Z);
    CostSoFar.Add(Origin, 0.f);
    Open.HeapPush({ Origin, Heuristic(Origin) }, ByCost);

    const float SlopeTan = FMath::Max(FMath::Tan(FMath::DegreesToRadians(Profile->MaxSlopeDegrees * Query.SlopeMargin)), 0.01f);
    static const FIntPoint Neighbours[] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
    bool bFound = false;
    FIntPoint Reached = Origin;
    int32 Expanded = 0;
    while (Open.Num() > 0)
    {
        FOpenCell Current;
        Open.HeapPop(Current, ByCost);
        if (Current.Cost > CostSoFar[Current.Cell] + Heuristic(Current.Cell) + 0.01f)
        {
            continue;
        }
        if ((Current.Cell - GoalCell).SizeSquared() <= 2)
        {
            bFound = true;
            Reached = Current.Cell;
            break;
        }
        if (++Expanded >= Query.MaxCells)
        {
            UE_LOG(LogNsVehiclePath, Warning, TEXT("FindPath stopped at the cell limit of %d. Raise Max Cells or the grid spacing"), Query.MaxCells);
            break;
        }

        const FVector From(GridOrigin.X + Current.Cell.X * Spacing, GridOrigin.Y + Current.Cell.Y * Spacing, GroundZ[Current.Cell]);
        for (const FIntPoint& Step : Neighbours)
        {
            const FIntPoint Next = Current.Cell + Step;
            const FVector Above(GridOrigin.X + Next.X * Spacing, GridOrigin.Y + Next.Y * Spacing, From.Z + Query.TraceHeight);
            FVector Ground;
            if (IsBlocked(Above, Query) || !FindGround(World, Above, Query.TraceHeight + Profile->MaxDropHeight, Query, Ground) || !CanStep(World, *Profile, From, Ground, Query, Current.Cell == Origin))
            {
                continue;
            }

            // A climb costs more, and a step beside a drop or water costs more, so the way keeps to gentle ground away from edges
            const float Run = FVector::Dist2D(From, Ground);
            const float Slope = FMath::Max(Ground.Z - From.Z, 0.f) / FMath::Max(Run, 1.f) / SlopeTan;
            const float Edge = Query.EdgeCost > 0.f && IsNearEdge(World, *Profile, From, Ground, Query) ? Query.EdgeCost : 0.f;
            const float NewCost = CostSoFar[Current.Cell] + Run * (1.f + Query.SlopeCost * Slope * Slope + Edge);
            const float* const OldCost = CostSoFar.Find(Next);
            if (OldCost == nullptr || NewCost < *OldCost)
            {
                CostSoFar.Add(Next, NewCost);
                GroundZ.Add(Next, Ground.Z);
                CameFrom.Add(Next, Current.Cell);
                Open.HeapPush({ Next, NewCost + Heuristic(Next) }, ByCost);
            }
        }
    }

    if (!bFound)
    {
        UE_LOG(LogNsVehiclePath, Warning, TEXT("FindPath found no way from %s to %s with %s"), *Start.ToString(), *Goal.ToString(), *Profile->GetName());
        return Path;
    }

    for (FIntPoint Cell = Reached; ; Cell = CameFrom[Cell])
    {
        Path.Add(FVector(GridOrigin.X + Cell.X * Spacing, GridOrigin.Y + Cell.Y * Spacing, GroundZ[Cell]));
        if (Cell == Origin)
        {
            break;
        }
    }
    Algo::Reverse(Path);

    // The way begins where the vehicle stands, then joins the grid
    if (FVector::DistSquared2D(StartGround, OriginGround) > 1.f)
    {
        Path.Insert(StartGround, 0);
    }
    return Path;
}

TArray<FVector> UNsVehiclePathfinder::SmoothPath(const UObject* const WorldContextObject, const TArray<FVector>& Path, const UNsVehicleReachProfile* const Profile, const FNsVehiclePathQuery& Query, const int32 MaxSkip)
{
    const UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    if (World == nullptr || Profile == nullptr || Path.Num() < 3)
    {
        return Path;
    }

    // From each kept point, the line goes to the farthest later point that it reaches in a straight line, up to the skip limit
    TArray<FVector> Smooth;
    Smooth.Add(Path[0]);
    TArray<FVector> Line;
    int32 Anchor = 0;
    while (Anchor < Path.Num() - 1)
    {
        int32 Next = Anchor + 1;
        TArray<FVector> Best;
        for (int32 Candidate = FMath::Min(Anchor + FMath::Max(MaxSkip, 2), Path.Num() - 1); Candidate > Anchor + 1; --Candidate)
        {
            Line.Reset();
            if (TraceStraight(World, *Profile, Path[Anchor], Path[Candidate], Query, Anchor == 0, Line))
            {
                Next = Candidate;
                Best = Line;
                break;
            }
        }
        if (Best.Num() > 0)
        {
            Smooth.Append(Best);
        }
        else
        {
            Smooth.Add(Path[Next]);
        }
        Anchor = Next;
    }
    return Smooth;
}

bool UNsVehiclePathfinder::TraceStraight(const UWorld* const World, const UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query, const bool bAllowStartInside, TArray<FVector>& OutPoints)
{
    const int32 Steps = FMath::Max(1, FMath::CeilToInt(FVector::Dist2D(From, To) / FMath::Max(Query.GridSpacing, 1.f)));
    FVector Previous = From;
    for (int32 Step = 1; Step <= Steps; ++Step)
    {
        const FVector Point = FMath::Lerp(From, To, static_cast<float>(Step) / Steps);
        FVector Ground;
        if (IsBlocked(Point, Query)
            || !FindGround(World, FVector(Point.X, Point.Y, Previous.Z + Query.TraceHeight), Query.TraceHeight + Profile.MaxDropHeight, Query, Ground)
            || !CanStep(World, Profile, Previous, Ground, Query, bAllowStartInside && Step == 1)
            || (Query.EdgeCost > 0.f && IsNearEdge(World, Profile, Previous, Ground, Query)))
        {
            return false;
        }
        OutPoints.Add(Ground);
        Previous = Ground;
    }
    return true;
}

bool UNsVehiclePathfinder::IsBlocked(const FVector& Point, const FNsVehiclePathQuery& Query)
{
    for (const FVector& Blocked : Query.BlockedPoints)
    {
        if (FVector::Dist2D(Point, Blocked) < Query.BlockedRadius)
        {
            return true;
        }
    }
    return false;
}

bool UNsVehiclePathfinder::FindGround(const UWorld* const World, const FVector& Above, const float TraceDepth, const FNsVehiclePathQuery& Query, FVector& OutGround)
{
    FHitResult Hit;
    // The ground is the simple collision that the wheels drive on. A complex trace hits branches and leaves, for example a tree canopy above the ground
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NsVehiclePathGround), false);
    AddIgnored(Params, Query);
    if (!World->LineTraceSingleByChannel(Hit, Above, Above - FVector(0.f, 0.f, TraceDepth), Query.TraceChannel, Params))
    {
        return false;
    }
    OutGround = Hit.ImpactPoint;
    return true;
}

bool UNsVehiclePathfinder::IsWater(const UWorld* const World, const FVector& Ground, const FNsVehiclePathQuery& Query)
{
    if (Query.WaterClassNameFilter.IsEmpty())
    {
        return false;
    }

    // The test reads the class name, so the plugin needs no dependency on the Water plugin
    TArray<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, Ground + FVector(0.f, 0.f, 3.f), FQuat::Identity, FCollisionObjectQueryParams::AllObjects, FCollisionShape::MakeSphere(2.f));
    for (const FOverlapResult& Overlap : Overlaps)
    {
        const AActor* const Owner = Overlap.GetActor();
        if (Owner != nullptr && Owner->GetClass()->GetName().Contains(Query.WaterClassNameFilter))
        {
            return true;
        }
    }
    return false;
}

bool UNsVehiclePathfinder::CanStep(const UWorld* const World, const UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query, const bool bAllowStartInside)
{
    return TestStep(World, Profile, From, To, Query, bAllowStartInside, nullptr);
}

FString UNsVehiclePathfinder::DescribeStep(const UObject* const WorldContextObject, const UNsVehicleReachProfile* const Profile, const FVector From, const FVector To, const FNsVehiclePathQuery& Query, const bool bAllowStartInside)
{
    const UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    if (World == nullptr || Profile == nullptr)
    {
        return TEXT("no world or no profile");
    }
    FVector FromGround;
    FVector ToGround;
    if (!FindGround(World, From + FVector(0.f, 0.f, Query.TraceHeight), Query.TraceHeight * 2.f, Query, FromGround))
    {
        return TEXT("no ground at the start");
    }
    if (!FindGround(World, To + FVector(0.f, 0.f, Query.TraceHeight), Query.TraceHeight * 2.f, Query, ToGround))
    {
        return TEXT("no ground at the end");
    }
    FString Reason;
    TestStep(World, *Profile, FromGround, ToGround, Query, bAllowStartInside, &Reason);
    return Reason;
}

bool UNsVehiclePathfinder::TestStep(const UWorld* const World, const UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query, const bool bAllowStartInside, FString* const OutReason)
{
    const auto Fail = [OutReason](const FString& Reason)
    {
        if (OutReason != nullptr)
        {
            *OutReason = Reason;
        }
        return false;
    };

    // A rise passes as a step, or as a slope that the vehicle climbs. A drop passes down to the drop limit, and it is one way
    if (!CanClimb(Profile, From, To, Query))
    {
        return Fail(FString::Printf(TEXT("too steep: %.1f cm over %.1f cm"), To.Z - From.Z, FVector::Dist2D(From, To)));
    }

    // The ground between two grid points can hide a short steep bump, so each half of the step must pass too
    FVector Middle;
    const FVector MiddleAbove = (From + To) * 0.5f + FVector(0.f, 0.f, Query.TraceHeight);
    if (!FindGround(World, MiddleAbove, Query.TraceHeight + Profile.MaxDropHeight, Query, Middle))
    {
        return Fail(TEXT("no ground at the middle"));
    }
    if (!CanClimb(Profile, From, Middle, Query) || !CanClimb(Profile, Middle, To, Query))
    {
        return Fail(FString::Printf(TEXT("steep bump at the middle: %.1f, %.1f, %.1f"), From.Z, Middle.Z, To.Z));
    }
    if (!Profile.bCrossesWater && IsWater(World, To, Query))
    {
        return Fail(TEXT("water"));
    }

    // A box of the vehicle size moves from one point to the next above the step. A wall, a narrow gap, or a low roof stops it
    // With a vehicle, the sweep uses the collision of its root, so an object that blocks the vehicle, for example a wire fence, blocks the way
    const float HalfHeight = Profile.Height * 0.5f;
    const FVector Lift(0.f, 0.f, Profile.MaxStepHeight + HalfHeight + 1.f);
    const FVector Direction = (To - From).GetSafeNormal2D();
    const FQuat Rotation = Direction.IsNearlyZero() ? FQuat::Identity : Direction.Rotation().Quaternion();
    // The box starts just above the step height, so an object that the wheels cannot climb, for example a low root, stops the step. The sweep ignores the ground itself
    const FCollisionShape Box = FCollisionShape::MakeBox(FVector(1.f, Profile.HalfWidth + Query.Clearance, HalfHeight + 0.5f));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NsVehiclePathSweep), false);
    AddIgnored(Params, Query);
    const UPrimitiveComponent* const Body = Query.Vehicle != nullptr ? Cast<UPrimitiveComponent>(Query.Vehicle->GetRootComponent()) : nullptr;
    const ECollisionChannel Channel = Body != nullptr ? Body->GetCollisionObjectType() : Query.TraceChannel.GetValue();
    const FCollisionResponseParams Responses = Body != nullptr ? FCollisionResponseParams(Body->GetCollisionResponseToChannels()) : FCollisionResponseParams::DefaultResponseParam;
    const FVector End = To + FVector(0.f, 0.f, FMath::Max(From.Z - To.Z, 0.f)) + Lift;
    const float WallNormalZ = FMath::Cos(FMath::DegreesToRadians(Query.WallAngle));

    // The box touches the ground on a slope or a cross slope. The sweep ignores a ground-like part of the actor under the step and sweeps again, so only a wall or a prop stops the step
    // A wall that the start point already touches, for example a stump beside a parked vehicle, is ignored on the first step only
    const AActor* const GroundFrom = FindGroundActor(World, From, Query);
    const AActor* const GroundTo = FindGroundActor(World, To, Query);
    for (int32 Try = 0; Try < MaxSweepTries; ++Try)
    {
        FHitResult Hit;
        if (!World->SweepSingleByChannel(Hit, From + Lift, End, Rotation, Channel, Box, Params, Responses))
        {
            return true;
        }
        const AActor* const HitActor = Hit.GetActor();
        const bool bGround = Hit.Normal.Z >= WallNormalZ && IsGroundActor(HitActor, GroundFrom, GroundTo);

        // A pawn moves, for example a rider on the vehicle or a character on the way, so it does not block the plan. The driver backs off if one stands in the way
        const bool bPawn = Cast<APawn>(HitActor) != nullptr;

        // A component with no physics collision, for example an interaction trigger, does not stop the vehicle
        const UPrimitiveComponent* const HitComponent = Hit.GetComponent();
        const bool bQueryOnly = HitComponent != nullptr && !CollisionEnabledHasPhysics(HitComponent->GetCollisionEnabled());
        if (!bGround && !bPawn && !bQueryOnly && !(bAllowStartInside && Hit.bStartPenetrating))
        {
            return Fail(FString::Printf(TEXT("blocked by %s, %s, normal z %.2f%s"), *GetNameSafe(HitActor), *GetNameSafe(HitComponent), Hit.Normal.Z, Hit.bStartPenetrating ? TEXT(", starts inside") : TEXT("")));
        }
        Params.AddIgnoredComponent(Hit.GetComponent());
    }
    return Fail(TEXT("too many ground parts in the sweep"));
}

bool UNsVehiclePathfinder::CanClimb(const UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query)
{
    const float Rise = To.Z - From.Z;
    const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Atan2(Rise, FMath::Max(FVector::Dist2D(From, To), 1.f)));
    return Rise >= -Profile.MaxDropHeight && (Rise <= Profile.MaxStepHeight || SlopeDegrees <= Profile.MaxSlopeDegrees * Query.SlopeMargin);
}

AActor* UNsVehiclePathfinder::FindGroundActor(const UWorld* const World, const FVector& Ground, const FNsVehiclePathQuery& Query)
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NsVehiclePathGroundActor), false);
    AddIgnored(Params, Query);
    const FVector Start = Ground + FVector(0.f, 0.f, 5.f);
    return World->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.f, 0.f, 15.f), Query.TraceChannel, Params) ? Hit.GetActor() : nullptr;
}

bool UNsVehiclePathfinder::IsGroundActor(const AActor* const Actor, const AActor* const GroundFrom, const AActor* const GroundTo)
{
    if (Actor == nullptr)
    {
        return false;
    }
    if (Actor == GroundFrom || Actor == GroundTo)
    {
        return true;
    }

    // A terrain tile next to the tile under the step has the same class and is much larger than a prop
    const AActor* const Ground = GroundFrom != nullptr ? GroundFrom : GroundTo;
    return Ground != nullptr && Actor->GetClass() == Ground->GetClass() && Actor->GetComponentsBoundingBox().GetExtent().GetMax() * 2.f > TerrainTileSize;
}

bool UNsVehiclePathfinder::IsNearEdge(const UWorld* const World, const UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query)
{
    // The ground one grid step to each side of the target point. A void, a drop deeper than half the drop limit, water, or a bank steeper than the slope limit marks an edge
    // A wheel that runs along the foot of a steep bank rides up it and tips the vehicle
    const float SideDistance = Profile.HalfWidth + Query.GridSpacing;
    const float BankRise = Profile.MaxStepHeight + SideDistance * FMath::Tan(FMath::DegreesToRadians(Profile.MaxSlopeDegrees * Query.SlopeMargin));
    const FVector Side = FVector::CrossProduct((To - From).GetSafeNormal2D(), FVector::UpVector) * SideDistance;
    for (const float Sign : { 1.f, -1.f })
    {
        const FVector Point = To + Side * Sign;
        FVector Ground;
        if (!FindGround(World, Point + FVector(0.f, 0.f, Query.TraceHeight), Query.TraceHeight + Profile.MaxDropHeight, Query, Ground)
            || Ground.Z < To.Z - Profile.MaxDropHeight * 0.5f
            || Ground.Z > To.Z + BankRise
            || (!Profile.bCrossesWater && IsWater(World, Ground, Query)))
        {
            return true;
        }
    }
    return false;
}

void UNsVehiclePathfinder::AddIgnored(FCollisionQueryParams& Params, const FNsVehiclePathQuery& Query)
{
    if (Query.Vehicle != nullptr)
    {
        Params.AddIgnoredActor(Query.Vehicle);
    }
    for (const TObjectPtr<AActor>& Actor : Query.IgnoredActors)
    {
        Params.AddIgnoredActor(Actor);
    }
}
