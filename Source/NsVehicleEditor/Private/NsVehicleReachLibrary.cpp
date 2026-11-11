// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleReachLibrary.h"

#include "Containers/Queue.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
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
    if (!FindGround(World, Start + FVector(0.f, 0.f, Settings->TraceHeight), Start.Z + Settings->TraceHeight + 1000000.f, StartGround))
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
            if (!FindGround(World, Above, Settings->TraceHeight + Profile->MaxDropHeight, Ground) || !CanStep(World, *Profile, From, Ground))
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

bool UNsVehicleReachLibrary::FindGround(const UWorld* const World, const FVector& Above, const float TraceDepth, FVector& OutGround)
{
    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(NsVehicleReachGround), true);
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

bool UNsVehicleReachLibrary::CanStep(const UWorld* const World, const UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To)
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
    FHitResult Hit;
    return !World->SweepSingleByChannel(Hit, From + Lift, To + FVector(0.f, 0.f, FMath::Max(From.Z - To.Z, 0.f)) + Lift, Rotation, Settings->TraceChannel, Box, Params);
}
