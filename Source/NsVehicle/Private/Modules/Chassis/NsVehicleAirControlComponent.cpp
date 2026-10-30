// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Chassis/NsVehicleAirControlComponent.h"
#include "AsyncTickFunctions.h"
#include "Wheels/NsVehicleSuspensionComponent.h"

void UNsVehicleAirControlComponent::ApplyAirControl(UPrimitiveComponent* PhysicsBody, const FVector& Forward, const FVector& Right, const FVector& Up, const TArray<UNsVehicleSuspensionComponent*>& Suspensions, const int32 InAirWheelCount, const float PitchInput, const float YawInput, const float RollInput) const
{
    const FVector Torque = (Up * YawStrength * YawInput) + (Right * PitchStrength * PitchInput) + (Forward * RollStrength * RollInput);

    if (Suspensions.Num() > 0)
    {
        int32 AirborneCount = 0;
        for (UNsVehicleSuspensionComponent* Suspension : Suspensions)
        {
            if (Suspension->IsInAir() && !Suspension->IsWheelDetached())
            {
                AirborneCount++;
            }
        }

        if (InAirWheelCount > AirborneCount)
        {
            return;
        }
    }

    UAsyncTickFunctions::ATP_AddTorque(PhysicsBody, Torque, true);
}
