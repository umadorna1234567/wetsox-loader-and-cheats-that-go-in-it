# Wetsox

C++20 / Qt 6 Quick desktop loader with independently packaged game modules. Start `out/bin/Wetsox.exe`. The current game backend is Far Cry 5; its native gameplay source is built into our own DLL. The donor application is not launched.

## Portable layout

```text
out/bin/
  Wetsox.exe
  WetsoxGameLoader.exe
  settings.json                 # appearance, shared/local links, library edits
  cheats/
    farcry5/
      WetsoxFC5.dll
      cover.jpg
      game.json
      licenses/
  configs/
    farcry5/
      My config.json
      Another config.json
      .last-session.json
```

Keep the Qt DLLs, plugins, QML folders, loading helper and licenses alongside the launcher. `settings.json` and `configs` are user data: do not include your personal copies in a public loader download. Existing Nexus appearance settings migrate from the previous per-user location when no portable settings file exists. The folder containing Wetsox.exe must be writable to save changes.

## Installing and distributing a game pack

Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\package.ps1` after building to create three separate downloads in the root `nexus stuff` folder: `Wetsox-loader.zip` (application and runtime dependencies), `Wetsox-FarCry5.zip` (the `farcry5` folder), and `Wetsox-FarCry4.zip` (the `farcry4` folder). Personal settings/configs are excluded. Each archive includes installation instructions.

Distribute the entire `cheats/farcry5` folder, including `game.json`, artwork and license notices. Recipients put it inside the `cheats` folder beside Wetsox.exe. The loader scans on startup and every second, so complete new packs appear automatically while it is open. Removing the folder removes the installed card. The image and menu definitions are read from the pack, not bundled into the launcher.

`game.json` contains formatVersion 1, a unique lowercase id, display name/subtitle, backend, DLL filename, artwork filename, and declarative sections/controls. DLL and image paths must name files inside that same folder. Missing, incomplete or invalid packages are skipped until complete. Copying an arbitrary game's DLL does not create a working game backend: additional games need compatible runtime bindings. Currently the loader has the Far Cry 5 backend.

Start Far Cry 5 and load a single-player save, then click its card. Successful loading hides the library and opens the Qt menu; failures leave the library visible with a message. Close the menu to return to the loader. The menu is a desktop window; borderless/windowed game mode is recommended for using it alongside the game. Exit Wetsox to disable features. If the launcher crashes, features disable after three seconds without updates, on the next rendered frame. Injected modules stay mapped until game exit: restart the game before updating a loaded DLL or switching from the old Nexus module.

## Customization and configs

Appearance changes save automatically after every edit, including each menu's shared/custom setting. Shared appearance edits propagate to linked windows; unlinking restores that window's own appearance. Sun/moon mode remembers its palette and preserves sizing, font, accent and other customization. The logo and window branding use Wetsox and a W.

Feature options and keybinds also restore automatically on reopening/restarting via `.last-session.json`. Use the menu's **Configs** page to save named presets, load them, or delete them. Named files are saved in `configs/<game id>/<name>.json` beside Wetsox.exe. Loading a config updates the current game options immediately. Config names allow letters, numbers, spaces, underscores and dashes. Appearance is saved separately in settings.json rather than overwritten by feature presets.

## Far Cry 5

Options include handheld aiming, hold-key recording, target/body-part selection, sticky lock, aim FOV circle, optional projectile compensation, box/bone ESP, independent colors and outlines, ammunition controls, and on-foot/vehicle camera FOV. ESP reads poses each rendered frame; discovery and velocity sampling run separately. Identified vehicle cover is ignored for aiming; walls and unresolved colliders still block. Projectile penetration is unchanged.

Unsupported weapon ballistic data now falls back to direct aim instead of disabling aiming (including the reported Vector case). Handheld aiming is allowed in vehicles using the same verified look/camera path; it falls back to direct aim because vehicle projectile inheritance is not calibrated. A camera-pose mismatch prevents an unsafe angle write and appears in status. This does not add mounted-turret aiming. Live Vector and handheld-in-car behavior still needs gameplay verification. The engine build remains fingerprint-checked.

## Build and verification

Use Windows x64 MSVC, Qt 6.5+ Quick/Controls/Dialogs/Test, CMake and Python 3. The workspace SDK is used automatically when present:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

The script builds, runs tests, deploys into `out/bin`, and removes known obsolete Nexus runtime copies. `-Run` launches Wetsox. CMake's internal `Nexus` target/QML URI remain private implementation names; the executable is Wetsox.exe.

Seven suites cover appearance persistence, interactions, real QML controls/configs, package discovery/removal, portable config persistence and validation, targeting mathematics, DLL loading/rendering, and cross-process settings transfer. Runtime integration tests use a disposable DX11 host, not the live game. `Wetsox.exe --smoke-test --screenshots <directory>` can capture the UI using temporary settings/configs and the SDK QtTest module.
