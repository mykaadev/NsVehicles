// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Interior/NsVehicleHandbrakeLeverComponent.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleHandbrakeLeverComponent::UNsVehicleHandbrakeLeverComponent()
{
    InputRange = FVector2D(0.f, 1.f);
    OutputRange = FVector2D(0.f, 30.f);
    RotationAxis = ENsVehicleRotatorAxis::Pitch;
}

float UNsVehicleHandbrakeLeverComponent::GetDriveValue(const ANsVehicle& Vehicle) const
{
    return Vehicle.GetControls()->GetHandbrakeInput() ? 1.f : 0.f;
}
