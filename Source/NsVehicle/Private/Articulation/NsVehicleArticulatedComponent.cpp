// Copyright (C) 2025 nulled.softworks. All rights reserved.


#include "Articulation/NsVehicleArticulatedComponent.h"


UNsVehicleArticulatedComponent::UNsVehicleArticulatedComponent()
{

}


void UNsVehicleArticulatedComponent::BeginPlay()
{
    Super::BeginPlay();

    BaseLocation = GetRelativeLocation();
    BaseRotation = GetRelativeRotation();
}


void UNsVehicleArticulatedComponent::Update(float deltaTime)
{
    FRotator modifiedRotator = BaseRotation;
    FVector modifiedLocation = BaseLocation;

    float t = 60.f / FMath::Pow(deltaTime, -1.f);
    Movement = bClamped ? FMath::Clamp(Movement + (InputValue * MovementSpeed * t), Min, Max) : (Movement + (InputValue * MovementSpeed * t));

    if (bUseSmoothing)
    {
        SmoothMovement = FMath::FInterpTo(SmoothMovement, Movement, deltaTime, SmoothingSpeed);
    }
    else
    {
        SmoothMovement = Movement;
    }


    switch (Axis)
    {
    case ENsVehicleArticulationAxis::X:
        modifiedLocation.X = BaseLocation.X + SmoothMovement;
        break;
    case ENsVehicleArticulationAxis::Y:
        modifiedLocation.Y = BaseLocation.Y + SmoothMovement;
        break;
    case ENsVehicleArticulationAxis::Z:
        modifiedLocation.Z = BaseLocation.Z + SmoothMovement;
        break;
    case ENsVehicleArticulationAxis::Roll:
        modifiedRotator.Roll = BaseRotation.Roll + SmoothMovement;
        break;
    case ENsVehicleArticulationAxis::Pitch:
        modifiedRotator.Pitch = BaseRotation.Pitch + SmoothMovement;
        break;
    case ENsVehicleArticulationAxis::Yaw:
        modifiedRotator.Yaw = BaseRotation.Yaw + SmoothMovement;
        break;
    }

    SetRelativeLocationAndRotation(modifiedLocation, modifiedRotator, true, nullptr, ETeleportType::ResetPhysics);
}


void UNsVehicleArticulatedComponent::SetInput(float value)
{
    InputValue = value;
}


float UNsVehicleArticulatedComponent::GetMovement()
{
    return Movement;
}
