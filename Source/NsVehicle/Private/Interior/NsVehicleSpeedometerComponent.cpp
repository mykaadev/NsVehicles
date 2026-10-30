// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Interior/NsVehicleSpeedometerComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleSpeedometerComponent::UNsVehicleSpeedometerComponent()
{
    InputRange = FVector2D(0.f, 200.f);
    OutputRange = FVector2D(0.f, 270.f);
    RotationAxis = ENsVehicleRotatorAxis::Roll;
}

float UNsVehicleSpeedometerComponent::GetDriveValue(const ANsVehicle& Vehicle) const
{
    return Vehicle.GetSpeedKPH();
}
