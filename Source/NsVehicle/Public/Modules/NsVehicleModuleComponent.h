// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NsVehicleModuleComponent.generated.h"

class ANsVehicle;

/** Base class for one vehicle system, for example the engine or the brakes. ANsVehicle makes one of each module and calls it in a fixed order */
UCLASS(Abstract, ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleModuleComponent : public UActorComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleModuleComponent();

    //~ Begin UActorComponent Interface
    virtual void OnRegister() override;
    //~ End UActorComponent Interface

    /** Returns the vehicle that owns this module */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    ANsVehicle* GetVehicle() const;

// Variables
private:
    /** The vehicle that owns this module, cached when the module registers */
    UPROPERTY(Transient)
    TObjectPtr<ANsVehicle> OwnerVehicle;
};
