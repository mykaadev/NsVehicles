// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "Vehicle/NsVehicleTypes.h"
#include "NsVehicleNetworkComponent.generated.h"

/** Replicates the vehicle state from the server to the clients */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleNetworkComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleNetworkComponent();

    /** Copies the physics velocities to the replicated values on the server, or applies them on a client */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Network")
    void AsyncReplicatedTick();

    /** Returns what the vehicle replicates */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Network")
    ENsVehicleReplicationType GetReplicationMethod() const;

    /** Writes the vehicle state to the replicated server state */
    void ServerStateSync(bool bUpdatePhysics);

    /** Reads the replicated server state into the vehicle on a client */
    void ClientStateSync(bool bUpdatePhysics);

    /** Copies the server transform to a client at the fast sync interval. Call it on the game thread */
    void UpdateFastSync();

// Variables
protected:
    /** What the vehicle replicates. Full sends movement and data. Data Only sends data for another movement system. None sends nothing */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NsVehicle|Network")
    ENsVehicleReplicationType ReplicationMethod = ENsVehicleReplicationType::ERT_None;

    /** Makes the clients copy the server transform at an interval. Most projects do not need it */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NsVehicle|Network", meta = (EditCondition = "ReplicationMethod==ENsVehicleReplicationType::ERT_Full", EditConditionHides))
    bool bUseServerFastSync = true;

    /** The number of game frames between two transform syncs */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NsVehicle|Network", meta = (EditCondition = "bUseServerFastSync && ReplicationMethod==ENsVehicleReplicationType::ERT_Full", EditConditionHides))
    int32 ServerFastSyncLatency = 500;

private:
    /** The replicated vehicle state from the server */
    UPROPERTY(Replicated)
    FNsVehicleServerState ServerState;

    /** The replicated linear velocity from the server */
    UPROPERTY(Replicated)
    FVector ServerVelocity = FVector::ZeroVector;

    /** The replicated angular velocity from the server */
    UPROPERTY(Replicated)
    FVector ServerAngularVelocity = FVector::ZeroVector;

    /** The game frames since the last transform sync */
    int32 FastServerSyncCount = 0;
};
