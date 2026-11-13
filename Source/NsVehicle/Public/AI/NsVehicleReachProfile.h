// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NsVehicleReachProfile.generated.h"

/**
 * What one vehicle setup can drive over. Make one profile for each wheel setup, for example standard tires and big tires
 * The reach tool paints the ground that the vehicle reaches from a start point, and the path search of the AI driver finds a way with it
 */
UCLASS(BlueprintType)
class NSVEHICLE_API UNsVehicleReachProfile : public UDataAsset
{
    GENERATED_BODY()

// Variables
public:
    /** The highest step up that the vehicle drives over, for example a root or a curb */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach", meta = (ClampMin = "0.0", Units = "cm"))
    float MaxStepHeight = 4.f;

    /** The steepest slope that the vehicle drives up */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach", meta = (ClampMin = "0.0", ClampMax = "89.0", Units = "deg"))
    float MaxSlopeDegrees = 22.f;

    /** The deepest drop that the vehicle survives. A drop is one way: the vehicle cannot drive back up it */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach", meta = (ClampMin = "0.0", Units = "cm"))
    float MaxDropHeight = 60.f;

    /** Half the width of the vehicle. A gap narrower than the full width stops it */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach", meta = (ClampMin = "1.0", Units = "cm"))
    float HalfWidth = 8.f;

    /** The height of the vehicle. A roof lower than this stops it */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach", meta = (ClampMin = "1.0", Units = "cm"))
    float Height = 12.f;

    /** The tightest turn of the vehicle, measured at the center of the body. The path search gives no turn tighter than this. 0 allows any turn, as the reach tool does */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach", meta = (ClampMin = "0.0", Units = "cm"))
    float MinTurnRadius = 0.f;

    /** True when the vehicle crosses water, for example with floats */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reach")
    bool bCrossesWater = false;
};
