// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleCheatsSubsystem.h"

#include "AI/NsVehicleAI.h"
#include "AI/NsVehiclePathfinder.h"
#include "AI/NsVehicleReachProfile.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Vehicle/NsVehicle.h"

#if WITH_NSCHEATS
#include "NsCheatsSubsystem.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogNsVehicleCheats, Log, All);

#define LOCTEXT_NAMESPACE "NsVehicleCheats"

UNsVehicleCheatsSubsystem::UNsVehicleCheatsSubsystem()
{
    RandomRadius = 5000.f;
    bLandscapeOnly = false;
    bAllowUnderWater = false;
    bShowDriveState = true;
    StatusLogLines = 8;
}

bool UNsVehicleCheatsSubsystem::ShouldCreateSubsystem(UObject* const Outer) const
{
#if UE_BUILD_SHIPPING || !WITH_NSCHEATS
    return false;
#else
    // The tab is for a game world, not for the editor world or a preview
    const UWorld* const World = Cast<UWorld>(Outer);
    return World != nullptr && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
#endif
}

void UNsVehicleCheatsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RegisterCheats();
}

void UNsVehicleCheatsSubsystem::Deinitialize()
{
#if WITH_NSCHEATS
    const UGameInstance* const GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
    if (UNsCheatsSubsystem* const Cheats = GameInstance != nullptr ? GameInstance->GetSubsystem<UNsCheatsSubsystem>() : nullptr)
    {
        Cheats->UnregisterCheatsByOwner(this);
    }
#endif
    Super::Deinitialize();
}

void UNsVehicleCheatsSubsystem::RegisterCheats()
{
#if WITH_NSCHEATS
    const UGameInstance* const GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
    UNsCheatsSubsystem* const Cheats = GameInstance != nullptr ? GameInstance->GetSubsystem<UNsCheatsSubsystem>() : nullptr;
    if (Cheats == nullptr)
    {
        return;
    }

    const TWeakObjectPtr<UNsVehicleCheatsSubsystem> WeakThis(this);
    FNsCheatBuilder Builder = Cheats->ForProvider(this, TEXT("Vehicle AI"));
    Builder.Section(TEXT("Drive"))
        .Picker(TEXT("NsVehicle.Cheats.Profile"), LOCTEXT("Profile", "Tire profile"),
            FNsCheatGetPickerOptionsDelegate::CreateUObject(this, &ThisClass::GetProfileOptions),
            FNsCheatGetPickerValueDelegate::CreateLambda([WeakThis]() { const UNsVehicleReachProfile* const Profile = WeakThis.IsValid() ? WeakThis->GetProfile() : nullptr; return Profile != nullptr ? FName(*Profile->GetPathName()) : NAME_None; }),
            FNsCheatSetPickerValueDelegate::CreateLambda([WeakThis](const FName Value) { if (WeakThis.IsValid()) { WeakThis->PickedProfile = Value; } }),
            FNsCheatExecuteDelegate(),
            LOCTEXT("ProfileDescription", "The reach profile that the plan uses: the step, the slope, the drop, and the width of the wheels."))
        .Button(TEXT("NsVehicle.Cheats.DrivePicked"), LOCTEXT("DrivePicked", "Drive to a picked point"),
            FNsCheatExecuteDelegate::CreateUObject(this, &ThisClass::DriveToPickedPoint),
            LOCTEXT("DrivePickedDescription", "A free camera opens. Fly with the mouse and WASD, left click on the ground to pick the goal, or press Esc to cancel."))
        .Button(TEXT("NsVehicle.Cheats.DriveRandom"), LOCTEXT("DriveRandom", "Drive to a random point"),
            FNsCheatExecuteDelegate::CreateUObject(this, &ThisClass::DriveToRandomPoint),
            LOCTEXT("DriveRandomDescription", "Picks a random point in the radius around the vehicle with the filters below, and drives there."))
        .Float(TEXT("NsVehicle.Cheats.Radius"), LOCTEXT("Radius", "Random radius (m)"),
            FNsCheatGetFloatDelegate::CreateLambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->RandomRadius * 0.01f : 0.f; }),
            FNsCheatSetFloatDelegate::CreateLambda([WeakThis](const float Value) { if (WeakThis.IsValid()) { WeakThis->RandomRadius = FMath::Max(Value, 1.f) * 100.f; } }))
        .Toggle(TEXT("NsVehicle.Cheats.LandscapeOnly"), LOCTEXT("LandscapeOnly", "Landscape only"),
            FNsCheatGetBoolDelegate::CreateLambda([WeakThis]() { return WeakThis.IsValid() && WeakThis->bLandscapeOnly; }),
            FNsCheatSetBoolDelegate::CreateLambda([WeakThis](const bool bValue) { if (WeakThis.IsValid()) { WeakThis->bLandscapeOnly = bValue; } }),
            LOCTEXT("LandscapeOnlyDescription", "A random point is on the landscape, not on a prop or a building."))
        .Toggle(TEXT("NsVehicle.Cheats.UnderWater"), LOCTEXT("UnderWater", "Under water allowed"),
            FNsCheatGetBoolDelegate::CreateLambda([WeakThis]() { return WeakThis.IsValid() && WeakThis->bAllowUnderWater; }),
            FNsCheatSetBoolDelegate::CreateLambda([WeakThis](const bool bValue) { if (WeakThis.IsValid()) { WeakThis->bAllowUnderWater = bValue; } }),
            LOCTEXT("UnderWaterDescription", "A random point can be on the bed under water. Off keeps only dry ground."))
        .Toggle(TEXT("NsVehicle.Cheats.ShowState"), LOCTEXT("ShowState", "Show the way and the state"),
            FNsCheatGetBoolDelegate::CreateLambda([WeakThis]() { return WeakThis.IsValid() && WeakThis->bShowDriveState; }),
            FNsCheatSetBoolDelegate::CreateLambda([WeakThis](const bool bValue) { if (WeakThis.IsValid()) { WeakThis->bShowDriveState = bValue; if (UNsVehicleAI* const AI = WeakThis->FindOrAddAI()) { AI->SetDrawDebug(bValue); } } }),
            LOCTEXT("ShowStateDescription", "Draws the way on the ground and shows the state line during a drive."))
        .Button(TEXT("NsVehicle.Cheats.RepeatStart"), LOCTEXT("RepeatStart", "Repeat from the same start"),
            FNsCheatExecuteDelegate::CreateLambda([WeakThis]() { if (WeakThis.IsValid()) { WeakThis->RepeatLastDrive(true); } }),
            LOCTEXT("RepeatStartDescription", "Moves the vehicle back to the start of the last drive and drives it again. Use it to test a fix."))
        .Button(TEXT("NsVehicle.Cheats.RepeatHere"), LOCTEXT("RepeatHere", "Repeat from here"),
            FNsCheatExecuteDelegate::CreateLambda([WeakThis]() { if (WeakThis.IsValid()) { WeakThis->RepeatLastDrive(false); } }),
            LOCTEXT("RepeatHereDescription", "Drives to the goal of the last drive from where the vehicle is now."))
        .Button(TEXT("NsVehicle.Cheats.Stop"), LOCTEXT("Stop", "Stop the drive"),
            FNsCheatExecuteDelegate::CreateUObject(this, &ThisClass::StopDrive));

    Builder.Section(TEXT("Report"))
        .DebugText(TEXT("NsVehicle.Cheats.Status"), LOCTEXT("Status", "Drive status"),
            FNsCheatDebugTextDelegate::CreateUObject(this, &ThisClass::GetStatusText))
        .Button(TEXT("NsVehicle.Cheats.LastStall"), LOCTEXT("LastStall", "Go to the last stall"),
            FNsCheatExecuteDelegate::CreateUObject(this, &ThisClass::GoToLastStall),
            LOCTEXT("LastStallDescription", "Moves the vehicle to where the last drive gave up."))
        .Button(TEXT("NsVehicle.Cheats.SaveReport"), LOCTEXT("SaveReport", "Save the drive report"),
            FNsCheatExecuteDelegate::CreateUObject(this, &ThisClass::SaveDriveReport),
            LOCTEXT("SaveReportDescription", "Writes the log of the last drive to Saved/QA/Drive_<time>.md."));

#if WITH_EDITOR
    // The hunt runs in the editor module of NsVehicles, so it is in Play In Editor only
    Builder.Section(TEXT("Hunt"))
        .Picker(TEXT("NsVehicle.Cheats.HuntList"), LOCTEXT("HuntList", "Hunt list"),
            FNsCheatGetPickerOptionsDelegate::CreateUObject(this, &ThisClass::GetHuntOptions),
            FNsCheatGetPickerValueDelegate::CreateLambda([WeakThis]() { return WeakThis.IsValid() ? WeakThis->PickedHunt : NAME_None; }),
            FNsCheatSetPickerValueDelegate::CreateLambda([WeakThis](const FName Value) { if (WeakThis.IsValid()) { WeakThis->PickedHunt = Value; } }))
        .Button(TEXT("NsVehicle.Cheats.RunHunt"), LOCTEXT("RunHunt", "Run the hunt"),
            FNsCheatExecuteDelegate::CreateLambda([WeakThis]()
            {
                if (WeakThis.IsValid() && !WeakThis->PickedHunt.IsNone() && GEngine != nullptr)
                {
                    GEngine->Exec(WeakThis->GetWorld(), *FString::Printf(TEXT("NsVehicle.Hunt %s"), *WeakThis->PickedHunt.ToString()));
                }
            }),
            LOCTEXT("RunHuntDescription", "Drives to each target of the hunt list and writes a report to Saved/NsVehicle."))
        .Button(TEXT("NsVehicle.Cheats.StopHunt"), LOCTEXT("StopHunt", "Stop the hunt"),
            FNsCheatExecuteDelegate::CreateLambda([WeakThis]() { if (WeakThis.IsValid() && GEngine != nullptr) { GEngine->Exec(WeakThis->GetWorld(), TEXT("NsVehicle.HuntStop")); } }));
#endif
#endif
}

void UNsVehicleCheatsSubsystem::DriveToPickedPoint()
{
#if WITH_NSCHEATS
    UNsCheatsSubsystem* const Cheats = GetWorld()->GetGameInstance() != nullptr ? GetWorld()->GetGameInstance()->GetSubsystem<UNsCheatsSubsystem>() : nullptr;
    if (Cheats == nullptr)
    {
        return;
    }
    const TWeakObjectPtr<UNsVehicleCheatsSubsystem> WeakThis(this);
    const bool bStarted = Cheats->PickWorldPoint(FNsCheatPointPickedDelegate::CreateLambda([WeakThis](const bool bPicked, const FVector& Point)
    {
        if (bPicked && WeakThis.IsValid())
        {
            WeakThis->DriveTo(Point);
        }
    }));
    if (!bStarted)
    {
        Notify(TEXT("The point picker could not start. See the log"), true);
    }
#endif
}

void UNsVehicleCheatsSubsystem::DriveToRandomPoint()
{
    const ANsVehicle* const Vehicle = FindPlayerVehicle();
    if (Vehicle == nullptr)
    {
        Notify(TEXT("Drive to a random point needs a vehicle of the player"), true);
        return;
    }
    FNsVehiclePointFilter Filter;
    Filter.bLandscapeOnly = bLandscapeOnly;
    Filter.bAllowUnderWater = bAllowUnderWater;
    FVector Goal;
    if (!UNsVehiclePathfinder::FindRandomPoint(this, Vehicle->GetActorLocation(), RandomRadius, Filter, FNsVehiclePathQuery(), Goal))
    {
        Notify(FString::Printf(TEXT("No random point in %.0f m that the filters keep"), RandomRadius * 0.01f), true);
        return;
    }
    DriveTo(Goal);
}

bool UNsVehicleCheatsSubsystem::DriveTo(const FVector Goal)
{
    UNsVehicleAI* const AI = FindOrAddAI();
    UNsVehicleReachProfile* const Profile = GetProfile();
    if (AI == nullptr || Profile == nullptr)
    {
        Notify(TEXT("Drive To needs a vehicle of the player and a reach profile in the project"), true);
        return false;
    }
    AI->SetDrawDebug(bShowDriveState);
    AI->SetPathQuery(FNsVehiclePathQuery());
    if (!AI->StartDriveTo(Goal, Profile))
    {
        Notify(FString::Printf(TEXT("No way to %s with %s"), *Goal.ToCompactString(), *Profile->GetName()), true);
        return false;
    }
    Notify(FString::Printf(TEXT("Drive to %s with %s, %d waypoints"), *Goal.ToCompactString(), *Profile->GetName(), AI->GetWaypoints().Num()), false);
    return true;
}

void UNsVehicleCheatsSubsystem::RepeatLastDrive(const bool bFromSameStart)
{
    UNsVehicleAI* const AI = FindOrAddAI();
    if (AI == nullptr || !AI->RepeatLastDrive(bFromSameStart))
    {
        Notify(TEXT("No last drive to repeat, or no way from here"), true);
    }
}

void UNsVehicleCheatsSubsystem::StopDrive()
{
    if (UNsVehicleAI* const AI = FindOrAddAI())
    {
        AI->StopDrive();
    }
}

void UNsVehicleCheatsSubsystem::GoToLastStall()
{
    ANsVehicle* const Vehicle = FindPlayerVehicle();
    const UNsVehicleAI* const AI = FindOrAddAI();
    if (Vehicle == nullptr || AI == nullptr || AI->GetLastStuckCause().IsEmpty())
    {
        Notify(TEXT("No stall in the last drive"), true);
        return;
    }
    Vehicle->SetActorLocation(AI->GetLastStuckLocation() + FVector(0.f, 0.f, 20.f), false, nullptr, ETeleportType::TeleportPhysics);
    if (UPrimitiveComponent* const Body = Cast<UPrimitiveComponent>(Vehicle->GetRootComponent()))
    {
        Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
}

void UNsVehicleCheatsSubsystem::SaveDriveReport()
{
    const UNsVehicleAI* const AI = FindOrAddAI();
    const FString Path = AI != nullptr ? AI->SaveDriveReport() : FString();
    Notify(Path.IsEmpty() ? TEXT("No drive to report") : FString::Printf(TEXT("Drive report: %s"), *Path), Path.IsEmpty());
}

ANsVehicle* UNsVehicleCheatsSubsystem::FindPlayerVehicle() const
{
    const APlayerController* const Controller = GetWorld()->GetFirstPlayerController();
    APawn* const Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
    if (Pawn == nullptr)
    {
        return nullptr;
    }
    if (ANsVehicle* const Vehicle = Cast<ANsVehicle>(Pawn))
    {
        return Vehicle;
    }

    // The player can stand beside the vehicle, or ride in a vehicle that does not possess the player
    ANsVehicle* Nearest = nullptr;
    double NearestDistance = TNumericLimits<double>::Max();
    for (TActorIterator<ANsVehicle> It(GetWorld()); It; ++It)
    {
        const double Distance = FVector::DistSquared(It->GetActorLocation(), Pawn->GetActorLocation());
        if (Distance < NearestDistance)
        {
            Nearest = *It;
            NearestDistance = Distance;
        }
    }
    return Nearest;
}

UNsVehicleAI* UNsVehicleCheatsSubsystem::FindOrAddAI() const
{
    ANsVehicle* const Vehicle = FindPlayerVehicle();
    if (Vehicle == nullptr)
    {
        return nullptr;
    }
    UNsVehicleAI* AI = Vehicle->FindComponentByClass<UNsVehicleAI>();
    if (AI == nullptr)
    {
        AI = NewObject<UNsVehicleAI>(Vehicle);
        AI->RegisterComponent();
    }
    return AI;
}

UNsVehicleReachProfile* UNsVehicleCheatsSubsystem::GetProfile() const
{
    if (!PickedProfile.IsNone())
    {
        if (UNsVehicleReachProfile* const Picked = LoadObject<UNsVehicleReachProfile>(nullptr, *PickedProfile.ToString()))
        {
            return Picked;
        }
    }
    const TArray<FAssetData> Assets = FindAssets(UNsVehicleReachProfile::StaticClass()->GetClassPathName());
    return Assets.Num() > 0 ? Cast<UNsVehicleReachProfile>(Assets[0].GetAsset()) : nullptr;
}

TArray<FAssetData> UNsVehicleCheatsSubsystem::FindAssets(const FTopLevelAssetPath& ClassPath)
{
    TArray<FAssetData> Assets;
    FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByClass(ClassPath, Assets);
    Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
    return Assets;
}

#if WITH_NSCHEATS

TArray<FNsCheatPickerOption> UNsVehicleCheatsSubsystem::GetProfileOptions() const
{
    TArray<FNsCheatPickerOption> Options;
    for (const FAssetData& Asset : FindAssets(UNsVehicleReachProfile::StaticClass()->GetClassPathName()))
    {
        FNsCheatPickerOption& Option = Options.AddDefaulted_GetRef();
        Option.Value = FName(*Asset.GetObjectPathString());
        Option.Label = FText::FromName(Asset.AssetName);
    }
    return Options;
}

TArray<FNsCheatPickerOption> UNsVehicleCheatsSubsystem::GetHuntOptions() const
{
    TArray<FNsCheatPickerOption> Options;
#if WITH_EDITOR
    // The hunt list class is in the editor module, so the search uses its class path and needs no dependency
    for (const FAssetData& Asset : FindAssets(FTopLevelAssetPath(TEXT("/Script/NsVehicleEditor"), TEXT("NsVehicleHuntList"))))
    {
        FNsCheatPickerOption& Option = Options.AddDefaulted_GetRef();
        Option.Value = FName(*Asset.GetObjectPathString());
        Option.Label = FText::FromName(Asset.AssetName);
    }
#endif
    return Options;
}
#endif

FText UNsVehicleCheatsSubsystem::GetStatusText() const
{
    const ANsVehicle* const Vehicle = FindPlayerVehicle();
    const UNsVehicleAI* const AI = Vehicle != nullptr ? Vehicle->FindComponentByClass<UNsVehicleAI>() : nullptr;
    if (AI == nullptr)
    {
        return LOCTEXT("NoDrive", "No drive yet");
    }
    FString Text = AI->GetDriveStatus();
    const TArray<FString>& Log = AI->GetDriveLog();
    for (int32 Index = FMath::Max(0, Log.Num() - StatusLogLines); Index < Log.Num(); ++Index)
    {
        Text += TEXT("\n") + Log[Index];
    }
    return FText::FromString(Text);
}

void UNsVehicleCheatsSubsystem::Notify(const FString& Message, const bool bProblem)
{
    if (bProblem)
    {
        UE_LOG(LogNsVehicleCheats, Warning, TEXT("%s"), *Message);
    }
    else
    {
        UE_LOG(LogNsVehicleCheats, Display, TEXT("%s"), *Message);
    }
    // The engine debug messages use red for a problem and cyan for a note, as the other NsVehicle tools do
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 8.f, bProblem ? FColor::Red : FColor::Cyan, Message);
    }
}

#undef LOCTEXT_NAMESPACE
