// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "NsVehicleValuesBoardComponent.generated.h"

/**
 * The vehicle values board: a widget in the world that floats above the rear of a vehicle and turns to the camera each frame
 * The cheat subsystem adds it to the vehicle of the player and removes it again. The settings give the widget, the size, and the offset
 */
UCLASS(Transient)
class UNsVehicleValuesBoardComponent : public UWidgetComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleValuesBoardComponent();

    //~ Begin UActorComponent Interface
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** Sets the vehicle, the widget class, the size, and the place of the board from the settings. Call it before the component registers */
    void SetupBoard(class ANsVehicle* InVehicle);

private:
    /** Turns the board to the camera of the first player */
    void FaceCamera();

// Variables
private:
    /** The vehicle whose values the board shows */
    TWeakObjectPtr<class ANsVehicle> Vehicle;
};
