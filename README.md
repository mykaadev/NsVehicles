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
NsVehicles drives physics vehicles in Unreal Engine: cars, karts, trucks, bikes, and hover vehicles. One physics body moves the vehicle. Suspension components trace the ground and push the body up. Module components add the drive, brake, steering, and other forces. It is for gameplay programmers and designers who want a vehicle that they can tune in the Details panel.

NsVehicles is not a full tire model, and it is not a Chaos Vehicles replacement. The wheels are traces, not colliders. The drive force acts on the body, not on each wheel. There is no clutch.

## 📦 Features
- **Modules**: One component for each system, from the engine to the air control. Each vehicle gets all of them.
- **Two powertrains**: An arcade powertrain with a top speed and no gearbox, and a standard powertrain with RPM, gears, and drag.
- **Driven wheels**: Mark the driven wheels and their torque share. Driven wheels lose grip and spin when the torque is too high.
- **Suspension traces**: A sphere trace with a line trace fallback. Use a custom trace channel to ignore surfaces.
- **Wheel detach**: Release a wheel as a free physics body, and attach it again.
- **Interior parts**: Tachometer, speedometer, steering wheel, pedals, and handbrake lever.
- **Articulated parts**: Move a part on one axis, for example a forklift arm.
- **Spline AI**: A PID controller that gives a steering input to follow a spline.
- **Replication**: Full or Data Only replication, plus six custom input slots that go to the server.
- **Take recorder mode**: Spin the wheels from the actor movement for recording and playback.

## ⚙️ Requirements
- Unreal Engine 5.7+
- Platforms: Win64, Mac, Linux, Android, and iOS.
- The bundled `AsyncTickPhysics` module. It is part of this repository.
- Recommended: **Project Settings > Physics > Tick Physics Async**, so that the vehicle step runs on the physics thread.

## 🛠️ Installation
1. **Clone** or download this repository.
2. Copy the `NsVehicles` folder into the `Plugins` folder of your project.
3. For C++, add `"NsVehicle"` and `"AsyncTickPhysics"` to `PublicDependencyModuleNames` in the `Build.cs` file of your game module. `ANsVehicle` derives from `AAsyncTickPawn`.
4. Generate the project files.
5. In **Edit > Plugins**, enable **NsVehicle**.

## 🚀 Getting Started
1. Make a Blueprint class with `NsVehicle` as the parent.
2. Add a Static Mesh component as the root. Give it a mesh with simple collision, and enable **Simulate Physics**.
3. Add four `NsVehicleSuspensionComponent` components under the root, one above each wheel position.
4. On each suspension component, set **Wheel Mesh**. On the two front ones, enable **Is Steering Wheel**.
5. In the input events, call `Get Controls`, then `Set Throttle Input`, `Set Brake Input`, and `Set Steering Input`.

The default Engine module uses the arcade powertrain, so the vehicle drives with no other setup. In C++:

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
- `UNsVehicleSuspensionComponent` – One wheel: the trace, the spring, the tire forces, the wheel mesh, and detach.
- `UNsVehicleInteriorComponent` – The base class of the interior parts.
- `UNsVehicleArticulatedComponent` – A mesh that moves on one axis from an input.
- `UNsVehicleAI` – Spline following with a PID steering output.

### Main Functions
- `GetControls()->SetThrottleInput` / `SetBrakeInput` / `SetSteeringInput` – Drive the vehicle.
- `GetControls()->ApplyHandbrakeInput` – Pull the handbrake.
- `GetControls()->SetExtReplicationInput` – Send a custom value from -1 to 1 to the server, in slot 0 to 5.
- `DetachWheel` / `ReattachWheel` – Release a wheel with an impulse, and attach it again.
- `Jump` – Add an upward velocity change when the vehicle is on the ground.

## 🚧 Limits
- Keep the actor scale at 1. The physics step ignores the actor scale, so a scaled vehicle puts the wheels in the wrong place.
- Wheel detach and reattach do not replicate.
- The modules are fixed. You cannot replace a module with a subclass.
- The Blueprint `Event Async Tick` runs two times in each physics step.

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
