// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/** The log of the NsVehicles cheats: the drive tools and the values board */
NSVEHICLECHEATS_API DECLARE_LOG_CATEGORY_EXTERN(LogNsVehicleCheats, Log, All);

/** The module of the NsVehicles cheats. The Vehicle AI tab of the NsCheats window comes from UNsVehicleCheatsSubsystem */
class FNsVehicleCheatsModule : public IModuleInterface
{
};
