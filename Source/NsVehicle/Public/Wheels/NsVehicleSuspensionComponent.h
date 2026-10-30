// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CollisionShape.h"
#include "Engine/HitResult.h"
#include "NsVehicleSuspensionComponent.generated.h"

/** The axle that the handbrake of a wheel belongs to */
UENUM(BlueprintType)
enum class ENsVehicleHandbrakeWheelType : uint8
{
    /** The wheel has no handbrake axle */
    None UMETA(DisplayName = "None"),
    /** The wheel is on the front handbrake axle */
    Front UMETA(DisplayName = "Front"),
    /** The wheel is on the rear handbrake axle */
    Rear UMETA(DisplayName = "Rear")
};

class UStaticMesh;
class UStaticMeshComponent;

/** Simulates and renders one vehicle wheel and its suspension travel */
UCLASS(ClassGroup = (NsVehicle), meta = (BlueprintSpawnableComponent))
class NSVEHICLE_API UNsVehicleSuspensionComponent : public USceneComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Constructor */
    UNsVehicleSuspensionComponent();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** Empty. It stays for source compatibility */
    void ConstructWheelMeshComponent();

    /** Runs the suspension trace and applies the spring, tyre, and drivetrain forces. ANsVehicle calls it. Another pawn must call it from a tick, an async tick is best */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void UpdatePhysics(float DeltaTime, float transmittedTorque, float slipForward, float slipBackwards, float throttle, float brakes);

    /** Returns the wheel mesh component. Call GetStaticMesh on it to get the wheel mesh asset */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    UStaticMeshComponent* GetWheelMeshComponent();

    /** Sets the spring stiffness in N/m */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void SetStiffness(float inStiffness);

    /** Sets the spring damping */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void SetDamping(float inDamping);

    /** Sets the maximum spring extension in cm */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void SetExtensionLength(float inLength);

    /** Sets the steering flag, the driven flag, and the torque share from C++ or Blueprint */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void ConfigureDrivetrain(bool bInSteeringWheel, bool bInDrivenWheel, float InTorqueReceived);

    /** Empty. The wheels render during play */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent Construction", meta = (DeprecatedFunction, DeprecationMessage = "Function has been deprecated. Wheels will be rendered during play"))
    void RenderWheelMesh();

    /** Applies the forces to a different physics body, for example a trailer */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void SetNewPhysicsBody(UPrimitiveComponent* physicsBody);

    /** Releases the wheel mesh as a free physics body and stops all forces of this suspension. The impulse is a velocity change in cm/s. This is local only and does not replicate */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void DetachWheel(FVector Impulse);

    /** Puts a detached wheel mesh back on this suspension and starts the suspension forces again */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void ReattachWheel();

    /** Returns true after DetachWheel, until ReattachWheel */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    bool IsWheelDetached() const;

    /** Returns true when the wheel does not touch a surface */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    bool IsInAir();

    /** Returns the current spring length in cm */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    float GetCurrentLength();

    /** Returns the hit result of the suspension trace. It includes the physical material of the surface */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    FHitResult GetHitResult();

    /** Returns the spring compression from 0 to 1 */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    float GetCompression();

    /** Returns true when the handbrake affects this wheel */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    bool IsHandbrakeWheel();

    /** Returns the steering angle in degrees */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    float GetSteeringRotation();

    /** Returns the Z offset in cm that places the wheel mesh */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    float GetWheelRelativeOffset();

    /** Returns the wheel rotation step for the wheel animation */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    FRotator GetWheelLocalRotation();

    /** Returns true when the drivetrain sends torque to this wheel */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    bool GetIsDrivenWheel();

    /** Returns the ratio of the applied torque to the friction limit. Above 1 the wheel slips */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    float GetSlipRatio();

    /** Returns the smoothed throttle slip value */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    float GetSlipGamma();

    /** Returns a point near the tyre contact for tyre smoke or trails */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    FVector GetOffsetImpactPoint(float zOffset);

    /** Returns the handbrake axle of this wheel */
    ENsVehicleHandbrakeWheelType GetWheelHandbrake();

    /** Returns true when the wheel mesh turns 180 degrees, usually a left wheel */
    bool IsLeftWheel();

    /** Returns the wheel radius in cm */
    float GetWheelRadius();

    /** Returns the wheel spin multiplier */
    float GetAngularVelocityMultiplier();

    /** Returns the constant burnout spin */
    float GetBurnoutRotation();

    /** Returns true when the wheel mesh mirrors on its local Y axis */
    UFUNCTION(BlueprintPure, Category = "NsVehicleSuspensionComponent")
    bool GetMirrorWheelOnLocalY() const;

    /** Places a wheel mesh at an approximate rest position in the editor. Call it in the construction script only */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Suspension|Procedural")
    void StaticallyRenderWheel(UStaticMeshComponent* MeshComponent);

    /** Destroys the editor wheel mesh from StaticallyRenderWheel. Call it in Begin Play only */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Suspension|Procedural")
    void StaticWheelCallback(UStaticMeshComponent* MeshComponent);

    /** Sets the scale of the wheel mesh and the trace sphere */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void SetWheelScale(float inScale);

    /** Mirrors the wheel mesh on its local Y axis. The suspension pivot does not move */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void SetMirrorWheelOnLocalY(bool bInMirrorWheelOnLocalY);

    /** Calculates the wheel radius again from the wheel mesh. Use it after a wheel mesh change at runtime */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void RecalculateWheelRadius();

    /** Places, steers, and spins the wheel mesh on the game thread. ANsVehicle calls it every frame */
    void UpdateTick(float DeltaTime);

    /** Sets the steering input and the steering response speed */
    void SetSteeringInput(float inSteering, float sensitivity);

    /** Spins a wheel mesh from the actor movement for the take recorder */
    UFUNCTION(BlueprintCallable, Category = "NsVehicleSuspensionComponent")
    void SimulateInternalAnimation(float deltaTime, UStaticMeshComponent* wheelMeshComponent);

    /** Sets the animation mode and the actor velocity from ANsVehicle */
    void UpdateAnimationState(bool bInUseAnimationSystem, float inLocationVelocity);

    /** Returns true when this wheel steers */
    bool IsSteeringWheel();

    /** Turns the suspension to the steering angle. The server sets it, and the clients copy it */
    void UpdateSteeringGeometry(float DeltaTime);

    /** Sets the handbrake input */
    void SetHandbrakeInput(bool bInHandbrakeOn);

    /** Starts the client steering blend when a new steering angle arrives */
    UFUNCTION()
    void OnRep_Steering();

private:
    /** Makes a new wheel mesh component and attaches it to this suspension */
    void InstantiateWheel();

    /** Sets the wheel mesh scale, with the mirror option */
    void ApplyWheelVisualScale(float InScale);

    /** Returns the wheel mesh scale vector for a uniform scale, with the mirror option */
    FVector ResolveWheelVisualScale(float InScale) const;

    /** Returns the largest absolute scale of the wheel mesh */
    float GetWheelVisualScale() const;

    /** Logs warnings for a suspension setup that cannot hold the vehicle */
    void ValidateSuspensionSetup() const;

// Variables
public:
    /** The wheel radius from StaticallyRenderWheel. It is not in use */
    float WheelStaticRadius = 0.f;

    /** The actor location in the previous animation frame */
    FVector Previous_UAS_Location = FVector::ZeroVector;

    /** The actor location in this animation frame */
    FVector Current_UAS_Location = FVector::ZeroVector;

    /** True when the take recorder animation system spins the wheels */
    bool bUseAnimationSystem = false;

    /** The forward velocity in m/s from the actor movement */
    float LocationVelocity = 0.f;

    /** The steering angle in degrees from the server */
    UPROPERTY(ReplicatedUsing=OnRep_Steering)
    float ServerSteeringRotation;

    /** The time in seconds since the last steering update on a client */
    float ClientTimeSinceUpdate;

    /** The time in seconds between the last two steering updates on a client */
    float ClientTimeBetweenLastUpdates;

    /** The steering angle at the start of the client blend */
    float ClientStartSteeringRotation;

    /** Stops all forces of this suspension. Use it only for special cases */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    bool bZeroForceComponent = false;

private:
    /** The spring stiffness in N/m. Higher values give a higher rest height and more spring. A heavier vehicle needs a higher stiffness */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    float Stiffness = 22500.f;

    /** How fast the spring comes to rest. Higher values give less bounce */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    float Damping = 1250.f;

    /** The maximum spring extension in cm from the suspension component */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    float ExtensionLength = 50.f;

    /** Applies contact forces with no contact. This is not safe. Keep it off */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    bool bOverrideContact = false;

    /** True when this wheel gets the steering input */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    bool bIsSteeringWheel = false;

    /** The trace channel of the suspension. Use a custom channel to ignore surfaces */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    TEnumAsByte<ECollisionChannel> CollisionChannel = ECollisionChannel::ECC_WorldStatic;

    /** Keeps the pitch and roll of the suspension when it steers. If rotation glitches occur, turn it off and turn the wheel mesh instead */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Core", meta = (AllowPrivateAccess = "true"))
    bool bAllowFullRotation = false;

    /** Uses the first child static mesh component as the wheel mesh, so the wheel shows in the editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Wheel", meta = (AllowPrivateAccess = "true"))
    bool bInheritMeshFromComponent = false;

    /** The wheel mesh that the suspension makes in Begin Play */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Wheel", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bInheritMeshFromComponent==false", EditConditionHides))
    UStaticMesh* WheelMesh;

    /** Hides the wheel mesh. It applies one time, in Begin Play */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Wheel", meta = (AllowPrivateAccess = "true"))
    bool bHideWheelMesh = false;

    /** Turns the wheel mesh 180 degrees. Use it when the wheel faces the wrong direction, usually on the left side */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Wheel", meta = (AllowPrivateAccess = "true"))
    bool bRotateWheel = false;

    /** Mirrors the wheel mesh on its local Y axis. The suspension pivot does not move */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Wheel", meta = (AllowPrivateAccess = "true"))
    bool bMirrorWheelOnLocalY = false;

    /** Moves the wheel mesh on the Z axis in cm. It has no effect on physics */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "NsVehicle|Suspension|Wheel", meta = (AllowPrivateAccess = "true"))
    float WheelZOffset = 0.f;

    /** True when the handbrake affects this wheel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Wheel|Handbrake", meta = (AllowPrivateAccess = "true"))
    bool bHandbrakeWheel = false;

    /** The handbrake axle of this wheel. The vehicle counts the wheels on each axle */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Handbrake", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bHandbrakeWheel==true", EditConditionHides))
    ENsVehicleHandbrakeWheelType HandbrakeWheelType = ENsVehicleHandbrakeWheelType::Front;

    /** Scales the wheel spin during the handbrake */
    float HandbrakeVelocityMultiplier = 1.f;

    /** The handbrake input */
    bool bHandbrakeInput = false;

    /** Uses this wheel with the drivetrain system. Also enable the drivetrain on the vehicle. In this mode, LateralForceScalar, WheelAngularVelocityMultiplier, BurnoutRotation, and IdleLock do not change at runtime */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"))
    bool bUseWithDrivetrain = false;

    /** True when the drivetrain sends torque to this wheel */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true", EditConditionHides))
    bool bDrivenWheel = false;

    /** The share of the drive torque for this wheel, from 0 to 1. For example, 0.5 on each rear wheel for RWD */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0"), meta = (ClampMax = "1"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float TorqueReceived = 0.5f;

    /** The friction for the traction limit. Higher values give less slip */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float FrictionCoefficient = 1.5f;

    /** The lateral force scalar when the wheel does not slip */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float MaxLateralForceScalar = 3000.f;

    /** The lateral force scalar when the wheel slips */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float SlippingLateralForceScalar = 250.f;

    /** How much faster the wheel spins when it slips */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float SlippingAngularVelocity = 4.f;

    /** The maximum wheel spin with throttle in the air */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float AirRotationMultiplier = 20.f;

    /** How fast the wheel spins up with throttle in the air */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float InAirSpeedUp = 10.f;

    /** How fast the wheel slows down with no throttle in the air */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Drivetrain", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==true && bDrivenWheel==true", EditConditionHides))
    float InAirSlowdownSpeed = 2.f;

    /** The drive torque on this wheel */
    float AppliedTorque = 0.f;

    /** The ratio of the applied torque to the friction limit */
    float FrRatio = 0.f;

    /** The smoothed throttle value for the slip blend */
    float ThrottleSlipGamma = 0.f;

    /** True when the wheel slips */
    bool Gamma = false;

    /** The spin multiplier target from the slip state */
    float DynamicAngularVelocity = 1.f;

    /** The lateral force target from the slip state */
    float DynamicLateralForce = 2000.f;

    /** The grip scalar. Higher values give more lateral force. Balance it on all wheels. More at the front gives oversteer. More at the rear gives understeer */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Tyre", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==false", EditConditionHides))
    float LateralForceScalar = 2000.f;

    /** The maximum lateral tyre force in N */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Tyre", meta = (AllowPrivateAccess = "true"))
    float MaxLateralForce = 8500.f;

    /** The grip scalar at very low speed. Keep it higher than LateralForceScalar to stop low speed slip */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Tyre", meta = (AllowPrivateAccess = "true"))
    float LowSpeedLateralForceScalar = 4000.f;

    /** A constant wheel spin. Use it for a burnout at zero speed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "NsVehicle|Suspension|Burnout", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==false", EditConditionHides))
    float BurnoutRotation = 0.f;

    /** The spring compression from 0 to 1 for GetCompression */
    float OutCompression = 0.f;

    /** The steering input from -1 to 1 */
    float SteeringInput = 0.f;

    /** The steering response speed */
    float SensitvityInput = 0.f;

    /** Moves the lateral force point from the center of mass height. It controls the body roll, like an anti-roll bar. Negative values are valid */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Tyre", meta = (AllowPrivateAccess = "true"))
    float ContactForceOffsetLocation = 0.f;

    /** Scales the wheel spin. Increase it for a burnout or a drift */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "NsVehicle|Suspension|Tyre", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==false", EditConditionHides))
    float WheelAngularVelocityMultiplier = 1.f;

    /** Opposes the forward movement at the tyre contact */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Tyre", meta = (AllowPrivateAccess = "true"))
    float ResistanceForceCoefficient = 0.f;

    /** The maximum resistance force in N. It has no effect when ResistanceForceCoefficient is 0 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|Tyre", meta = (AllowPrivateAccess = "true"))
    float MaxResistanceForce = 5000.f;

    /** The maximum steering angle in degrees. The lateral force is perpendicular to the wheel direction */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|InputResponse", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bIsSteeringWheel==true", EditConditionHides))
    float MaxSteeringAngle = 40.f;

    /** How fast the wheel steers back to 0 degrees */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|InputResponse", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bIsSteeringWheel==true", EditConditionHides))
    float SteeringReleaseSpeed = 12.f;

    /** Uses a sphere trace, and a line trace when the sphere trace is not suitable. Off uses a line trace only, which can suit arcade behavior */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Raycast", meta = (AllowPrivateAccess = "true"))
    bool bHybridRaycast = true;

    /** The trace sphere radius in cm when there is no wheel mesh. The wheel mesh sets the radius when it exists */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Raycast", meta = (AllowPrivateAccess = "true"))
    float Radius =33.f;

    /** The impact normal limit for the sphere trace. Values near 1 keep the sphere trace more often */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Raycast", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bHybridRaycast==true", EditConditionHides), meta = (ClampMin = "0"), meta = (ClampMax = "1"))
    float ImpactNormalTolerance = 0.9f;

    /** The maximum contact height in cm for the sphere trace. Lower values keep the contact near the bottom of the wheel */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Raycast", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bHybridRaycast==true", EditConditionHides))
    float ImpactHeightTolerance = 30.f;

    /** Draws the suspension trace and the contact */
    UPROPERTY(EditAnywhere, Category = "NsVehicle|Suspension|Raycast", meta = (AllowPrivateAccess = "true"))
    bool bDebug = false;

    /** The start yaw of the suspension in degrees */
    int Direction = 0;

    /** The wheel mesh component that this suspension moves */
    UStaticMeshComponent* WheelMeshComponent;

    /** The wheel radius in cm */
    float WheelRadius = 0.315f;

    /** The steering angle in degrees */
    float SteeringRotation = 0.f;

    /** The Z offset in cm that places the wheel mesh */
    float WheelRelativeOffset = 0.f;

    /** The wheel rotation step for the wheel animation */
    FRotator WheelLocalRotation = FRotator::ZeroRotator;

    /** The time in seconds of the last async physics step */
    float AsyncTime = 0.f;

    /** The time in seconds of the last game frame */
    float GameTime = 0.f;

    /** The physics body that gets the forces */
    UPrimitiveComponent* VehiclePrimitiveComponent;

    /** The world transform of the suspension in the last physics step */
    FTransform PhysicsTransform;

    /** The sphere for the suspension trace */
    FCollisionShape SweepShape;

    /** The local direction from the contact to the suspension */
    FVector ImpactNormal = FVector::ZeroVector;

    /** The dot product of the down direction and the impact normal */
    float ImpactDot = 0.f;

    /** The hit result of the suspension trace */
    FHitResult Hit;

    /** The spring length in cm */
    float CurrentLength = 40.f;

    /** The spring compression in the previous physics step */
    float PreviousCompression = 0.f;

    /** The local velocity in m/s at the suspension */
    FVector LinearVelocity = FVector::ZeroVector;

    /** True when the last trace was a line trace */
    bool bSingleRaycast = false;

    /** The start of the suspension trace */
    FVector Start = FVector::ZeroVector;

    /** The end of the suspension trace */
    FVector End = FVector::ZeroVector;

    /** The wheel Z offset for the animation */
    float wheelAnimZ = 0.f;

    /** The wheel rotation step for the animation */
    FRotator angularAnimRotator = FRotator::ZeroRotator;

    /** Locks the wheel spin when the vehicle stops, to prevent micro rotation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|IdleLock", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==false", EditConditionHides))
    bool bUseIdleLock = true;

    /** The time in seconds after a stop before the wheel spin locks */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Suspension|IdleLock", meta = (AllowPrivateAccess = "true"), meta = (EditCondition = "bUseWithDrivetrain==false", EditConditionHides))
    float IdleLockTime = 1.5f;

    /** True when the wheel spin is locked */
    bool bIsIdleLocked = false;

    /** The time in seconds that the wheel is below the idle lock speed */
    float IdleLockCount = 0;

    /** The speed in KPH below which the idle lock timer starts */
    float IdleLockSpeedThreshold = 1.f;

    /** True between DetachWheel and ReattachWheel. The suspension then applies no force and does not move the wheel mesh */
    bool bWheelDetached = false;

    friend class AVehicleDebugHUD;
};
