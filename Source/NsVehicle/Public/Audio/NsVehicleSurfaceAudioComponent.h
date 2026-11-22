// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Components/AudioComponent.h"
#include "NsVehicleSurfaceAudioComponent.generated.h"

/**
 * The roll sound of the wheels. It plays the loop of the surface under most grounded wheels, with a volume that follows the speed
 * A vehicle that rolls onto the water surface type plays the splash sound one time. The owner must be an ANsVehicle
 */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleSurfaceAudioComponent : public UAudioComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleSurfaceAudioComponent(const FObjectInitializer& ObjectInitializer);

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

private:
    /** Finds the surface under most grounded wheels of a vehicle. Returns false when no wheel is on the ground */
    static bool FindRollSurface(const class ANsVehicle& Vehicle, EPhysicalSurface& OutSurface);

// Variables
protected:
    /** The roll loop of each surface type. A surface with no entry uses Default Roll Loop */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio")
    TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<class USoundBase>> RollLoops;

    /** The roll loop of a surface with no entry in Roll Loops. Empty plays no loop there */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio")
    TObjectPtr<class USoundBase> DefaultRollLoop;

    /** The surface type of water, for the splash. Default plays no splash */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio")
    TEnumAsByte<EPhysicalSurface> WaterSurface;

    /** The sound when the vehicle rolls onto the water surface type */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio")
    TObjectPtr<class USoundBase> SplashSound;

    /** The speed that gives the full roll volume */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio", meta = (ClampMin = "0.1", Units = "km/h"))
    float FullSpeedKph;

    /** The roll volume at the full speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio", meta = (ClampMin = "0.0"))
    float FullSpeedVolume;

    /** The vehicle must be faster than this to splash */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio", meta = (ClampMin = "0.0", Units = "km/h"))
    float SplashSpeedKph;

    /** The time of the fade in of a loop and of its fade out */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio", meta = (ClampMin = "0.0", Units = "s"))
    float FadeSeconds;

    /** The vehicle must be faster than this to play a roll loop */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio", meta = (ClampMin = "0.0", Units = "km/h"))
    float MinRollSpeedKph;

    /** The number of checks in a row, ten each second, that a new surface must stay before its loop plays */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Surface Audio", meta = (ClampMin = "1"))
    int32 SwitchChecks;

private:
    /** The surface of the loop that plays now */
    TEnumAsByte<EPhysicalSurface> RollSurface;

    /** True while a roll loop plays */
    bool bRolling;

    /** The surface of the last checks, which can become the roll surface */
    TEnumAsByte<EPhysicalSurface> CandidateSurface;

    /** The number of checks in a row with the candidate surface */
    int32 CandidateChecks;
};
