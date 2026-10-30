// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interior/NsVehicleInteriorComponent.h"
#include "NsVehicleHandbrakeLeverComponent.generated.h"

/** Turns a handbrake lever from the handbrake input */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleHandbrakeLeverComponent : public UNsVehicleInteriorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleHandbrakeLeverComponent();

protected:
    //~ Begin UNsVehicleInteriorComponent Interface
    virtual float GetDriveValue(const ANsVehicle& Vehicle) const override;
    //~ End UNsVehicleInteriorComponent Interface
};
