// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interior/NsVehicleInteriorComponent.h"
#include "NsVehicleTachometerComponent.generated.h"

/** Turns a tachometer needle from the engine RPM */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleTachometerComponent : public UNsVehicleInteriorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleTachometerComponent();

protected:
    //~ Begin UNsVehicleInteriorComponent Interface
    virtual float GetDriveValue(const ANsVehicle& Vehicle) const override;
    //~ End UNsVehicleInteriorComponent Interface
};
