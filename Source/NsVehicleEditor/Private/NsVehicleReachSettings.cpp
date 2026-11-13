// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleReachSettings.h"

UNsVehicleReachSettings::UNsVehicleReachSettings()
{
    GridSpacing = 50.f;
    MaxCells = 400000;
    TraceHeight = 300.f;
    TraceChannel = ECC_WorldStatic;
    WaterClassNameFilter = TEXT("WaterBody");
    ReachedColor = FColor(60, 200, 90);
    CompareOnlyColor = FColor(240, 150, 40);
    PointSize = 6.f;
    DrawSeconds = 120.f;
    PathSlopeCost = 2.f;
    PathSlopeMargin = 0.8f;
    PathEdgeCost = 4.f;
}
