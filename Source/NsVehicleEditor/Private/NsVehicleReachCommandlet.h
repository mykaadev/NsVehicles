// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "NsVehicleReachCommandlet.generated.h"

/**
 * Runs the reach tool with no editor window and writes the reached points to a file
 * Usage: -run=NsVehicleReach -Map=/Game/Maps/MyMap -Start=X,Y,Z -Profile=/Game/Path/Profile [-Compare=/Game/Path/Profile] -Out=Path.csv [-Extent=20000]
 * It loads the World Partition cells in the extent around the start before the run
 */
UCLASS()
class UNsVehicleReachCommandlet : public UCommandlet
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleReachCommandlet();

    //~ Begin UCommandlet Interface
    virtual int32 Main(const FString& Params) override;
    //~ End UCommandlet Interface
};
