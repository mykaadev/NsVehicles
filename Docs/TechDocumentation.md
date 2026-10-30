# NsVehicle Technical Documentation

## Architecture

### Modules

| Module | Type | Content |
|---|---|---|
| `NsVehicle` | Runtime | The vehicle pawn, the vehicle modules, the wheels, the interior parts, the articulated part, and the AI steering |
| `AsyncTickPhysics` | Runtime, third party | `AAsyncTickPawn` and `UAsyncTickActorComponent`, plus `UAsyncTickFunctions` for force and velocity calls on the physics thread |

### Folders

| Folder | Classes |
|---|---|
| `Vehicle/` | `ANsVehicle`, and `NsVehicleTypes.h` with the shared enums and structs |
| `Modules/` | `UNsVehicleModuleComponent`, the base class of all modules |
| `Modules/Powertrain/` | `UNsVehicleEngineComponent`, `UNsVehicleTransmissionComponent`, `UNsVehicleDrivetrainComponent` |
| `Modules/Chassis/` | `UNsVehicleBrakeComponent`, `UNsVehicleSteeringComponent`, `UNsVehicleBodyDynamicsComponent`, `UNsVehicleAerodynamicsComponent`, `UNsVehicleAirControlComponent`, `UNsVehicleAntiGravityComponent`, `UNsVehicleIdleLockComponent` |
| `Modules/Control/` | `UNsVehicleControlsComponent` |
| `Modules/Network/` | `UNsVehicleNetworkComponent` |
| `Modules/Animation/` | `UNsVehicleAnimationComponent` |
| `Wheels/` | `UNsVehicleSuspensionComponent`, `UNsVehicleStaticWheel`, `UNsVehicleWheel` |
| `Interior/` | `UNsVehicleInteriorComponent` (abstract) and its six subclasses |
| `Articulation/` | `UNsVehicleArticulatedComponent` |
| `AI/` | `UNsVehicleAI` |

The `Public` and `Private` folders use the same layout.

### Main classes

- `ANsVehicle` owns the physics state: the physics transform, the local velocity, the speed, the acceleration, the yaw rate, and the airborne state. It makes one of each module in the constructor and calls them in a fixed order. It holds no tuning values, except `InAirWheelCount` and `bAutoWakeRigidBodies`.
- A module derives from `UNsVehicleModuleComponent`. It does not tick. It caches the owner vehicle in `OnRegister`. It holds the settings and the state of one system. Its physics functions take the values they need as arguments, or read them from the vehicle getters.
- `UNsVehicleSuspensionComponent` traces the ground, applies the spring, tyre, and drivetrain forces at its location, and places and spins its wheel mesh.

### Physics step order

`ANsVehicle::NativeAsyncTick` runs on the physics thread when async physics is on:

1. `AAsyncTickPawn::NativeAsyncTick`, which runs the Blueprint `AsyncTick` event.
2. `UpdateGroundContact`: Drivetrain averages the wheel slip, and the vehicle sets `bInAir`.
3. `UpdateAngularMotion`: yaw rate and yaw acceleration.
4. `IdleLock->UpdateIdleLock`.
5. The replication branch from `Network->GetReplicationMethod()`. It runs `UpdatePhysicsCore`, the Blueprint `AsyncTick` event, and the server or client state sync.
6. `UpdateWheelPhysics`: each suspension component gets the wheel torque and the inputs, then runs its trace and forces.

`UpdatePhysicsCore`:

1. Reads the physics transform.
2. Standard powertrain: Engine torque, Transmission gear ratio and automatic shift, Brakes force, Drivetrain effective power, drive force, BodyDynamics pitch, Aerodynamics drag, Engine RPM, Transmission shift timers. If the torque curve is missing, the step stops here.
3. Arcade powertrain: Engine arcade torque, Brakes arcade force, drive force, BodyDynamics pitch.
4. Steering drift torque, if the vehicle is on the ground.
5. `UpdateMotionState`: local velocity, speed, acceleration.
6. Brakes handbrake, AirControl, AntiGravity, BodyDynamics lean steer.

### Game thread order

`ANsVehicle::Tick`:

1. Wake the rigid bodies.
2. `UpdateAttachedComponents`: Steering sends the input to the steering wheels. Each suspension component places, steers, and spins its wheel. The interior and articulated parts update.
3. `Network->UpdateFastSync`.
4. `AntiGravity->UpdateGameThread`.
5. The game thread velocity for `GetSideSlipAngleGT`.
6. `Animation->UpdateLocationVelocity`, and the wheel spin in animation mode.

### Data flow

The modules do not hold pointers to each other. They read other modules through the vehicle getters, for example `GetVehicle()->GetTransmission()`. The vehicle passes the physics body, the direction vectors, and the inputs as arguments. The physics thread and the game thread share the module state with no lock, as the original single class did.

## Lifetime

- The vehicle makes the modules with `CreateDefaultSubobject` in its constructor. A Blueprint cannot remove them.
- `BeginPlay` finds the suspension, interior, and articulated components with `GetComponents`. Components added after `BeginPlay` are not updated.
- `UNsVehicleAnimationComponent::InitializeAnimationSystem` replaces the suspension list with the tagged children of the root.
- `AAsyncTickPawn` adds the pawn to the async tick manager in `BeginPlay` and removes it in `EndPlay`.
- During pause, the physics step does not run, and the vehicle applies no force.
- `DetachWheel` moves the wheel mesh out of the attachment tree. The wheel mesh stays owned by the vehicle actor, so it is destroyed with the vehicle. `ReattachWheel` stops its physics, turns its collision off, and attaches it to the suspension component again.

## Extension Points

- Subclass `ANsVehicle` in C++ or Blueprint. Set the module defaults in the constructor with the module setters, for example `GetEngine()->SetArcadeMaxSpeed(60.f)`.
- Module replacement is not supported yet. `ANsVehicle` has no `FObjectInitializer` constructor, and the module physics functions are not virtual. Add both when a project needs a custom module.
- Subclass `UNsVehicleInteriorComponent` and override `GetDriveValue` for a new interior part.
- Use the Blueprint `AsyncTick` event for project physics on the physics thread.
- Use `SetExtReplicationInput` for project input that must reach the server.

## Dependencies

| Dependency | Kind | Why |
|---|---|---|
| `Core`, `CoreUObject`, `Engine` | Public, engine | Base types and components |
| `AsyncTickPhysics` | Public, bundled | `ANsVehicle` derives from `AAsyncTickPawn` |
| `Slate`, `SlateCore` | Private, engine | Kept from the original module. No code uses them now |
| `Chaos` | Private, in `AsyncTickPhysics` | Physics thread callbacks |

The plugin has no optional dependencies.

## Performance

- The vehicle ticks on the game thread and on the physics thread. The modules do not tick.
- Each suspension component does one sphere sweep, and sometimes one line trace, each physics step.
- The physics step has no heap allocation. `GetSuspension` returns a copy of the array, so call it outside hot loops.
- `bAutoWakeRigidBodies` wakes the body every frame. Turn it off for many vehicles.

## Networking

- `UNsVehicleControlsComponent` sends the input to the server with reliable server RPCs. It replicates by default.
- `UNsVehicleNetworkComponent` replicates `FNsVehicleServerState` and the server velocities. It replicates by default.
- Full: the server simulates and sends the movement and the data. The clients apply the server velocities.
- Data Only: the server sends the data. Another system moves the clients, for example Replicate Movement or Network Prediction. The clients also simulate.
- None: no replication. This is the default.
- When the method is not None, `BeginPlay` makes the suspension components replicate, so that the steering angle replicates.
- `DetachWheel` and `ReattachWheel` are local only.

## Testing

There are no automated tests yet.

Manual test path in the Sandbox project:

1. Build `SandboxEditor`.
2. Open `/Game/Maps/Playground` and place `BP_TestVehicle`, or use the `summon /Game/Vehicles/TestVehicle/BP_TestVehicle.BP_TestVehicle_C` console command in a game session.
3. Run `set SandboxTestVehicle bLogDriveState true`.
4. Make sure that the speed goes up and down, and the steering angle changes sign.
5. Run `DetachTestWheel 0`, then `ReattachTestWheel 0`. Make sure that the front left wheel shows `detached 1`, then `detached 0`.

For a test with no input, set **Auto Detach Time** and **Auto Reattach Delay** on the car instance.

Headless form of the same test:

```
UnrealEditor.exe Sandbox.uproject /Game/Maps/Playground -game -nullrhi -unattended -ExecCmds="summon /Game/Vehicles/TestVehicle/BP_TestVehicle.BP_TestVehicle_C, set SandboxTestVehicle bLogDriveState true"
```

Then read `Saved/Logs/Sandbox.log` for `SandboxTestVehicle t=`.

## Change Rules

- Keep the physics step order. The tuning of current vehicles depends on it.
- Keep the force formulas. Some keep old behavior on purpose, with a code comment: the handbrake force applies one time for each wheel after the first handbrake wheel, the RPM formula uses `60 / 2 * PI`, and the Blueprint `AsyncTick` event runs two times.
- Keep the module subobject names (`Engine`, `Brakes`, and the others). Blueprint overrides of module settings use these names.
- Keep `UNsVehicleSuspensionComponent` property names. Wheel setups in Blueprints use them.
- Update this file and `UserGuide.md` in the same change as a feature change.

### Migration from the single class version

The module split is a breaking change for C++ and Blueprint users of `ANsVehicle`:

| Old | New |
|---|---|
| `Vehicle->SetThrottleInput(x)` and the other input functions | `Vehicle->GetControls()->SetThrottleInput(x)` |
| `SetExtReplicationInput1(x)` to `SetExtReplicationInput6(x)` | `GetControls()->SetExtReplicationInput(0..5, x)` |
| Engine settings and `GetRPM` | `GetEngine()` |
| Gear, differential, and wheel radius settings | `GetTransmission()` |
| `bUseDrivetrain` and the slip settings | `GetDrivetrain()` |
| Brake, reverse, handbrake, and external resistance settings | `GetBrakes()` |
| Steering clamps, sensitivity, and drift torque | `GetSteering()` |
| Pitch moments and lean steer, `InitializeLeanSteer` | `GetBodyDynamics()` |
| Drag settings | `GetAerodynamics()` |
| Air control strengths | `GetAirControl()` |
| Anti gravity settings, `InitializeAntiGravity` | `GetAntiGravity()` |
| Idle lock settings | `GetIdleLock()` |
| Replication settings, `AsyncReplicatedTick` | `GetNetwork()` |
| Animation mode settings, `InitializeAnimationSystem` | `GetAnimation()` |
| `UNsVehicleInteriorComponent` with an Animation Type | One subclass for each value, for example `UNsVehicleTachometerComponent` |
| Include `Vehicle/NsVehicleSuspensionComponent.h` | Include `Wheels/NsVehicleSuspensionComponent.h` |

Values that a Blueprint set on the old vehicle properties do not move to the modules. Set them again on the modules.
