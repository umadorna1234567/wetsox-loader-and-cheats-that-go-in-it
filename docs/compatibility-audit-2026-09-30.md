# Compatibility and fix-first audit

Audited the current implementation against Desktop/fix first.txt.

| Requested fix | Finding |
| --- | --- |
| FC4 NO MOD / installed-pack badge | Valid FC4/FC5/JC4 manifests advertise installed/ready; fixed an additional metadata mismatch that could advertise READY for an incorrect/unsupported backend. |
| Pack missing after extracting a download wrapper | Shared discovery recursively finds nested manifests; tests now cover all three game IDs, missing artwork, invalid backend identity, and direct-install precedence over duplicates. Both library and launch use this scanner. |
| Backend folder and portable layout | Root remains Wetsox.exe, backend, cheats, configs. Helpers, Qt app and configs resolve relative to the install root rather than the current directory. |
| FC4 aim alignment and 1440p reports | Runtime uses camera direction angle, not projected screen offset. Camera tests cover 4:3, 16:9, 21:9 and 1x/2x/4x/8x zoom. A separate camera-position check still applies. This is not proof that all remote users' failures are resolved. |
| Controller binds | Shared Xbox/XInput and known Sony native input reaches FC4/FC5 aim and JC4 aim/teleport. Every named controller code is tested through both Far Cry IPC sessions. Prior physical DualSense L2/R2/Cross capture was verified. A live FC4 L2 test now confirms the runtime enters target selection on press and returns to Off on release. Fixed the standalone Win key parser. |
| Just Cause 4 integration | All 22 pack controls/actions are present; 17 gameplay options have bridge assignments. Grapple/Hoverboard tabs, image and licenses are present. Original Solis menu/proxy are excluded. Port uses torso/mouse-step aim; DLC movement options still require their content. Feature-by-feature gameplay is not established by the UI/math tests. |

## Build checks

FC4 verifies FC64.dll SHA-256 7304078fb5bfec149c93804f842295fa8a6f9913e6b280523c74389a4272803a.
FC5 verifies FC_m64.dll SHA-256 00833fae4d5d70213158a146ca98439b28a8e962b934885ecccd906204880af2.
Both installed engine files passed the shared read-only diagnostic. These checks are independent of computer identity and display resolution. Different whole-file hashes may sometimes be compatible, but no alternate engine build was available to verify its offsets; acceptance was not widened.

Hashing and expected values now share one implementation between loader and runtimes. The loader checks before loading a gameplay module and distinguishes missing engine, unreadable file, unknown fingerprint and later initialization failure. A --check-build command works without starting a game or loading a cheat. Failure messages include actual and expected hashes. Tests exercise known SHA-256 vectors, mismatch rejection, default failure, missing files, and diagnostic content.

JC4 keeps its executable timestamp and code-byte guards. The local executable passed timestamp 1565905714, engine marker and all 11 byte checks (Steam build 4110618). Failure logging now records an unexpected timestamp or failing preferred code address. No new build was enabled and no guard bypass was added.

## Verification and limits

The automated suite has 17 tests, including native loading/status/stop/restart in disposable Far Cry test hosts, UI interactions, package/config handling, native controller mapping and camera math. Test-host loading verifies the bridge and lifecycle, not real game offsets. Audit logs: build-audit-final.log and test-audit-final.log.

The affected users' builds and gameplay conditions are still needed to confirm their particular aim failures. Live FC4 verification completed with the updated deployed module (PID 12056): the build gate and hooks initialized, frames/projections advanced, and 18 entities were discovered. L2 presses reached aimState 4 (no eligible target) and releases returned to aimState 0. No alignment rejection occurred in the recorded sample. No eligible target was inside the aim cone and aimWrites stayed zero, so this verifies activation, not target-lock accuracy. The temporary settings were restored without changing config files. Log: controller-live-audit.log. FC5/JC4 were not live-tested again in this audit.
