# Far Cry 5 reuse assessment

Integration update: useful source now lives in `games/farcry5` and builds NexusFC5.dll/NexusGameLoader.exe. Nexus does not launch or control the donor application. The old native/ImGui menus and input capture hooks are excluded. The following assessment records the earlier donor review; README.md describes the current integration.

Reviewed 2026-09-25 against the source in `far cry 5 mod menu`.

**Recommendation: reuse the game backend and retain the current Qt/QML interface. A complete restart is unnecessary.**

## Verification performed

- Configured and compiled a fresh x64 Release build with Visual Studio 2026 in `build-fc5-review`.
- Used the distinct DLL name `FC5MenuReview` so the supplied binaries remain untouched.
- All six CTest suites passed: targeting, native menu smoke, input capture, DLL lifecycle, overlay rendering, and loader integration.
- Rendering/loading tests used the project's disposable DX11 host. No live Far Cry 5 process was used for this review.
- Reviewed source, exported interfaces, build configuration, and the supplied integration notes. Historical gameplay confirmations in those notes have not been independently repeated here.

## Reuse map

| Component | Decision | What it provides |
| --- | --- | --- |
| `src/targeting.cpp`, `include/fc5/targeting.hpp` | Reuse | Target selection, filters, hit locations, smoothing, interception math, sticky-target preference, settings structures. Platform-independent C++20 core. |
| `src/runtime.cpp`, `include/fc5/runtime.hpp` | Reuse with compatibility validation | Entity discovery, projection, human/animal classification and bone data, cover checks, on-foot aim, magazine preservation, reserve ammunition, reversible vehicle camera FOV, runtime diagnostics. |
| `src/overlay.cpp` | Split and adapt | Retain useful DX11 rendering/lifecycle and ESP drawing; separate settings/gameplay updates from the old ImGui menu. |
| `src/input.cpp` | Reuse selectively and retest | Mouse/keyboard capture, cursor handling, and device reacquisition. Its existing behavior assumes a menu inside the game window. |
| `src/loader.cpp` | Adapt behind Nexus | Existing load/start/status/stop machinery and target selection. No need for another user-facing loader. |
| `src/menu_win32.cpp`, `src/preview_main.cpp` | Replace as the product UI | Legacy menu and INI persistence; the new Qt/QML interface already supersedes them. Keep temporarily as reference/test fixtures if useful. |
| `tests`, `research`, `tools` | Retain | Useful regression coverage and evidence for game bindings. Research findings still need interpretation and build validation. |

## Feature boundaries

The source contains real implementations for on-foot aiming, entity/animal markers, selected body-part markers, magazine preservation, reserve ammunition, and vehicle camera FOV. The notes report earlier live checks for these, with limitations.

Do not equate the following with completed features:

- Mounted aiming: explicitly rejected/pending in the runtime.
- Vehicle speed: controls are disabled; no complete multiplier implementation.
- Projectile compensation and moving-target lead: implemented math and partial runtime data, but broad live accuracy remains unverified.
- Human hostility: uses Cult/Blessed faction classification, not verified dynamic hostility per character.
- Input recovery/cursor behavior: the notes describe prior gameplay problems and pending live validation after the fixes. Automated input tests pass.
- Our current health, stamina, no-clip, movement multipliers, world/weather/teleport, crafting, and most miscellaneous controls have no corresponding implementation in this backend.
- Marker/bone data is reusable, but our entire Visuals panel is not already implemented. For example, existing named body-part markers are not the same as a complete skeleton-line renderer.
- Aim FOV and smoothing need semantic mapping: the backend uses full cone diameter in degrees and a time constant in seconds; our preview slider ranges/labels are not a direct match.

## Integration work needed

1. Keep Nexus as the standalone Qt/QML application and the FC5 runtime as game-specific code running inside Far Cry 5. Its memory accesses target its own process; loading it into Nexus would not give it the game's state.
2. Introduce a versioned local settings/status bridge, with validation and synchronized settings snapshots. The supplied exports expose start/stop/status, but not an external settings interface. Settings currently live inside `overlay.cpp` and are changed by ImGui.
3. Separate the old ImGui control menu from the runtime update loop. Keep the useful in-game ESP rendering without opening a second, competing menu.
4. Map recorded key bindings to runtime input handling. The old overlay only selects among three fixed activation keys; it cannot consume the new arbitrary keyboard/mouse bindings as-is.
5. Map only supported capabilities into the new UI. Explicitly unavailable features should stay unavailable until implemented and tested.
6. Rework menu visibility/input ownership for the external Qt window, including focus changes and closing behavior. The current Qt menu does not automatically render inside the game's DX11 swap chain.
7. Use the existing engine fingerprint gate and report unsupported game builds clearly. `FC5OverlayStart` currently ignores the return value of `runtime::start()`, so overlay startup alone is not evidence that game features initialized.
8. Use a fresh build directory. Supplied CMake caches refer to the old desktop path. The source uses C++20 while Nexus uses C++17; configure the backend target appropriately.

## Lifecycle and evidence notes

- The overlay pins its DLL until game exit. Stopping hooks is not equivalent to physically unloading the module; preserve that distinction in the new loader.
- Overlay settings are session-local and separate from the legacy preview INI. Nexus should own the eventual persistent feature settings, rather than relying on the old preview configuration.
- Documentation mixes historical runtime revisions: README names Runtime12, integration notes lead with Runtime13, and the supplied runtime cache names Runtime15. Treat the freshly built source and current tests as the review baseline.
- Preserve the bundled ImGui and MinHook license notices if their code is included in the resulting distribution.

Next recommended implementation: extract the supported runtime/settings interface and connect a small, already-implemented feature set to the existing QML menu, then perform explicit offline gameplay validation. Keep the current visual design.
