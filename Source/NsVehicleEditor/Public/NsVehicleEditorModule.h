// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "UObject/StrongObjectPtr.h"

NSVEHICLEEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogNsVehicleEditor, Log, All);

/** The editor module of NsVehicles. It holds the reach tool, the drive-to tool, the hunt, and their console commands */
class FNsVehicleEditorModule : public IModuleInterface
{
// Functions
public:
    //~ Begin IModuleInterface
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    //~ End IModuleInterface

private:
    /** Runs the reach tool in the editor world from the console: NsVehicle.Reach X Y Z ProfilePath [CompareProfilePath] */
    static void RunReachCommand(const TArray<FString>& Args);

    /** Drives the vehicle of the player in Play In Editor to a point on a way that the path search finds: NsVehicle.DriveTo X Y Z [ProfilePath] */
    static void RunDriveToCommand(const TArray<FString>& Args);

    /** Stops the drive of NsVehicle.DriveTo */
    static void RunDriveStopCommand(const TArray<FString>& Args);

    /** Returns the vehicle of the first player in Play In Editor: the pawn, or the vehicle nearest to the pawn */
    static class ANsVehicle* FindPlayerVehicle(class UWorld* World);

    /** Returns the AI component of a vehicle, and adds one when it has none */
    static class UNsVehicleAI* FindOrAddAI(class ANsVehicle* Vehicle);

    /** Starts a hunt for weird spots in Play In Editor: NsVehicle.Hunt HuntListPath */
    static void RunHuntCommand(const TArray<FString>& Args);

    /** Stops the hunt and writes its report */
    static void RunHuntStopCommand(const TArray<FString>& Args);

// Variables
private:
    /** The console command of the reach tool */
    class IConsoleObject* ReachCommand = nullptr;

    /** The console command that starts a drive to a point */
    class IConsoleObject* DriveToCommand = nullptr;

    /** The console command that stops the drive */
    class IConsoleObject* DriveStopCommand = nullptr;

    /** The console command that starts a hunt */
    class IConsoleObject* HuntCommand = nullptr;

    /** The console command that stops a hunt */
    class IConsoleObject* HuntStopCommand = nullptr;

    /** The runner of the current hunt */
    static TStrongObjectPtr<class UNsVehicleHuntRunner> HuntRunner;
};
