// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleSelfRightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleSelfRightComponent::UNsVehicleSelfRightComponent()
{
    bUseSelfRight = false;
    OverturnedUpDot = 0.65f;
    RollPulseInterval = 0.22f;
    HopVelocity = 150.f;
    RollAngularImpulse = 5.f;
    MaxVerticalSpeed = 30.f;
    GroundReach = 3.f;
    GroundChannel = ECC_Visibility;
    LastRollTime = -1000000.0;
}

void UNsVehicleSelfRightComponent::SetSelfRightEnabled(const bool bEnabled)
{
    bUseSelfRight = bEnabled;
}

bool UNsVehicleSelfRightComponent::IsOverturned() const
{
    const UPrimitiveComponent* const Body = GetBody();
    if (Body == nullptr)
    {
        return false;
    }

    // A vehicle on a slope leans with the ground, so the test compares the vehicle with the ground under it, not with the world up
    FHitResult Hit;
    const FVector GroundUp = TraceBodyGround(Hit) ? Hit.ImpactNormal : FVector::UpVector;
    return FVector::DotProduct(Body->GetUpVector(), GroundUp) < OverturnedUpDot;
}

bool UNsVehicleSelfRightComponent::IsBodyOnGround() const
{
    // At the top of a hop the vertical speed is also near zero, so only a body that touches the ground counts
    FHitResult Hit;
    return TraceBodyGround(Hit);
}

bool UNsVehicleSelfRightComponent::TraceBodyGround(FHitResult& OutHit) const
{
    const UPrimitiveComponent* const Body = GetBody();
    const UWorld* const World = GetWorld();
    if (Body == nullptr || World == nullptr)
    {
        return false;
    }

    const FVector Center = Body->Bounds.Origin;
    const FVector End = Center - FVector::UpVector * (Body->Bounds.BoxExtent.Z + GroundReach);
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NsVehicleSelfRightGround), false, GetOwner());
    return World->LineTraceSingleByChannel(OutHit, Center, End, GroundChannel, QueryParams);
}

bool UNsVehicleSelfRightComponent::TryRollFromSteering(const float SteeringInput)
{
    UPrimitiveComponent* const Body = GetBody();
    const UWorld* const World = GetWorld();
    if (!bUseSelfRight || SteeringInput == 0.f || Body == nullptr || World == nullptr || !Body->IsSimulatingPhysics())
    {
        return false;
    }

    const double Now = World->GetTimeSeconds();
    const bool bResting = FMath::Abs(Body->GetPhysicsLinearVelocity().Z) <= MaxVerticalSpeed;
    if (Now - LastRollTime < RollPulseInterval || !bResting || !IsOverturned() || !IsBodyOnGround())
    {
        return false;
    }

    LastRollTime = Now;
    Body->AddImpulse(FVector::UpVector * HopVelocity, NAME_None, true);
    Body->AddAngularImpulseInRadians(Body->GetForwardVector() * SteeringInput * RollAngularImpulse, NAME_None, true);
    return true;
}

void UNsVehicleSelfRightComponent::UpdateSelfRight()
{
    const ANsVehicle* const Vehicle = GetVehicle();
    if (bUseSelfRight && Vehicle != nullptr)
    {
        TryRollFromSteering(Vehicle->GetControls()->GetSteeringInput());
    }
}

UPrimitiveComponent* UNsVehicleSelfRightComponent::GetBody() const
{
    const AActor* const Owner = GetOwner();
    return Owner != nullptr ? Cast<UPrimitiveComponent>(Owner->GetRootComponent()) : nullptr;
}
