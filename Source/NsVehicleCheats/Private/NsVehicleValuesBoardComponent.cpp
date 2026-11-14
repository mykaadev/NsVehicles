// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "NsVehicleValuesBoardComponent.h"

#include "NsVehicleCheats.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NsVehicleCheatsSettings.h"
#include "NsVehicleValuesWidget.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleValuesBoardComponent::UNsVehicleValuesBoardComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // Behavior, not look: the board is a world object that never collides, and it turns to the camera by itself, not with the vehicle
    SetWidgetSpace(EWidgetSpace::World);
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetCanEverAffectNavigation(false);
    SetGenerateOverlapEvents(false);
    SetUsingAbsoluteRotation(true);
    SetUsingAbsoluteScale(true);
}

void UNsVehicleValuesBoardComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (UNsVehicleValuesWidget* const ValuesWidget = Cast<UNsVehicleValuesWidget>(GetUserWidgetObject()))
    {
        if (ValuesWidget->GetVehicle() != Vehicle.Get())
        {
            ValuesWidget->SetVehicle(Vehicle.Get());
        }
    }
    FaceCamera();
}

void UNsVehicleValuesBoardComponent::SetupBoard(ANsVehicle* const InVehicle)
{
    Vehicle = InVehicle;
    const UNsVehicleCheatsSettings* const Settings = GetDefault<UNsVehicleCheatsSettings>();
    const UPrimitiveComponent* const Body = InVehicle != nullptr ? Cast<UPrimitiveComponent>(InVehicle->GetRootComponent()) : nullptr;
    if (Body == nullptr)
    {
        UE_LOG(LogNsVehicleCheats, Error, TEXT("The NsVehicle values board needs a vehicle with a primitive root"));
        return;
    }

    TSubclassOf<UNsVehicleValuesWidget> BoardClass = Settings->ValuesWidgetClass.LoadSynchronous();
    if (BoardClass == nullptr)
    {
        BoardClass = UNsVehicleValuesWidget::StaticClass();
    }
    SetWidgetClass(BoardClass);
    const FIntPoint BoardSize(FMath::Max(Settings->BoardDrawSize.X, 16), FMath::Max(Settings->BoardDrawSize.Y, 16));
    SetDrawSize(FVector2D(BoardSize));

    // The board is as wide as the vehicle times the width scale, so it fits a toy car and a truck. It stands on the top of the rear, with its bottom edge there
    const FBoxSphereBounds LocalBounds = Body->CalcBounds(FTransform::Identity);
    const float BoardWidth = FMath::Max(LocalBounds.BoxExtent.Y * 2.f * Settings->BoardWidthScale, 1.f);
    const float Scale = BoardWidth / BoardSize.X;
    const float BoardHeight = BoardSize.Y * Scale;
    const FVector RearTop(LocalBounds.Origin.X - LocalBounds.BoxExtent.X, LocalBounds.Origin.Y, LocalBounds.Origin.Z + LocalBounds.BoxExtent.Z);
    SetRelativeLocation(RearTop + FVector(0.f, 0.f, BoardHeight * 0.5f) + Settings->BoardOffset);
    SetWorldScale3D(FVector(Scale));
}

void UNsVehicleValuesBoardComponent::FaceCamera()
{
    const UWorld* const World = GetWorld();
    const APlayerController* const Player = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    if (Player == nullptr || Player->PlayerCameraManager == nullptr)
    {
        return;
    }

    // The widget shows its face along the X axis of the component, so the X axis points at the camera
    const FVector ToCamera = Player->PlayerCameraManager->GetCameraLocation() - GetComponentLocation();
    if (!ToCamera.IsNearlyZero())
    {
        SetWorldRotation(ToCamera.Rotation());
    }
}
