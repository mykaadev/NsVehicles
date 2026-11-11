// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

NSVEHICLEEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogNsVehicleEditor, Log, All);

/** The editor module of NsVehicles. It holds the reach tool and its console command */
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

// Variables
private:
    /** The console command of the reach tool */
    class IConsoleObject* ReachCommand = nullptr;
};
