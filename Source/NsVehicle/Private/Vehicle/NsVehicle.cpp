// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Vehicle/NsVehicle.h"
#include "Articulation/NsVehicleArticulatedComponent.h"
#include "AsyncTickFunctions.h"
#include "Components/PrimitiveComponent.h"
#include "Interior/NsVehicleInteriorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Modules/Animation/NsVehicleAnimationComponent.h"
#include "Modules/Chassis/NsVehicleAerodynamicsComponent.h"
#include "Modules/Chassis/NsVehicleAirControlComponent.h"
#include "Modules/Chassis/NsVehicleAntiGravityComponent.h"
#include "Modules/Chassis/NsVehicleBodyDynamicsComponent.h"
#include "Modules/Chassis/NsVehicleBrakeComponent.h"
#include "Modules/Chassis/NsVehicleIdleLockComponent.h"
#include "Modules/Chassis/NsVehicleSteeringComponent.h"
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Modules/Network/NsVehicleNetworkComponent.h"
#include "Modules/Powertrain/NsVehicleDrivetrainComponent.h"
#include "Modules/Powertrain/NsVehicleEngineComponent.h"
#include "Modules/Powertrain/NsVehicleTransmissionComponent.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

ANsVehicle::ANsVehicle()
{
    PrimaryActorTick.bCanEverTick = true;

    SetNetUpdateFrequency(100);
    SetMinNetUpdateFrequency(100);

    Controls = CreateDefaultSubobject<UNsVehicleControlsComponent>(TEXT("Controls"));
    Engine = CreateDefaultSubobject<UNsVehicleEngineComponent>(TEXT("Engine"));
    Transmission = CreateDefaultSubobject<UNsVehicleTransmissionComponent>(TEXT("Transmission"));
    Drivetrain = CreateDefaultSubobject<UNsVehicleDrivetrainComponent>(TEXT("Drivetrain"));
    Brakes = CreateDefaultSubobject<UNsVehicleBrakeComponent>(TEXT("Brakes"));
    Steering = CreateDefaultSubobject<UNsVehicleSteeringComponent>(TEXT("Steering"));
    BodyDynamics = CreateDefaultSubobject<UNsVehicleBodyDynamicsComponent>(TEXT("BodyDynamics"));
    Aerodynamics = CreateDefaultSubobject<UNsVehicleAerodynamicsComponent>(TEXT("Aerodynamics"));
    AirControl = CreateDefaultSubobject<UNsVehicleAirControlComponent>(TEXT("AirControl"));
    AntiGravity = CreateDefaultSubobject<UNsVehicleAntiGravityComponent>(TEXT("AntiGravity"));
    IdleLock = CreateDefaultSubobject<UNsVehicleIdleLockComponent>(TEXT("IdleLock"));
    Network = CreateDefaultSubobject<UNsVehicleNetworkComponent>(TEXT("Network"));
    Animation = CreateDefaultSubobject<UNsVehicleAnimationComponent>(TEXT("Animation"));
}

void ANsVehicle::BeginPlay()
{
    Super::BeginPlay();

    PrimitiveComponent = Cast<UPrimitiveComponent>(GetRootComponent());
    if (PrimitiveComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle '%s' has no primitive root component. The vehicle does not simulate"), *GetPathNameSafe(this));
    }

    GetComponents(SuspensionComponents);
    GetComponents(InteriorComponents);
    GetComponents(ArticulatedComponents);

    if (Network->GetReplicationMethod() != ENsVehicleReplicationType::ERT_None)
    {
        for (UNsVehicleSuspensionComponent* Suspension : SuspensionComponents)
        {
            Suspension->SetIsReplicated(true);
        }
    }
}

void ANsVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (PrimitiveComponent == nullptr)
    {
        return;
    }

    if (bAutoWakeRigidBodies)
    {
        PrimitiveComponent->WakeAllRigidBodies();
    }

    UpdateAttachedComponents(DeltaTime);
    Network->UpdateFastSync();
    AntiGravity->UpdateGameThread(PrimitiveComponent);

    LocalVelocityGT = UKismetMathLibrary::InverseTransformDirection(GetActorTransform(), GetVelocity());

    Animation->UpdateLocationVelocity();
    if (Animation->IsAnimationModeEnabled())
    {
        Animation->SpinSuspensionWheels(SuspensionComponents);
    }
}

void ANsVehicle::NativeAsyncTick(float DeltaTime)
{
    // The parent runs the Blueprint Async Tick event. The replication branches below run it one more time, as the original vehicle did
    Super::NativeAsyncTick(DeltaTime);

    if (PrimitiveComponent == nullptr)
    {
        return;
    }

    AsyncDeltaTime = DeltaTime;

    UpdateGroundContact();
    UpdateAngularMotion();
    IdleLock->UpdateIdleLock(DeltaTime);

    const bool bAuthority = HasAuthority();
    switch (Network->GetReplicationMethod())
    {
    case ENsVehicleReplicationType::ERT_Full:
        if (bAuthority)
        {
            UpdatePhysicsCore(DeltaTime);
            AsyncTick(DeltaTime);
            Network->ServerStateSync(true);
        }
        else
        {
            Network->ClientStateSync(true);
        }
        break;

    case ENsVehicleReplicationType::ERT_DataOnly:
        if (bAuthority)
        {
            UpdatePhysicsCore(DeltaTime);
            AsyncTick(DeltaTime);
            Network->ServerStateSync(false);
        }
        else
        {
            Network->ClientStateSync(false);
            UpdatePhysicsCore(DeltaTime);
            AsyncTick(DeltaTime);
        }
        break;

    case ENsVehicleReplicationType::ERT_None:
        UpdatePhysicsCore(DeltaTime);
        AsyncTick(DeltaTime);
        break;
    }

    UpdateWheelPhysics(DeltaTime);
}

UNsVehicleControlsComponent* ANsVehicle::GetControls() const
{
    return Controls;
}

UNsVehicleEngineComponent* ANsVehicle::GetEngine() const
{
    return Engine;
}

UNsVehicleTransmissionComponent* ANsVehicle::GetTransmission() const
{
    return Transmission;
}

UNsVehicleDrivetrainComponent* ANsVehicle::GetDrivetrain() const
{
    return Drivetrain;
}

UNsVehicleBrakeComponent* ANsVehicle::GetBrakes() const
{
    return Brakes;
}

UNsVehicleSteeringComponent* ANsVehicle::GetSteering() const
{
    return Steering;
}

UNsVehicleBodyDynamicsComponent* ANsVehicle::GetBodyDynamics() const
{
    return BodyDynamics;
}

UNsVehicleAerodynamicsComponent* ANsVehicle::GetAerodynamics() const
{
    return Aerodynamics;
}

UNsVehicleAirControlComponent* ANsVehicle::GetAirControl() const
{
    return AirControl;
}

UNsVehicleAntiGravityComponent* ANsVehicle::GetAntiGravity() const
{
    return AntiGravity;
}

UNsVehicleIdleLockComponent* ANsVehicle::GetIdleLock() const
{
    return IdleLock;
}

UNsVehicleNetworkComponent* ANsVehicle::GetNetwork() const
{
    return Network;
}

UNsVehicleAnimationComponent* ANsVehicle::GetAnimation() const
{
    return Animation;
}

void ANsVehicle::Jump(const float Amount)
{
    if (!bInAir)
    {
        UAsyncTickFunctions::ATP_AddImpulse(PrimitiveComponent, PhysicsTransform.GetRotation().GetUpVector() * Amount, true);
    }
}

void ANsVehicle::RTWakeRigidBodies()
{
    PrimitiveComponent->WakeAllRigidBodies();
}

void ANsVehicle::SetInAir(const bool bAirborne)
{
    bInAir = bAirborne;
}

bool ANsVehicle::IsInAir() const
{
    return bInAir;
}

float ANsVehicle::GetSpeedKPH() const
{
    return Speed * 3.6f;
}

float ANsVehicle::GetLinearVelocity() const
{
    return GetVelocityVector().X;
}

FVector ANsVehicle::GetVelocityVector() const
{
    return UKismetMathLibrary::InverseTransformDirection(PhysicsTransform, UAsyncTickFunctions::ATP_GetLinearVelocity(PrimitiveComponent)) * 0.01f;
}

FTransform ANsVehicle::GetPhysicsTransform() const
{
    return PhysicsTransform;
}

float ANsVehicle::GetSideSlipAngle() const
{
    const FVector Velocity = GetVelocityVector();
    return FMath::RadiansToDegrees(FMath::Atan(UKismetMathLibrary::SafeDivide(Velocity.Y, Velocity.X)));
}

float ANsVehicle::GetSideSlipAngleGT() const
{
    return FMath::RadiansToDegrees(FMath::Atan(UKismetMathLibrary::SafeDivide(LocalVelocityGT.Y, LocalVelocityGT.X)));
}

UPrimitiveComponent* ANsVehicle::GetPrimitiveRoot() const
{
    return PrimitiveComponent;
}

float ANsVehicle::GetLongitudinalG() const
{
    return Acceleration.X / 9.81;
}

float ANsVehicle::GetLateralG() const
{
    return Acceleration.Y / 9.81;
}

float ANsVehicle::GetVerticalG() const
{
    return Acceleration.Z / 9.81;
}

float ANsVehicle::GetAngularVelocity() const
{
    return AngularVelocity;
}

float ANsVehicle::GetAngularAcceleration() const
{
    return AngularAcceleration;
}

TArray<UNsVehicleSuspensionComponent*> ANsVehicle::GetSuspension() const
{
    return SuspensionComponents;
}

void ANsVehicle::SetSuspensionComponents(const TArray<UNsVehicleSuspensionComponent*>& NewSuspensions)
{
    SuspensionComponents = NewSuspensions;
}

float ANsVehicle::GetLinearSpeed() const
{
    return Speed;
}

void ANsVehicle::SetReplicatedSpeed(const float NewSpeed)
{
    Speed = NewSpeed;
}

void ANsVehicle::UpdateGroundContact()
{
    if (Drivetrain->IsDrivetrainEnabled())
    {
        const int32 GroundedDrivenWheels = Drivetrain->UpdateWheelSlip(SuspensionComponents);
        bInAir = GroundedDrivenWheels < 1;
        return;
    }

    int32 AirborneWheels = 0;
    for (UNsVehicleSuspensionComponent* Suspension : SuspensionComponents)
    {
        if (Suspension->IsInAir() && !Suspension->IsWheelDetached())
        {
            AirborneWheels++;
        }
    }

    bInAir = AirborneWheels >= InAirWheelCount;
}

void ANsVehicle::UpdateAngularMotion()
{
    LastAngularVelocity = AngularVelocity;
    AngularVelocity = UKismetMathLibrary::InverseTransformDirection(PhysicsTransform, UAsyncTickFunctions::ATP_GetAngularVelocity(PrimitiveComponent)).Z;
    AngularAcceleration = (AngularVelocity - LastAngularVelocity) * (1.f / AsyncDeltaTime);
}

void ANsVehicle::UpdatePhysicsCore(float DeltaTime)
{
    PhysicsTransform = UAsyncTickFunctions::ATP_GetTransform(PrimitiveComponent);

    const float VelocityDirection = FMath::Sign(GetLinearVelocity());
    const FVector Forward = PhysicsTransform.GetRotation().GetForwardVector();
    const FVector Right = PhysicsTransform.GetRotation().GetRightVector();
    const FVector Up = PhysicsTransform.GetRotation().GetUpVector();

    if (Engine->GetPowertrainType() == ENsVehicleBehaviorType::Standard)
    {
        if (!UpdateStandardPowertrain(DeltaTime, Forward, Right))
        {
            return;
        }
    }
    else
    {
        UpdateArcadePowertrain(Forward, Right, VelocityDirection);
    }

    if (!bInAir)
    {
        Steering->ApplyDriftTorque(PrimitiveComponent, Up, GetLinearVelocity(), Controls->GetSteeringInput());
    }

    UpdateMotionState(DeltaTime);

    const FVector WorldVelocity = UAsyncTickFunctions::ATP_GetLinearVelocity(PrimitiveComponent);
    Brakes->ApplyHandbrake(PrimitiveComponent, SuspensionComponents, Controls->GetHandbrakeInput(), bInAir, WorldVelocity, LocalVelocity.X);
    AirControl->ApplyAirControl(PrimitiveComponent, Forward, Right, Up, SuspensionComponents, InAirWheelCount, Controls->GetAirPitchInput(), Controls->GetAirYawInput(), Controls->GetAirRollInput());
    AntiGravity->ApplyAntiGravity(PrimitiveComponent, PhysicsTransform, bInAir, AsyncDeltaTime);
    BodyDynamics->ApplyLeanSteer(PrimitiveComponent, Up, Forward, Controls->GetSteeringInput(), GetSpeedKPH(), bInAir);
}

bool ANsVehicle::UpdateStandardPowertrain(float DeltaTime, const FVector& Forward, const FVector& Right)
{
    const float WheelRadius = Transmission->GetPhysicsWheelRadius();
    const float DifferentialRatio = Transmission->GetStandardDifferentialRatio();
    const float VelocityDirection = FMath::Sign(GetLinearVelocity());
    const float Throttle = Controls->GetThrottleInput();
    const float BrakeInput = Controls->GetBrakesInput();

    if (!Engine->UpdateStandardTorque())
    {
        return false;
    }

    const float GearRatio = Transmission->UpdateActiveGearRatio();
    Transmission->UpdateAutomaticShift(Engine->GetRPM(), Engine->GetMaxRPM(), GetSpeedKPH(), bInAir, Drivetrain->IsDrivetrainEnabled());

    const float EngineBrakeForce = Engine->GetEngineBrakeForce(Throttle, GearRatio, VelocityDirection);
    const float WheelForce = Engine->GetEngineCurrentTorque() * GearRatio * DifferentialRatio * (1.f / WheelRadius) * Engine->GetRpmLimitTorqueScale(Transmission->GetUpshiftRatio());

    if (!bInAir)
    {
        float BrakeDirection = 0.f;
        const float BrakeForce = Brakes->GetStandardBrakeForce(Engine->GetRPM(), Engine->GetMaxRPM(), GetSpeedKPH(), VelocityDirection, BrakeDirection);
        Drivetrain->UpdateEffectivePower(Throttle, AsyncDeltaTime);

        const float DriveForce = (WheelForce * Transmission->GetTransmissionEfficiency() * Drivetrain->GetEffectivePower() * Throttle) + (BrakeForce * BrakeInput * BrakeDirection) + EngineBrakeForce;
        const float ResultantForce = DriveForce * 100.f;

        UAsyncTickFunctions::ATP_AddForce(PrimitiveComponent, Forward * ResultantForce, false);
        UAsyncTickFunctions::ATP_AddTorque(PrimitiveComponent, -Right * ResultantForce * 10.f, false);

        BodyDynamics->ApplyAccelerationPitch(PrimitiveComponent, Right, WheelForce * Throttle);
        BodyDynamics->ApplyBrakePitch(PrimitiveComponent, Right, BrakeForce * BrakeInput * BrakeDirection);
    }

    Aerodynamics->ApplyDrag(PrimitiveComponent, Forward, GetLinearVelocity(), VelocityDirection);
    Engine->UpdateStandardRPM(DeltaTime, GearRatio, DifferentialRatio, WheelRadius, GetLinearVelocity(), Throttle, BrakeInput, bInAir, *Drivetrain);
    Transmission->UpdateShiftTimers(DeltaTime);

    return true;
}

void ANsVehicle::UpdateArcadePowertrain(const FVector& Forward, const FVector& Right, const float VelocityDirection)
{
    const float ArcadeTorque = Engine->UpdateArcadeTorque(GetSpeedKPH());
    const float BrakeForce = Brakes->GetArcadeBrakeForce(GetSpeedKPH());
    const float MaxDriveForce = ArcadeTorque * Transmission->GetCurrentGearRatio() * Transmission->GetDifferentialRatio() * (1.f / Transmission->GetPhysicsWheelRadius());

    const float DriveForce = (Controls->GetThrottleInput() * MaxDriveForce) - (Controls->GetBrakesInput() * BrakeForce);
    const float ResultantForce = DriveForce + (Brakes->GetExternalResistanceForce() * -VelocityDirection);

    if (!bInAir)
    {
        UAsyncTickFunctions::ATP_AddForce(PrimitiveComponent, ResultantForce * Forward * 100.f, false);
        BodyDynamics->ApplyAccelerationPitch(PrimitiveComponent, Right, DriveForce);
    }
}

void ANsVehicle::UpdateMotionState(float DeltaTime)
{
    PreviousLocalVelocity = LocalVelocity;
    LocalVelocity = UKismetMathLibrary::InverseTransformDirection(PhysicsTransform, UAsyncTickFunctions::ATP_GetLinearVelocity(PrimitiveComponent));
    Speed = LocalVelocity.X * 0.01;
    Acceleration = (LocalVelocity - PreviousLocalVelocity) * 0.01 * (1.f / DeltaTime);
}

void ANsVehicle::UpdateWheelPhysics(float DeltaTime)
{
    const float Throttle = Controls->GetThrottleInput();
    const float BrakeInput = Controls->GetBrakesInput();
    const float WheelTorque = Engine->GetEngineCurrentTorque() * Transmission->GetCurrentGearRatio() * Transmission->GetDifferentialRatio() * Throttle * Transmission->GetPhysicsWheelRadius();
    const float AnimationVelocity = Animation->GetLocationVelocity().X * Animation->GetGlobalWheelSpeedMultiplier();

    for (UNsVehicleSuspensionComponent* Suspension : SuspensionComponents)
    {
        if (Suspension != nullptr)
        {
            Suspension->UpdatePhysics(DeltaTime, WheelTorque, Drivetrain->GetSlipInitiateSpeed(), Drivetrain->GetSlipRestoreSpeed(), Throttle, BrakeInput);
            Suspension->UpdateAnimationState(Animation->IsAnimationModeEnabled(), AnimationVelocity);
        }
    }
}

void ANsVehicle::UpdateAttachedComponents(float DeltaTime)
{
    Steering->UpdateWheelSteering(SuspensionComponents, Controls->GetSteeringInput(), GetSpeedKPH());

    for (UNsVehicleSuspensionComponent* Suspension : SuspensionComponents)
    {
        Suspension->UpdateTick(DeltaTime);
        Suspension->SetHandbrakeInput(Controls->GetHandbrakeInput());
    }

    for (UNsVehicleInteriorComponent* Interior : InteriorComponents)
    {
        Interior->Update(DeltaTime, *this);
    }

    for (UNsVehicleArticulatedComponent* Articulated : ArticulatedComponents)
    {
        Articulated->Update(DeltaTime);
    }
}
