# NsVehicle User Guide

## Purpose

NsVehicle is a runtime plugin for physics vehicles: cars, karts, trucks, bikes, and hover vehicles. One physics body moves the vehicle. Suspension components push it up from the ground, and module components add the drive, brake, steering, and other forces.

NsVehicle is not a Chaos Vehicle replacement with a full tyre model. It does not simulate a clutch, a skeletal mesh vehicle, or wheel collision with the world. The wheels are traces.

## Setup

1. Put the plugin in the `Plugins` folder of the project, or in a folder that `AdditionalPluginDirectories` lists.
2. Add `"NsVehicle"` to the `Plugins` list of the `.uproject` file.
3. For C++, add `"NsVehicle"` and `"AsyncTickPhysics"` to `PublicDependencyModuleNames` in your `.Build.cs` file. A class that derives from `ANsVehicle` needs both, because `ANsVehicle` derives from `AAsyncTickPawn`.
4. For replication, enable **Project Settings > Physics > Tick Physics Async**. Use a step of 0.008333 (1/120) or a little less.

## Quick Start

### Blueprint

1. Make a Blueprint class with `NsVehicle` as the parent.
2. Add a Static Mesh component as the root. Set a mesh with simple collision. Enable **Simulate Physics**.
3. Add four `NsVehicleSuspensionComponent` components under the root. Put them where the wheel tops are.
4. On each suspension component, set **Wheel Mesh**. On the two front wheels, enable **Is Steering Wheel**.
5. In the input events, call `Get Controls`, then `Set Throttle Input`, `Set Brake Input`, and `Set Steering Input`.

The vehicle makes all modules by default. With no other setup, it drives with the arcade powertrain.

### C++

```cpp
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Vehicle/NsVehicle.h"

void AMyPlayerController::DriveForward(ANsVehicle* Vehicle, const float Input)
{
    Vehicle->GetControls()->SetThrottleInput(Input);
}
```

The Sandbox project has a full example: `ASandboxTestVehicle` in `Source/Sandbox/SandboxTestVehicle.h`, and `BP_TestVehicle` in `/Game/Vehicles/TestVehicle`. It drives forward and back, and steers left and right. While you drive it, the console commands `DetachTestWheel 0` and `ReattachTestWheel 0` take the front left wheel off and put it back. Use 0 to 3 for the four wheels.

## Common Tasks

### Find a setting

Each system is a module component on the vehicle. Select the module in the Components panel to see its settings. In Blueprint or C++, call the module getter on the vehicle, for example `GetEngine`, `GetBrakes`, or `GetSteering`.

| Module | Getter | What it does |
|---|---|---|
| Controls | `GetControls` | Holds the driver input and sends it to the server |
| Engine | `GetEngine` | Makes the torque and the RPM. Selects arcade or standard |
| Transmission | `GetTransmission` | Owns the gears, the differential, and the automatic gearbox |
| Drivetrain | `GetDrivetrain` | Removes power when the driven wheels slip |
| Brakes | `GetBrakes` | Owns the brakes, the reverse drive, and the handbrake |
| Steering | `GetSteering` | Limits the steering by speed and turns the steering wheels |
| BodyDynamics | `GetBodyDynamics` | Adds the body pitch and the bike lean |
| Aerodynamics | `GetAerodynamics` | Adds the air drag in the standard powertrain |
| AirControl | `GetAirControl` | Turns the vehicle in the air |
| AntiGravity | `GetAntiGravity` | Pulls the vehicle to the surface under it |
| IdleLock | `GetIdleLock` | Holds the vehicle in place when it stops |
| Network | `GetNetwork` | Replicates the vehicle state |
| Animation | `GetAnimation` | Spins the wheels for the take recorder |

### Make a wheel mesh

Center the wheel mesh at its origin. Put the axle on the Y axis. The suspension uses the mesh height to get the wheel radius.

### Show the wheels in the editor

1. Add a Static Mesh component under each suspension component. Set the same wheel mesh.
2. On the suspension component, enable **Inherit Mesh From Component**.

With this setup, do not attach other components to the suspension component. Attach them to the root.

### Set up RWD, FWD, or AWD

1. On the Engine module, set **Powertrain Type** to Standard.
2. On the Drivetrain module, enable **Use Drivetrain**.
3. On each suspension component, enable **Use With Drivetrain**.
4. On each driven wheel, enable **Driven Wheel**. Set **Torque Received**, for example 0.5 on each rear wheel for RWD.

The number of driven wheels sets the drivetrain. 6WD and 8WD also work.

### Detach a wheel

Call `DetachWheel` on a suspension component. The impulse is a velocity change in cm/s.

```cpp
FrontLeftWheel->DetachWheel(FVector(0.f, -600.f, 400.f));
```

The wheel mesh becomes a free physics body, and the suspension stops all its forces. The wheel mesh must have simple collision, or the wheel does not fall. The vehicle ignores detached wheels when it counts airborne wheels.

To put the wheel back, call `ReattachWheel` on the same suspension component. The wheel mesh stops its physics, goes back under the suspension component, and the suspension forces start again.

```cpp
FrontLeftWheel->ReattachWheel();
```

### Set up the handbrake

1. On the Brakes module, set **Handbrake Strength**.
2. On each suspension component that the handbrake holds, enable **Handbrake Wheel**. Set **Handbrake Wheel Type** to Front or Rear.
3. Call `Get Controls`, then `Apply Handbrake Input`.

For a drift, lower **Lateral Force Scalar** on the rear wheels while the handbrake is on.

### Animate the interior

Add one of these components under the vehicle. The vehicle updates it every frame.

| Component | Value | Default input range | Default angle range |
|---|---|---|---|
| `NsVehicleTachometerComponent` | Engine RPM | 0 to 7000 | 0 to 270, roll |
| `NsVehicleSpeedometerComponent` | Speed in KPH | 0 to 200 | 0 to 270, roll |
| `NsVehicleSteeringWheelComponent` | Steering input | -1 to 1 | -450 to 450, roll |
| `NsVehicleThrottlePedalComponent` | Throttle input | 0 to 1 | 0 to 20, pitch |
| `NsVehicleBrakePedalComponent` | Brake input | 0 to 1 | 0 to 20, pitch |
| `NsVehicleHandbrakeLeverComponent` | Handbrake input | 0 to 1 | 0 to 30, pitch |

Each one has **Input Range**, **Output Range**, **Rotation Axis**, **Use Smoothing**, **Smoothing Speed**, and **Invert**.

### Move a part, for example a forklift arm

Add an `NsVehicleArticulatedComponent`. Set **Axis**, **Min**, **Max**, and **Movement Speed**. Call `Set Input` with a value from -1 to 1.

### Follow a spline with AI

1. Add an `NsVehicleAI` component to the vehicle.
2. Call `Set Target Spline` with a spline component.
3. In the tick, call `Update`. Send the output to `Set Steering Input`.

Tune **Kp** first, then **Kd** and **Kd2**. Use **Ki** to remove a constant offset.

### Ignore some surfaces

1. In **Project Settings > Collision**, add a trace channel with the default response Block.
2. On each suspension component, set **Collision Channel** to the new channel.
3. On the objects that the wheels must ignore, set the response for the new channel to Ignore.

### Change the handling with the center of mass

The collision shape of the root mesh changes the handling. To change it:

- On the root mesh, set **Center Of Mass Offset**. A negative X makes the vehicle drift more. A positive X makes it more stable. A negative Z stops roll over.
- Or, use a hidden collision mesh as the root, and a visible mesh with no collision as a child. Then many vehicles can share one handling setup.

### Replicate the vehicle

Recommended setup:

1. On the actor, enable **Replicate Movement**.
2. On the Network module, set **Replication Method** to Data Only.
3. On each suspension component, disable **Component Replicates**.
4. Optional: enable the Network Prediction plugin. If rotation desync occurs, set **Angle Lerp** to 0.05.

High update rate setup: set **Replication Method** to Full and disable **Replicate Movement**.

For a custom movement system, use Data Only or None.

### Send custom input to the server

Call `Get Controls`, then `Set Ext Replication Input` with an index from 0 to 5 and a value from -1 to 1. Read it with `Get Ext Replication Input`, for example in Event Async Tick.

### Record with the take recorder

1. On the Animation module, enable **Enable Animation Mode**. Do this only to record.
2. In the take recorder settings, disable **Reduce Keys**.
3. In the take recorder list, disable all wheel components.

## Rules

- Do not scale the vehicle actor. Use a larger mesh and keep the actor scale at 1.
- Make the root component a primitive component that simulates physics.
- Give every wheel mesh simple collision if you detach wheels.
- Keep **Enable Animation Mode** off for gameplay.
- Call `InitializeLeanSteer` and `InitializeAntiGravity` one time in Begin Play, before you use lean steer or anti gravity.
- Read `GetSideSlipAngle` in Event Async Tick only, and `GetSideSlipAngleGT` in Event Tick only.

## Settings

The tables give the default and the effect of each setting. The tooltips in the editor give the same text.

### Vehicle

| Setting | Default | Effect |
|---|---|---|
| In Air Wheel Count | 2 | Wheels off the ground that make the vehicle airborne. An airborne vehicle gets no drive force |
| Auto Wake Rigid Bodies | true | Wakes the physics body every frame. Turn off for many replicated vehicles, then call `RTWakeRigidBodies` |

### Engine

| Setting | Default | Effect |
|---|---|---|
| Powertrain Type | Arcade | Arcade: constant torque, no gearbox. Standard: torque from RPM and gears |
| Engine Torque | 500 | Peak torque in Nm |
| Max Speed | 318 | Arcade top speed in KPH |
| Max RPM | 7000 | Standard maximum RPM |
| Rpm Limit Torque Floor | 0.06 | Torque scale that stays at the RPM limit |
| RPM Multiplier | 1 | RPM scale. Other values can be unstable |
| Torque Implementation | Constant | Constant torque, or torque from a curve |
| Engine Torque Curve | none | Normalized torque curve. X is RPM |
| Engine Brake Coefficient | 0.35 | Off-throttle deceleration |

### Transmission

| Setting | Default | Effect |
|---|---|---|
| Transmission Efficiency | 0.7 | Scales the acceleration. No effect on top speed |
| Shift Delay | 0.2 | Shift time in seconds |
| Differential Ratio | 3.05 | Standard final drive ratio |
| Gear Ratios | empty | One ratio for each gear. Empty uses 3.5 |
| Wheel Radius | 0.28 | Drive wheel radius in meters |
| Automatic | true | Automatic gearbox |
| Use Speed Based Shifting | false | Shifts from speed, not RPM |
| Speed Shifting Ratios | empty | Downshift (X) and upshift (Y) speed for each gear |
| Upshift Ratio | 0.85 | RPM ratio for an upshift |
| Downshift Ratio | 0.3 | RPM ratio for a downshift |
| Lock Shift | false | Stops all shifts |

### Drivetrain

| Setting | Default | Effect |
|---|---|---|
| Use Drivetrain | false | Enables the wheel slip power loss |
| Effective Power | 1 | Part of the power that gets to the wheels |
| Slip Power Reduction | 1 | Power loss when the wheels slip |
| Slip Initiate Speed | 15 | How fast the slip starts |
| Slip Restore Speed | 0.5 | How fast the traction comes back |
| In Air Speed Up | 10 | RPM rise in the air |
| In Air Slowdown Speed | 2 | RPM fall in the air |

### Brakes

| Setting | Default | Effect |
|---|---|---|
| Brake Force | 15000 | Brake force in N. Also the reverse force |
| Use Brake As Reverse | true | The brake input drives backward |
| Max Speed In Reverse | 60 | Reverse top speed in KPH |
| Handbrake Strength | 7500 | Handbrake force in N |

### Steering

| Setting | Default | Effect |
|---|---|---|
| Drift Torque | 0 | Helper yaw torque that follows the steering |
| Steering Input Clamp | No Clamp | How the maximum steering changes with speed |
| Constant Steering Clamp | 1 | Maximum steering at all speeds |
| Linear Steering Input Clamp | 0 | Speed range to maximum steering range |
| Curve Steering Input Clamp | none | Maximum steering as a curve of speed |
| Use Sensitivity Clamp | false | Slower steering response at high speed |
| Steering Sensitivity | 5 | Steering response speed |
| Steering Sensitivity Clamp | 0 | Speed range to response range |

### Body Dynamics

| Setting | Default | Effect |
|---|---|---|
| Dynamic Pitch Moment Multiplier | 0 | Pitch under acceleration |
| Dynamic Brake Moment Multiplier | 0 | Pitch under braking |
| Use Steer Lean | false | Bike lean in a turn |
| Steer Lean Coefficient | 4.85 | Lean force |
| Steer Lean Min Speed | 5 | Minimum speed in KPH for lean |
| Steer Lean Mapping | 0.9, 0.99 | Ground normal range for the lean strength |

### Aerodynamics, Air Control, Anti Gravity, Idle Lock

| Module | Setting | Default | Effect |
|---|---|---|---|
| Aerodynamics | Coefficient Of Drag | 0.3 | Drag coefficient |
| Aerodynamics | Cross Section Area | 1.587 | Frontal area in m^2 |
| Aerodynamics | Air Density | 1.225 | Air density in kg/m^3 |
| Air Control | Pitch, Yaw, Roll Strength | 2 | Air control torque on each axis |
| Anti Gravity | Anti Gravity Enabled | false | Enables the system |
| Anti Gravity | Gravity Strength | 980 | Gravity in cm/s^2 |
| Anti Gravity | Z, X, Y Strength | 1.2, 1.02, 1.02 | Pull to the surface, roll back stop, side slip stop |
| Anti Gravity | Off Contact Threshold | 1 | Seconds the system stays on with no contact |
| Anti Gravity | Added Mass | 0 | Extra mass in kg |
| Idle Lock | Use Idle Lock | true | Holds the stopped vehicle |
| Idle Lock | Idle Lock Time | 1.5 | Seconds before the hold |
| Idle Lock | Idle Lock Speed Threshold | 10 | Speed in KPH below which the timer starts |
| Idle Lock | Idle Lock Stiffness | 3000 | Hold force on slopes |

### Network and Animation

| Module | Setting | Default | Effect |
|---|---|---|---|
| Network | Replication Method | None | Full, Data Only, or None |
| Network | Use Server Fast Sync | true | Clients copy the server transform at an interval (Full only) |
| Network | Server Fast Sync Latency | 500 | Game frames between syncs |
| Animation | Enable Animation Mode | false | Take recorder wheel spin |
| Animation | Animation Global Wheel Speed Multiplier | 1 | Wheel spin scale in animation mode |
| Animation | Anim Delta Time | 1/60 | Fixed frame time for the animation |

The suspension component settings are in its tooltips. The main ones are **Stiffness** (22500 N/m), **Damping** (1250), **Extension Length** (50 cm), **Lateral Force Scalar** (2000), and **Max Steering Angle** (40 degrees).

## Debug

- On a suspension component, enable **Debug** to draw the trace. The color goes from green to red with the compression.
- The log category is `LogTemp`. Search for `NsVehicle`. Setup errors log as `Error`. Setup risks log as `Warning`, for example a short **Extension Length** or a wheel radius mismatch.
- `ASandboxTestVehicle` has **Log Drive State**. It logs the speed, the inputs, and the wheel state each second. In a game session, run `set SandboxTestVehicle bLogDriveState true`.

## Limits

- The vehicle jumps after a stop: lower **Idle Lock Stiffness** or disable the idle lock.
- The vehicle does not move in multiplayer: disable the idle lock.
- The vehicle cannot be scaled.
- Wheel detach and reattach are local only. They do not replicate.
- The Blueprint Async Tick event runs two times in each physics step. The parent pawn runs it, and the vehicle runs it again. This matches the original behavior.
- More than three replicated vehicles on a server can crash Chaos. Disable **Auto Wake Rigid Bodies** and call `RTWakeRigidBodies` from the input events.
- There is no clutch simulation.
