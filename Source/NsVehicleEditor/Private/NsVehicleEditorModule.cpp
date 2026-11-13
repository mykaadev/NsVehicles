// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleEditorModule.h"

#include "AI/NsVehicleAI.h"
#include "DrawDebugHelpers.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"
#include "NsVehicleHuntList.h"
#include "NsVehicleHuntRunner.h"
#include "NsVehicleReachLibrary.h"
#include "AI/NsVehicleReachProfile.h"
#include "NsVehicleReachSettings.h"
#include "Vehicle/NsVehicle.h"

DEFINE_LOG_CATEGORY(LogNsVehicleEditor);

TStrongObjectPtr<UNsVehicleHuntRunner> FNsVehicleEditorModule::HuntRunner;

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
    HuntCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("NsVehicle.Hunt"),
        TEXT("In Play In Editor, drives the player vehicle to each target of a hunt list and writes a report of weird spots. Usage: NsVehicle.Hunt /Game/Path/HuntList"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&FNsVehicleEditorModule::RunHuntCommand),
        ECVF_Default);
    HuntStopCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("NsVehicle.HuntStop"),
        TEXT("Stops the hunt and writes the report of the drives so far"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&FNsVehicleEditorModule::RunHuntStopCommand),
        ECVF_Default);
}

void FNsVehicleEditorModule::ShutdownModule()
{
    for (IConsoleObject** Command : { &ReachCommand, &DriveToCommand, &DriveStopCommand, &HuntCommand, &HuntStopCommand })
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
    UNsVehicleAI* const AI = FindOrAddAI(Vehicle);
    AI->SetDrawDebug(true);
    AI->SetPathQuery(UNsVehicleReachLibrary::MakeQuery(true));
    if (!AI->StartDriveTo(Goal, Profile))
    {
        const FString Message = FString::Printf(TEXT("NsVehicle.DriveTo: no way from %s to %s with %s"), *Vehicle->GetActorLocation().ToString(), *Goal.ToString(), *Profile->GetName());
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("%s"), *Message);
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(-1, Settings->DrawSeconds, FColor::Red, Message);
        }
        return;
    }

    // The way shows its climb against the profile: green is easy, yellow is near the slope limit of the profile, red is over it
    const TArray<FVector>& Path = AI->GetWaypoints();
    float Steepest = 0.f;
    float Length = 0.f;
    for (int32 Index = 1; Index < Path.Num(); ++Index)
    {
        const float Climb = FMath::RadiansToDegrees(FMath::Atan2(Path[Index].Z - Path[Index - 1].Z, FMath::Max(FVector::Dist2D(Path[Index - 1], Path[Index]), 1.f)));
        const float Part = Climb / FMath::Max(Profile->MaxSlopeDegrees, 1.f);
        const FColor Color = Part > 1.f ? FColor::Red : Part > 0.7f ? FColor::Yellow : Settings->ReachedColor;
        Steepest = FMath::Max(Steepest, Climb);
        Length += FVector::Dist(Path[Index - 1], Path[Index]);
        DrawDebugLine(World, Path[Index - 1] + FVector(0.f, 0.f, 6.f), Path[Index] + FVector(0.f, 0.f, 6.f), Color, false, Settings->DrawSeconds, 0, 2.f);
    }
    const FString Summary = FString::Printf(TEXT("NsVehicle.DriveTo: %s drives %.0f m to %s with %s. Steepest climb %.0f of %.0f deg"),
        *Vehicle->GetActorNameOrLabel(), Length / 100.f, *Goal.ToString(), *Profile->GetName(), Steepest, Profile->MaxSlopeDegrees);
    UE_LOG(LogNsVehicleEditor, Display, TEXT("%s"), *Summary);
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, Settings->DrawSeconds, FColor::Cyan, Summary);
    }
}

void FNsVehicleEditorModule::RunDriveStopCommand(const TArray<FString>& Args)
{
    UWorld* const World = GEditor != nullptr ? GEditor->PlayWorld.Get() : nullptr;
    if (World == nullptr)
    {
        return;
    }
    for (TActorIterator<ANsVehicle> It(World); It; ++It)
    {
        UNsVehicleAI* const AI = It->FindComponentByClass<UNsVehicleAI>();
        if (AI != nullptr && AI->IsDriving())
        {
            AI->StopDrive();
        }
    }
}

void FNsVehicleEditorModule::RunHuntCommand(const TArray<FString>& Args)
{
    UWorld* const World = GEditor != nullptr ? GEditor->PlayWorld.Get() : nullptr;
    UNsVehicleHuntList* const List = Args.Num() >= 1 ? LoadObject<UNsVehicleHuntList>(nullptr, *Args[0]) : nullptr;
    if (World == nullptr || List == nullptr)
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("NsVehicle.Hunt runs in Play In Editor. Usage: NsVehicle.Hunt /Game/Path/HuntList"));
        return;
    }
    if (HuntRunner.IsValid() && HuntRunner->IsRunning())
    {
        HuntRunner->Stop();
    }
    HuntRunner.Reset(NewObject<UNsVehicleHuntRunner>());
    HuntRunner->Start(World, List, FindPlayerVehicle(World));
}

void FNsVehicleEditorModule::RunHuntStopCommand(const TArray<FString>& Args)
{
    if (HuntRunner.IsValid())
    {
        HuntRunner->Stop();
    }
}

UNsVehicleAI* FNsVehicleEditorModule::FindOrAddAI(ANsVehicle* const Vehicle)
{
    UNsVehicleAI* AI = Vehicle->FindComponentByClass<UNsVehicleAI>();
    if (AI == nullptr)
    {
        AI = NewObject<UNsVehicleAI>(Vehicle);
        AI->RegisterComponent();
    }
    return AI;
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
