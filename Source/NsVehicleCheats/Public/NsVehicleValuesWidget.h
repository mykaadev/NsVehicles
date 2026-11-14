// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NsVehicleValuesWidget.generated.h"

/** The live values of one wheel of a vehicle */
USTRUCT(BlueprintType)
struct NSVEHICLECHEATS_API FNsVehicleWheelValues
{
    GENERATED_BODY()

    /** The name of the suspension component */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    FName Name;

    /** The spring compression from 0 to 1 */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    float Compression = 0.f;

    /** True when the wheel does not touch a surface */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    bool bInAir = false;

    /** True when the wheel is detached from the vehicle */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    bool bDetached = false;

    /** The name of the physical material under the wheel, or None */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    FName Surface;

    /** The surface grip scale of the wheel. It is 1 with the surface grip off */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    float GripScale = 1.f;
};

/** The live values of a vehicle that the values board shows */
USTRUCT(BlueprintType)
struct NSVEHICLECHEATS_API FNsVehicleValues
{
    GENERATED_BODY()

    /** The forward speed in KPH. A drive backward is negative */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    float SpeedKph = 0.f;

    /** The current gear */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    int32 Gear = 0;

    /** The engine RPM */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    float RPM = 0.f;

    /** The throttle input from 0 to 1 */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    float Throttle = 0.f;

    /** The brake input from 0 to 1 */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    float Brake = 0.f;

    /** The steering input from -1 to 1 */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    float Steering = 0.f;

    /** True when the vehicle counts as airborne */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    bool bInAir = false;

    /** True when the idle lock holds the vehicle */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    bool bIdleLocked = false;

    /** True when the parked hold stops the physics of the vehicle */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    bool bParkedHeld = false;

    /** True when the vehicle lies on its side or on its roof */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    bool bOverturned = false;

    /** The values of each wheel */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values")
    TArray<FNsVehicleWheelValues> Wheels;
};

/**
 * The widget of the vehicle values board, a cheat that shows the live values of a vehicle on a board in the world
 * Make a Widget Blueprint with this parent for the look of the board. A text block named ValuesText gets the values as text each frame, so the Blueprint needs no graph
 * With no Blueprint, or with a Blueprint that has no widgets, the widget makes one plain text block
 */
UCLASS(Blueprintable)
class NSVEHICLECHEATS_API UNsVehicleValuesWidget : public UUserWidget
{
    GENERATED_BODY()

// Functions
public:
    //~ Begin UUserWidget Interface
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    //~ End UUserWidget Interface

    /** Sets the vehicle whose values the widget shows */
    void SetVehicle(class ANsVehicle* InVehicle);

    /** Returns the vehicle whose values the widget shows, or null */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Values")
    class ANsVehicle* GetVehicle() const;

    /** Returns the live values of the vehicle. All values are 0 with no vehicle */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Values")
    FNsVehicleValues GetValues() const;

    /** Returns the live values of the vehicle as lines of text */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Values")
    FText GetValuesText() const;

    /** Reads the live values of a vehicle */
    static FNsVehicleValues ReadValues(class ANsVehicle* InVehicle);

// Variables
protected:
    /** The text block that gets the values as text each frame. Optional */
    UPROPERTY(BlueprintReadOnly, Category = "NsVehicle|Values", meta = (BindWidgetOptional))
    TObjectPtr<class UTextBlock> ValuesText;

private:
    /** The vehicle whose values the widget shows */
    TWeakObjectPtr<class ANsVehicle> Vehicle;
};
