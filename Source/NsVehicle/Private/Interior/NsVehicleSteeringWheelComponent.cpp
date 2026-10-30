// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Interior/NsVehicleSteeringWheelComponent.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleSteeringWheelComponent::UNsVehicleSteeringWheelComponent()
{
    InputRange = FVector2D(-1.f, 1.f);
    OutputRange = FVector2D(-450.f, 450.f);
    RotationAxis = ENsVehicleRotatorAxis::Roll;
}

float UNsVehicleSteeringWheelComponent::GetDriveValue(const ANsVehicle& Vehicle) const
{
    return Vehicle.GetControls()->GetSteeringInput();
}
