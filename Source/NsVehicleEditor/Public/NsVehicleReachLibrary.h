// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NsVehicleReachLibrary.generated.h"

/**
 * The reach tool: from a start point, it finds all the ground that a vehicle with a reach profile can drive to
 * It tests a grid. A step to the next point passes when the slope, the step, the drop, the width, the roof, and the water allow it
 * Use it to see which places a wheel setup opens, and to find a way that a level designer did not plan. Editor only
 */
UCLASS()
class NSVEHICLEEDITOR_API UNsVehicleReachLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

// Functions
public:
    /** Returns the ground points that the profile reaches from the start, inside the bounds. An invalid bounds box means no limit but the cell limit of the settings */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Reach", meta = (WorldContext = "WorldContextObject"))
    static TArray<FVector> ComputeReach(const UObject* WorldContextObject, FVector Start, const class UNsVehicleReachProfile* Profile, FBox Bounds);

    /** Returns the points of the compare run that the base run does not reach: the places that the compare profile opens */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Reach")
    static TArray<FVector> GetOnlyInCompare(const TArray<FVector>& BasePoints, const TArray<FVector>& ComparePoints);

    /** Draws the points in the world for the drawing time of the settings */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Reach", meta = (WorldContext = "WorldContextObject"))
    static void DrawReach(const UObject* WorldContextObject, const TArray<FVector>& Points, FColor Color);

    /** Writes the points to a text file, one point for each line as X,Y,Z. Returns true when the file is written */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Reach")
    static bool WriteReachFile(const TArray<FVector>& Points, const FString& FilePath);

    /** Finds a way that the profile can drive from the start to the goal, with A* on the reach grid. The traces ignore the ignored actors, for example the vehicle at the start. Returns the ground points of the way, or an empty array when the goal is out of reach */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Reach", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "IgnoredActors"))
    static TArray<FVector> FindPath(const UObject* WorldContextObject, FVector Start, FVector Goal, const class UNsVehicleReachProfile* Profile, const TArray<class AActor*>& IgnoredActors);

private:
    /** Finds the ground under a point. The trace ignores the ignored actors. Returns false when there is no ground in the trace */
    static bool FindGround(const class UWorld* World, const FVector& Above, float TraceDepth, FVector& OutGround, const TArray<class AActor*>& IgnoredActors);

    /** True when the ground point is water */
    static bool IsWater(const class UWorld* World, const FVector& Ground);

    /** True when the vehicle of the profile can drive from one ground point to the next. The sweep ignores the ignored actors */
    static bool CanStep(const class UWorld* World, const class UNsVehicleReachProfile& Profile, const FVector& From, const FVector& To, const TArray<class AActor*>& IgnoredActors);
};
