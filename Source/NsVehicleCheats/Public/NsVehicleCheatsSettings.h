// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "NsVehicleCheatsSettings.generated.h"

/** The settings of the vehicle cheats: the look, the size, and the place of the vehicle values board */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Ns Vehicle Cheats"))
class NSVEHICLECHEATS_API UNsVehicleCheatsSettings : public UDeveloperSettings
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleCheatsSettings();

// Variables
public:
    /** The widget of the values board. Empty uses a plain text board */
    UPROPERTY(Config, EditAnywhere, Category = "Values Board")
    TSoftClassPtr<class UNsVehicleValuesWidget> ValuesWidgetClass;

    /** The size of the widget of the board in pixels */
    UPROPERTY(Config, EditAnywhere, Category = "Values Board", meta = (ClampMin = "16"))
    FIntPoint BoardDrawSize;

    /** The width of the board as a part of the width of the vehicle. 1 makes the board as wide as the vehicle */
    UPROPERTY(Config, EditAnywhere, Category = "Values Board", meta = (ClampMin = "0.05"))
    float BoardWidthScale;

    /** The offset of the board from the top of the rear of the vehicle, in the space of the vehicle */
    UPROPERTY(Config, EditAnywhere, Category = "Values Board", meta = (Units = "cm"))
    FVector BoardOffset;
};
