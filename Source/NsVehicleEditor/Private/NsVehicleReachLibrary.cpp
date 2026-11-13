// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleReachLibrary.h"

#include "Containers/Queue.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "NsVehicleEditorModule.h"
#include "AI/NsVehicleReachProfile.h"
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

    const FNsVehiclePathQuery Query = MakeQuery(false);
    const float Spacing = Query.GridSpacing;
    FVector StartGround;
    // The start can be any height above the ground, so its trace goes all the way down
    if (!UNsVehiclePathfinder::FindGround(World, Start + FVector(0.f, 0.f, Query.TraceHeight), Start.Z + Query.TraceHeight + 1000000.f, Query, StartGround))
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
            const FVector Above(Start.X + Next.X * Spacing, Start.Y + Next.Y * Spacing, From.Z + Query.TraceHeight);
            if (Bounds.IsValid && !Bounds.IsInsideXY(Above))
            {
                continue;
            }
            FVector Ground;
            if (!UNsVehiclePathfinder::FindGround(World, Above, Query.TraceHeight + Profile->MaxDropHeight, Query, Ground) || !UNsVehiclePathfinder::CanStep(World, *Profile, From, Ground, Query, Cell == Origin))
            {
                continue;
            }

            Visited.Add(Next, Ground.Z);
            Reached.Add(Ground);
            if (Reached.Num() >= Query.MaxCells)
            {
                UE_LOG(LogNsVehicleEditor, Warning, TEXT("ComputeReach stopped at the cell limit of %d. Raise Max Cells or the grid spacing"), Query.MaxCells);
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

FNsVehiclePathQuery UNsVehicleReachLibrary::MakeQuery(const bool bForPath)
{
    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    FNsVehiclePathQuery Query;
    Query.GridSpacing = Settings->GridSpacing;
    Query.MaxCells = Settings->MaxCells;
    Query.TraceHeight = Settings->TraceHeight;
    Query.TraceChannel = Settings->TraceChannel;
    Query.WaterClassNameFilter = Settings->WaterClassNameFilter;
    // The reach tool shows all the ground that the profile reaches, so it uses the full limit and no edge cost
    Query.SlopeMargin = bForPath ? Settings->PathSlopeMargin : 1.f;
    Query.SlopeCost = Settings->PathSlopeCost;
    Query.EdgeCost = bForPath ? Settings->PathEdgeCost : 0.f;
    // The reach tool shows where the vehicle fits, so it needs no free space beside the vehicle
    Query.Clearance = bForPath ? Settings->PathClearance : 0.f;
    return Query;
}
