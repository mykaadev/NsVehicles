// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Audio/NsVehicleMotorAudioComponent.h"
#include "GameFramework/Pawn.h"
#include "Modules/Powertrain/NsVehicleEngineComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleMotorAudioComponent::UNsVehicleMotorAudioComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Behavior, not look: the controller change of the owner starts the sound, so the sound must not start by itself. The sound follows the speed with the tick only while it runs
    bAutoActivate = false;
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;

    IdlePitch = 0.7f;
    FullSpeedPitch = 1.6f;
    IdleVolume = 0.35f;
    FullSpeedVolume = 0.8f;
    FullSpeedKph = 15.f;
    bFollowEngineRPM = false;
    FadeSeconds = 0.3f;
    bRunning = false;
}

void UNsVehicleMotorAudioComponent::BeginPlay()
{
    Super::BeginPlay();

    if (APawn* const Owner = Cast<APawn>(GetOwner()))
    {
        Owner->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &UNsVehicleMotorAudioComponent::HandleControllerChanged);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle motor audio '%s' must be on a pawn. The motor sound never starts"), *GetPathNameSafe(this));
    }
    RefreshRunning();
}

void UNsVehicleMotorAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (APawn* const Owner = Cast<APawn>(GetOwner()))
    {
        Owner->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UNsVehicleMotorAudioComponent::HandleControllerChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void UNsVehicleMotorAudioComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float Alpha = GetDriveAlpha();
    SetPitchMultiplier(FMath::Lerp(IdlePitch, FullSpeedPitch, Alpha));
    SetVolumeMultiplier(FMath::Lerp(IdleVolume, FullSpeedVolume, Alpha));
}

bool UNsVehicleMotorAudioComponent::IsMotorRunning() const
{
    return bRunning;
}

void UNsVehicleMotorAudioComponent::HandleControllerChanged(APawn* const Pawn, AController* const OldController, AController* const NewController)
{
    RefreshRunning();
}

void UNsVehicleMotorAudioComponent::RefreshRunning()
{
    const APawn* const Owner = Cast<APawn>(GetOwner());
    const bool bDriven = Owner != nullptr && Owner->IsPlayerControlled();
    if (bDriven == bRunning)
    {
        return;
    }

    bRunning = bDriven;
    if (bRunning)
    {
        SetVolumeMultiplier(IdleVolume);
        FadeIn(FadeSeconds);
    }
    else
    {
        FadeOut(FadeSeconds, 0.f);
    }
    SetComponentTickEnabled(bRunning);
}

float UNsVehicleMotorAudioComponent::GetDriveAlpha() const
{
    if (bFollowEngineRPM)
    {
        const ANsVehicle* const Vehicle = Cast<ANsVehicle>(GetOwner());
        const UNsVehicleEngineComponent* const Engine = Vehicle != nullptr ? Vehicle->GetEngine() : nullptr;
        return Engine != nullptr && Engine->GetMaxRPM() > 0.f ? FMath::Clamp(Engine->GetRPM() / Engine->GetMaxRPM(), 0.f, 1.f) : 0.f;
    }

    // The velocity is in cm/s, and 0.036 turns it into km/h. The size counts a drive backward too
    return FMath::Clamp(GetOwner()->GetVelocity().Size() * 0.036f / FMath::Max(FullSpeedKph, 0.1f), 0.f, 1.f);
}
