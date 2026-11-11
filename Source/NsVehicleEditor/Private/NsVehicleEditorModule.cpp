// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleEditorModule.h"

#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "NsVehicleReachLibrary.h"
#include "NsVehicleReachProfile.h"
#include "NsVehicleReachSettings.h"

DEFINE_LOG_CATEGORY(LogNsVehicleEditor);

void FNsVehicleEditorModule::StartupModule()
{
    ReachCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("NsVehicle.Reach"),
        TEXT("Paints the ground that a vehicle reaches. Usage: NsVehicle.Reach X Y Z ProfilePath [CompareProfilePath]"),
        FConsoleCommandWithArgsDelegate::CreateStatic(&FNsVehicleEditorModule::RunReachCommand),
        ECVF_Default);
}

void FNsVehicleEditorModule::ShutdownModule()
{
    if (ReachCommand != nullptr)
    {
        IConsoleManager::Get().UnregisterConsoleObject(ReachCommand);
        ReachCommand = nullptr;
    }
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
