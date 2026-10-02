# Wetsox

C++20 / Qt 6 Quick desktop loader with independently packaged game modules. Start `out/bin/Wetsox.exe`. Far Cry 5, Far Cry 4 and the converted Just Cause 4 module use Wetsox menus and per-game configs. The donor applications are not launched.

## Portable layout

```text
out/bin/
  Wetsox.exe
  backend/                     # Qt application, runtime DLLs, plugins and helper
  cheats/
    farcry5/                   # WetsoxFC5.dll, game.json, cover.jpg, licenses/
    farcry4/                   # WetsoxFC4.dll, game.json, cover.jpg, licenses/
    justcause4/                # WetsoxJC4.dll, game.json, cover.svg, licenses/
  configs/
    settings.json              # shared/local appearance and library edits
    farcry5/                   # saved feature presets and .last-session.json
    farcry4/
    justcause4/
```

Keep `backend` beside `Wetsox.exe`. The launcher forwards arguments and the application
resolves cheats/configs relative to that launcher, regardless of the working directory.
Existing portable settings migrate into configs/settings.json. User configs remain in
place. The loader folder must be writable to save settings.

## Installing and distributing game packs

Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\package.ps1` after building.
The root `nexus stuff` folder contains separate loader, Far Cry 5, Far Cry 4 and Just
Cause 4 ZIPs. Personal settings and configs are excluded. Every archive has instructions.
Update the loader and game packs together for session protocol v5; restart games before
loading updated DLLs. For old flat-layout downloads, extract into a fresh folder and copy
cheats/configs across. Move an old root settings.json into configs/settings.json.

Copy a complete pack inside `cheats`. Nested extraction wrappers are supported, for
example `cheats/download-name/farcry4/game.json`. Discovery runs every second and
prefers direct installs when duplicate IDs exist. Missing artwork uses a placeholder;
missing DLLs or invalid manifests remain unavailable. DLL paths stay within their pack.

Start the game and load a single-player save, then click its installed card. Successful
loading opens its Wetsox menu and hides the library. Closing the menu restores the library.
Modules stay resident until the game exits. Exiting the loader disables features through
its session timeout. Keyboard, mouse, XInput controllers, and native DualSense/DS4 bindings are
supported: click a binding field, release already-held inputs, then press the desired
button, trigger or stick direction. Native Sony buttons share the existing Pad binding
names: Cross = Pad A, Circle = Pad B, Square = Pad X, Triangle = Pad Y,
L1/R1 = Pad LB/RB, and L2/R2 = Pad LT/RT. No XInput mapper is required
for the supported Sony models (DualSense, DualSense Edge, and official DS4 v1/v2).

## Just Cause 4

Converted from the supplied Solis gameplay source; the old proxy DLL and menu are not
used. Includes invulnerability, ammo, wingsuit boost/rockets/speed, hoverboard and grapple
settings, vehicle boost, world speed, character ESP/tracers, torso aim assist and
waypoint/objective teleport controls. Same supported Steam build 4110618 checks as the
source project. Disable the old Solis xinput9_1_0.dll before launching; do not load both.
The local old proxy was preserved as xinput9_1_0.dll.solis-backup.

Original limitations remain: torso bounds are estimated, aim assist has no wall check,
and special movement features require their corresponding content to be unlocked.
JC4 uses a vector placeholder cover that can be replaced through the game editor.
The converted pack passed live build-check, hook-startup and renderer verification.
Feature-by-feature gameplay confirmation is pending; automated math/menu tests are
not gameplay confirmation. Third-party notices are included in the game pack.

## Far Cry 4

Installed through the same loader. Its camera alignment guard now compares world-space
angles (8-degree bound), independent of render resolution, aspect ratio and scope zoom.
Eye-position, camera validity, player lifetime, collision and bounded-write checks remain.
The reported 1440p failures need confirmation on affected systems; resolution alone is
not established as the cause. Physical-projectile prediction remains a direct-aim fallback.

## Customization and configs

Appearance changes save automatically after every edit, including each menu's shared/custom setting. Shared appearance edits propagate to linked windows; unlinking restores that window's own appearance. Sun/moon mode remembers its palette and preserves sizing, font, accent and other customization. The logo and window branding use Wetsox and a W.

Feature options and keybinds also restore automatically on reopening/restarting via `.last-session.json`. Use the menu's **Configs** page to save named presets, load them, or delete them. Named files are saved in `configs/<game id>/<name>.json` beside Wetsox.exe. Loading a config updates the current game options immediately. Config names allow letters, numbers, spaces, underscores and dashes. Appearance is saved separately in configs/settings.json rather than overwritten by feature presets.

## Far Cry 5

Options include handheld aiming, hold-key recording, target/body-part selection, sticky lock, aim FOV circle, optional projectile compensation, box/bone ESP, independent colors and outlines, ammunition controls, and on-foot/vehicle camera FOV. ESP reads poses each rendered frame; discovery and velocity sampling run separately. Identified vehicle cover is ignored for aiming; walls and unresolved colliders still block. Projectile penetration is unchanged.

Unsupported weapon ballistic data now falls back to direct aim instead of disabling aiming (including the reported Vector case). Handheld aiming is allowed in vehicles using the same verified look/camera path; it falls back to direct aim because vehicle projectile inheritance is not calibrated. A camera-pose mismatch prevents an unsafe angle write and appears in status. This does not add mounted-turret aiming. Live Vector and handheld-in-car behavior still needs gameplay verification. The engine fingerprint is recorded for diagnostics and does not block a different hash.

## Build and verification

Use Windows x64 MSVC, Qt 6.5+ Quick/Controls/Dialogs/Test, CMake and Python 3. The workspace SDK is used automatically when present:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

The script builds, runs tests, deploys into `out/bin`, and removes known obsolete Nexus runtime copies. `-Run` launches Wetsox. CMake's internal `Nexus` target/QML URI remain private implementation names; the executable is Wetsox.exe.

Regression suites cover appearance persistence, interactions, real QML controls/configs, package discovery/removal, portable config persistence and validation, targeting mathematics, DLL loading/rendering, and cross-process settings transfer. Runtime integration tests use a disposable DX11 host, not the live game. `Wetsox.exe --smoke-test --screenshots <directory>` can capture the UI using temporary settings/configs and the SDK QtTest module.

Expanded appearance and layout controls are described in [Appearance customization](docs/appearance-customization-2026-09-30.md). Select Liquid glass in Appearance; use Edit layout in a game menu to rearrange cards and adjust individual elements. Named theme profiles include layout, visibility, and rainbow settings. The fresh local verification build is tools/build_appearance.ps1.


## Engine compatibility and troubleshooting

Far Cry 4 checks the SHA-256 of FC64.dll; Far Cry 5 checks FC_m64.dll.
These identify the engine build, not a PC, graphics card, display resolution, or user.
Both packs attempt startup with any readable engine hash. SHA-256 differences are
informational, not a compatibility guarantee. Existing memory-read, hook-install,
camera, and aim-pose checks remain active. Unknown layouts can still fail or crash.
The DLLs no longer repeat a blocking fingerprint check internally.

The helper distinguishes a missing engine, an unreadable file, an unverified hash,
and runtime initialization failure. Check without loading a cheat:

```powershell
.\backend\WetsoxGameLoader.exe --game farcry4 --check-build "C:\path\to\Far Cry 4\bin\FC64.dll"
.\backend\WetsoxGameLoader.exe --game farcry5 --check-build "C:\path\to\FarCry5\bin\FC_m64.dll"
```

Just Cause 4 checks the executable timestamp and code bytes for Steam build 4110618.
Its pack-local WetsoxJC4.log records a failed timestamp or code-check address.
Another mod's hook can also fail a code check. Do not replace game engine DLLs to
force a match. Share the full error and game edition/store/version with the author.

The FC4 alignment guard compares world-space directions rather than screen pixels.
Its regression tests cover aspect ratio and zoom changes, but this does not establish
that every reported alignment failure on another installation has been resolved.

For a support report, keep the game running and execute from the loader folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\backend\Collect-game-diagnostics.ps1 -Game farcry5
```

Return to the game and try aiming during the 15-second capture. Send the text file
created under `diagnostics`. Use `-Game farcry4` for FC4. Reports include engine and
section hashes, file version, and existing runtime rejection counters. They do not
inject a module or change settings. For an offline engine report add `-EnginePath`.

FC5 camera alignment now uses a conservative 3-degree world-space limit rather
than a zoom-dependent screen-space threshold. This addresses one possible cause;
affected-user gameplay verification is still needed.

## Feature hotkeys

Each game has a separate Hotkeys tab, excluded from Home. The Key button beside
any feature toggle opens and highlights its binding. Existing aim/teleport key
pickers moved here. Keyboard, mouse, and supported controller bindings use the
same click-to-record picker. The keyboard menu shortcut is also editable here.

The feature's normal toggle is its master switch. With Use hotkey disabled it
stays enabled. Hold gates it while the binding is held. Toggle starts off and
changes once per press; holding a button does not repeat. Unbound enabled hotkeys
remain inactive. Changing a rule resets its latch; alt-tabbing or recording a
binding cannot generate a fresh press. Toggle latches are session state, not
saved config state. Hotkey rules are saved in `_hotkeys` within each game config
and the last session. Legacy aim and teleport bindings are read without rewriting
user files during the update.

Input evaluation runs in the loader every 16 ms, with one controller snapshot per
poll, and publishes effective feature settings to the active game. Session IPC is
now v6: update the loader and all game packs together and restart running games.
