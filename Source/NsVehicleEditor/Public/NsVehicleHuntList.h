// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NsVehicleHuntList.generated.h"

/** One place that the hunt drives to */
USTRUCT(BlueprintType)
struct NSVEHICLEEDITOR_API FNsVehicleHuntTarget
{
    GENERATED_BODY()

    /** The name of the place in the report */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt")
    FString Name;

    /** The place */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt")
    FVector Location = FVector::ZeroVector;
};

/** A box that the vehicle must not enter. An entry in the hunt is a spot that a player can reach and must not */
USTRUCT(BlueprintType)
struct NSVEHICLEEDITOR_API FNsVehicleHuntZone
{
    GENERATED_BODY()

    /** The name of the zone in the report */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt")
    FString Name;

    /** The center of the box */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt")
    FVector Center = FVector::ZeroVector;

    /** Half the size of the box on each axis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt", meta = (Units = "cm"))
    FVector Extent = FVector(500.f);
};

/**
 * A hunt for weird spots: the AI driver drives to each target in turn with the profile, and the report lists what it reached, where it got stuck, and each no-go zone it entered
 * Run it in Play In Editor with NsVehicle.Hunt /Game/Path/HuntList. Editor only
 */
UCLASS(BlueprintType)
class NSVEHICLEEDITOR_API UNsVehicleHuntList : public UDataAsset
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleHuntList();

// Variables
public:
    /** The wheel setup of the hunt */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt")
    TObjectPtr<class UNsVehicleReachProfile> Profile;

    /** The places to drive to, in this order. Each drive starts where the last one ended */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt")
    TArray<FNsVehicleHuntTarget> Targets;

    /** The boxes that the vehicle must not enter */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt")
    TArray<FNsVehicleHuntZone> NoGoZones;

    /** The time limit of one drive */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hunt", meta = (ClampMin = "10.0", Units = "s"))
    float TargetTimeLimit;
};
