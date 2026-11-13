// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NsVehicleCheatsSubsystem.generated.h"

/**
 * The Vehicle AI tab of the NsCheats window: drive the vehicle of the player to a picked point or a random point with Drive To, watch the drive, repeat it, and save its report
 * It registers only when the NsCheats plugin is present. It does not exist in Shipping builds
 */
UCLASS()
class NSVEHICLECHEATS_API UNsVehicleCheatsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleCheatsSubsystem();

    //~ Begin USubsystem Interface
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    /** Opens the point picker of NsCheats, then drives to the picked point */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Cheats")
    void DriveToPickedPoint();

    /** Finds a random point with the filter around the vehicle, then drives to it */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Cheats")
    void DriveToRandomPoint();

    /** Drives the vehicle of the player to a goal with the picked profile. Returns false when there is no way */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Cheats")
    bool DriveTo(FVector Goal);

    /** Drives the last drive again. With From Same Start, the vehicle first moves back to the start of that drive */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Cheats")
    void RepeatLastDrive(bool bFromSameStart);

    /** Stops the drive. The player drives again */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Cheats")
    void StopDrive();

    /** Moves the vehicle to where the last drive gave up, to look at the place or to try it by hand */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Cheats")
    void GoToLastStall();

    /** Writes the report of the last drive to the Saved/QA folder */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Cheats")
    void SaveDriveReport();

private:
    /** Registers the entries of the Vehicle AI tab */
    void RegisterCheats();

    /** Returns the vehicle that the player drives, or the vehicle nearest to the pawn of the player */
    class ANsVehicle* FindPlayerVehicle() const;

    /** Returns the AI component of the vehicle of the player, and adds one when it has none */
    class UNsVehicleAI* FindOrAddAI() const;

    /** Returns the picked reach profile, or the first one in the project */
    class UNsVehicleReachProfile* GetProfile() const;

    /** Returns the assets of a class in the project, sorted by name */
    static TArray<struct FAssetData> FindAssets(const FTopLevelAssetPath& ClassPath);

    /** Returns the reach profiles of the project as picker options */
    TArray<struct FNsCheatPickerOption> GetProfileOptions() const;

    /** Returns the hunt lists of the project as picker options. Editor only */
    TArray<struct FNsCheatPickerOption> GetHuntOptions() const;

    /** Returns the drive status and the last lines of the drive log */
    FText GetStatusText() const;

    /** Shows a message on screen and in the log */
    static void Notify(const FString& Message, bool bProblem);

// Variables
private:
    /** The radius of the search for a random point */
    float RandomRadius;

    /** Keeps only random points on the landscape */
    bool bLandscapeOnly;

    /** Keeps random points under water too */
    bool bAllowUnderWater;

    /** Draws the way and shows the drive state during a drive */
    bool bShowDriveState;

    /** The number of log lines that the status shows */
    int32 StatusLogLines;

    /** The picked reach profile, as its object path */
    FName PickedProfile;

    /** The picked hunt list, as its object path */
    FName PickedHunt;
};
