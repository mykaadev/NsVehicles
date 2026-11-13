// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleReachCommandlet.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "NsVehicleEditorModule.h"
#include "NsVehicleReachLibrary.h"
#include "AI/NsVehicleReachProfile.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionEditorLoaderAdapter.h"

UNsVehicleReachCommandlet::UNsVehicleReachCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UNsVehicleReachCommandlet::Main(const FString& Params)
{
    FString MapPath;
    FString StartText;
    FString ProfilePath;
    FString ComparePath;
    FString OutPath;
    float Extent = 20000.f;
    FParse::Value(*Params, TEXT("Map="), MapPath);
    FParse::Value(*Params, TEXT("Start="), StartText, false);
    FParse::Value(*Params, TEXT("Profile="), ProfilePath);
    FParse::Value(*Params, TEXT("Compare="), ComparePath);
    FParse::Value(*Params, TEXT("Out="), OutPath);
    FParse::Value(*Params, TEXT("Extent="), Extent);

    TArray<FString> StartParts;
    StartText.ParseIntoArray(StartParts, TEXT(","));
    const UNsVehicleReachProfile* const Profile = LoadObject<UNsVehicleReachProfile>(nullptr, *ProfilePath);
    if (MapPath.IsEmpty() || StartParts.Num() != 3 || Profile == nullptr || OutPath.IsEmpty())
    {
        UE_LOG(LogNsVehicleEditor, Error, TEXT("Usage: -run=NsVehicleReach -Map= -Start=X,Y,Z -Profile= [-Compare=] -Out= [-Extent=]"));
        return 1;
    }
    const FVector Start(FCString::Atof(*StartParts[0]), FCString::Atof(*StartParts[1]), FCString::Atof(*StartParts[2]));

    UWorld* const World = UEditorLoadingAndSavingUtils::LoadMap(MapPath);
    if (World == nullptr)
    {
        UE_LOG(LogNsVehicleEditor, Error, TEXT("NsVehicleReach cannot load the map %s"), *MapPath);
        return 1;
    }

    // A World Partition map loads only the cells that an editor loader asks for. The run needs the ground and the walls around the start
    const FBox Region(Start - FVector(Extent, Extent, 100000.f), Start + FVector(Extent, Extent, 100000.f));
    if (UWorldPartition* const WorldPartition = World->GetWorldPartition())
    {
        UWorldPartitionEditorLoaderAdapter* const Adapter = WorldPartition->CreateEditorLoaderAdapter<FLoaderAdapterShape>(World, Region, TEXT("NsVehicleReach"));
        Adapter->GetLoaderAdapter()->Load();
    }

    // A commandlet does not tick, and the physics scene adds new bodies to its query structure only in a tick. A few editor frames make the ground and the walls traceable
    for (int32 Frame = 0; Frame < 3; ++Frame)
    {
        World->Tick(LEVELTICK_ViewportsOnly, 1.f / 30.f);
    }

    int32 ActorCount = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        ++ActorCount;
    }
    UE_LOG(LogNsVehicleEditor, Display, TEXT("NsVehicleReach: %d actors loaded, World Partition %s"), ActorCount, World->GetWorldPartition() != nullptr ? (World->GetWorldPartition()->IsInitialized() ? TEXT("ready") : TEXT("not ready")) : TEXT("none"));

    const TArray<FVector> Base = UNsVehicleReachLibrary::ComputeReach(World, Start, Profile, Region);
    UE_LOG(LogNsVehicleEditor, Display, TEXT("NsVehicleReach: %d points with %s"), Base.Num(), *Profile->GetName());
    bool bWritten = UNsVehicleReachLibrary::WriteReachFile(Base, OutPath);

    if (const UNsVehicleReachProfile* const Compare = ComparePath.IsEmpty() ? nullptr : LoadObject<UNsVehicleReachProfile>(nullptr, *ComparePath))
    {
        const TArray<FVector> Only = UNsVehicleReachLibrary::GetOnlyInCompare(Base, UNsVehicleReachLibrary::ComputeReach(World, Start, Compare, Region));
        UE_LOG(LogNsVehicleEditor, Display, TEXT("NsVehicleReach: %s opens %d more points"), *Compare->GetName(), Only.Num());
        bWritten &= UNsVehicleReachLibrary::WriteReachFile(Only, FPaths::GetPath(OutPath) / FPaths::GetBaseFilename(OutPath) + TEXT("_CompareOnly.csv"));
    }

    if (!bWritten)
    {
        UE_LOG(LogNsVehicleEditor, Error, TEXT("NsVehicleReach cannot write %s"), *OutPath);
        return 1;
    }
    return 0;
}
