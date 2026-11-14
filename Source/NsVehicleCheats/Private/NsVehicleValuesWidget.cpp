// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleValuesWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Modules/Chassis/NsVehicleIdleLockComponent.h"
#include "Modules/Chassis/NsVehicleSelfRightComponent.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Modules/Powertrain/NsVehicleEngineComponent.h"
#include "Modules/Powertrain/NsVehicleTransmissionComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Vehicle/NsVehicle.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

void UNsVehicleValuesWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // A board with no layout still shows the values. Code makes the one text block, because the plugin has no Widget Blueprint asset. The text keeps the default style
    if (ValuesText == nullptr && WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
    {
        ValuesText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ValuesText"));
        WidgetTree->RootWidget = ValuesText;
    }
}

void UNsVehicleValuesWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (ValuesText != nullptr)
    {
        ValuesText->SetText(GetValuesText());
    }
}

void UNsVehicleValuesWidget::SetVehicle(ANsVehicle* const InVehicle)
{
    Vehicle = InVehicle;
}

ANsVehicle* UNsVehicleValuesWidget::GetVehicle() const
{
    return Vehicle.Get();
}

FNsVehicleValues UNsVehicleValuesWidget::GetValues() const
{
    return ReadValues(Vehicle.Get());
}

FText UNsVehicleValuesWidget::GetValuesText() const
{
    const FNsVehicleValues Values = GetValues();
    FString Text = FString::Printf(TEXT("%.1f km/h   gear %d   %.0f rpm\nthrottle %.2f   brake %.2f   steer %+.2f"),
        Values.SpeedKph, Values.Gear, Values.RPM, Values.Throttle, Values.Brake, Values.Steering);
    if (Values.bInAir || Values.bIdleLocked || Values.bParkedHeld || Values.bOverturned)
    {
        Text += FString::Printf(TEXT("\n%s%s%s%s"), Values.bInAir ? TEXT("in air  ") : TEXT(""), Values.bIdleLocked ? TEXT("idle lock  ") : TEXT(""),
            Values.bParkedHeld ? TEXT("parked hold  ") : TEXT(""), Values.bOverturned ? TEXT("overturned") : TEXT(""));
    }
    for (const FNsVehicleWheelValues& Wheel : Values.Wheels)
    {
        const TCHAR* const State = Wheel.bDetached ? TEXT("detached") : Wheel.bInAir ? TEXT("air") : TEXT("ground");
        Text += FString::Printf(TEXT("\n%s: %.2f %s %s grip %.2f"), *Wheel.Name.ToString(), Wheel.Compression, State, *Wheel.Surface.ToString(), Wheel.GripScale);
    }
    return FText::FromString(Text);
}

FNsVehicleValues UNsVehicleValuesWidget::ReadValues(ANsVehicle* const InVehicle)
{
    FNsVehicleValues Values;
    if (InVehicle == nullptr)
    {
        return Values;
    }

    const UNsVehicleControlsComponent* const Controls = InVehicle->GetControls();
    Values.SpeedKph = InVehicle->GetSpeedKPH();
    Values.Gear = InVehicle->GetTransmission()->GetCurrentGear();
    Values.RPM = InVehicle->GetEngine()->GetRPM();
    Values.Throttle = Controls->GetThrottleInput();
    Values.Brake = Controls->GetBrakesInput();
    Values.Steering = Controls->GetSteeringInput();
    Values.bInAir = InVehicle->IsInAir();
    Values.bIdleLocked = InVehicle->GetIdleLock()->IsIdleLocked();
    Values.bParkedHeld = InVehicle->GetIdleLock()->IsParkedHeld();
    Values.bOverturned = InVehicle->GetSelfRight()->IsOverturned();

    for (UNsVehicleSuspensionComponent* const Suspension : InVehicle->GetSuspension())
    {
        if (Suspension == nullptr)
        {
            continue;
        }

        FNsVehicleWheelValues& Wheel = Values.Wheels.AddDefaulted_GetRef();
        Wheel.Name = Suspension->GetFName();
        Wheel.Compression = Suspension->GetCompression();
        Wheel.bInAir = Suspension->IsInAir();
        Wheel.bDetached = Suspension->IsWheelDetached();
        const UPhysicalMaterial* const Material = Suspension->GetHitResult().PhysMaterial.Get();
        Wheel.Surface = Material != nullptr ? Material->GetFName() : NAME_None;
        Wheel.GripScale = Suspension->GetSurfaceGripScale();
    }
    return Values;
}
