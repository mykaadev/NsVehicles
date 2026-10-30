// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "NsVehicleArticulatedComponent.generated.h"

/** The axis that an articulated component moves on */
UENUM(BlueprintType)
enum class ENsVehicleArticulationAxis : uint8
{
    /** Moves on the local X axis */
    X UMETA(DisplayName = "X Location"),
    /** Moves on the local Y axis */
    Y UMETA(DisplayName = "Y Location"),
    /** Moves on the local Z axis */
    Z UMETA(DisplayName = "Z Location"),
    /** Turns on the roll axis */
    Roll UMETA(DisplayName = "Roll Rotation"),
    /** Turns on the pitch axis */
    Pitch UMETA(DisplayName = "Pitch Rotation"),
    /** Turns on the yaw axis */
    Yaw UMETA(DisplayName = "Yaw Rotation")
};

/** Moves a mesh on one axis from an input value, for example a forklift arm */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent, DisplayName = "NsVehicle Articulated Component"))
class NSVEHICLE_API UNsVehicleArticulatedComponent : public UStaticMeshComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleArticulatedComponent();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    /** Moves the mesh from the input value. ANsVehicle calls it every frame */
    void Update(float deltaTime);

    /** Sets the movement input. Positive and negative values move in opposite directions */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle")
    void SetInput(float value);

    /** Returns the movement from the start position */
    UFUNCTION(BlueprintPure, Category = "NsVehicle")
    float GetMovement();

// Variables
protected:
    /** The movement axis. Only one axis moves at a time */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articulation", meta = (AllowPrivateAccess = "true"))
    ENsVehicleArticulationAxis Axis = ENsVehicleArticulationAxis::Z;

    /** Clamps the movement between Min and Max. Turn it off for a part that turns freely */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articulation", meta = (AllowPrivateAccess = "true"))
    bool bClamped = true;

    /** The minimum movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articulation", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bClamped==true", EditConditionHides))
    float Min = 0.f;

    /** The maximum movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articulation", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bClamped==true", EditConditionHides))
    float Max= 100.f;

    /** The movement for each second at full input */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articulation", meta = (AllowPrivateAccess = "true"))
    float MovementSpeed = 1.f;

    /** Smooths the movement */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articulation", meta = (AllowPrivateAccess = "true"))
    bool bUseSmoothing = true;

    /** How fast the smoothed movement follows the target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Articulation", meta = (AllowPrivateAccess = "true"))
    float SmoothingSpeed = 10.f;

    /** The movement input */
    float InputValue = 0.f;

    /** The target movement from the start position */
    float Movement = 0.f;

    /** The smoothed movement from the start position */
    float SmoothMovement = 0.f;

    /** The relative location at Begin Play */
    FVector BaseLocation = FVector::ZeroVector;

    /** The relative rotation at Begin Play */
    FRotator BaseRotation = FRotator::ZeroRotator;
};
