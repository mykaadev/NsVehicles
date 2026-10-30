// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Interior/NsVehicleInteriorComponent.h"

void UNsVehicleInteriorComponent::BeginPlay()
{
    Super::BeginPlay();

    BaseRotation = GetRelativeRotation();
}

void UNsVehicleInteriorComponent::Update(const float DeltaTime, const ANsVehicle& Vehicle)
{
    const float Direction = bInvert ? -1.f : 1.f;
    const float TargetRotation = FMath::GetMappedRangeValueClamped(InputRange, OutputRange, Direction * GetDriveValue(Vehicle));

    Rotation = bUseSmoothing ? FMath::FInterpTo(Rotation, TargetRotation, DeltaTime, SmoothingSpeed) : TargetRotation;

    FRotator NewRotation = BaseRotation;
    switch (RotationAxis)
    {
    case ENsVehicleRotatorAxis::Yaw:
        NewRotation.Yaw = Rotation;
        break;
    case ENsVehicleRotatorAxis::Roll:
        NewRotation.Roll = Rotation;
        break;
    case ENsVehicleRotatorAxis::Pitch:
        NewRotation.Pitch = Rotation;
        break;
    }

    SetRelativeRotation(NewRotation);
}

float UNsVehicleInteriorComponent::GetDriveValue(const ANsVehicle& Vehicle) const
{
    return 0.f;
}
