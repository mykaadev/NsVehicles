// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NsVehiclePathfinder.generated.h"

/** The grid, the traces, and the costs of one path search or one reach run */
USTRUCT(BlueprintType)
struct NSVEHICLE_API FNsVehiclePathQuery
{
    GENERATED_BODY()

    /** Constructor */
    FNsVehiclePathQuery();

    /** The distance between two tested points */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "5.0", Units = "cm"))
    float GridSpacing;

    /** The largest number of expanded points. The search stops when it reaches it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "100"))
    int32 MaxCells;

    /** The height above the ground at which the trace for the next point starts */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "0.0", Units = "cm"))
    float TraceHeight;

    /** The collision channel of the ground traces. Without a vehicle, the sweeps use it too */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path")
    TEnumAsByte<ECollisionChannel> TraceChannel;

    /** A part of this text in the class name of an actor marks it as water, for example WaterBody */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path")
    FString WaterClassNameFilter;

    /** The part of the slope limit of the profile that the search uses. A vehicle stalls at its exact limit on rough ground, so the way keeps this margin. The reach tool uses 1 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float SlopeMargin;

    /** The extra cost of a climb. At 0 the way is the shortest. Higher values keep the way on gentle ground */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "0.0"))
    float SlopeCost;

    /** The extra cost of a step beside a drop, a void, or water. Higher values keep the way away from edges and shores */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "0.0"))
    float EdgeCost;

    /** The vehicle that drives the way. The sweeps use the collision of its root, so an object that blocks the vehicle blocks the way. The traces ignore it */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path")
    TObjectPtr<class AActor> Vehicle;

    /** Actors that the traces and the sweeps ignore, for example the driver on the vehicle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path")
    TArray<TObjectPtr<class AActor>> IgnoredActors;

    /** Points that the way must not pass, for example where the vehicle got stuck before */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path")
    TArray<FVector> BlockedPoints;

    /** The radius around each blocked point that the way must not pass */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "0.0", Units = "cm"))
    float BlockedRadius;

    /** The free space on each side of the vehicle that a step needs, so the way keeps away from props and the vehicle does not scrape them in turns */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "0.0", Units = "cm"))
    float Clearance;

    /** A surface that the step sweep hits counts as a wall when it is steeper than this angle. A flatter surface is ground, which the ground and step tests judge */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Path", meta = (ClampMin = "1.0", ClampMax = "89.0", Units = "deg"))
    float WallAngle;
};

/**
 * Finds a way that a vehicle with a reach profile can drive, with A* on a grid of ground points
 * A step to the next point passes when the slope, the step, the drop, the width, the roof, and the water allow it. The AI driver and the reach tool use the same tests
 */
UCLASS()
class NSVEHICLE_API UNsVehiclePathfinder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

// Functions
public:
    /** Finds a way from the start to the goal. Returns the ground points of the way, or an empty array when the goal is out of reach */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Path", meta = (WorldContext = "WorldContextObject"))
    static TArray<FVector> FindPath(const UObject* WorldContextObject, FVector Start, FVector Goal, const class UNsVehicleReachProfile* Profile, const FNsVehiclePathQuery& Query);

    /** Replaces each run of grid points with a straight line when the line passes the same step tests away from edges. A grid way turns in steps of 45 degrees, so a vehicle on the raw way weaves. The result keeps a ground point at each grid spacing */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Path", meta = (WorldContext = "WorldContextObject"))
    static TArray<FVector> SmoothPath(const UObject* WorldContextObject, const TArray<FVector>& Path, const class UNsVehicleReachProfile* Profile, const FNsVehiclePathQuery& Query, int32 MaxSkip = 12);

    /** Finds the ground under a point. Returns false when there is no ground in the trace */
    static bool FindGround(const class UWorld* World, const FVector& Above, float TraceDepth, const FNsVehiclePathQuery& Query, FVector& OutGround);

    /** True when the ground point is water */
    static bool IsWater(const class UWorld* World, const FVector& Ground, const FNsVehiclePathQuery& Query);

    /** True when the vehicle of the profile can drive from one ground point to the next. The slope limit is the limit of the profile times the slope margin. With Allow Start Inside, a wall that the start point already touches does not block */
    static bool CanStep(const class UWorld* World, const class UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query, bool bAllowStartInside = false);

private:
    /** Finds the ground points of a straight line between two ground points at the grid spacing. Returns false when a step fails the tests, crosses a blocked point, or runs beside an edge */
    static bool TraceStraight(const class UWorld* World, const class UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query, bool bAllowStartInside, TArray<FVector>& OutPoints);

    /** True when the point is inside the radius of a blocked point of the query */
    static bool IsBlocked(const FVector& Point, const FNsVehiclePathQuery& Query);

    /** True when the vehicle can climb or drop from one ground point to the next: a rise up to the step height, a slope up to the limit times the margin, a drop up to the drop limit */
    static bool CanClimb(const class UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query);

    /** Returns the actor of the ground at a ground point, or null */
    static const class AActor* FindGroundActor(const class UWorld* World, const FVector& Ground, const FNsVehiclePathQuery& Query);

    /** True when a hit actor is the ground under the step, or a terrain tile of the same class next to it */
    static bool IsGroundActor(const class AActor* Actor, const class AActor* GroundFrom, const class AActor* GroundTo);

    /** True when the ground beside a step falls away, or is water. The cost of such a step goes up by the edge cost */
    static bool IsNearEdge(const class UWorld* World, const class UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const FNsVehiclePathQuery& Query);

    /** Fills the query params with the vehicle and the ignored actors */
    static void AddIgnored(FCollisionQueryParams& Params, const FNsVehiclePathQuery& Query);

// Variables
private:
    /** The number of ground-like parts that one step sweep ignores before the step counts as blocked */
    static constexpr int32 MaxSweepTries = 8;

    /** The size above which an actor of the ground class counts as a terrain tile, not a prop */
    static constexpr float TerrainTileSize = 2000.f;
};
