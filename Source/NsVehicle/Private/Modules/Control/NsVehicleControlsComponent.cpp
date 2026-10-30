// Copyright (C) 2025 nulled.softworks. All rights reserved.

#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Modules/Chassis/NsVehicleSteeringComponent.h"
#include "Modules/Powertrain/NsVehicleTransmissionComponent.h"
#include "Vehicle/NsVehicle.h"

UNsVehicleControlsComponent::UNsVehicleControlsComponent()
{
    SetIsReplicatedByDefault(true);
}

void UNsVehicleControlsComponent::SetThrottleInput(const float Value)
{
    const bool bUpshifting = GetVehicle()->GetTransmission()->IsGearboxUpshifting();
    Throttle = (bUpshifting ? 0.f : Value) * GetControllableValue();
    ServerSetThrottleInput(Throttle);
}

void UNsVehicleControlsComponent::ServerSetThrottleInput_Implementation(const float Value)
{
    Throttle = Value * GetControllableValue();
}

bool UNsVehicleControlsComponent::ServerSetThrottleInput_Validate(const float Value)
{
    return FMath::Abs(Value) <= 1;
}

void UNsVehicleControlsComponent::SetBrakeInput(const float Value)
{
    Brakes = Value * GetControllableValue();
    ServerSetBrakeInput(Brakes);
}

void UNsVehicleControlsComponent::ServerSetBrakeInput_Implementation(const float Value)
{
    Brakes = Value * GetControllableValue();
}

bool UNsVehicleControlsComponent::ServerSetBrakeInput_Validate(const float Value)
{
    return FMath::Abs(Value) <= 1;
}

void UNsVehicleControlsComponent::SetSteeringInput(const float Value)
{
    const ANsVehicle* const Vehicle = GetVehicle();
    SteeringInput = Vehicle->GetSteering()->ClampSteeringInput(Value, Vehicle->GetSpeedKPH()) * GetControllableValue();
    ServerSetSteeringInput(SteeringInput);
}

void UNsVehicleControlsComponent::ServerSetSteeringInput_Implementation(const float Value)
{
    SteeringInput = Value * GetControllableValue();
}

bool UNsVehicleControlsComponent::ServerSetSteeringInput_Validate(const float Value)
{
    return FMath::Abs(Value) <= 1;
}

void UNsVehicleControlsComponent::ApplyHandbrakeInput(const bool bValue)
{
    bHandbrakeInput = bValue;
    ServerApplyHandbrakeInput(bHandbrakeInput);
}

void UNsVehicleControlsComponent::ServerApplyHandbrakeInput_Implementation(const bool bValue)
{
    bHandbrakeInput = bValue;
}

bool UNsVehicleControlsComponent::ServerApplyHandbrakeInput_Validate(const bool bValue)
{
    return true;
}

void UNsVehicleControlsComponent::SetControllable(const bool bNewInputEnabled)
{
    bInputEnabled = bNewInputEnabled;
    ServerSetControllable(bInputEnabled);
}

void UNsVehicleControlsComponent::ServerSetControllable_Implementation(const bool bNewInputEnabled)
{
    bInputEnabled = bNewInputEnabled;
}

bool UNsVehicleControlsComponent::ServerSetControllable_Validate(const bool bNewInputEnabled)
{
    return true;
}

void UNsVehicleControlsComponent::SetAirPitchInput(const float Value)
{
    PitchInput = Value * GetControllableValue();
    ServerSetAirPitchInput(Value);
}

void UNsVehicleControlsComponent::ServerSetAirPitchInput_Implementation(const float Value)
{
    PitchInput = Value * GetControllableValue();
}

bool UNsVehicleControlsComponent::ServerSetAirPitchInput_Validate(const float Value)
{
    return FMath::Abs(Value) <= 1;
}

void UNsVehicleControlsComponent::SetAirYawInput(const float Value)
{
    YawInput = Value * GetControllableValue();
    ServerSetAirYawInput(Value);
}

void UNsVehicleControlsComponent::ServerSetAirYawInput_Implementation(const float Value)
{
    YawInput = Value * GetControllableValue();
}

bool UNsVehicleControlsComponent::ServerSetAirYawInput_Validate(const float Value)
{
    return FMath::Abs(Value) <= 1;
}

void UNsVehicleControlsComponent::SetAirRollInput(const float Value)
{
    RollInput = Value * GetControllableValue();
    ServerSetAirRollInput(Value);
}

void UNsVehicleControlsComponent::ServerSetAirRollInput_Implementation(const float Value)
{
    RollInput = Value * GetControllableValue();
}

bool UNsVehicleControlsComponent::ServerSetAirRollInput_Validate(const float Value)
{
    return FMath::Abs(Value) <= 1;
}

void UNsVehicleControlsComponent::SetExtReplicationInput(const int32 Index, const float Value)
{
    if (Index < 0 || Index >= ExtReplicationInputCount)
    {
        UE_LOG(LogTemp, Error, TEXT("NsVehicle '%s' got custom input index %d. Use 0 to %d"), *GetPathNameSafe(GetOwner()), Index, ExtReplicationInputCount - 1);
        return;
    }

    ExtReplicationInputs[Index] = Value;
    ServerSetExtReplicationInput(Index, Value);
}

void UNsVehicleControlsComponent::ServerSetExtReplicationInput_Implementation(const int32 Index, const float Value)
{
    ExtReplicationInputs[Index] = Value;
}

bool UNsVehicleControlsComponent::ServerSetExtReplicationInput_Validate(const int32 Index, const float Value)
{
    return Index >= 0 && Index < ExtReplicationInputCount && FMath::Abs(Value) <= 1;
}

float UNsVehicleControlsComponent::GetExtReplicationInput(const int32 Index) const
{
    return Index >= 0 && Index < ExtReplicationInputCount ? ExtReplicationInputs[Index] : 0.f;
}

float UNsVehicleControlsComponent::GetThrottleInput() const
{
    return Throttle;
}

float UNsVehicleControlsComponent::GetBrakesInput() const
{
    return Brakes;
}

float UNsVehicleControlsComponent::GetSteeringInput() const
{
    return SteeringInput;
}

bool UNsVehicleControlsComponent::GetHandbrakeInput() const
{
    return bHandbrakeInput;
}

bool UNsVehicleControlsComponent::IsControllable() const
{
    return bInputEnabled;
}

float UNsVehicleControlsComponent::GetAirPitchInput() const
{
    return PitchInput;
}

float UNsVehicleControlsComponent::GetAirYawInput() const
{
    return YawInput;
}

float UNsVehicleControlsComponent::GetAirRollInput() const
{
    return RollInput;
}

void UNsVehicleControlsComponent::SetReplicatedInputs(const float NewThrottle, const float NewBrakes, const float NewSteeringInput)
{
    Throttle = NewThrottle;
    Brakes = NewBrakes;
    SteeringInput = NewSteeringInput;
}

float UNsVehicleControlsComponent::GetControllableValue() const
{
    return bInputEnabled ? 1.f : 0.f;
}
