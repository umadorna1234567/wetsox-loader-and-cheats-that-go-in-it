# Integration inspection

Inspected 2026-09-19. Intended use: Steam, offline single-player.

## Current revision: Runtime13

Runtime13 is loaded after stopping Runtime12. The user reported controls did not return after closing Runtime12, plus a cursor flickering at screen center since earlier builds. Suspended DirectInput devices are now retained and explicitly reacquired on closing (or after held mouse buttons release); shutdown also resumes them. ImGui's Win32 cursor replacement is disabled so it cannot restore the OS arrow during gameplay. All six suites pass, including an acquired-keyboard capture/close/reacquisition regression check. Live recovery and flicker validation is pending.

## Runtime12 features

Runtime12 is loaded after stopping Runtime11. All six suites passed, including a new DirectInput capture test. Mouse and keyboard DirectInput `Acquire`, `GetDeviceState`, and `GetDeviceData` are intercepted while the menu is open; devices are unacquired to release exclusive input, state becomes neutral and buffered event counts become zero. `ClipCursor` and `SetCursorPos` are intercepted for the foreground game while the menu is visible. Closing restores cursor confinement and waits for held mouse buttons to release before resuming gameplay polling. Initial game telemetry reports 1,068 blocked polls. User validation of free cursor/no click-through/control restoration is pending. Gamepad input is not intercepted.

Sticky aim is enabled by default and configurable. It prefers the previous target while the hold key remains down, subject to the same alive/category/bone/FOV/visibility checks. It releases on key-up, invalid state or obstruction. Automated tests verify keeping a farther candidate and switching after obstruction. Live sticky behavior is pending.

The user specifically chose ignoring only an occupant's own vehicle, with unrelated walls still blocking aim. Target CPawn ridable attachment resolves the vehicle entity/type. Visibility ignores collider results belonging to that vehicle ID, retains all other blockers and still requires a target collision. This exception is configurable and defaults on. Live occupant visibility testing is pending; child vehicle parts with separate entity IDs may need additional verified ownership handling.

ESP now includes Cult/Blessed-only versus all humans, with independent category colors and a separate animal toggle. Classification is based on faction, not dynamic hostility. The previous source-only ESP change is now built and loaded.

The loader now retries transient `ERROR_BAD_LENGTH` module snapshots with a bounded retry, following a reproducible integration-test failure during module loading. Tests passed after the correction.

The user confirmed Runtime11 vehicle FOV changes and restores on disable/exit. The occupied car's entity ID was `0x8557430D8006E460`; wheeled physics component vtable RVA `0x42D83A8` and CFCXVehicle vtable `0x441D978`. Live pointers are diagnostic snapshots only and must be resolved dynamically. Speed multiplier and mounted aiming remain unfinished.

## Runtime11 vehicle FOV

Runtime11 is loaded after stopping Runtime10. All five automated suites passed. Vehicle camera FOV now uses the native override fields in the player's camera aspect (`q(pawn+0x2A68)+0x1A0`): enabled byte +0x80, blend +0x84, elapsed fields +0x88, target radians +0x90. Native setter `0x1BA1EB0` confirms degrees-to-radians conversion. Only a ridable attachment with vehicle component type `0x7EFD7DA9` activates the override. It saves the original fields and restores them on disable, vehicle exit or shutdown, provided the camera identity and ownership fields still match. An existing game override is left alone. A dedicated mutex serializes render updates with shutdown. Live FOV change/restoration testing is pending.

The user confirmed Runtime10 acquires with travel time enabled on the AR-C. Telemetry showed speed 280, gravity -20, drop distance 40, simulation step about 0.01074 seconds and a 0.655-second estimated flight; 1,577 successful aim writes and 571 blocked/unconfirmed cover queries. This verifies acquisition, not long-range impact accuracy or moving-target lead.

Source-only change after Runtime11: ESP now offers Cult/Blessed-only versus all humans, retaining animals as a separate category, and uses separate colors for Cult/Blessed, other humans and animals. This remains faction-based classification rather than dynamic hostility. It has not yet been built or loaded.

## Runtime10 projectile compensation

Runtime10 is loaded after stopping Runtime9; all five automated suites passed. Travel-time aiming is now connected experimentally for recognized on-foot bullet weapons (CFCXWeapon vtable `0x44018E0`, primary firing module vtable `0x4439FA0`, physics enabled). It resolves the equipped inventory item every aim update, reads speed/gravity/drop multipliers, requires zero native firing offset and no mounted inheritance, and rejects unsupported combinations. Maximum flight time is conservatively limited by the unmodified runtime range field divided by speed. The simulation timestep is read from the game timing object at global `0x4EB59F8`, double +0x58.

The targeting core now supports explicit-Euler gravity after a distance threshold, solving each timestep interval analytically. Independent forward-integration tests passed for 20/40/100/220-unit moving-target shots at 30/60/120 Hz. The model currently rejects nonzero inherited velocity with discrete gravity because that makes the distance threshold direction-dependent. Future changes in timestep remain unpredictable; no pixel-perfect claim is justified. Position-derived target velocities use successive registry samples (approximately 250 ms) and a constant-velocity assumption; this does not predict future acceleration or bone animation. Live travel-time acquisition and firing validation are pending.

AR-C launch calibration: user fired while stationary; Runtime9 captured two local launches with speed 280, gravity -20, drop distance 40 and inherited velocity (0,0,0). Player firing origin is body camera position (`q(pawn+0x2A68)+0x190`) plus rotated firing offset (`+0x528` in that object). Observed offset was zero. Native launch caller `0x1E375AE` only reads pawn velocity for weapon byte +0x95 enabled; normal on-foot shots initialize inherited velocity to zero. The observer forwards shots unchanged.

## Runtime9 diagnostics

Runtime9 is loaded after stopping Runtime8. All five automated suites passed. It observes local-owned launches at RVA `0x1E10CF0` with five arguments: firing module, pawn, origin, direction, inherited velocity. It forwards all arguments and the original return register unchanged. Status reports hook installation and launch sample values; sample collection awaits the user's shot. Projectile compensation remains disabled pending launch and integration validation.

Read-only weapon inspection found the active inventory resource ID at inventory+0xA8; item lookup uses the inventory tree (root +0x1A8, sentinel +0x198, key node+0x20, item node+0x28). Weapon inventory items hold an entity reference at +0x48. Native equipped-weapon function `0x1AC35D0` confirms the route through inventory virtual getters +0x2C8 and +0x1F8, followed by the weapon component. The component's primary firing module is at +0x1E0 (also observed +0x50), and module+0x50 points back to the owning weapon. Module+0xC0 is the firing descriptor: physics flag +0x510, speed +0x514, gravity +0x518, drop distance +0x51C. Effective values multiply weapon floats +0x7DC, +0x7E0 and +0x7E4. Two observed bullet configurations were 280/-20/30 and 280/-20/40.

Native launch code adds the fifth-argument inherited velocity to direction times speed. Native update `0x1E3097A` compares accumulated distance (bullet+0x54) against effective drop distance. Once past it, the step computes displacement from the previous velocity and adds gravity times timestep to the next velocity. Therefore a continuous-gravity solver does not exactly reproduce long-range discrete stepping; launch origin, timestep and inherited velocity must be accounted for before claiming accuracy. Bullet records are 0x80 bytes in module+0x7A8, count in upper32(module+0x7B0).

Vehicle research: script GetVehicleSpeed invokes `0x2FA4FB0`, derives the magnitude of a physics component's velocity (vfunc +0x1D0). SetVehicleSpeed invokes `0x2FBFDA0`, normalizes current velocity and writes a scaled vector through vfunc +0x1C0. Repeatedly multiplying that setter would compound speed each frame and is not a suitable direct implementation of a stable speed multiplier. No vehicle writes are enabled.

## Runtime8 cover-check validation

Loaded `build/runtime/Release/FC5MenuRuntime8.dll` after stopping Runtime7. Older versions remain mapped but stopped. All five automated suites passed. These suites validate targeting logic and overlay/loader lifecycle, not native gameplay behavior.

User-confirmed gameplay results through Runtime7: human markers track positions; a cougar receives the independently colored animal marker; named body parts follow animation; no reload preserves the magazine; unlimited reserve ammunition works and normal consumption returns when disabled; direct on-foot aiming moves the view onto the selected head with Projectile travel time off.

Runtime8 adds mandatory obstruction checks before each aim write. It follows the fingerprinted native `IsEntityInRay` path: filter construction RVA `0x70AD30`, mask `0x2DBF`, group zero; world pointer at `0x4F7E828`; query RVA `0x793250`, flags 5. Arguments are origin and displacement, not origin and endpoint. The query acquires/releases the native physics read lock. Hit entries are 48 bytes, collider ID at +0x18, resolved through `0x78D890`; native array cleanup uses `0x6DF020` and `0xE81370`. The selected target must occur in results; every unrelated or unresolved collider blocks aim. Empty, excessive, or unreadable results block aim. Candidates are tested in aim preference order, with at most eight queries per frame. No target receives a write without a successful query in that update.

The user confirmed Runtime8 aims at an exposed target and stops behind cover. This validates the tested scene; partial cover with another selected body part, additional species, terrain, doors, vehicles, fences and scope changes still need coverage. The native entity `IsVisible` flag was inspected and rejected as evidence of unobstructed line of sight. ESP remains independent of aim visibility and can display behind cover.

Remaining: active weapon ballistics, drop-distance semantics, prediction inputs, mounted aiming, vehicle FOV and vehicle speed; verified hostility-based human ESP filtering. Current human aim filters identify Cult/Blessed by faction, not dynamic individual hostility. Settings reset when switching DLL revisions.

The following sections record earlier milestones; their pending-feature descriptions are historical.

Architecture selected by the user: internal x64 DLL. The project builds `FC5Menu.dll` with a DX11 Present/ResizeBuffers overlay, Insert-key menu toggle, startup/status/shutdown exports, and `FC5MenuLoader.exe`. Rendering, resize, input toggling, shutdown/restart, and cross-process loading passed in the local DX11 test host. A captured backbuffer verified the actual menu pixels. Gameplay bindings remain pending.

## Live game check

Loaded into the user's paused offline Far Cry 5 session (PID 8016). Sandbox access initially returned Windows error 5; the same loader succeeded outside the sandbox without changing the game's protections or files. Status reported `Running: 1`, `DX11 ready: 1`, `Visible: 1`, with frames increasing from 439 to 1333. A subsequent stop returned success and status confirmed `Running: 0`, `DX11 ready: 0`, and `Visible: 0` after 2200 frames. Restart returned success and status subsequently confirmed another 1393 rendered frames. The user confirmed that the menu is visible and Insert hides and reopens it. In-game resizing and gameplay features have not been tested.

Shutdown detaches hooks and releases UI/device resources but intentionally retains the DLL and hook trampolines until process exit. This prevents an in-flight callback from returning into unmapped code. The older native-menu-only test still verifies complete unloading when overlay hooks have never started.

- Game folder: `C:\Program Files (x86)\Steam\steamapps\common\FarCry5`
- Steam app: `552520`
- Installed Steam build: `18766066`
- Engine module: `bin\FC_m64.dll`, 250234888 bytes
- Engine module SHA-256: `00833FAE4D5D70213158A146CA98439B28A8E962B934885ECCCD906204880AF2`
- Engine version resource: `1, 0, 0, 0`; this generic version is insufficient as a compatibility identifier.
- Existing `FCModInstaller` directory contains settings, logs, backups, and an empty `ModifiedFilesFC5` directory. Its presence alone does not confirm an installed runtime loader or active packages.
- Module export inspection found editor-prefixed camera/object functions. Their availability does not establish a supported campaign gameplay API or verified calling signatures.
- No game files were changed. The loading utility lives in the workspace and does not require a third-party mod loader.

## Sources examined

- [Resistance mod](https://downloads.fcmodding.com/fc5/resistance-mod/) documents existing ammunition, reload, FOV, and scripting-related packages. Package availability is not proof of the live entity/weapon access required here.
- [CryHook5](https://github.com/Force67/CryHook5) is a community Lua/asset hook archived in December 2019. Compatibility with the inspected engine module has not been established. Its historical signatures and offsets must not be assumed valid.

## Next integration milestone

The read-only runtime adapter now verifies the engine SHA-256 and enumerates campaign pawn entities from the entity registry. The local player's position changed consistently after the user walked, and the live DLL subsequently reported 14 other pawns. All five local tests passed after adding this adapter. These tests cover overlay lifecycle and targeting math; they do not validate gameplay bindings.

The active revision is `build/runtime/Release/FC5MenuRuntime4.dll`, loaded after stopping the earlier overlays. Initial HUD caller filters saw no samples. Call telemetry identified an active caller at RVA `0x5876BA`; after including it and matching the engine's negative-Z handling, 1,887 projection samples matched the engine with zero mismatches. The user confirmed yellow diagnostic markers follow pawn positions. Markers represent entity origins, not skeletons, and expire when camera samples are stale. Tests for occlusion, scopes, different aspect ratios, vehicles, and all species remain outstanding.

The animal lookup is now connected: animals use CEntity with CAnimalAgent, whereas humans use CPawnEntity. The paused cougar scene contained an animal 1.6 game units from the player. Runtime4 reports 18 tracked entities including six animals. All five local tests passed. ESP now has Animals and All humans toggles and separate colors; the user confirmed the cougar receives an Animal marker and changing Animal color changes that marker, with All humans disabled. Other animal species remain untested.

Skeleton mapping, human team classification, weapon ballistics, and control writes remain incomplete. Aiming, ammunition, reload, and vehicle controls are still previews. No gameplay data writes have been implemented.

Complete these bindings before enabling live features. Use module fingerprints to reject unverified builds, preserve original values for reversible overrides, and report unsupported weapons/species explicitly.

## Animal binding evidence

For the fingerprinted engine only: generic CEntity vtable RVA `0x425D400`; component descriptor at entity+0xC8, animal-query slot signed int at descriptor+0x44, component array at entity+0xA8. CAnimalAgent vtable RVA `0x45728B8` and its animal-interface getter RVA `0x2898CE0` distinguish animals from vehicles and humans. The getter was disassembled, not remotely called. Entity positions remain float3 at +0x60. Unrecognized agent variants are omitted rather than guessed.
