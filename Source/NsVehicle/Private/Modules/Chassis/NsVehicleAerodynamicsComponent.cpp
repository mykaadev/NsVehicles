// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleAerodynamicsComponent.h"
#include "AsyncTickFunctions.h"

void UNsVehicleAerodynamicsComponent::SetAerodynamicDrag(const float NewCoefficientOfDrag, const float NewCrossSectionArea)
{
    CoefficientOfDrag = FMath::Max(0.f, NewCoefficientOfDrag);
    CrossSectionArea = FMath::Max(0.f, NewCrossSectionArea);
}

float UNsVehicleAerodynamicsComponent::GetDragForce() const
{
    return Drag;
}

void UNsVehicleAerodynamicsComponent::ApplyDrag(UPrimitiveComponent* PhysicsBody, const FVector& Forward, const float LinearVelocity, const float VelocityDirection)
{
    Drag = 0.5f * AirDensity * FMath::Pow(LinearVelocity, 2) * CoefficientOfDrag * CrossSectionArea;
    UAsyncTickFunctions::ATP_AddForce(PhysicsBody, Forward * 100 * Drag * -VelocityDirection, false);
}
