// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/NsVehiclePathfinder.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NsVehicleReachLibrary.generated.h"

/**
 * The reach tool: from a start point, it finds all the ground that a vehicle with a reach profile can drive to
 * It tests a grid with the step tests of UNsVehiclePathfinder, so the reach tool and the AI driver agree
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

    /** Returns a path query with the grid, the traces, and the costs of the reach settings. A reach run uses the full slope limit and no edge cost */
    static FNsVehiclePathQuery MakeQuery(bool bForPath);
};
