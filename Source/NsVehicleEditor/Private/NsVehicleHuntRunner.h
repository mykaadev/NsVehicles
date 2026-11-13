// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UObject/Object.h"
#include "NsVehicleHuntRunner.generated.h"

/** The result of one drive of a hunt */
struct FNsVehicleHuntResult
{
    /** The name of the target */
    FString Target;

    /** Arrived, stuck, no way, or timeout */
    FString Result;

    /** The time of the drive in seconds */
    float Seconds = 0.f;

    /** The new plans of the drive */
    int32 Replans = 0;

    /** Where the drive ended */
    FVector End = FVector::ZeroVector;

    /** Why the drive ended, when it did not arrive */
    FString Cause;
};

/** Runs one hunt in Play In Editor: drives to each target of a hunt list in turn, watches the no-go zones, and writes the report */
UCLASS()
class UNsVehicleHuntRunner : public UObject
{
    GENERATED_BODY()

// Functions
public:
    /** Starts a hunt with the vehicle of the player. Returns false when the world, the list, or the vehicle is missing */
    bool Start(class UWorld* InWorld, class UNsVehicleHuntList* InList, class ANsVehicle* InVehicle);

    /** Stops the hunt and writes the report of the drives so far */
    void Stop();

    /** True while a hunt runs */
    bool IsRunning() const { return TickHandle.IsValid(); }

private:
    /** Starts the drive to the next target, or ends the hunt after the last one */
    void StartNextTarget();

    /** Watches the drive and the zones. Returns true to tick again */
    bool Tick(float DeltaTime);

    /** Ends the current drive with a result */
    void FinishTarget(const FString& Result, const FString& Cause);

    /** Writes the report to the Saved folder and shows a summary */
    void WriteReport();

// Variables
private:
    /** The world of the hunt */
    TWeakObjectPtr<class UWorld> World;

    /** The hunt list */
    UPROPERTY()
    TObjectPtr<class UNsVehicleHuntList> List;

    /** The vehicle that drives */
    TWeakObjectPtr<class ANsVehicle> Vehicle;

    /** The AI driver of the vehicle */
    TWeakObjectPtr<class UNsVehicleAI> AI;

    /** The index of the current target */
    int32 TargetIndex = INDEX_NONE;

    /** The time of the current drive */
    float DriveSeconds = 0.f;

    /** The results of the drives */
    TArray<FNsVehicleHuntResult> Results;

    /** One line for each no-go zone entry */
    TArray<FString> ZoneEntries;

    /** The zones that the vehicle is in now, so an entry counts one time */
    TSet<int32> ZonesInside;

    /** The ticker of the hunt */
    FTSTicker::FDelegateHandle TickHandle;
};
