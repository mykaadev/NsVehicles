// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Interior/NsVehicleThrottlePedalComponent.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleThrottlePedalComponent::UNsVehicleThrottlePedalComponent()
{
    InputRange = FVector2D(0.f, 1.f);
    OutputRange = FVector2D(0.f, 20.f);
    RotationAxis = ENsVehicleRotatorAxis::Pitch;
}

float UNsVehicleThrottlePedalComponent::GetDriveValue(const ANsVehicle& Vehicle) const
{
    return Vehicle.GetControls()->GetThrottleInput();
}
