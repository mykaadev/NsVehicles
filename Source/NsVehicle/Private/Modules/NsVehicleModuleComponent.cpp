// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/NsVehicleModuleComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleModuleComponent::UNsVehicleModuleComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UNsVehicleModuleComponent::OnRegister()
{
    Super::OnRegister();

    OwnerVehicle = Cast<ANsVehicle>(GetOwner());

    if (OwnerVehicle == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle module '%s' must be on an ANsVehicle actor"), *GetPathNameSafe(this));
    }
}

ANsVehicle* UNsVehicleModuleComponent::GetVehicle() const
{
    return OwnerVehicle;
}
