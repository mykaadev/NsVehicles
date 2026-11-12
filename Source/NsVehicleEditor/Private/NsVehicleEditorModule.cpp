// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleEditorModule.h"

#include "AI/NsVehicleAI.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "NsVehicleReachLibrary.h"
#include "NsVehicleReachProfile.h"
#include "NsVehicleReachSettings.h"
#include "Vehicle/NsVehicle.h"

DEFINE_LOG_CATEGORY(LogNsVehicleEditor);

TWeakObjectPtr<AActor> FNsVehicleEditorModule::DriveSplineActor;

void FNsVehicleEditorModule::StartupModule()
{
    ReachCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("NsVehicle.Reach"),
        TEXT("Paints the ground that a vehicle reaches. Usage: NsVehicle.Reach X Y Z ProfilePath [CompareProfilePath]"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&FNsVehicleEditorModule::RunReachCommand),
        ECVF_Default);
    DriveToCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("NsVehicle.DriveTo"),
        TEXT("In Play In Editor, drives the vehicle of the player to a point on a way that the profile can drive. Usage: NsVehicle.DriveTo X Y Z [ProfilePath]"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&FNsVehicleEditorModule::RunDriveToCommand),
        ECVF_Default);
    DriveStopCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("NsVehicle.DriveStop"),
        TEXT("Stops the drive of NsVehicle.DriveTo"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&FNsVehicleEditorModule::RunDriveStopCommand),
        ECVF_Default);
}

void FNsVehicleEditorModule::ShutdownModule()
{
    for (IConsoleObject** Command : { &ReachCommand, &DriveToCommand, &DriveStopCommand })
    {
        if (*Command != nullptr)
        {
            IConsoleManager::Get().UnregisterConsoleObject(*Command);
            *Command = nullptr;
        }
    }
}

ANsVehicle* FNsVehicleEditorModule::FindPlayerVehicle(UWorld* const World)
{
    const APlayerController* const Controller = World->GetFirstPlayerController();
    APawn* const Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
    if (Pawn == nullptr)
    {
        return nullptr;
    }
    if (ANsVehicle* const Vehicle = Cast<ANsVehicle>(Pawn))
    {
        return Vehicle;
    }

    // The player can stand beside the vehicle, or sit in a vehicle that does not possess the player
    ANsVehicle* Nearest = nullptr;
    double NearestDistance = TNumericLimits<double>::Max();
    for (TActorIterator<ANsVehicle> It(World); It; ++It)
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

void FNsVehicleEditorModule::RunDriveToCommand(const TArray<FString>& Args)
{
    UWorld* const World = GEditor != nullptr ? GEditor->PlayWorld.Get() : nullptr;
    if (World == nullptr || Args.Num() < 3)
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("NsVehicle.DriveTo runs in Play In Editor. Usage: NsVehicle.DriveTo X Y Z [ProfilePath]"));
        return;
    }

    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    const UNsVehicleReachProfile* const Profile = Args.Num() >= 4 ? LoadObject<UNsVehicleReachProfile>(nullptr, *Args[3]) : Settings->DriveProfile.LoadSynchronous();
    ANsVehicle* const Vehicle = FindPlayerVehicle(World);
    if (Profile == nullptr || Vehicle == nullptr)
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("NsVehicle.DriveTo needs a profile (an argument or Drive Profile in the reach settings) and a player vehicle"));
        return;
    }

    const FVector Goal(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
    // The traces start inside the vehicle, so they ignore it and the actors on it, for example the driver
    TArray<AActor*> Ignored;
    Vehicle->GetAttachedActors(Ignored, false, true);
    Ignored.Add(Vehicle);
    const TArray<FVector> Path = UNsVehicleReachLibrary::FindPath(World, Vehicle->GetActorLocation(), Goal, Profile, Ignored);
    if (Path.Num() < 2)
    {
        return;
    }

    RunDriveStopCommand(TArray<FString>());

    // The spline takes a point at each spline spacing, so the steering follows the way and not each grid cell
    AActor* const SplineActor = World->SpawnActor<AActor>();
    USplineComponent* const Spline = NewObject<USplineComponent>(SplineActor);
    SplineActor->SetRootComponent(Spline);
    Spline->RegisterComponent();
    Spline->ClearSplinePoints(false);
    FVector Last = Path[0];
    Spline->AddSplinePoint(Last, ESplineCoordinateSpace::World, false);
    for (int32 Index = 1; Index < Path.Num(); ++Index)
    {
        if (Index == Path.Num() - 1 || FVector::Dist2D(Path[Index], Last) >= Settings->DriveSplineSpacing)
        {
            Last = Path[Index];
            Spline->AddSplinePoint(Last, ESplineCoordinateSpace::World, false);
        }
    }
    Spline->UpdateSpline();
    DriveSplineActor = SplineActor;

    UNsVehicleAI* AI = Vehicle->FindComponentByClass<UNsVehicleAI>();
    if (AI == nullptr)
    {
        AI = NewObject<UNsVehicleAI>(Vehicle);
        AI->RegisterComponent();
    }
    AI->SetDrawDebug(true);
    AI->StartDrive(Spline);

    // The way shows its climb against the profile: green is easy, yellow is near the slope limit of the profile, red is over it
    float Steepest = 0.f;
    for (int32 Index = 1; Index < Path.Num(); ++Index)
    {
        const float Climb = FMath::RadiansToDegrees(FMath::Atan2(Path[Index].Z - Path[Index - 1].Z, FMath::Max(FVector::Dist2D(Path[Index - 1], Path[Index]), 1.f)));
        const float Part = Climb / FMath::Max(Profile->MaxSlopeDegrees, 1.f);
        const FColor Color = Part > 1.f ? FColor::Red : Part > 0.7f ? FColor::Yellow : Settings->ReachedColor;
        Steepest = FMath::Max(Steepest, Climb);
        DrawDebugLine(World, Path[Index - 1] + FVector(0.f, 0.f, 5.f), Path[Index] + FVector(0.f, 0.f, 5.f), Color, false, Settings->DrawSeconds, 0, 3.f);
    }
    const FString Summary = FString::Printf(TEXT("NsVehicle.DriveTo: %s drives %.0f m to %s with %s. Steepest climb %.0f of %.0f deg"),
        *Vehicle->GetActorNameOrLabel(), Spline->GetSplineLength() / 100.f, *Goal.ToString(), *Profile->GetName(), Steepest, Profile->MaxSlopeDegrees);
    UE_LOG(LogNsVehicleEditor, Display, TEXT("%s"), *Summary);
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, Settings->DrawSeconds, FColor::Cyan, Summary);
    }
}

void FNsVehicleEditorModule::RunDriveStopCommand(const TArray<FString>& Args)
{
    if (AActor* const SplineActor = DriveSplineActor.Get())
    {
        UWorld* const World = SplineActor->GetWorld();
        for (TActorIterator<ANsVehicle> It(World); It; ++It)
        {
            UNsVehicleAI* const AI = It->FindComponentByClass<UNsVehicleAI>();
            if (AI != nullptr && AI->IsDriving())
            {
                AI->StopDrive();
            }
        }
        SplineActor->Destroy();
    }
    DriveSplineActor.Reset();
}

void FNsVehicleEditorModule::RunReachCommand(const TArray<FString>& Args)
{
    UWorld* const World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (World == nullptr || Args.Num() < 4)
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("Usage: NsVehicle.Reach X Y Z ProfilePath [CompareProfilePath]"));
        return;
    }

    const FVector Start(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), FCString::Atof(*Args[2]));
    const UNsVehicleReachProfile* const Profile = LoadObject<UNsVehicleReachProfile>(nullptr, *Args[3]);
    if (Profile == nullptr)
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("NsVehicle.Reach cannot load the profile %s"), *Args[3]);
        return;
    }

    const UNsVehicleReachSettings* const Settings = GetDefault<UNsVehicleReachSettings>();
    const TArray<FVector> Base = UNsVehicleReachLibrary::ComputeReach(World, Start, Profile, FBox(ForceInit));
    UNsVehicleReachLibrary::DrawReach(World, Base, Settings->ReachedColor);
    UE_LOG(LogNsVehicleEditor, Display, TEXT("NsVehicle.Reach: %d points with %s"), Base.Num(), *Profile->GetName());

    if (Args.Num() >= 5)
    {
        if (const UNsVehicleReachProfile* const Compare = LoadObject<UNsVehicleReachProfile>(nullptr, *Args[4]))
        {
            const TArray<FVector> Only = UNsVehicleReachLibrary::GetOnlyInCompare(Base, UNsVehicleReachLibrary::ComputeReach(World, Start, Compare, FBox(ForceInit)));
            UNsVehicleReachLibrary::DrawReach(World, Only, Settings->CompareOnlyColor);
            UE_LOG(LogNsVehicleEditor, Display, TEXT("NsVehicle.Reach: %s opens %d more points"), *Compare->GetName(), Only.Num());
        }
    }
}

IMPLEMENT_MODULE(FNsVehicleEditorModule, NsVehicleEditor)
