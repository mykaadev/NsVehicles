// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "NsVehicleWheel.generated.h"

/** Renders a wheel that the take recorder animation system spins */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleWheel : public UStaticMeshComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleWheel();

    /** Returns true when the wheel spin is inverted, usually a left wheel */
    bool IsLeftWheel();

// Variables
private:
    /** Inverts the wheel spin. Use it when the wheel faces the wrong direction, usually on the left side */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Animation System", meta = (AllowPrivateAccess = "true"))
    bool bRotateWheel = false;
};
