<!-- GH_ONLY_START -->
<h1 align="center">
  <br>
  <a href="https://github.com/mykaadev/NsVehicles"><img src="https://raw.githubusercontent.com/mykaadev/NsVehicles/refs/heads/main/Resources/Banner.png" alt="NsVehicles" width="400"></a>
</h1>

<h4 align="center">Modular physics vehicles for Unreal Engine: cars, karts, trucks, bikes, and hover vehicles.</h4>

<div align="center">
    <a href="https://github.com/mykaadev/NsVehicles/commits/main"><img src="https://img.shields.io/github/last-commit/mykaadev/NsVehicles?style=plastic&logo=github&logoColor=white" alt="GitHub Last Commit"></a>
    <a href="https://github.com/mykaadev/NsVehicles/issues"><img src="https://img.shields.io/github/issues-raw/mykaadev/NsVehicles?style=plastic&logo=github&logoColor=white" alt="GitHub Issues"></a>
    <a href="https://github.com/mykaadev/NsVehicles/pulls"><img src="https://img.shields.io/github/issues-pr-raw/mykaadev/NsVehicles?style=plastic&logo=github&logoColor=white" alt="GitHub Pull Requests"></a>
    <a href="https://github.com/mykaadev/NsVehicles"><img src="https://img.shields.io/github/stars/mykaadev/NsVehicles?style=plastic&logo=github" alt="GitHub Stars"></a>
    <a href="https://twitter.com/mykaadev/"><img src="https://img.shields.io/twitter/follow/mykaadev?style=plastic&logo=x" alt="Twitter Follow"></a>

<p style="display:none;">
  <a href="#-summary">👀 Summary</a> •
  <a href="#-features">📦 Features</a> •
  <a href="#-requirements">⚙️ Requirements</a> •
  <a href="#-installation">🛠️ Installation</a> •
  <a href="#-getting-started">🚀 Getting Started</a> •
  <a href="#-modules">🧩 Modules</a> •
  <a href="#-api">🔧 API</a> •
  <a href="#-limits">🚧 Limits</a> •
  <a href="#-documentation">📚 Documentation</a> •
  <a href="#-credits">❤️ Credits</a> •
  <a href="#-support">📞 Support</a> •
  <a href="#-license">📃 License</a>
</p>
<a href="https://buymeacoffee.com/mykaadev"><img src="https://www.svgrepo.com/show/476855/coffee-to-go.svg" alt="Coffee" width="50px"></a>
<p><b>Buy me a coffee!</b></p>
</div>
<!-- GH_ONLY_END -->

## 👀 Summary
NsVehicle drives physics vehicles: cars, karts, trucks, bikes, and hover vehicles. One physics body moves the vehicle. Suspension components push it up from the ground, and module components add the drive, brake, steering, and other forces.

NsVehicle is not a full tyre model. It does not simulate a clutch or a skeletal mesh vehicle, and the wheels are traces, not colliders.

## 📦 Features
- **Modules**: One component for each system, from the engine to the air control. The vehicle makes all of them by default.
- **Two powertrains**: An arcade powertrain that drives with no setup, and a standard powertrain with gears, a differential, and drag.
- **Any drivetrain**: RWD, FWD, AWD, 6WD, and 8WD, from the driven wheels.
- **Suspension traces**: Ignore surfaces with a custom trace channel.
- **Wheel detach**: Take a wheel off as a free physics body, and put it back.
- **Interior parts**: Tachometer, speedometer, steering wheel, pedals, and handbrake lever, animated each frame.
- **Articulated parts**: Move a part such as a forklift arm.
- **Spline AI**: Follow a spline with a tunable steering controller.
- **Replication**: Data only or full replication, plus custom input slots for the server.
- **Take recorder**: Record the wheel motion.

## ⚙️ Requirements
Unreal Engine 5.7+

For replication, enable **Project Settings > Physics > Tick Physics Async**, with a step of 0.008333 (1/120) or a little less.

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the folder into the `Plugins` folder of your project.
3. Add `"NsVehicle"` and `"AsyncTickPhysics"` to `PublicDependencyModuleNames` in `Build.cs`. `ANsVehicle` derives from `AAsyncTickPawn`, so a C++ child class needs both.
4. Generate the project files and enable the plugin.

## 🚀 Getting Started
1. Make a Blueprint class with `NsVehicle` as the parent.
2. Add a Static Mesh component as the root. Set a mesh with simple collision, and enable **Simulate Physics**.
3. Add four `NsVehicleSuspensionComponent` components under the root, at the tops of the wheels.
4. On each suspension component, set **Wheel Mesh**. On the two front wheels, enable **Is Steering Wheel**.
5. In the input events, call `Get Controls`, then `Set Throttle Input`, `Set Brake Input`, and `Set Steering Input`.

With no other setup, the vehicle drives with the arcade powertrain. In C++:

```cpp
#include "Modules/Control/NsVehicleControlsComponent.h"
#include "Vehicle/NsVehicle.h"

void AMyPlayerController::DriveForward(ANsVehicle* Vehicle, const float Input)
{
    Vehicle->GetControls()->SetThrottleInput(Input);
}
```

## 🧩 Modules
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

## 🔧 API
### Main Classes
- `ANsVehicle` – The vehicle pawn. It runs the physics step and calls the modules in a fixed order.
- `UNsVehicleModuleComponent` – The base class of every module.
- `UNsVehicleSuspensionComponent` – One wheel: the trace, the spring, the wheel mesh, and detach and reattach.
- `UNsVehicleInteriorComponent` – The base class of the interior parts.
- `UNsVehicleArticulatedComponent` – A part that moves on one axis from an input of -1 to 1.
- `UNsVehicleAI` – Spline following with a PID steering output.

### Main Functions
- `GetControls()->SetThrottleInput` / `SetBrakeInput` / `SetSteeringInput` – Drive the vehicle.
- `GetControls()->ApplyHandbrakeInput` – Pull the handbrake.
- `GetControls()->SetExtReplicationInput` – Send a custom input from -1 to 1 to the server, in one of 6 slots.
- `DetachWheel` / `ReattachWheel` – Take a wheel off with an impulse, and put it back. Local only.

## 🚧 Limits
- Do not scale the vehicle actor. Use a larger mesh and keep the actor scale at 1.
- Wheel detach and reattach do not replicate.
- More than three replicated vehicles on a server can crash Chaos. Disable **Auto Wake Rigid Bodies** and wake the bodies from the input events.
- There is no clutch simulation.

## 📚 Documentation
- [User Guide](https://mykaadev.github.io/plugins/nsvehicles/user-guide/) – Setup, common tasks, settings, and debug tools.
- [Technical Documentation](https://mykaadev.github.io/plugins/nsvehicles/tech-documentation/) – Architecture, lifetime, extension points, and tests.

<!-- GH_ONLY_START -->
## ❤️ Credits
<a href="https://github.com/mykaadev/NsVehicles/graphs/contributors"><img src="https://contrib.rocks/image?repo=mykaadev/NsVehicles"/></a>

## 📞 Support
Reach out via the **[profile page](https://github.com/mykaadev)**.

## 📃 License
[![License](https://img.shields.io/badge/license-MIT-green)](https://www.tldrlegal.com/license/mit-license)
<!-- GH_ONLY_END -->
