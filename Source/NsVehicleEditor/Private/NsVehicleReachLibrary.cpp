// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleReachLibrary.h"

#include "Algo/Reverse.h"
#include "Containers/Queue.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"
#include "NsVehicleEditorModule.h"
#include "NsVehicleReachProfile.h"
#include "NsVehicleReachSettings.h"

TArray<FVector> UNsVehicleReachLibrary::ComputeReach(const UObject* const WorldContextObject, const FVector Start, const UNsVehicleReachProfile* const Profile, const FBox Bounds)
{
    TArray<FVector> Reached;
    const UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    if (World == nullptr || Profile == nullptr)
    {
        UE_LOG(LogNsVehicleEditor, Error, TEXT("ComputeReach needs a world and a profile"));
        return Reached;
    }

    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    const float Spacing = Settings->GridSpacing;
    FVector StartGround;
    // The start can be any height above the ground, so its trace goes all the way down
    if (!FindGround(World, Start + FVector(0.f, 0.f, Settings->TraceHeight), Start.Z + Settings->TraceHeight + 1000000.f, StartGround, TArray<AActor*>()))
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("ComputeReach found no ground under the start %s"), *Start.ToString());
        return Reached;
    }

    // A flood fill over a grid that starts at the start point. Each cell keeps the ground height where the vehicle stood
    TMap<FIntPoint, float> Visited;
    TQueue<FIntPoint> Open;
    const FIntPoint Origin(0, 0);
    Visited.Add(Origin, StartGround.Z);
    Open.Enqueue(Origin);
    Reached.Add(StartGround);

    static const FIntPoint Neighbours[] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
    FIntPoint Cell;
    while (Open.Dequeue(Cell))
    {
        const FVector From(Start.X + Cell.X * Spacing, Start.Y + Cell.Y * Spacing, Visited[Cell]);
        for (const FIntPoint& Step : Neighbours)
        {
            const FIntPoint Next = Cell + Step;
            if (Visited.Contains(Next))
            {
                continue;
            }
            const FVector Above(Start.X + Next.X * Spacing, Start.Y + Next.Y * Spacing, From.Z + Settings->TraceHeight);
            if (Bounds.IsValid && !Bounds.IsInsideXY(Above))
            {
                continue;
            }
            FVector Ground;
            if (!FindGround(World, Above, Settings->TraceHeight + Profile->MaxDropHeight, Ground, TArray<AActor*>()) || !CanStep(World, *Profile, From, Ground, TArray<AActor*>()))
            {
                continue;
            }

            Visited.Add(Next, Ground.Z);
            Reached.Add(Ground);
            if (Reached.Num() >= Settings->MaxCells)
            {
                UE_LOG(LogNsVehicleEditor, Warning, TEXT("ComputeReach stopped at the cell limit of %d. Raise Max Cells or the grid spacing"), Settings->MaxCells);
                return Reached;
            }
            Open.Enqueue(Next);
        }
    }
    return Reached;
}

TArray<FVector> UNsVehicleReachLibrary::GetOnlyInCompare(const TArray<FVector>& BasePoints, const TArray<FVector>& ComparePoints)
{
    // Both runs use the same grid, so the X and Y of a point name its cell
    TSet<FIntPoint> BaseCells;
    for (const FVector& Point : BasePoints)
    {
        BaseCells.Add(FIntPoint(FMath::RoundToInt(Point.X), FMath::RoundToInt(Point.Y)));
    }
    TArray<FVector> Only;
    for (const FVector& Point : ComparePoints)
    {
        if (!BaseCells.Contains(FIntPoint(FMath::RoundToInt(Point.X), FMath::RoundToInt(Point.Y))))
        {
            Only.Add(Point);
        }
    }
    return Only;
}

void UNsVehicleReachLibrary::DrawReach(const UObject* const WorldContextObject, const TArray<FVector>& Points, const FColor Color)
{
    const UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }
    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    for (const FVector& Point : Points)
    {
        DrawDebugPoint(World, Point + FVector(0.f, 0.f, 2.f), Settings->PointSize, Color, false, Settings->DrawSeconds);
    }
}

bool UNsVehicleReachLibrary::WriteReachFile(const TArray<FVector>& Points, const FString& FilePath)
{
    TArray<FString> Lines;
    Lines.Reserve(Points.Num());
    for (const FVector& Point : Points)
    {
        Lines.Add(FString::Printf(TEXT("%.1f,%.1f,%.1f"), Point.X, Point.Y, Point.Z));
    }
    return FFileHelper::SaveStringArrayToFile(Lines, *FilePath);
}

TArray<FVector> UNsVehicleReachLibrary::FindPath(const UObject* const WorldContextObject, const FVector Start, const FVector Goal, const UNsVehicleReachProfile* const Profile, const TArray<AActor*>& IgnoredActors)
{
    TArray<FVector> Path;
    const UWorld* const World = WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    if (World == nullptr || Profile == nullptr)
    {
        UE_LOG(LogNsVehicleEditor, Error, TEXT("FindPath needs a world and a profile"));
        return Path;
    }

    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    const float Spacing = Settings->GridSpacing;

    // A vehicle stalls at its exact slope limit on rough ground, so the way keeps a margin under the limit of the profile
    UNsVehicleReachProfile* const PathProfile = DuplicateObject<UNsVehicleReachProfile>(Profile, GetTransientPackage());
    PathProfile->MaxSlopeDegrees *= Settings->PathSlopeMargin;

    FVector StartGround;
    if (!FindGround(World, Start + FVector(0.f, 0.f, Settings->TraceHeight), Start.Z + Settings->TraceHeight + 1000000.f, StartGround, IgnoredActors))
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("FindPath found no ground under the start %s"), *Start.ToString());
        return Path;
    }

    // The grid of the reach tool, with the start at cell 0,0. Each cell keeps the ground height where the vehicle stood
    const FIntPoint GoalCell(FMath::RoundToInt((Goal.X - Start.X) / Spacing), FMath::RoundToInt((Goal.Y - Start.Y) / Spacing));
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
    GroundZ.Add(Origin, StartGround.Z);
    CostSoFar.Add(Origin, 0.f);
    Open.HeapPush({ Origin, Heuristic(Origin) }, ByCost);

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
        if (++Expanded >= Settings->MaxCells)
        {
            UE_LOG(LogNsVehicleEditor, Warning, TEXT("FindPath stopped at the cell limit of %d. Raise Max Cells or the grid spacing"), Settings->MaxCells);
            break;
        }

        const FVector From(Start.X + Current.Cell.X * Spacing, Start.Y + Current.Cell.Y * Spacing, GroundZ[Current.Cell]);
        for (const FIntPoint& Step : Neighbours)
        {
            const FIntPoint Next = Current.Cell + Step;
            const FVector Above(Start.X + Next.X * Spacing, Start.Y + Next.Y * Spacing, From.Z + Settings->TraceHeight);
            FVector Ground;
            if (!FindGround(World, Above, Settings->TraceHeight + PathProfile->MaxDropHeight, Ground, IgnoredActors) || !CanStep(World, *PathProfile, From, Ground, IgnoredActors))
            {
                continue;
            }

            // A steep step costs more, so the way keeps to gentle ground when it can
            const float Run = FVector::Dist2D(From, Ground);
            const float Slope = FMath::Max(Ground.Z - From.Z, 0.f) / FMath::Max(Run, 1.f) / FMath::Max(FMath::Tan(FMath::DegreesToRadians(PathProfile->MaxSlopeDegrees)), 0.01f);
            const float NewCost = CostSoFar[Current.Cell] + Run * (1.f + Settings->PathSlopeCost * Slope * Slope);
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
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("FindPath found no way from %s to %s with %s"), *Start.ToString(), *Goal.ToString(), *Profile->GetName());
        return Path;
    }

    for (FIntPoint Cell = Reached; ; Cell = CameFrom[Cell])
    {
        Path.Add(FVector(Start.X + Cell.X * Spacing, Start.Y + Cell.Y * Spacing, GroundZ[Cell]));
        if (Cell == Origin)
        {
            break;
        }
    }
    Algo::Reverse(Path);
    return Path;
}

bool UNsVehicleReachLibrary::FindGround(const UWorld* const World, const FVector& Above, const float TraceDepth, FVector& OutGround, const TArray<AActor*>& IgnoredActors)
{
    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NsVehicleReachGround), true);
    Params.AddIgnoredActors(IgnoredActors);
    if (!World->LineTraceSingleByChannel(Hit, Above, Above - FVector(0.f, 0.f, TraceDepth), Settings->TraceChannel, Params))
    {
        return false;
    }
    OutGround = Hit.ImpactPoint;
    return true;
}

bool UNsVehicleReachLibrary::IsWater(const UWorld* const World, const FVector& Ground)
{
    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    if (Settings->WaterClassNameFilter.IsEmpty())
    {
        return false;
    }

    // The test reads the class name, so the plugin needs no dependency on the Water plugin
    TArray<FOverlapResult> Overlaps;
    World->OverlapMultiByObjectType(Overlaps, Ground + FVector(0.f, 0.f, 3.f), FQuat::Identity, FCollisionObjectQueryParams::AllObjects, FCollisionShape::MakeSphere(2.f));
    for (const FOverlapResult& Overlap : Overlaps)
    {
        const AActor* const Owner = Overlap.GetActor();
        if (Owner != nullptr && Owner->GetClass()->GetName().Contains(Settings->WaterClassNameFilter))
        {
            return true;
        }
    }
    return false;
}

bool UNsVehicleReachLibrary::CanStep(const UWorld* const World, const UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const TArray<AActor*>& IgnoredActors)
{
    const float Rise = To.Z - From.Z;
    const float Run = FVector::Dist2D(From, To);
    if (Rise < -Profile.MaxDropHeight)
    {
        return false;
    }

    // A rise passes as a step, or as a slope that the vehicle climbs. A drop passes down to the drop limit, and it is one way
    const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Atan2(Rise, FMath::Max(Run, 1.f)));
    if (Rise > Profile.MaxStepHeight && SlopeDegrees > Profile.MaxSlopeDegrees)
    {
        return false;
    }
    if (!Profile.bCrossesWater && IsWater(World, To))
    {
        return false;
    }

    // A box of the size of the vehicle moves from one point to the next above the step. A wall, a narrow gap, or a low roof stops it
    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    const float HalfHeight = Profile.Height * 0.5f;
    const FVector Lift(0.f, 0.f, Profile.MaxStepHeight + HalfHeight + 1.f);
    const FVector Direction = (To - From).GetSafeNormal2D();
    const FQuat Rotation = Direction.IsNearlyZero() ? FQuat::Identity : Direction.Rotation().Quaternion();
    const FCollisionShape Box = FCollisionShape::MakeBox(FVector(1.f, Profile.HalfWidth, FMath::Max(HalfHeight - Profile.MaxStepHeight * 0.5f, 1.f)));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NsVehicleReachSweep), false);
    Params.AddIgnoredActors(IgnoredActors);
    FHitResult Hit;
    return !World->SweepSingleByChannel(Hit, From + Lift, To + FVector(0.f, 0.f, FMath::Max(From.Z - To.Z, 0.f)) + Lift, Rotation, Settings->TraceChannel, Box, Params);
}
