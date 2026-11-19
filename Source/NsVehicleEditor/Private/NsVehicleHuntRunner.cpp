// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleHuntRunner.h"

#include "AI/NsVehicleAI.h"
#include "AI/NsVehicleReachProfile.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NsVehicleEditorModule.h"
#include "NsVehicleHuntList.h"
#include "NsVehicleReachLibrary.h"
#include "Vehicle/NsVehicle.h"

bool UNsVehicleHuntRunner::Start(UWorld* const InWorld, UNsVehicleHuntList* const InList, ANsVehicle* const InVehicle)
{
    if (InWorld == nullptr || InList == nullptr || InList->Profile == nullptr || InVehicle == nullptr)
    {
        UE_LOG(LogNsVehicleEditor, Warning, TEXT("NsVehicle.Hunt needs Play In Editor, a hunt list with a profile, and a player vehicle"));
        return false;
    }

    World = InWorld;
    List = InList;
    Vehicle = InVehicle;
    UNsVehicleAI* Driver = InVehicle->FindComponentByClass<UNsVehicleAI>();
    if (Driver == nullptr)
    {
        Driver = NewObject<UNsVehicleAI>(InVehicle);
        Driver->RegisterComponent();
    }
    Driver->SetDrawDebug(true);
    Driver->SetPathQuery(UNsVehicleReachLibrary::MakeQuery(true));
    AI = Driver;
    Results.Reset();
    ZoneEntries.Reset();
    ZonesInside.Reset();
    TargetIndex = INDEX_NONE;
    TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick));
    StartNextTarget();
    return true;
}

void UNsVehicleHuntRunner::Stop()
{
    if (!IsRunning())
    {
        return;
    }
    if (UNsVehicleAI* const Driver = AI.Get())
    {
        Driver->StopDrive();
    }
    if (List != nullptr && List->Targets.IsValidIndex(TargetIndex) && Results.Num() == TargetIndex)
    {
        FinishTarget(TEXT("stopped"), TEXT("the hunt was stopped"));
    }
    FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
    TickHandle.Reset();
    WriteReport();
}

void UNsVehicleHuntRunner::StartNextTarget()
{
    // A drive that a stop scheduled before it ended does not start
    if (!IsRunning())
    {
        return;
    }
    ++TargetIndex;
    DriveSeconds = 0.f;
    UNsVehicleAI* const Driver = AI.Get();
    if (List == nullptr || Driver == nullptr || !List->Targets.IsValidIndex(TargetIndex))
    {
        Stop();
        return;
    }

    const FNsVehicleHuntTarget& Target = List->Targets[TargetIndex];
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Cyan, FString::Printf(TEXT("NsVehicle.Hunt: drive %d of %d to %s"), TargetIndex + 1, List->Targets.Num(), *Target.Name));
    }
    if (!Driver->StartDriveTo(Target.Location, List->Profile))
    {
        FinishTarget(TEXT("no way"), Driver->GetLastStuckCause());
    }
}

bool UNsVehicleHuntRunner::Tick(const float DeltaTime)
{
    UNsVehicleAI* const Driver = AI.Get();
    const ANsVehicle* const Car = Vehicle.Get();
    if (!World.IsValid() || Driver == nullptr || Car == nullptr || List == nullptr)
    {
        // Play In Editor ended, so the hunt ends with the drives so far
        TickHandle.Reset();
        WriteReport();
        return false;
    }

    // An entry into a no-go zone counts one time, until the vehicle leaves the zone
    const FVector Location = Car->GetActorLocation();
    for (int32 Index = 0; Index < List->NoGoZones.Num(); ++Index)
    {
        const FNsVehicleHuntZone& Zone = List->NoGoZones[Index];
        const bool bInside = FBox(Zone.Center - Zone.Extent, Zone.Center + Zone.Extent).IsInside(Location);
        if (bInside && !ZonesInside.Contains(Index))
        {
            ZonesInside.Add(Index);
            const FString TargetName = List->Targets.IsValidIndex(TargetIndex) ? List->Targets[TargetIndex].Name : FString();
            ZoneEntries.Add(FString::Printf(TEXT("| %s | %s | %.0f, %.0f, %.0f |"), *Zone.Name, *TargetName, Location.X, Location.Y, Location.Z));
            if (GEngine != nullptr)
            {
                GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, FString::Printf(TEXT("NsVehicle.Hunt: entered the no-go zone %s"), *Zone.Name));
            }
        }
        else if (!bInside)
        {
            ZonesInside.Remove(Index);
        }
    }

    if (!List->Targets.IsValidIndex(TargetIndex) || Results.Num() > TargetIndex)
    {
        return true;
    }

    DriveSeconds += DeltaTime;
    if (!Driver->IsDriving())
    {
        const bool bArrived = Driver->GetLastStuckCause().IsEmpty() && FVector::Dist2D(Location, List->Targets[TargetIndex].Location) < 300.f;
        FinishTarget(bArrived ? TEXT("arrived") : TEXT("stuck"), Driver->GetLastStuckCause());
    }
    else if (DriveSeconds > List->TargetTimeLimit)
    {
        Driver->StopDrive();
        FinishTarget(TEXT("timeout"), TEXT("over the time limit"));
    }
    return true;
}

void UNsVehicleHuntRunner::FinishTarget(const FString& Result, const FString& Cause)
{
    const UNsVehicleAI* const Driver = AI.Get();
    const ANsVehicle* const Car = Vehicle.Get();
    FNsVehicleHuntResult& Entry = Results.AddDefaulted_GetRef();
    Entry.Target = List->Targets[TargetIndex].Name;
    Entry.Result = Result;
    Entry.Seconds = DriveSeconds;
    Entry.Replans = Driver != nullptr ? Driver->GetReplanCount() : 0;
    Entry.End = Car != nullptr ? Car->GetActorLocation() : FVector::ZeroVector;
    Entry.Cause = Cause;
    UE_LOG(LogNsVehicleEditor, Display, TEXT("NsVehicle.Hunt: %s %s after %.0f s, %d new plans, at %s %s"), *Entry.Target, *Result, DriveSeconds, Entry.Replans, *Entry.End.ToString(), *Cause);

    // The next drive starts on the next tick, so a drive that ends in its first tick does not nest
    if (TickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float) { StartNextTarget(); return false; }));
    }
}

void UNsVehicleHuntRunner::WriteReport()
{
    TArray<FString> Lines;
    Lines.Add(FString::Printf(TEXT("# Hunt %s"), List != nullptr ? *List->GetName() : TEXT("")));
    Lines.Add(FString());
    Lines.Add(FString::Printf(TEXT("Profile: %s. Date: %s"), List != nullptr && List->Profile != nullptr ? *List->Profile->GetName() : TEXT("none"), *FDateTime::Now().ToString()));
    Lines.Add(FString());
    Lines.Add(TEXT("| Target | Result | Time | New plans | End | Cause |"));
    Lines.Add(TEXT("|---|---|---|---|---|---|"));
    int32 Arrived = 0;
    for (const FNsVehicleHuntResult& Entry : Results)
    {
        Arrived += Entry.Result == TEXT("arrived") ? 1 : 0;
        Lines.Add(FString::Printf(TEXT("| %s | %s | %.0f s | %d | %.0f, %.0f, %.0f | %s |"), *Entry.Target, *Entry.Result, Entry.Seconds, Entry.Replans, Entry.End.X, Entry.End.Y, Entry.End.Z, *Entry.Cause));
    }
    Lines.Add(FString());
    Lines.Add(FString::Printf(TEXT("No-go zone entries: %d"), ZoneEntries.Num()));
    if (ZoneEntries.Num() > 0)
    {
        Lines.Add(FString());
        Lines.Add(TEXT("| Zone | During the drive to | Where |"));
        Lines.Add(TEXT("|---|---|---|"));
        Lines.Append(ZoneEntries);
    }

    const FString Folder = FPaths::ProjectSavedDir() / TEXT("NsVehicle");
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Path = Folder / FString::Printf(TEXT("Hunt_%s.md"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
    FFileHelper::SaveStringArrayToFile(Lines, *Path);
    const FString Summary = FString::Printf(TEXT("NsVehicle.Hunt: %d of %d targets reached, %d no-go zone entries. Report: %s"), Arrived, Results.Num(), ZoneEntries.Num(), *Path);
    UE_LOG(LogNsVehicleEditor, Display, TEXT("%s"), *Summary);
    if (GEngine != nullptr)
    {
        GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Cyan, Summary);
    }
}
