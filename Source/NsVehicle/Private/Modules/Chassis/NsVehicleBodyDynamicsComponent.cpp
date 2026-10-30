// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleBodyDynamicsComponent.h"
#include "AsyncTickFunctions.h"
#include "GameFramework/Actor.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

void UNsVehicleBodyDynamicsComponent::InitializeLeanSteer(UNsVehicleSuspensionComponent* SuspensionComponent)
{
    if (SuspensionComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle '%s' got no suspension component in InitializeLeanSteer. Lean steer stays off"), *GetPathNameSafe(GetOwner()));
        return;
    }

    LeanSteerSuspension = SuspensionComponent;
}

void UNsVehicleBodyDynamicsComponent::ApplyAccelerationPitch(UPrimitiveComponent* PhysicsBody, const FVector& Right, const float DriveForce) const
{
    if (DynamicPitchMomentMultiplier != 0)
    {
        UAsyncTickFunctions::ATP_AddTorque(PhysicsBody, -Right * DriveForce * 100.f * DynamicPitchMomentMultiplier, false);
    }
}

void UNsVehicleBodyDynamicsComponent::ApplyBrakePitch(UPrimitiveComponent* PhysicsBody, const FVector& Right, const float BrakeForce) const
{
    if (DynamicBrakeMomentMultiplier != 0)
    {
        UAsyncTickFunctions::ATP_AddTorque(PhysicsBody, -Right * BrakeForce * 100.f * DynamicBrakeMomentMultiplier, false);
    }
}

void UNsVehicleBodyDynamicsComponent::ApplyLeanSteer(UPrimitiveComponent* PhysicsBody, const FVector& Up, const FVector& Forward, const float SteeringInput, const float SpeedKPH, const bool bInAir) const
{
    if (!bUseSteerLean || LeanSteerSuspension == nullptr || bInAir)
    {
        return;
    }

    const float LeanAlpha = FVector::DotProduct(LeanSteerSuspension->GetHitResult().ImpactNormal, Up);
    const float LeanScale = FMath::GetMappedRangeValueClamped(SteerLeanMapping, FVector2D(0.f, SteerLeanCoefficient), LeanAlpha);
    const float LeanStrength = FMath::GetMappedRangeValueClamped(FVector2D(0.f, SteerLeanMinSpeed), FVector2D(0.f, 1.f), SpeedKPH);
    UAsyncTickFunctions::ATP_AddTorque(PhysicsBody, SteeringInput * -1.f * LeanScale * LeanStrength * Forward, true);
}
