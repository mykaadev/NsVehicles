// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Animation/NsVehicleAnimationComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "Vehicle/NsVehicle.h"
#include "Wheels/NsVehicleSuspensionComponent.h"
#include "Wheels/NsVehicleWheel.h"

void UNsVehicleAnimationComponent::InitializeAnimationSystem()
{
    ANsVehicle* const Vehicle = GetVehicle();

    TArray<USceneComponent*> Children;
    Vehicle->GetRootComponent()->GetChildrenComponents(true, Children);

    TArray<UNsVehicleSuspensionComponent*> TaggedSuspensions;
    AnimWheels.Empty();

    for (USceneComponent* Child : Children)
    {
        if (Child->ComponentTags.Num() == 0)
        {
            continue;
        }

        if (Child->ComponentTags[0] == FName("NsVehicleSuspensionComponent"))
        {
            TaggedSuspensions.Add(Cast<UNsVehicleSuspensionComponent>(Child));
        }
        else if (Child->ComponentTags[0] == FName("NsVehicleWheel"))
        {
            AnimWheels.Add(Cast<UNsVehicleWheel>(Child));
        }
    }

    Vehicle->SetSuspensionComponents(TaggedSuspensions);
}

void UNsVehicleAnimationComponent::SimulateInternalAnimation()
{
    UpdateLocationVelocity();

    if (bEnableAnimationMode)
    {
        SimulateInternalWheelAnimation();
    }
}

void UNsVehicleAnimationComponent::SimulateInternalWheelAnimation()
{
    for (UNsVehicleWheel* Wheel : AnimWheels)
    {
        const float Radius = Wheel->GetStaticMesh()->GetBoundingBox().GetSize().Z / 2.f;
        const float AngularVelocity = LocationVelocity.X / (Radius * -1.f);
        const float Direction = Wheel->IsLeftWheel() ? -1.f : 1.f;

        Wheel->AddLocalRotation(FRotator(AngularVelocity * 100.f * Direction * AnimationGlobalWheelSpeedMultiplier, 0.f, 0.f));
    }
}

FVector UNsVehicleAnimationComponent::GetLocationVelocity() const
{
    return LocationVelocity;
}

bool UNsVehicleAnimationComponent::IsAnimationModeEnabled() const
{
    return bEnableAnimationMode;
}

float UNsVehicleAnimationComponent::GetGlobalWheelSpeedMultiplier() const
{
    return AnimationGlobalWheelSpeedMultiplier;
}

void UNsVehicleAnimationComponent::UpdateLocationVelocity()
{
    const FTransform ActorTransform = GetOwner()->GetActorTransform();
    PreviousLocation = CurrentLocation;
    CurrentLocation = ActorTransform.GetLocation();

    const FVector WorldLocationVelocity = (CurrentLocation - PreviousLocation) * (1.f / 100.f) * (1.f / AnimDeltaTime);
    LocationVelocity = UKismetMathLibrary::InverseTransformDirection(ActorTransform, WorldLocationVelocity);
}

void UNsVehicleAnimationComponent::SpinSuspensionWheels(const TArray<UNsVehicleSuspensionComponent*>& Suspensions) const
{
    for (UNsVehicleSuspensionComponent* Suspension : Suspensions)
    {
        const float AngularVelocity = LocationVelocity.X / (Suspension->GetWheelRadius() * -1.f);
        const float Direction = Suspension->IsLeftWheel() ? -1.f : 1.f;
        const float Pitch = (AngularVelocity + Suspension->GetBurnoutRotation()) * 100.f * Direction * Suspension->GetAngularVelocityMultiplier();

        Suspension->GetWheelMeshComponent()->AddLocalRotation(FRotator(Pitch, 0.f, 0.f));
    }
}
