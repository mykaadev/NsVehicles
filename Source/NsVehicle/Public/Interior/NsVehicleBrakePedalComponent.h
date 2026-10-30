// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interior/NsVehicleInteriorComponent.h"
#include "NsVehicleBrakePedalComponent.generated.h"

/** Turns a brake pedal from the brake input */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleBrakePedalComponent : public UNsVehicleInteriorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleBrakePedalComponent();

protected:
    //~ Begin UNsVehicleInteriorComponent Interface
    virtual float GetDriveValue(const ANsVehicle& Vehicle) const override;
    //~ End UNsVehicleInteriorComponent Interface
};
