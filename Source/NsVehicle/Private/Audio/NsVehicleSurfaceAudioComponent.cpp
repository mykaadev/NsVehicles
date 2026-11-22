// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Audio/NsVehicleSurfaceAudioComponent.h"

#include "Kismet/GameplayStatics.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Vehicle/NsVehicle.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

UNsVehicleSurfaceAudioComponent::UNsVehicleSurfaceAudioComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Behavior, not look: the tick picks the loop of the surface, so the component must not play a sound by itself
    bAutoActivate = false;
    PrimaryComponentTick.bCanEverTick = true;

    // The surface and the speed change slowly for the ear, so ten checks each second are enough
    PrimaryComponentTick.TickInterval = 0.1f;

    WaterSurface = SurfaceType_Default;
    FullSpeedKph = 15.f;
    FullSpeedVolume = 0.7f;
    SplashSpeedKph = 2.f;
    FadeSeconds = 0.15f;
    MinRollSpeedKph = 0.5f;
    SwitchChecks = 3;
    RollSurface = SurfaceType_Default;
    bRolling = false;
    CandidateSurface = SurfaceType_Default;
    CandidateChecks = 0;
}

void UNsVehicleSurfaceAudioComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!GetOwner()->IsA<ANsVehicle>())
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle surface audio '%s' needs an ANsVehicle owner. It plays no sound"), *GetPathNameSafe(this));
        SetComponentTickEnabled(false);
    }
}

void UNsVehicleSurfaceAudioComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const ANsVehicle* const Vehicle = Cast<ANsVehicle>(GetOwner());
    if (Vehicle == nullptr)
    {
        return;
    }

    // 1 cm/s is 0.036 km/h
    const float SpeedKph = Vehicle->GetVelocity().Size() * 0.036f;
    EPhysicalSurface Surface = SurfaceType_Default;
    const bool bOnGround = FindRollSurface(*Vehicle, Surface);

    // A wheel on a border of two surfaces can change the result at each check. A new surface counts only after it stays
    if (Surface != CandidateSurface)
    {
        CandidateSurface = Surface;
        CandidateChecks = 0;
    }
    CandidateChecks = FMath::Min(CandidateChecks + 1, SwitchChecks);
    const EPhysicalSurface StableSurface = bRolling && CandidateChecks < SwitchChecks ? RollSurface.GetValue() : Surface;

    const TObjectPtr<USoundBase>* const Found = RollLoops.Find(StableSurface);
    USoundBase* const Loop = Found != nullptr ? Found->Get() : DefaultRollLoop.Get();
    if (!bOnGround || SpeedKph < MinRollSpeedKph || Loop == nullptr)
    {
        if (bRolling)
        {
            FadeOut(FadeSeconds, 0.f);
            bRolling = false;
        }
        return;
    }

    if (!bRolling || StableSurface != RollSurface)
    {
        if (WaterSurface != SurfaceType_Default && StableSurface == WaterSurface && RollSurface != WaterSurface && SpeedKph > SplashSpeedKph && SplashSound != nullptr)
        {
            UGameplayStatics::PlaySoundAtLocation(this, SplashSound, GetComponentLocation());
        }

        // SetSound on a playing component restarts it at full volume, so the old loop stops first and the new loop fades in
        if (IsPlaying())
        {
            Stop();
        }
        RollSurface = StableSurface;
        SetSound(Loop);
        FadeIn(FadeSeconds);
        bRolling = true;
    }
    SetVolumeMultiplier(FMath::Clamp(SpeedKph / FullSpeedKph, 0.f, 1.f) * FullSpeedVolume);
}

bool UNsVehicleSurfaceAudioComponent::FindRollSurface(const ANsVehicle& Vehicle, EPhysicalSurface& OutSurface)
{
    TArray<TPair<EPhysicalSurface, int32>, TInlineAllocator<4>> Counts;
    for (UNsVehicleSuspensionComponent* const Suspension : Vehicle.GetSuspension())
    {
        if (Suspension == nullptr || Suspension->IsWheelDetached() || Suspension->IsInAir())
        {
            continue;
        }
        const EPhysicalSurface Surface = UPhysicalMaterial::DetermineSurfaceType(Suspension->GetHitResult().PhysMaterial.Get());
        TPair<EPhysicalSurface, int32>* const Count = Counts.FindByPredicate([Surface](const TPair<EPhysicalSurface, int32>& Entry) { return Entry.Key == Surface; });
        if (Count != nullptr)
        {
            ++Count->Value;
        }
        else
        {
            Counts.Emplace(Surface, 1);
        }
    }

    int32 Best = 0;
    for (const TPair<EPhysicalSurface, int32>& Count : Counts)
    {
        if (Count.Value > Best)
        {
            Best = Count.Value;
            OutSurface = Count.Key;
        }
    }
    return Best > 0;
}
