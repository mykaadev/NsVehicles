// Copyright (C) 2025 nulled.softworks. All rights reserved.


#include "Wheels/NsVehicleWheel.h"

UNsVehicleWheel::UNsVehicleWheel()
{
    ComponentTags.Add(FName("NsVehicleWheel"));
}

bool UNsVehicleWheel::IsLeftWheel()
{
    return bRotateWheel;
}
