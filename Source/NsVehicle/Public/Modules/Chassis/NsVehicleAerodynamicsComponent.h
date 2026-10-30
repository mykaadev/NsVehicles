// Copyright (C) 2025 nulled.softworks. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/NsVehicleModuleComponent.h"
#include "NsVehicleAerodynamicsComponent.generated.h"

class UPrimitiveComponent;

/** Adds the air drag. The standard powertrain uses it. The arcade top speed replaces it in the arcade powertrain */
UCLASS(ClassGroup = (NsVehicle))
class NSVEHICLE_API UNsVehicleAerodynamicsComponent : public UNsVehicleModuleComponent
{
    GENERATED_BODY()

// Functions
public:
    /** Sets the drag coefficient and the frontal area in m^2 */
    UFUNCTION(BlueprintCallable, Category = "NsVehicle|Aerodynamics")
    void SetAerodynamicDrag(float NewCoefficientOfDrag, float NewCrossSectionArea);

    /** Returns the drag force in N from the last physics step */
    UFUNCTION(BlueprintPure, Category = "NsVehicle|Aerodynamics")
    float GetDragForce() const;

    /** Adds the drag force against the forward movement. The linear velocity is in m/s */
    void ApplyDrag(UPrimitiveComponent* PhysicsBody, const FVector& Forward, float LinearVelocity, float VelocityDirection);

// Variables
protected:
    /** The drag coefficient. Most road cars are between 0.25 and 0.4 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Aerodynamics")
    float CoefficientOfDrag = 0.3f;

    /** The frontal area in m^2 that the drag acts on. Higher values give more drag */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Aerodynamics")
    float CrossSectionArea = 1.5872454f;

    /** The air density in kg/m^3. The default is the density at sea level */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NsVehicle|Aerodynamics")
    float AirDensity = 1.225f;

private:
    /** The drag force in N from the last physics step */
    float Drag = 0.f;
};
