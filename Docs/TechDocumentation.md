# NsVehicles Technical Documentation

## Architecture

### Modules

The descriptor is `NsVehicle.uplugin`. It has three modules. The two runtime modules are on Win64, Mac, IOS, Android, and Linux only.

| Module | Type | Loading phase | Content |
|---|---|---|---|
| `AsyncTickPhysics` | Runtime | Default | `AAsyncTickPawn`, `UAsyncTickActorComponent`, the Chaos sim callback that calls them, and `UAsyncTickFunctions` for physics thread force and velocity calls |
| `NsVehicle` | Runtime | Default | The vehicle pawn, the modules, the wheels, the interior parts, the articulated part, the AI steering and drive mode, the reach profile `UNsVehicleReachProfile`, and the A* plan `UNsVehiclePathfinder` |
| `NsVehicleEditor` | Editor | Default | The reach tool: `UNsVehicleReachSettings`, `UNsVehicleReachLibrary`, the `NsVehicle.Reach` console command, and the `NsVehicleReach` commandlet. The drive-to tool: the `NsVehicle.DriveTo` and `NsVehicle.DriveStop` console commands. The hunt tool: `UNsVehicleHuntList`, `UNsVehicleHuntRunner`, and the `NsVehicle.Hunt` and `NsVehicle.HuntStop` console commands |

| Module | Type | Loading phase | Content |
|---|---|---|---|
| `NsVehicleCheats` | DeveloperTool | Default | `UNsVehicleCheatsSubsystem`, the Vehicle AI tab of the NsCheats window. It links `NsCheatsRuntime` only when the NsCheats plugin is present, and defines `WITH_NSCHEATS`. Without NsCheats the subsystem does not start |

`UNsVehicleReachProfile` moved from `NsVehicleEditor` to `NsVehicle`. `Config/DefaultNsVehicle.ini` holds a class redirect, so the profile assets of a project load with no change.

`AsyncTickPhysics` is bundled in `Source/ThirdParty/AsyncTickPhysics/Source/AsyncTickPhysics/`. It has no descriptor of its own. `NsVehicle.uplugin` lists it.

### Folders

All folders are under `Source/NsVehicle/Public/` and `Source/NsVehicle/Private/`. Both use the same layout.

| Folder | Classes |
|---|---|
| (root) | `FNsVehicleModule` in `NsVehicleModule.h`. Its startup and shutdown are empty |
| `Vehicle/` | `ANsVehicle`, and `NsVehicleTypes.h` with the shared enums and structs |
| `Modules/` | `UNsVehicleModuleComponent`, the base class of all modules |
| `Modules/Control/` | `UNsVehicleControlsComponent` |
| `Modules/Powertrain/` | `UNsVehicleEngineComponent`, `UNsVehicleTransmissionComponent`, `UNsVehicleDrivetrainComponent` |
| `Modules/Chassis/` | `UNsVehicleBrakeComponent`, `UNsVehicleSteeringComponent`, `UNsVehicleBodyDynamicsComponent`, `UNsVehicleAerodynamicsComponent`, `UNsVehicleAirControlComponent`, `UNsVehicleAntiGravityComponent`, `UNsVehicleIdleLockComponent` |
| `Modules/Network/` | `UNsVehicleNetworkComponent` |
| `Modules/Animation/` | `UNsVehicleAnimationComponent` |
| `Wheels/` | `UNsVehicleSuspensionComponent`, `UNsVehicleStaticWheel`, `UNsVehicleWheel` |
| `Interior/` | `UNsVehicleInteriorComponent` (abstract) and its six subclasses |
| `Articulation/` | `UNsVehicleArticulatedComponent` |
| `AI/` | `UNsVehicleAI`, `UNsVehiclePathfinder` with `FNsVehiclePathQuery`, `UNsVehicleReachProfile` |

### Main Classes

- `ANsVehicle` derives from `AAsyncTickPawn`. It owns the physics state: the physics transform, the local velocity, the speed, the acceleration, the yaw rate, and the airborne state. It makes one of each module in its constructor and calls them in a fixed order. Its only tuning values are `InAirWheelCount` and `bAutoWakeRigidBodies`.
- `UNsVehicleModuleComponent` is the abstract base of the 13 modules. A module does not tick. It caches the owner `ANsVehicle` in `OnRegister`. Its physics functions take values as arguments, or read them through the vehicle getters.
- `UNsVehicleSuspensionComponent` is a scene component. It traces the ground, applies the spring, lateral, and resistance forces at its location, runs the slip model for a driven wheel, and places and spins its wheel mesh. The wheel scale applies to the sweep sphere, the trace length, the wheel height, and the wheel spin through `GetScaledWheelRadius`. A scale of 1 behaves as before.
- `UNsVehicleInteriorComponent` is an abstract static mesh component. It maps one vehicle value to a rotation on one axis.
- `UNsVehicleArticulatedComponent` is a static mesh component that moves on one axis from an input.
- `UNsVehicleAI` is an actor component. It turns the lateral distance to a spline into a steering value with a PID controller, and the caller sends that value to the Controls module. In the drive mode (`StartDrive`) its tick sets the throttle, the brake, and the steering of the Controls module itself: pure pursuit to a point ahead on the spline, a drive backward after a stall, and the events `OnDriveArrived` and `OnDriveStuck`.
- In Drive To (`StartDriveTo`), `UNsVehicleAI` plans the way with `UNsVehiclePathfinder::FindPath`, smooths it with `SmoothPath`, and follows the ground points with pure pursuit. The steering distance grows with the forward speed. The steering input is the heading error minus the yaw rate times `SteerDamping`, rate limited by `SteerRate`. The target speed falls from `MaxDriveSpeed` to `CornerSpeed` with the turn in the next `CornerLookAhead`, and near the goal. The throttle is `CruiseThrottle` plus the speed error over `SpeedBand`. The brake works only when the forward speed is half a band over the target, because a brake input at rest drives the vehicle backward. A stall leads to a drive backward, then a new plan from the current place, with the stall place in `BlockedPoints`. `DescribeBlocker` sweeps ahead of the owner to name the actor that holds it back, or reports the pitch.
- `UNsVehiclePathfinder` is the shared test of the reach tool and of Drive To. `FindGround` traces down for the ground. `CanStep` tests the step height, the slope, and the drop with `CanClimb` for the whole step and for each half, with a ground trace at the middle. The slope limit is the profile limit times `SlopeMargin`. Then a box of the profile size plus `Clearance` sweeps the step, from just above the step height to the vehicle height, so an object that the wheels cannot climb stops the step. When `FNsVehiclePathQuery::Vehicle` is set, the sweep uses the object type and the responses of the vehicle root body, so the plan sees what the vehicle collides with. The sweep ignores a hit and sweeps again (up to `MaxSweepTries`) when the hit is ground (a surface flatter than `WallAngle` on the ground actor, or on a terrain tile of the same class larger than `TerrainTileSize`), a pawn, or a component with no physics collision. A start inside a wall is allowed on the first step from the start only. `FindPath` runs A* on a grid that aligns to the world at multiples of `GridSpacing`. The start joins the nearest of the four grid points around it that it can drive to, and the way begins at the start. `FindGround` and `FindGroundActor` trace simple collision only, because a complex trace hits branches and leaves above the ground. `DescribeStep` runs the same test as `CanStep` through `TestStep` and returns the reason of a failure. `FindPath` runs A* on that grid. The step cost is the run times one plus `SlopeCost` times the square of the slope against the limit, plus `EdgeCost` times the run when the ground one grid step to the side is a drop, a void, water, or a bank that rises steeper than the slope limit. The heuristic is the straight distance to the goal cell.
- `SmoothPath` keeps the first point. From each kept point it tries the farthest later point up to `MaxSkip` grid points away, and goes nearer until a straight line passes `CanStep`, the blocked points, and the edge test at each grid step. The line adds its ground points, so the way keeps the height of the ground.
- `UNsVehicleReachLibrary::MakeQuery` makes the query of the editor tools from the settings: margin 1 and no edge cost for the reach tool, the path settings for the drive-to tool and the hunt.
- `UNsVehicleHuntRunner` is a transient object that the editor module keeps. It ticks on the core ticker, starts one Drive To after the other, tests the no-go boxes each tick, and writes the report when the list ends, when Play In Editor ends, or on `NsVehicle.HuntStop`.
- `UNsVehicleStaticWheel` spins its mesh from the root velocity in its own tick. It does not need `ANsVehicle`.
- `UNsVehicleWheel` is a static mesh with the tag `NsVehicleWheel`. The Animation module spins it.

### Physics Step Order

The `AsyncTickPhysics` sim callback calls `ANsVehicle::NativeAsyncTick` in `OnPreSimulate_Internal`, one time for each physics step:

1. `AAsyncTickPawn::NativeAsyncTick`, which runs the Blueprint `AsyncTick` event.
2. `UpdateGroundContact`. With the drivetrain on, `Drivetrain->UpdateWheelSlip` averages the slip, and the vehicle is airborne when no driven wheel is on the ground. Otherwise the vehicle counts airborne wheels against `InAirWheelCount`. Detached wheels do not count.
3. `UpdateAngularMotion`: the yaw rate and the yaw acceleration.
4. `IdleLock->UpdateIdleLock`.
5. The branch on `Network->GetReplicationMethod()`. The Networking section gives each branch.
6. `UpdateWheelPhysics`: each suspension component gets the wheel torque and the inputs, then runs its trace and forces.

`UpdatePhysicsCore`:

1. Reads the physics transform with `ATP_GetTransform`.
2. Standard powertrain (`UpdateStandardPowertrain`): Engine torque, Transmission gear ratio and automatic shift, engine brake force, then on the ground: Brakes force, Drivetrain effective power, drive force and drive pitch torque, BodyDynamics pitch. Then Aerodynamics drag, Engine RPM, and Transmission shift timers. If the torque curve is missing, `UpdatePhysicsCore` returns at once and applies no force in that step.
3. Arcade powertrain (`UpdateArcadePowertrain`): Engine arcade torque, Brakes arcade force, external resistance, then on the ground: drive force and BodyDynamics pitch.
4. Steering drift torque, on the ground only.
5. `UpdateMotionState`: local velocity, speed, and acceleration.
6. Brakes handbrake, AirControl, AntiGravity, and BodyDynamics lean steer.

The drive force acts at the body center of mass through `ATP_AddForce`. The wheel torque does not move the vehicle. It only feeds the slip model of the driven wheels.

### Game Thread Order

`ANsVehicle::Tick`:

1. `WakeAllRigidBodies`, if `bAutoWakeRigidBodies` is true.
2. `UpdateAttachedComponents`: Steering sends the input and the response speed to the steering wheels. Each suspension component places, steers, and spins its wheel, and draws its debug shapes. Then the interior and articulated parts update.
3. `Network->UpdateFastSync`.
4. `AntiGravity->UpdateGameThread`.
5. The game thread local velocity for `GetSideSlipAngleGT`.
6. `Animation->UpdateLocationVelocity`, and `SpinSuspensionWheels` in animation mode.

### Data Flow

The modules do not hold pointers to each other. They read other modules through the vehicle getters, for example `GetVehicle()->GetTransmission()`. The vehicle passes the physics body, the direction vectors, and the inputs as arguments.

The physics thread and the game thread share the vehicle and module state with no lock. For example, the Controls setters run on the game thread, and the physics step reads the same fields.

The arcade powertrain uses the active gear ratio (3) and the active differential ratio (3.15). Only `SetCurrentGearRatio` and `SetDifferentialRatio` change them. The **Differential Ratio** property applies to the standard powertrain only.

## Lifetime

1. The `NsVehicle` module and the `AsyncTickPhysics` module load in the Default phase.
2. `FAsyncTickPhysicsModule::StartupModule` binds to `FPhysicsDelegates::OnPhysSceneInit` and `OnPhysSceneTerm`. It makes one `FAsyncTickManager` for each physics scene, and deletes it when the scene ends.
3. The `ANsVehicle` constructor makes the 13 modules with `CreateDefaultSubobject`. Their subobject names are `Controls`, `Engine`, `Transmission`, `Drivetrain`, `Brakes`, `Steering`, `BodyDynamics`, `Aerodynamics`, `AirControl`, `AntiGravity`, `IdleLock`, `Network`, and `Animation`. It also sets the net update frequency and the minimum net update frequency to 100.
4. Each module caches its owner in `OnRegister`. It logs an error if the owner is not an `ANsVehicle`.
5. `AAsyncTickPawn::BeginPlay` adds the pawn to the async tick manager. `EndPlay` removes it.
6. `ANsVehicle::BeginPlay` caches the root as the physics body. It finds the suspension, interior, and articulated components with `GetComponents`. Components added later are not in these lists.
7. `UNsVehicleSuspensionComponent::BeginPlay` makes the wheel mesh with `NewObject` and the actor as the outer, or it uses its first child static mesh. Then it reads the wheel radius and logs setup warnings.
8. `UNsVehicleEngineComponent::BeginPlay` and `UNsVehicleBrakeComponent::BeginPlay` store the torque and the brake force. `SetEngineTorqueEnabled` and `SetBrakesEnabled` restore these values.
9. `UNsVehicleAnimationComponent::InitializeAnimationSystem` replaces the suspension list with the children of the root whose first tag is `NsVehicleSuspensionComponent`.

`DetachWheel` moves the wheel mesh out of the attachment tree and makes it simulate. The actor still owns the wheel mesh, so the wheel mesh is destroyed with the vehicle. `ReattachWheel` stops its physics, turns its collision off, and attaches it to the suspension component again.

## Extension Points

- Subclass `ANsVehicle` in C++ or Blueprint. In a Blueprint, select a module in the Components panel and change its defaults. In a C++ constructor, use the module setters, for example `GetEngine()->SetArcadeMaxSpeed(60.f)`.
- Override `virtual void NativeAsyncTick(float DeltaTime)` in a C++ subclass. Call `Super::NativeAsyncTick` to keep the vehicle step.
- Use the Blueprint `AsyncTick` event for project logic on the physics step. Use the `ATP_` functions of `UAsyncTickFunctions` there, for example `ATP_AddForce`.
- Derive a component from `UAsyncTickActorComponent` and override `NativeAsyncTick` or the `AsyncTick` event to get the physics step in a component.
- Subclass `UNsVehicleInteriorComponent` and override `virtual float GetDriveValue(const ANsVehicle& Vehicle) const` for a new interior part.
- Call `UNsVehicleSuspensionComponent::SetNewPhysicsBody` to send the forces of a suspension to another body, for example a trailer.
- Call `UNsVehicleSuspensionComponent::UpdatePhysics` from a tick to use the suspension on a pawn that is not an `ANsVehicle`.
- Use `SetExtReplicationInput` for project input that must reach the server.

- Call `UNsVehiclePathfinder::FindPath` with your own `FNsVehiclePathQuery` for a way without a drive, for example for a game AI.
- Set `PathQuery` on `UNsVehicleAI` to change the grid, the costs, or the blocked radius of Drive To.

Module replacement is not supported. `ANsVehicle` has no `FObjectInitializer` constructor, and the module physics functions are not virtual.

## Dependencies

| Dependency | Kind | Why |
|---|---|---|
| `Core`, `CoreUObject`, `Engine` | Public, `NsVehicle` | Base types, components, and physics |
| `AsyncTickPhysics` | Public, `NsVehicle` | `ANsVehicle` derives from `AAsyncTickPawn` and calls `UAsyncTickFunctions` |
| `Slate`, `SlateCore` | Private, `NsVehicle` | Listed in `NsVehicle.Build.cs`. No `NsVehicle` source file uses them |
| `Core` | Public, `AsyncTickPhysics` | Base types |
| `CoreUObject`, `Engine`, `Slate`, `SlateCore`, `Chaos` | Private, `AsyncTickPhysics` | Physics scene delegates and the Chaos sim callback |

| `Core`, `CoreUObject`, `Engine`, `DeveloperSettings` | Public, `NsVehicleEditor` | The profile, the settings, and the queries |
| `UnrealEd` | Private, `NsVehicleEditor` | The map load, the commandlet, and the Play In Editor world of the drive-to tool and the hunt tool |
| `NsVehicle`, `AssetRegistry` | Public and private, `NsVehicleCheats` | The driver and the profile and hunt list search |
| `NsCheatsRuntime` (plugin NsCheats, optional) | Private, `NsVehicleCheats` | The cheat tab and the point picker. The `.uplugin` lists NsCheats as an optional plugin |
| `NsVehicle` | Private, `NsVehicleEditor` | `ANsVehicle`, `UNsVehicleAI`, `UNsVehiclePathfinder`, and the profile |

`AsyncTickPhysics.Build.cs` also calls `SetupModulePhysicsSupport`. The `.uplugin` lists NsCheats as an optional plugin. The reach tool tests water by the class name of an actor, so it needs no Water plugin.

## Performance

- The vehicle ticks on the game thread and runs one step for each physics step. The modules do not tick.
- Each suspension component does one sphere sweep in each physics step. When the sphere hit fails the normal or height check, or **Hybrid Raycast** is off, it also does one line trace.
- Each suspension component has tick enabled with an empty `TickComponent`. A `UNsVehicleAI` component ticks, and it does work only in the drive mode: one closest-key search on the spline and one component search for the Controls module in each tick.
- `UNsVehicleStaticWheel` reads the local bounds of its mesh every frame.
- `GetSuspension` returns a copy of the array. Do not call it in a hot loop.
- `bAutoWakeRigidBodies` wakes the body every frame.
- The reach tool runs only in the editor. Each tested point does one line trace, one sweep, and one overlap for water. A 400 by 400 m area at 50 cm is about 640000 points, so use a larger grid spacing or an extent for a first look.
- `FindPath` tests each neighbor of each expanded cell, so it can test a cell more than one time. It stops at **Max Cells** expanded cells. A goal far behind a wall can expand most of the reach area before the search fails. It runs in one frame on the game thread.
- Drive To plans one time at the start and one time after each stall, up to **Max Replans**. `SmoothPath` adds up to `MaxSkip` line tests for each kept point, each with one test for each grid step on the line. In each tick it does one projection on the waypoints near the current progress. `DescribeBlocker` sweeps one time, only when the driver gives up.
- The hunt runner tests each no-go box against the vehicle location once a tick. The cost grows with the number of boxes only.

## Networking

- `ANsVehicle` sets the net update frequency and the minimum net update frequency to 100.
- `UNsVehicleControlsComponent` replicates by default. Each setter sets the local value and calls a reliable server RPC with validation. The float RPCs reject values outside -1 to 1. `ServerSetExtReplicationInput` also rejects an index outside 0 to 5.
- `UNsVehicleNetworkComponent` replicates by default. It replicates `ServerState` (`FNsVehicleServerState`), `ServerVelocity`, and `ServerAngularVelocity`.
- `UNsVehicleSuspensionComponent` replicates `ServerSteeringRotation` with `OnRep_Steering`. When the method is not None, `ANsVehicle::BeginPlay` makes each suspension component replicate. The server sets the steering rotation, and the clients copy it.

The physics step branch:

| Method | Server | Client |
|---|---|---|
| Full | `UpdatePhysicsCore`, `AsyncTick`, `ServerStateSync(true)` | `ClientStateSync(true)`: copies the velocities, RPM, gear, inputs, and speed |
| Data Only | `UpdatePhysicsCore`, `AsyncTick`, `ServerStateSync(false)` | `ClientStateSync(false)`, then `UpdatePhysicsCore` and `AsyncTick` |
| None | `UpdatePhysicsCore`, `AsyncTick` | `UpdatePhysicsCore`, `AsyncTick` |

- In Full mode, a client does not run `UpdatePhysicsCore`. Thus `GetPhysicsTransform` on that client keeps its default value.
- In Full mode, `UpdateFastSync` sets the client actor transform to the server transform every `ServerFastSyncLatency` game frames, if `bUseServerFastSync` is true.
- The idle lock sets the body velocity only when the method is None.
- `AsyncReplicatedTick` is a separate Blueprint call. On the server it copies the body velocities to `ServerVelocity` and `ServerAngularVelocity`. On a client it applies them. The vehicle does not call it.
- The custom inputs go from the client to the server. They do not replicate to other clients.
- `DetachWheel` and `ReattachWheel` are local only.
- Drive To and the A* plan are local. Run them on the machine that owns the vehicle input.

## Testing

The plugin has no automated tests. Use these manual tests:

1. Reach: in a map, run `NsVehicle.Reach X Y Z /Game/Path/Profile`. Make sure that the green points stop at walls, steep slopes, and water.
2. Drive To: in Play In Editor, run `NsVehicle.DriveTo X Y Z /Game/Path/Profile` to a goal behind a fence. Make sure that the way goes around the fence and that the vehicle arrives.
3. Stall: steer against the drive for some seconds. Make sure that the vehicle backs off, plans again, and continues.
4. Hunt: run `NsVehicle.Hunt` with a list of two targets and one no-go box on the way. Make sure that the report lists both drives and the zone entry.

## Change Rules

- Keep the class redirect of `UNsVehicleReachProfile` in `Config/DefaultNsVehicle.ini`. Projects saved the profile assets with the old editor class path. Save such a profile one time, so that a search by class finds it.
- `UNsVehicleAI` keeps a drive log: each plan, each stall with its place, pitch, and cause, each slope margin change, and the result. `SaveDriveReport` writes it to `Saved/QA` and does nothing in Shipping.
- `FindRandomPoint` traces down from 1 km above the center at random places in the circle. Under water allowed, the trace ignores a water body and goes on to the bed.
- Keep the field names of the profile, the hunt list, and `FNsVehiclePathQuery`. Projects store them in data assets.
- Keep the console command names. Test scripts of projects call them.
