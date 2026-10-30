// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Interior/NsVehicleTachometerComponent.h"
#include "Modules/Powertrain/NsVehicleEngineComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleTachometerComponent::UNsVehicleTachometerComponent()
{
    InputRange = FVector2D(0.f, 7000.f);
    OutputRange = FVector2D(0.f, 270.f);
    RotationAxis = ENsVehicleRotatorAxis::Roll;
}

float UNsVehicleTachometerComponent::GetDriveValue(const ANsVehicle& Vehicle) const
{
    return Vehicle.GetEngine()->GetRPM();
}
