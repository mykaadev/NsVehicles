// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NsVehicleReachSettings.generated.h"

/** Project settings of the vehicle reach tool: the grid, the limits, the water test, and the colors of the drawing */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "NsVehicle Reach"))
class NSVEHICLEEDITOR_API UNsVehicleReachSettings : public UDeveloperSettings
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleReachSettings();

// Variables
public:
    /** The distance between two tested points */
    UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "5.0", Units = "cm"))
    float GridSpacing;

    /** The largest number of points of one run. A run stops when it reaches it */
    UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "100"))
    int32 MaxCells;

    /** The height above the ground at which the trace for the next point starts */
    UPROPERTY(Config, EditAnywhere, Category = "Grid", meta = (ClampMin = "0.0", Units = "cm"))
    float TraceHeight;

    /** The collision channel of the ground and the walls */
    UPROPERTY(Config, EditAnywhere, Category = "Grid")
    TEnumAsByte<ECollisionChannel> TraceChannel;

    /** A part of this text in the class name of an actor marks it as water, for example WaterBody */
    UPROPERTY(Config, EditAnywhere, Category = "Water")
    FString WaterClassNameFilter;

    /** The color of a reached point */
    UPROPERTY(Config, EditAnywhere, Category = "Drawing")
    FColor ReachedColor;

    /** The color of a point that only the compare profile reaches */
    UPROPERTY(Config, EditAnywhere, Category = "Drawing")
    FColor CompareOnlyColor;

    /** The size of a drawn point */
    UPROPERTY(Config, EditAnywhere, Category = "Drawing", meta = (ClampMin = "1.0"))
    float PointSize;

    /** The time that a drawing stays in the editor */
    UPROPERTY(Config, EditAnywhere, Category = "Drawing", meta = (ClampMin = "1.0", Units = "s"))
    float DrawSeconds;

    /** The extra cost of a climb in the path search. At 0 the way is the shortest. Higher values keep the way on gentle ground */
    UPROPERTY(Config, EditAnywhere, Category = "Drive To", meta = (ClampMin = "0.0"))
    float PathSlopeCost;

    /** The part of the slope limit of the profile that the path search uses. A vehicle stalls at its exact limit on rough ground, so the way keeps this margin */
    UPROPERTY(Config, EditAnywhere, Category = "Drive To", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float PathSlopeMargin;

    /** The profile that NsVehicle.DriveTo uses when the command names no profile */
    UPROPERTY(Config, EditAnywhere, Category = "Drive To")
    TSoftObjectPtr<class UNsVehicleReachProfile> DriveProfile;

    /** The distance between two points of the spline that NsVehicle.DriveTo makes from the way */
    UPROPERTY(Config, EditAnywhere, Category = "Drive To", meta = (ClampMin = "10.0", Units = "cm"))
    float DriveSplineSpacing;
};
