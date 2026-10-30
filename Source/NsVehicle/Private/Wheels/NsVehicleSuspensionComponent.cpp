// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Wheels/NsVehicleSuspensionComponent.h"
#include "AsyncTickFunctions.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"

#include "Physics/Experimental/PhysScene_Chaos.h"

//5.4
#include "Modules/ModuleInterface.h"
#include "PhysicsInterfaceDeclaresCore.h"

UNsVehicleSuspensionComponent::UNsVehicleSuspensionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    ComponentTags.Add(FName("NsVehicleSuspensionComponent"));
    /*
#if WITH_EDITOR
    WheelMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wheel Mesh"));
    //WheelMeshComponent->SetupAttachment();
    WheelMeshComponent->SetupAttachment(this);
    WheelMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);


#endif
    */
}

void UNsVehicleSuspensionComponent::ConstructWheelMeshComponent()
{

}

void UNsVehicleSuspensionComponent::BeginPlay()
{
    Super::BeginPlay();

    VehiclePrimitiveComponent = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());
    bool bValidChild = GetChildComponent(0) != nullptr;

    if (!bInheritMeshFromComponent || !bValidChild)
    {
        InstantiateWheel();

        if (VehiclePrimitiveComponent == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("FTSuspension: No valid primitive root component! in vehicle found"));
        }


        WheelMeshComponent->SetStaticMesh(WheelMesh);
    }
    else
    {
        TArray<USceneComponent*,FDefaultAllocator> wheels;
        GetChildrenComponents(true, wheels);

        if (wheels.Num() > 0)
        {
            if (Cast<UStaticMeshComponent>(GetChildComponent(0)) != nullptr)
            {
                WheelMeshComponent = Cast<UStaticMeshComponent>(GetChildComponent(0));
                WheelMeshComponent->SetIsReplicated(false);
                WheelMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                ApplyWheelVisualScale(WheelMeshComponent->GetRelativeScale3D().GetAbsMax());
            }
            else
            {
                InstantiateWheel();
            }
        }
        else
        {
            InstantiateWheel();
        }
    }



    if (bRotateWheel)
    {
        WheelMeshComponent->SetRelativeRotation((FRotator(0.f, 180.f, 0.f)));
    }

    if (WheelMeshComponent->GetStaticMesh() != nullptr)
    {
        WheelRadius = WheelMeshComponent->GetStaticMesh()->GetBoundingBox().GetSize().Z / 2.f;
    }
    else
    {
        WheelRadius = Radius;
    }

    SweepShape = FCollisionShape::MakeSphere(WheelRadius);

    Direction = this->GetRelativeRotation().Yaw;


    WheelMeshComponent->SetVisibility(!bHideWheelMesh);

    if (bUseWithDrivetrain)
    {
        bUseIdleLock = false;
    }

    ValidateSuspensionSetup();

//    UE_LOG(LogTemp, Warning, TEXT("Wheel Radius: %f"), WheelRadius);

}


void UNsVehicleSuspensionComponent::SetSteeringInput(float inSteering, float sensitivity)
{
    SteeringInput = inSteering;
    SensitvityInput = sensitivity;

    //ClientSetSteeringInput(SteeringInput, SensitvityInput);
}

void UNsVehicleSuspensionComponent::SimulateInternalAnimation(float deltaTime, UStaticMeshComponent* wheelMeshComponent)
{
    if(wheelMeshComponent==nullptr)
    {
        return;
    }

    FTransform t = GetOwner()->GetActorTransform();
    Previous_UAS_Location = Current_UAS_Location;
    Current_UAS_Location = t.GetLocation();

    FVector worldlocationVelocity = (Current_UAS_Location - Previous_UAS_Location) * (1.f / 100.f) * (1.f / deltaTime);
    LocationVelocity = UKismetMathLibrary::InverseTransformDirection(t, worldlocationVelocity).X;

    float v = LocationVelocity;
    float angularVelocity = (v * 100.f) / WheelRadius * -1.f;

    //If the frame rate is higher than 60, scale the rotation accordingly to compensate these extra rotations added every frame
    float timeScalar = 60.f / FMath::Pow(deltaTime, -1.f);

    FRotator angularRotator = FRotator( (angularVelocity - BurnoutRotation) * timeScalar, 0.f, 0.f);
    angularAnimRotator = angularRotator;

    int d = 1;
    if (bRotateWheel)
    {
        d = -1;
    }



    wheelMeshComponent->AddLocalRotation(( angularRotator * WheelAngularVelocityMultiplier * HandbrakeVelocityMultiplier * d));

    //Vertical displacement may need to be handled as well

}

void UNsVehicleSuspensionComponent::UpdateAnimationState(bool bInUseAnimationSystem, float inLocationVelocity)
{
    bUseAnimationSystem = bInUseAnimationSystem;
    LocationVelocity = inLocationVelocity;
}
/*
void UNsVehicleSuspensionComponent::ClientSetSteeringInput_Implementation(float inSteering, float sensitivity)
{
    SteeringInput = inSteering;
    SensitvityInput = sensitivity;
}
*/
//Set steering Sensitivity
void UNsVehicleSuspensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UNsVehicleSuspensionComponent, ServerSteeringRotation);
}

void UNsVehicleSuspensionComponent::UpdateSteeringGeometry(float DeltaTime)
{
    float steeringSpeed = SteeringInput == 0 ? SteeringReleaseSpeed : SensitvityInput;
    float r = FMath::FInterpTo(GetRelativeRotation().Yaw, (SteeringInput * MaxSteeringAngle), GameTime, steeringSpeed);
    SteeringRotation = r;
    FRotator baseRotation = this->GetRelativeRotation();


    if (GetOwner()->HasAuthority())
    {
        ServerSteeringRotation = r;
        //FRotator rotator = FRotator(baseRotation.Pitch, r, baseRotation.Roll);
        FRotator rotator = bAllowFullRotation ? FRotator(baseRotation.Pitch, r, baseRotation.Roll) : FRotator(0.f, r, 0.f);
        this->SetRelativeRotation(rotator);
    }
    else
    {
        ClientTimeSinceUpdate += DeltaTime;

        if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER)
        {
            //return;
        }

        float lerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;
        float smooth_q = FMath::LerpStable(ClientStartSteeringRotation, ServerSteeringRotation, lerpRatio);

        //FRotator qs = FRotator(baseRotation.Pitch, ServerSteeringRotation, baseRotation.Roll);
        FRotator qs = bAllowFullRotation ? FRotator(baseRotation.Pitch, ServerSteeringRotation, baseRotation.Roll) : FRotator(0.f, ServerSteeringRotation, 0.f);
        this->SetRelativeRotation(qs);
    }

}

void UNsVehicleSuspensionComponent::SetHandbrakeInput(bool bInHandbrakeOn)
{
    bHandbrakeInput = bInHandbrakeOn;
}


void UNsVehicleSuspensionComponent::OnRep_Steering()
{
    ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
    ClientTimeSinceUpdate = 0;

    float steeringSpeed = SteeringInput == 0 ? SteeringReleaseSpeed : SensitvityInput;
    float r = FMath::FInterpTo(GetRelativeRotation().Yaw, SteeringInput * MaxSteeringAngle, GameTime, steeringSpeed);
    //float r = SteeringInput * MaxSteeringAngle;
    ClientStartSteeringRotation = r;
    FRotator baseRotation = this->GetRelativeRotation();
    FRotator qs = bAllowFullRotation ? FRotator(0.f, ClientStartSteeringRotation, 0.f) : FRotator(baseRotation.Pitch,ClientStartSteeringRotation, baseRotation.Roll);
    //this->SetRelativeRotation(qs);
    //Maybe update relative rotation?
}

void UNsVehicleSuspensionComponent::InstantiateWheel()
{
    WheelMeshComponent = NewObject<UStaticMeshComponent>(GetOwner());

    if (WheelMeshComponent != nullptr)
    {
        WheelMeshComponent->RegisterComponent();
        WheelMeshComponent->SetWorldLocation(this->GetComponentLocation());
        WheelMeshComponent->SetWorldRotation(this->GetComponentRotation());

        WheelMeshComponent->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);

        WheelMeshComponent->SetIsReplicated(false);

        WheelMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        ApplyWheelVisualScale(WheelMeshComponent->GetRelativeScale3D().GetAbsMax());
        if (WheelMeshComponent) WheelMeshComponent->SetStaticMesh(WheelMesh);
    }
}


bool UNsVehicleSuspensionComponent::IsSteeringWheel()
{
    return bIsSteeringWheel;
}


void UNsVehicleSuspensionComponent::UpdateTick(float DeltaTime)
{
    GameTime = DeltaTime;

    if (bWheelDetached)
    {
        return;
    }

    float deformation = 0.05f * WheelRadius;
    deformation = 0.f;
    wheelAnimZ = 0.f;

    if (bSingleRaycast)
    {
        wheelAnimZ = -CurrentLength + (0.5f * WheelRadius * 2.f) - deformation;
        WheelMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -CurrentLength + (0.5f * WheelRadius * 2.f) - deformation));
        WheelRelativeOffset = -CurrentLength + (0.5f * WheelRadius * 2.f) - deformation;
    }
    else
    {
        wheelAnimZ = -CurrentLength - deformation;
        WheelMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -CurrentLength - deformation + WheelZOffset));
        WheelRelativeOffset = -CurrentLength - deformation;
    }

    UpdateSteeringGeometry(DeltaTime);


#pragma region DynamicCamber

#pragma endregion


    float v = LinearVelocity.X;
    float angularVelocity =  (v * 100.f) / WheelRadius * -1.f;

    //If the frame rate is higher than 60, scale the rotation accordingly to compensate these extra rotations added every frame
    float timeScalar = 60.f / FMath::Pow(DeltaTime, -1.f);

    FRotator angularRotator = FRotator((angularVelocity-BurnoutRotation)*timeScalar, 0.f, 0.f);
    angularAnimRotator = angularRotator;

    int d = 1;
    if (bRotateWheel)
    {
        d = -1;
    }

    if (!bIsIdleLocked && !(bHandbrakeInput && bHandbrakeWheel))
    {
        if (!bUseAnimationSystem)
        {
            WheelLocalRotation = angularRotator * WheelAngularVelocityMultiplier * HandbrakeVelocityMultiplier * d;
            WheelMeshComponent->AddLocalRotation((angularRotator * WheelAngularVelocityMultiplier * HandbrakeVelocityMultiplier * d));
        }
    }

    if (bIsIdleLocked)
    {
        FRotator staticRotator = FRotator(-BurnoutRotation * timeScalar, 0.f, 0.f);
        WheelLocalRotation = staticRotator * WheelAngularVelocityMultiplier * HandbrakeVelocityMultiplier * d;
    }

    else if ((bHandbrakeInput && bHandbrakeWheel))
    {
        WheelMeshComponent->AddLocalRotation(FRotator(0.f));
        WheelLocalRotation = FRotator::ZeroRotator;
    }


    if (bDebug)
    {
        UWorld* const World = GetWorld();
        if (World == nullptr)
        {
            return;
        }

        const FVector DebugEnd = End.IsNearlyZero() ? Start - PhysicsTransform.GetRotation().GetUpVector() * ExtensionLength : End;
        const FVector DebugTraceDelta = DebugEnd - Start;
        const float DebugTraceLength = DebugTraceDelta.Size();
        const FVector DebugTraceDirection = DebugTraceLength > UE_SMALL_NUMBER ? DebugTraceDelta / DebugTraceLength : -PhysicsTransform.GetRotation().GetUpVector();
        const float DebugRadius = SweepShape.GetSphereRadius();
        const float DebugCompression = FMath::Clamp(OutCompression, 0.f, 1.f);
        const FColor DebugPressureColor = FLinearColor::LerpUsingHSV(FLinearColor::Green, FLinearColor::Red, DebugCompression).ToFColor(true);
        constexpr float DebugLineThickness = 0.f;
        constexpr float DebugPointSize = 6.f;

        if (bSingleRaycast)
        {
            DrawDebugLine(World, Start, DebugEnd, FColor::Orange, false, -1.f, 1, DebugLineThickness);
        }
        else
        {
            const FVector DebugCapsuleCenter = Start + DebugTraceDelta * 0.5f;
            const FQuat DebugCapsuleRotation = FRotationMatrix::MakeFromZ(DebugTraceDirection).ToQuat();
            DrawDebugCapsule(World, DebugCapsuleCenter, DebugTraceLength * 0.5f + DebugRadius, DebugRadius, DebugCapsuleRotation, DebugPressureColor, false, -1.f, 1, DebugLineThickness);
        }

        if (Hit.bBlockingHit)
        {
            DrawDebugLine(World, Start, Hit.ImpactPoint, DebugPressureColor, false, -1.f, 1, DebugLineThickness);

            if (bSingleRaycast)
            {
                DrawDebugPoint(World, Hit.ImpactPoint, DebugPointSize, DebugPressureColor, false, -1.f, 1);
            }
            else
            {
                DrawDebugSphere(World, Hit.Location, DebugRadius, 12, DebugPressureColor, false, -1.f, 1, DebugLineThickness);
                DrawDebugPoint(World, Hit.ImpactPoint, DebugPointSize, FColor::Yellow, false, -1.f, 1);
            }
        }
    }


}

void UNsVehicleSuspensionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UNsVehicleSuspensionComponent::StaticallyRenderWheel(UStaticMeshComponent* MeshComponent)
{
    if (!ensure(MeshComponent != nullptr))return;

    FVector min = FVector(0);
    FVector max = FVector(0);
    MeshComponent->GetLocalBounds(min, max);
    float lRadius = max.Z / 2.f;
    MeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -ExtensionLength + (0.5f * lRadius * 2.f) +  ((9810.f/Stiffness)) ));
    //WheelStaticRadius = lRadius;
}

void UNsVehicleSuspensionComponent::StaticWheelCallback(UStaticMeshComponent* MeshComponent)
{
    MeshComponent->DestroyComponent();
}

void UNsVehicleSuspensionComponent::SetWheelScale(float inScale)
{
    if (WheelMeshComponent == nullptr)
    {
        return;
    }

    if (WheelMeshComponent->GetStaticMesh() != nullptr)
    {
        WheelRadius = WheelMeshComponent->GetStaticMesh()->GetBoundingBox().GetSize().Z / 2.f;
    }
    else
    {
        WheelRadius = Radius;
    }

    SweepShape = FCollisionShape::MakeSphere(WheelRadius * FMath::Abs(inScale));
    ApplyWheelVisualScale(inScale);
    ValidateSuspensionSetup();
}

void UNsVehicleSuspensionComponent::RecalculateWheelRadius()
{
    if (WheelMeshComponent->GetStaticMesh() != nullptr)
    {
        WheelRadius = WheelMeshComponent->GetStaticMesh()->GetBoundingBox().GetSize().Z / 2.f;
    }

    else
    {
        WheelRadius = Radius;
    }

    SweepShape = FCollisionShape::MakeSphere(WheelRadius * GetWheelVisualScale());
    ValidateSuspensionSetup();
}

void UNsVehicleSuspensionComponent::SetMirrorWheelOnLocalY(const bool bInMirrorWheelOnLocalY)
{
    bMirrorWheelOnLocalY = bInMirrorWheelOnLocalY;

    if (WheelMeshComponent != nullptr)
    {
        ApplyWheelVisualScale(WheelMeshComponent->GetRelativeScale3D().GetAbsMax());
    }
}

void UNsVehicleSuspensionComponent::ApplyWheelVisualScale(const float InScale)
{
    if (WheelMeshComponent != nullptr)
    {
        WheelMeshComponent->SetRelativeScale3D(ResolveWheelVisualScale(InScale));
    }
}

FVector UNsVehicleSuspensionComponent::ResolveWheelVisualScale(const float InScale) const
{
    const float SafeScale = FMath::Max(FMath::Abs(InScale), UE_SMALL_NUMBER);
    return FVector(SafeScale, bMirrorWheelOnLocalY ? -SafeScale : SafeScale, SafeScale);
}

float UNsVehicleSuspensionComponent::GetWheelVisualScale() const
{
    return WheelMeshComponent != nullptr ? FMath::Max(WheelMeshComponent->GetRelativeScale3D().GetAbsMax(), UE_SMALL_NUMBER) : 1.f;
}

void UNsVehicleSuspensionComponent::ValidateSuspensionSetup() const
{
    if (bZeroForceComponent || WheelMeshComponent == nullptr)
    {
        return;
    }

    const float CollisionRadiusCm = WheelRadius * GetWheelVisualScale();
    if (CollisionRadiusCm <= UE_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("NsVehicle suspension '%s' has an invalid wheel collision radius %.3f cm. Check wheel mesh, wheel scale, or fallback Radius"),
            *GetPathNameSafe(this),
            CollisionRadiusCm);
        return;
    }

    if (ExtensionLength < CollisionRadiusCm)
    {
        UE_LOG(LogTemp, Warning, TEXT("NsVehicle suspension '%s' has ExtensionLength %.3f cm shorter than wheel collision radius %.3f cm. The wheel can visually sink through the ground under compression; increase ExtensionLength or move the suspension attachment up"),
            *GetPathNameSafe(this),
            ExtensionLength,
            CollisionRadiusCm);
    }

    if (Stiffness <= UE_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("NsVehicle suspension '%s' has Stiffness %.3f while force is enabled. This wheel will not support the vehicle"),
            *GetPathNameSafe(this),
            Stiffness);
    }

    if (Damping <= UE_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("NsVehicle suspension '%s' has Damping %.3f while force is enabled. This wheel can bounce or collapse abruptly"),
            *GetPathNameSafe(this),
            Damping);
    }
}

void UNsVehicleSuspensionComponent::UpdatePhysics(float DeltaTime, float transmittedTorque, float slipForward, float slipBackwards, float throttle, float brakes)
{
    AsyncTime = DeltaTime;

    if (bWheelDetached)
    {
        return;
    }

    //IdleLock only for animation

    if (FMath::Abs((LinearVelocity.X * 3.6f)) < IdleLockSpeedThreshold)
    {
        if (!bIsIdleLocked)
        {
            IdleLockCount += DeltaTime;
        }

        if ((IdleLockCount > IdleLockTime) && bUseIdleLock)
        {
            bIsIdleLocked = true;
            IdleLockCount = 612.f;
        }
    }
    else
    {
        IdleLockCount = 0;
        bIsIdleLocked = false;
    }

    FTransform vehicleTransform = UAsyncTickFunctions::ATP_GetTransform(VehiclePrimitiveComponent);
    PhysicsTransform = GetRelativeTransform() * vehicleTransform;
    Start = PhysicsTransform.GetLocation();
    FVector up = PhysicsTransform.GetRotation().GetUpVector();
    FVector right = PhysicsTransform.GetRotation().GetRightVector();
    FVector fwd = PhysicsTransform.GetRotation().GetForwardVector();

    FVector worldPointVelocity = UAsyncTickFunctions::ATP_GetLinearVelocityAtPoint(VehiclePrimitiveComponent, PhysicsTransform.GetLocation());

    LinearVelocity = UKismetMathLibrary::InverseTransformDirection(PhysicsTransform, worldPointVelocity) * 0.01f;

#pragma region QueryParams

    FCollisionQueryParams queryparam = FCollisionQueryParams::DefaultQueryParam;
    const FName traceTag("Trace");

    queryparam.TraceTag = traceTag;
    queryparam.AddIgnoredActor(GetOwner());
    queryparam.AddIgnoredComponent(WheelMeshComponent);
    queryparam.bReturnPhysicalMaterial = true;


#pragma endregion



#pragma region Raycast

#pragma region Params

    float length = ExtensionLength;
    End = Start + (-up * length);

#pragma endregion

#pragma region PureSphereTrace

    //GetWorld()->SweepSingleByChannel(Hit, Start, Start + (-up * length), FQuat::Identity, ECC_WorldStatic, SweepShape, queryparam);
    GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, CollisionChannel, SweepShape, queryparam);

    FVector worldDirection = (PhysicsTransform.GetLocation() - Hit.ImpactPoint);
    worldDirection.Normalize();//optional
    ImpactNormal = UKismetMathLibrary::InverseTransformDirection(PhysicsTransform, worldDirection);
    ImpactDot = FMath::Abs(FVector::DotProduct(-up, ImpactNormal));

    //FVector localImpactPoint = UKismetMathLibrary::InverseTransformLocation(VehiclePhysicsBody->GetUnrealWorldTransform(), Hit.ImpactPoint);
    FVector localImpactPoint = UKismetMathLibrary::InverseTransformLocation(vehicleTransform, Hit.ImpactPoint);

    bool safteyCondition = FMath::Abs(ImpactNormal.Y) <= ImpactNormalTolerance && localImpactPoint.Z <= ImpactHeightTolerance;

    bSingleRaycast = false;

    if ( (Hit.bBlockingHit && safteyCondition && bHybridRaycast))
    {
        CurrentLength = (Start - Hit.Location).Size();
    }
    else
    {
        //Pure Linetrace//
        bSingleRaycast = true;
        length = ExtensionLength + WheelRadius + (0.125f * WheelRadius);
        End = Start - (up * length);
        //GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - (up * length), ECC_WorldStatic, queryparam);
        GetWorld()->LineTraceSingleByChannel(Hit, Start, End, CollisionChannel, queryparam);
        CurrentLength = (Start - Hit.Location).Size();

        if (!Hit.bBlockingHit)
        {
            CurrentLength = length;
        }
    }


#pragma endregion



    float compression = 1 - (CurrentLength / length);
    OutCompression = compression;

    float velocity = (compression - PreviousCompression) / AsyncTime;

    float force = (compression * Stiffness) + (Damping * velocity);

    if (Hit.bBlockingHit || bOverrideContact)
    {
        if (!bZeroForceComponent)
        {
            UAsyncTickFunctions::ATP_AddForceAtPosition(VehiclePrimitiveComponent, PhysicsTransform.GetLocation(), force * 100.f * Hit.Normal);
        }

#pragma region TyrePhysics

        if (!bIsIdleLocked)
        {
            float sy_low = -LinearVelocity.Y;
            //try if v.x == 0->sy_low = 0.f;
            float lowLateralForce = sy_low * LowSpeedLateralForceScalar;
            float lateralForce = FMath::Clamp(sy_low * LateralForceScalar, -MaxLateralForce, MaxLateralForce);
            float interp = FMath::GetMappedRangeValueClamped(FVector2D(3.f, 6.f), FVector2D(0.f, 1.f), LinearVelocity.Size());
            float lateralForce_dynamic = FMath::Lerp(lowLateralForce, lateralForce, interp);
            FVector com = UAsyncTickFunctions::ATP_GetCoMPosition(VehiclePrimitiveComponent);
            FVector z1 = FVector(0.f, 0.f, UKismetMathLibrary::InverseTransformLocation(PhysicsTransform, PhysicsTransform.GetLocation()).Z);
            FVector z2 = FVector(0.f, 0.f, UKismetMathLibrary::InverseTransformLocation(PhysicsTransform, com).Z);
            float comDistOffset = FVector::Dist(z1, z2);

            FVector forceLocation = PhysicsTransform.GetLocation() + (up * comDistOffset) + (-up * ContactForceOffsetLocation);

            if (!bZeroForceComponent)
            {
                UAsyncTickFunctions::ATP_AddForceAtPosition(VehiclePrimitiveComponent, forceLocation, lateralForce_dynamic * 100.f * right);
            }

            if (ResistanceForceCoefficient != 0.f)
            {
                float resistanceForce = FMath::Clamp(-LinearVelocity.X * ResistanceForceCoefficient, -MaxResistanceForce, MaxResistanceForce);

                if (!bZeroForceComponent)
                {
                    UAsyncTickFunctions::ATP_AddForceAtPosition(VehiclePrimitiveComponent, forceLocation, resistanceForce * 100.f * fwd);
                }
            }

        }

#pragma endregion

    }


    if (bUseWithDrivetrain)
    {
        if (bDrivenWheel)
        {
            AppliedTorque = transmittedTorque * TorqueReceived;

            float deltaX = (ExtensionLength - CurrentLength) * (1.f / 100.f);
            float ffMax = deltaX * Stiffness * FrictionCoefficient;


            FrRatio = AppliedTorque / ffMax;
            float iFrRatio = 1.f / FrRatio;

            Gamma = FrRatio > 1.f;

            DynamicAngularVelocity = Gamma ? SlippingAngularVelocity : 1.f;
            DynamicLateralForce = Gamma ? SlippingLateralForceScalar : MaxLateralForceScalar;
            float slipInterp = FMath::Lerp(slipBackwards, slipForward, throttle);

            ThrottleSlipGamma = FMath::FInterpTo(ThrottleSlipGamma, throttle, AsyncTime, slipInterp);

            LateralForceScalar = FMath::FInterpTo(LateralForceScalar, DynamicLateralForce, AsyncTime, slipInterp);
            float d = LinearVelocity.X < 0.f && ThrottleSlipGamma > 0.5f ? -1.f : 1.f;
            WheelAngularVelocityMultiplier = FMath::FInterpTo(WheelAngularVelocityMultiplier, d * DynamicAngularVelocity, AsyncTime, slipInterp);

            if (!Hit.bBlockingHit)
            {
                float airGamma = FMath::Lerp(InAirSlowdownSpeed, InAirSpeedUp, throttle);
                float dynSpeed=  (throttle-brakes) * AirRotationMultiplier;
                BurnoutRotation = FMath::Clamp(FMath::FInterpTo(BurnoutRotation, dynSpeed, AsyncTime, airGamma),0.f,10000.f);
            }
            else
            {
                BurnoutRotation = 0.f;
            }

        }
        else
        {
            float dynSpeed = Hit.bBlockingHit ? 0.f : 1.f;

            BurnoutRotation = FMath::FInterpTo(BurnoutRotation, dynSpeed, AsyncTime, InAirSlowdownSpeed);
        }

    }

    PreviousCompression = compression;

}

UStaticMeshComponent* UNsVehicleSuspensionComponent::GetWheelMeshComponent()
{
    return WheelMeshComponent;
}


void UNsVehicleSuspensionComponent::SetStiffness(float inStiffness)
{
    Stiffness = inStiffness;
}

void UNsVehicleSuspensionComponent::SetDamping(float inDamping)
{
    Damping = inDamping;
}

void UNsVehicleSuspensionComponent::SetExtensionLength(float inLength)
{
    ExtensionLength = inLength;
}

void UNsVehicleSuspensionComponent::ConfigureDrivetrain(const bool bInSteeringWheel, const bool bInDrivenWheel, const float InTorqueReceived)
{
    bIsSteeringWheel = bInSteeringWheel;
    bUseWithDrivetrain = bInDrivenWheel;
    bDrivenWheel = bInDrivenWheel;
    bZeroForceComponent = false;
    TorqueReceived = FMath::Clamp(InTorqueReceived, 0.f, 1.f);
}


void UNsVehicleSuspensionComponent::RenderWheelMesh()
{
    /*
#if WITH_EDITOR
    WheelMeshComponent->SetStaticMesh(WheelMesh);
    WheelMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, ExtensionLength * -1));

    if (bRotateWheel)
    {
        WheelMeshComponent->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
    }
#endif
*/
}

void UNsVehicleSuspensionComponent::DetachWheel(const FVector Impulse)
{
    if (bWheelDetached)
    {
        return;
    }

    if (WheelMeshComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle suspension '%s' has no wheel mesh component to detach. Call DetachWheel after Begin Play"), *GetPathNameSafe(this));
        return;
    }

    bWheelDetached = true;
    Hit = FHitResult();
    OutCompression = 0.f;

    const FVector InheritedVelocity = VehiclePrimitiveComponent != nullptr
        ? VehiclePrimitiveComponent->GetPhysicsLinearVelocityAtPoint(WheelMeshComponent->GetComponentLocation())
        : FVector::ZeroVector;

    WheelMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    WheelMeshComponent->SetVisibility(true);
    WheelMeshComponent->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
    WheelMeshComponent->SetSimulatePhysics(true);

    if (!WheelMeshComponent->IsSimulatingPhysics())
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle suspension '%s' cannot simulate the detached wheel. Give the wheel mesh simple collision"), *GetPathNameSafe(this));
        return;
    }

    WheelMeshComponent->SetPhysicsLinearVelocity(InheritedVelocity);
    WheelMeshComponent->AddImpulse(Impulse, NAME_None, true);
}

void UNsVehicleSuspensionComponent::ReattachWheel()
{
    if (!bWheelDetached)
    {
        return;
    }

    const float WheelScale = GetWheelVisualScale();

    WheelMeshComponent->SetSimulatePhysics(false);
    WheelMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WheelMeshComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
    WheelMeshComponent->SetRelativeRotation(bRotateWheel ? FRotator(0.f, 180.f, 0.f) : FRotator::ZeroRotator);
    ApplyWheelVisualScale(WheelScale);
    WheelMeshComponent->SetVisibility(!bHideWheelMesh);

    PreviousCompression = 0.f;
    bWheelDetached = false;
}

bool UNsVehicleSuspensionComponent::IsWheelDetached() const
{
    return bWheelDetached;
}

bool UNsVehicleSuspensionComponent::IsInAir()
{
    return !Hit.bBlockingHit;
}

float UNsVehicleSuspensionComponent::GetCurrentLength()
{
    return CurrentLength;
}


FHitResult UNsVehicleSuspensionComponent::GetHitResult()
{
    return Hit;
}

float UNsVehicleSuspensionComponent::GetCompression()
{
    return OutCompression;
}


bool UNsVehicleSuspensionComponent::IsHandbrakeWheel()
{
    return bHandbrakeWheel;
}


void UNsVehicleSuspensionComponent::SetNewPhysicsBody(UPrimitiveComponent* physicsBody)
{
    if (physicsBody == nullptr)
    {
        return;
    }
    else
    {
        VehiclePrimitiveComponent = physicsBody;
    }

}

FVector UNsVehicleSuspensionComponent::GetOffsetImpactPoint(float zOffset)
{
    return Hit.ImpactPoint + (this->GetForwardVector() * 20.f) + FVector(0.f,0.f,zOffset);
}

ENsVehicleHandbrakeWheelType UNsVehicleSuspensionComponent::GetWheelHandbrake()
{
    return HandbrakeWheelType;
}

bool UNsVehicleSuspensionComponent::IsLeftWheel()
{
    return bRotateWheel;
}

float UNsVehicleSuspensionComponent::GetWheelRadius()
{
    return WheelRadius;
}

float UNsVehicleSuspensionComponent::GetAngularVelocityMultiplier()
{
    return WheelAngularVelocityMultiplier;
}

float UNsVehicleSuspensionComponent::GetBurnoutRotation()
{
    return BurnoutRotation;
}

bool UNsVehicleSuspensionComponent::GetMirrorWheelOnLocalY() const
{
    return bMirrorWheelOnLocalY;
}

float UNsVehicleSuspensionComponent::GetSteeringRotation()
{
    return SteeringRotation;
}


float UNsVehicleSuspensionComponent::GetWheelRelativeOffset()
{
    return WheelRelativeOffset;
}


FRotator UNsVehicleSuspensionComponent::GetWheelLocalRotation()
{
    return WheelLocalRotation;
}

bool UNsVehicleSuspensionComponent::GetIsDrivenWheel()
{
    return bDrivenWheel;
}

float UNsVehicleSuspensionComponent::GetSlipRatio()
{
    return FrRatio;
}

float UNsVehicleSuspensionComponent::GetSlipGamma()
{
    return ThrottleSlipGamma;
}

/*
float UNsVehicleSuspensionComponent::GetWheelAnimLocation()
{
    return wheelAnimZ;
}

FRotator UNsVehicleSuspensionComponent::GetAnimationRotator()
{
    return angularAnimRotator;
}

*/
