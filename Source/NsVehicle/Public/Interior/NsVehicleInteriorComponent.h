// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "NsVehicleInteriorComponent.generated.h"

/** The rotation axis of an interior component */
UENUM(BlueprintType)
enum class ENsVehicleRotatorAxis : uint8
{
    /** Turns on the roll axis */
    Roll UMETA(DisplayName = "Roll"),
    /** Turns on the pitch axis */
    Pitch UMETA(DisplayName = "Pitch"),
    /** Turns on the yaw axis */
    Yaw UMETA(DisplayName = "Yaw")
};

class ANsVehicle;

/** Base class for an interior mesh that turns from one vehicle value. Add a subclass, for example a tachometer needle or a steering wheel */
UCLASS(Abstract, ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleInteriorComponent : public UStaticMeshComponent
{
    GENERATED_BODY()

// Functions
public:
    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    //~ End UActorComponent Interface

    /** Turns the mesh from the vehicle value. ANsVehicle calls it every frame */
    void Update(float DeltaTime, const ANsVehicle& Vehicle);

protected:
    /** Returns the vehicle value that drives the rotation */
    virtual float GetDriveValue(const ANsVehicle& Vehicle) const;

// Variables
protected:
    /** The value range that drives the rotation. X maps to the rest angle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Interior")
    FVector2D InputRange = FVector2D(0.f, 1.f);

    /** The angle range in degrees that the input range maps to. X is the rest angle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Interior")
    FVector2D OutputRange = FVector2D(0.f, 90.f);

    /** The rotation axis */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Interior")
    ENsVehicleRotatorAxis RotationAxis = ENsVehicleRotatorAxis::Roll;

    /** Smooths the rotation. It costs accuracy and response */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Interior")
    bool bUseSmoothing = false;

    /** How fast the smoothed rotation follows the target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Interior", meta = (EditCondition = "bUseSmoothing", EditConditionHides))
    float SmoothingSpeed = 5.f;

    /** Inverts the drive value before the mapping */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Interior")
    bool bInvert = false;

private:
    /** The relative rotation at Begin Play */
    FRotator BaseRotation = FRotator::ZeroRotator;

    /** The rotation now in degrees */
    float Rotation = 0.f;
};
