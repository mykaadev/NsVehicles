// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interior/NsVehicleInteriorComponent.h"
#include "NsVehicleSpeedometerComponent.generated.h"

/** Turns a speedometer needle from the speed in KPH */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleSpeedometerComponent : public UNsVehicleInteriorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleSpeedometerComponent();

protected:
    //~ Begin UNsVehicleInteriorComponent Interface
    virtual float GetDriveValue(const ANsVehicle& Vehicle) const override;
    //~ End UNsVehicleInteriorComponent Interface
};
