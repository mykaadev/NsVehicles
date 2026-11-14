// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "NsVehicleMotorAudioComponent.generated.h"

/**
 * The motor sound of a vehicle. It plays a looping sound while a player controls the owner pawn, with a pitch and a volume that follow the speed or the engine RPM
 * Set the looping sound in Sound. The sound fades in when a player gets in, and fades out when the player gets out
 */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleMotorAudioComponent : public UAudioComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleMotorAudioComponent(const FObjectInitializer& ObjectInitializer);

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** Returns true while a player controls the owner and the motor sound plays */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Motor Audio")
    bool IsMotorRunning() const;

private:
    /** Starts or stops the motor sound when the controller of the owner changes */
    UFUNCTION()
    void HandleControllerChanged(class APawn* Pawn, class AController* OldController, class AController* NewController);

    /** Starts the motor sound and the tick while a player controls the owner, and stops them otherwise */
    void RefreshRunning();

    /** Returns the drive amount from 0 at rest to 1 at the full speed or the full RPM */
    float GetDriveAlpha() const;

// Variables
protected:
    /** The pitch at rest */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Motor Audio", meta = (ClampMin = "0.1"))
    float IdlePitch;

    /** The pitch at the full speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Motor Audio", meta = (ClampMin = "0.1"))
    float FullSpeedPitch;

    /** The volume at rest */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Motor Audio", meta = (ClampMin = "0.0"))
    float IdleVolume;

    /** The volume at the full speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Motor Audio", meta = (ClampMin = "0.0"))
    float FullSpeedVolume;

    /** The speed that gives the full speed pitch and volume */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Motor Audio", meta = (ClampMin = "0.1", Units = "km/h", EditCondition = "!bFollowEngineRPM"))
    float FullSpeedKph;

    /** Follows the engine RPM against its maximum RPM in place of the speed. The owner must be an NsVehicle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Motor Audio")
    bool bFollowEngineRPM;

    /** The time of the fade when a player gets in or out */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Motor Audio", meta = (ClampMin = "0.0", Units = "s"))
    float FadeSeconds;

private:
    /** True while a player controls the owner and the motor sound plays */
    bool bRunning;
};
