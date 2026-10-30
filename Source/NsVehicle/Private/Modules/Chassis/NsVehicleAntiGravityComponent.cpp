// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleAntiGravityComponent.h"
#include "AsyncTickFunctions.h"
#include "Components/StaticMeshComponent.h"

void UNsVehicleAntiGravityComponent::InitializeAntiGravity(USceneComponent* AntiGravityComponent, UStaticMeshComponent* CoreMesh)
{
    if (AntiGravityComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle '%s' got no scene component in InitializeAntiGravity. Anti gravity stays off"), *GetPathNameSafe(GetOwner()));
        return;
    }

    if (CoreMesh == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle '%s' got no core mesh in InitializeAntiGravity. Anti gravity stays off"), *GetPathNameSafe(GetOwner()));
        return;
    }

    AntiGravity = AntiGravityComponent;
    AntiGravityCoreMesh = CoreMesh;
}

void UNsVehicleAntiGravityComponent::OverrideAntiGravityState(const bool bOn)
{
    bOverrideAntiGravity = bOn;
}

bool UNsVehicleAntiGravityComponent::IsAntiGravityActive() const
{
    return bAntiGravityActive;
}

void UNsVehicleAntiGravityComponent::UpdateGameThread(const UPrimitiveComponent* PhysicsBody)
{
    if (AntiGravity == nullptr)
    {
        return;
    }

    AntiGravityMass = AntiGravityCoreMesh->GetMass();
    AntiGravity->SetWorldRotation(FRotator(0.f, PhysicsBody->GetComponentRotation().Yaw, 0.f));
}

void UNsVehicleAntiGravityComponent::ApplyAntiGravity(UPrimitiveComponent* PhysicsBody, const FTransform& PhysicsTransform, const bool bInAir, const float DeltaTime)
{
    if (AntiGravity == nullptr || !bAntiGravityEnabled)
    {
        return;
    }

    AntiGravityOffContactTime = bInAir ? AntiGravityOffContactTime + DeltaTime : 0.f;
    bAntiGravityActive = AntiGravityOffContactTime < AntiGravityOffContactThreshold;

    if (!bAntiGravityActive && !bOverrideAntiGravity)
    {
        return;
    }

    const FQuat Rotation = PhysicsTransform.GetRotation();
    const float ZBase = FMath::Clamp(1.f - Rotation.GetUpVector().Dot(FVector(0.f, 0.f, 1.f)), -1.f, 1.f);
    const float XBase = AntiGravity->GetUpVector().Dot(Rotation.GetForwardVector());
    const float YBase = AntiGravity->GetUpVector().Dot(Rotation.GetRightVector());
    const float Mass = AntiGravityMass + AddedMass;

    const FVector ZForce = ZBase * Mass * Rotation.GetUpVector() * -GravityStrength * AntiGravityZStrength;
    const FVector XForce = XBase * Mass * Rotation.GetForwardVector() * GravityStrength * AntiGravityXStrength;
    const FVector YForce = YBase * Mass * Rotation.GetRightVector() * GravityStrength * AntiGravityYStrength;

    UAsyncTickFunctions::ATP_AddForce(PhysicsBody, ZForce, false, NAME_None);
    UAsyncTickFunctions::ATP_AddForce(PhysicsBody, XForce, false, NAME_None);
    UAsyncTickFunctions::ATP_AddForce(PhysicsBody, YForce, false, NAME_None);
}
