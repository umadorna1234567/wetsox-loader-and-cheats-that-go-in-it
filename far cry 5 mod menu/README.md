# Far Cry 5 menu project

**Current status: live overlay, human/animal ESP, named body parts, on-foot aiming, cover checks, ammunition controls and reversible vehicle FOV passed user checks. AR-C travel-time acquisition works; long-range impacts and moving-target lead remain unverified. Menu input capture, sticky aim and occupant-cover exceptions are now loaded for live tests. Mounted aiming and car speed remain unfinished.**

The selected architecture is an internal DLL. The current live revision is `build/runtime/Release/FC5MenuRuntime12.dll`; its matching loader defaults to that filename. Insert shows/hides the menu. ESP supports Cult/Blessed-only or all humans, plus animals, with separate category colors and optional named body-part markers. Aim activation supports right mouse, Left Alt or a mouse side button. Turn off Projectile travel time for direct aiming. Enabling it uses experimental weapon-specific compensation for recognized on-foot bullet weapons; unsupported modes block aim. Human enemy selection currently means Cult/Blessed factions, not a verified per-character hostility relation.

Aim requires a collision query to confirm the selected body part is reachable without an unrelated collider along the segment. Unknown/empty results block aiming. It rechecks every aim update and skips blocked targets. The user confirmed aiming works exposed and stops behind cover; other materials and scenes still need testing. ESP visibility is independent and may still display markers behind cover.

Sticky aim prefers the same eligible target while the aim key remains held. Releasing the key or losing eligibility clears it. The vehicle-occupant exception ignores only the occupant's own vehicle collider; unrelated cover continues to block aim. Both features are configurable and await live testing.

The launch observer forwards shot arguments unchanged and records muzzle position, initial direction, inherited velocity and firing-module properties. The user fired the AR-C while stationary; two samples reported speed 280, gravity -20, drop distance 40 and zero inherited velocity. Runtime10 reads the equipped weapon and models delayed, discrete gravity at the current simulation timestep. Constant-velocity prediction uses position samples; future acceleration, animation and timestep changes introduce error. Long-range shots and other weapons still need live validation.

## Load, check, and stop

Start Far Cry 5 normally through Steam, load an offline single-player save, and pause it. From the project folder:

```powershell
.\build\runtime\Release\FC5MenuLoader.exe
.\build\runtime\Release\FC5MenuLoader.exe --status
.\build\runtime\Release\FC5MenuLoader.exe --runtime-status
.\build\runtime\Release\FC5MenuLoader.exe --stop
```

The first command loads the DLL and starts the overlay. The status command must show `DX11 ready: 1` and an increasing frame count to confirm rendering; a successful load alone does not prove that it renders in Far Cry 5. Insert toggles visibility. The stop command disables hooks, restores the window procedure, and releases graphics resources. **The overlay DLL remains mapped until the game exits** to avoid unloading callbacks that may still be on another thread's stack. Restart the game before replacing the DLL with a new build.

This uses our own small loading utility; no third-party mod manager or files in the game directory are required. Only a running `FarCry5.exe` is selected by default. `--pid N` selects between multiple instances. The utility reports access or loading failures without attempting to bypass process protections. A timed-out remote call retains its argument memory because it may still be in use; exit the game before retrying.

The overlay captures DirectInput mouse/keyboard polling and window messages while open, releases exclusive devices, and suppresses cursor clipping/recentering. Closing resumes gameplay after held mouse buttons are released, preventing the closing click from firing. Automated input tests passed and the game reports intercepted polls; live free-cursor/no-click-through validation is pending. Gamepad input is not captured. DX12/Vulkan wrappers, alternate Present1 paths, device replacement, and other tools subclassing the same window are not validated. Shutdown reports busy if a later window subclass would prevent restoration.

For an interactive demo without launching the game:

```powershell
.\build\vs\Release\FC5OverlayHost.exe --interactive
```

## DLL entry point and lifecycle

`DWORD WINAPI FC5MenuRun(DWORD flags)` is exported with a C ABI. Call it outside `DllMain`, on a dedicated UI thread. Use `0` for the visible menu or `1` for the hidden menu creation/cleanup check. The call blocks until the window closes and returns zero on success. Unsupported flag bits return `ERROR_INVALID_PARAMETER`; concurrent calls return `ERROR_BUSY`.

The host must retain its `LoadLibrary` reference until the entry point returns, then it may call `FreeLibrary`. Closing the window tears down its controls, font, and window class before returning. Do not unload while the menu is running. `DllMain` only records the module handle; loading the DLL alone does not start a thread or install hooks. Settings are saved beside the DLL.

The legacy native menu entry point above remains available for the configuration preview. The overlay uses `DWORD WINAPI FC5OverlayStart(void*)`, `FC5OverlayStop(void*)`, and `FC5OverlayStatus(OverlayStatus*)`, defined in `include/fc5/overlay.hpp`. Call these outside loader lock. Once the overlay starts, it pins the DLL for the process lifetime; the native-menu-only unload test does not imply physical unloading after hooks have run.

Open `build/vs/Release/FC5MenuPreview.exe` to review the controls. Save configuration writes `fc5-menu.ini` beside the executable. The preview does not attach to Far Cry 5 or modify game files. Enabling a setting currently saves a preference only.

## Implemented

- Native Windows controls for aim filters, animal targeting, mounted-weapon targeting, head/chest/abdomen/pelvis selection, aim FOV, smoothing, travel time, prediction, and drop compensation.
- ESP filter and category-color configuration; separate ammunition and no-reload settings; vehicle camera FOV and speed settings.
- Configuration persistence and input validation in the native preview. Overlay settings currently last only for the process session and are separate from the preview INI.
- C++ targeting library with a constant-gravity, constant-target-velocity interception solver, inherited projectile velocity, earliest reachable intercept selection, hit-location availability checks, angular FOV selection, and frame-independent direction smoothing.
- DX11 overlay with all requested configuration categories, Insert toggle, resize recovery, pipeline render-target preservation, stop and restart support.
- Cross-process loading and status utility.
- Six automated suites: targeting, native menu creation, DirectInput capture/restoration, native DLL lifecycle, DX11 rendering/resize/toggle/shutdown/restart, and cross-process loading/status/stop/restart. The render test checks real backbuffer pixels and writes an `overlay-test.bmp` in the build directory.

The solver assumes no aerodynamic drag. The runtime supplies verified fields for recognized on-foot bullet weapons and rejects unsupported modes. Discrete gravity currently supports zero inherited velocity only. No spread adjustment exists. Accurate mathematical interception does not guarantee hits when targets change movement, bones animate, simulation timing changes, or spread affects the projectile. Vehicle rotation limits, projectile drag, and additional weapon behavior still require integration work.

## Remaining before gameplay use

- Validate cover queries, velocity and active weapon ballistics for this exact game build.
- Validate projectile compensation and connect mounted aiming, respecting turret limits.
- Add verified hostility-based ESP filtering and validate additional animal species.
- Implement a stable vehicle speed multiplier; vehicle FOV change and restoration passed the user's live test.
- Verify behavior in an offline single-player save, including animal skeleton mappings and scoped/vehicle weapons.

The menu is partially functional; the remaining controls must not be presented as completed features.

## Build and test

Requires Visual Studio 2022 or 2026 with Desktop development with C++, its CMake component, and Python 3:

```powershell
python tools/build.py
```

The build script selects the installed Visual Studio toolchain and runs CTest. Windows sources use the pinned Dear ImGui and MinHook checkouts in `third_party`. See `docs/third-party.md` for versions and licenses. With another C++20 compiler, the platform-independent core can also be built through CMake; the menu and overlay require Windows.

## Configuration behavior

- Aim FOV is the full angular diameter of the acquisition cone, distinct from camera FOV.
- Smoothing is a time constant in seconds; zero selects instant aiming.
- Travel time off means direct aim. Prediction and drop controls are disabled while preserving their saved preferences.
- Enemy humans and other humans together select everyone; animals are a separate category.
- Missing selected hit locations skip the target rather than substituting a different bone.
- Ammo and no-reload are separate settings. The user confirmed magazine preservation and reserve availability, including normal reserve consumption after disabling the override.

See `REQUIREMENTS.md` for the requested behavior and `docs/integration-status.md` for the local game inspection.
