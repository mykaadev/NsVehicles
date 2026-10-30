// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "NsVehicleTypes.generated.h"

/** How the vehicle limits a value as a function of speed */
UENUM(BlueprintType)
enum class ENsVehicleSpeedClamp : uint8
{
    /** The value has no speed limit */
    NoClamp UMETA(DisplayName = "No Clamp"),
    /** The value has the same limit at all speeds */
    Constant UMETA(DisplayName = "Constant"),
    /** The limit changes linearly with speed */
    Linear UMETA(DisplayName = "Linear"),
    /** The limit comes from a curve of speed */
    Curve UMETA(DisplayName = "Curve")
};

/** The powertrain model of the vehicle */
UENUM(BlueprintType)
enum class ENsVehicleBehaviorType : uint8
{
    /** Full powertrain with gears and torque as a function of RPM */
    Standard UMETA(DisplayName = "Standard"),
    /** Constant torque, no gearbox, and a top speed clamp */
    Arcade UMETA(DisplayName = "Arcade")
};

/** How the standard powertrain calculates the engine torque */
UENUM(BlueprintType)
enum class ENsVehicleTorqueImplementation : uint8
{
    /** The engine torque is the same at all RPM values */
    ETI_Constant UMETA(DisplayName = "Constant"),
    /** The engine torque comes from a normalized curve of RPM */
    ETI_Curve UMETA(DisplayName = "Curve")
};

/** What the vehicle sends over the network */
UENUM(BlueprintType)
enum class ENsVehicleReplicationType : uint8
{
    /** The vehicle replicates movement and data */
    ERT_Full UMETA(DisplayName = "Full"),
    /** The vehicle replicates data only. Another system moves the clients */
    ERT_DataOnly UMETA(DisplayName = "Data Only"),
    /** The vehicle replicates nothing */
    ERT_None UMETA(DisplayName = "None")
};

/** Maps an input range to an output range with a clamp */
USTRUCT(BlueprintType)
struct FNsVehicleRangedClamp
{
    GENERATED_USTRUCT_BODY()

// Functions
public:
    /** Constructor */
    FNsVehicleRangedClamp()
    {
        InputRange = FVector2D::ZeroVector;
        OutputRange = FVector2D::ZeroVector;
    }

// Variables
public:
    /** The source range, for example a speed range in KPH */
    UPROPERTY(EditAnywhere, Category = "Custom Data")
    FVector2D InputRange;

    /** The result range that the input range maps to */
    UPROPERTY(EditAnywhere, Category = "Custom Data")
    FVector2D OutputRange;
};

/** The vehicle state that the server sends to the clients */
USTRUCT()
struct FNsVehicleServerState
{
    GENERATED_USTRUCT_BODY()

// Functions
public:
    /** Constructor */
    FNsVehicleServerState()
    {
        ServerLinearVelocity = FVector::ZeroVector;
        ServerAngularVelocity = FVector::ZeroVector;
        ServerTransform = FTransform::Identity;
        ServerRPM = 0.f;
        ServerCurrentGear = 0.f;
        ServerCurrentGearRatio = 0.f;
        ServerThrottle = 0.f;
        ServerSteering = 0.f;
        ServerBrakes = 0.f;
        ServerSpeed = 0.f;
        ServerIsUpshifting = false;
        ServerIsDownshifting = false;
    }

// Variables
public:
    /** The world linear velocity of the physics body on the server */
    UPROPERTY()
    FVector ServerLinearVelocity;

    /** The world angular velocity of the physics body on the server */
    UPROPERTY()
    FVector ServerAngularVelocity;

    /** The physics transform of the vehicle on the server */
    UPROPERTY()
    FTransform ServerTransform;

    /** The engine RPM on the server */
    UPROPERTY()
    float ServerRPM;

    /** The gear index on the server */
    UPROPERTY()
    float ServerCurrentGear;

    /** The gear ratio on the server */
    UPROPERTY()
    float ServerCurrentGearRatio;

    /** The throttle input on the server */
    UPROPERTY()
    float ServerThrottle;

    /** The steering input on the server */
    UPROPERTY()
    float ServerSteering;

    /** The brake input on the server */
    UPROPERTY()
    float ServerBrakes;

    /** The forward speed in m/s on the server */
    UPROPERTY()
    float ServerSpeed;

    /** True while the gearbox upshifts on the server */
    UPROPERTY()
    bool ServerIsUpshifting;

    /** True while the gearbox downshifts on the server */
    UPROPERTY()
    bool ServerIsDownshifting;
};
