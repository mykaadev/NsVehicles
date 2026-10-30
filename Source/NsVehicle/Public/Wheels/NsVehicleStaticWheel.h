// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "NsVehicleStaticWheel.generated.h"

/** Spins a wheel mesh from the velocity of the actor root. Use it for a trailer or other wheels without suspension */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleStaticWheel : public UStaticMeshComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleStaticWheel();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** Spins the wheel mesh from the forward velocity of the actor root */
    UFUNCTION(BlueprintCallable, Category = "Static Vehicle Wheel")
    void Update(float deltaTime);

// Variables
private:
    /** Spins the wheel when true */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (AllowPrivateAccess = "true"))
    bool bEnabled = true;

    /** Inverts the spin direction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (AllowPrivateAccess = "true"))
    bool bInvertDirection = false;

    /** Scales the spin speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (AllowPrivateAccess = "true"))
    float RotationMultiplier = 1.f;

    /** The root physics body of the owner actor */
    UPrimitiveComponent* VehiclePrimitiveComponent;

    /** The minimum local bound of the wheel mesh */
    FVector minBound = FVector::ZeroVector;

    /** The maximum local bound of the wheel mesh */
    FVector maxBound = FVector::ZeroVector;
};
