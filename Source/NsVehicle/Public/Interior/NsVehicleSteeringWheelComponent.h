// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interior/NsVehicleInteriorComponent.h"
#include "NsVehicleSteeringWheelComponent.generated.h"

/** Turns a steering wheel from the steering input */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleSteeringWheelComponent : public UNsVehicleInteriorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleSteeringWheelComponent();

protected:
    //~ Begin UNsVehicleInteriorComponent Interface
    virtual float GetDriveValue(const ANsVehicle& Vehicle) const override;
    //~ End UNsVehicleInteriorComponent Interface
};
