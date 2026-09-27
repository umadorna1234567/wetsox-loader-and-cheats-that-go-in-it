# Far Cry 4 port â€” binding verification

Work in progress. The package manifest is not a completed gameplay module.
Do not deploy or publish it until the runtime and live checks are complete.

Engine: `FC64.dll`, SHA-256
`7304078fb5bfec149c93804f842295fa8a6f9913e6b280523c74389a4272803a`.
Addresses below are module-relative. Engine sections are unpacked at runtime;
local read-only snapshots under `build-Release/fc4-research` are research inputs,
not distributable package files. No third-party trainer implementation copied.

| Binding | Evidence |
| --- | --- |
| Local player | `191c580`: manager `[2e24c58]`, controller `[[manager+8]]`, ref `[[controller+8]+18]`, entity `[ref+10]` |
| Entity table | `[2dd48f8]`, bucket count +8, heads +10; node next +0, ID +8, ref +10 |
| Entity identity / transform | ID +8, matrix +20, position +50; CPawnEntity vtable 280d7f0 |
| Components | array +98, count +a0; component owner ref +8 |
| CPawn | vtable 27eeab0; state pointer +70 |
| Graphic | vtable 276e8a0; world matrix +120 |
| Bones | graphic +b0 array, +b8 count; stride 70, CRC32 name +0, local position +60; native getters 37c370/37c3a0 |
| Current look | c02a30 returns state+600; angles +8c |
| Desired look | c02a20 returns state+400; angles +8c |
| Look setter | 1927e80 writes BOTH current and desired angles |
| Eye pose | state+1d0 position, +1c0 quaternion; coordinate conversion still requires projection validation |
| Projection | 16d960(camera,result,point,model), matrix camera+170; active gameplay camera still requires live verification |
| Camera FOV | camera=state+1e0; native 1927980 setter / 1927ca0 getter; override block camera+80..97 |
| Health | counters component +40 pointer, health +18; native 1922ae0 and 1138230 |
| Active weapon | state+10c8 weapon aspect; ref +190, entity ref+10; CWeapon component (native ba48d0) |
| Magazine | CWeapon +a0 state pointer, magazine +104 (d4c150), setter d4fb80 (needs inspection) |
| Vehicle | cc17e0 reads state+800 relation table, key fd1ba782; full seat binding pending |

Native class metadata functions: CPawn 87dc80, CWeapon 8cfc10,
CFCXCountersComponent d60e30. Component lookup 7248e0.

Pending: faction/animal classification, ammo ownership and reserve behavior,
projectile properties, ray query/vehicle cover, camera selection, runtime,
loader routing, tests and live feature validation. Labels use Royal Army /
Royal Guard; this alone does not verify runtime faction values.

## Aiming crash investigation â€” 2026-09-26

The local dump `FarCry4.exe.9884.dmp` reports an access violation reading
address 8 at `FC64+1e958ac`, reached through `4af730 -> 4a8110 -> 483a83`
from `WetsoxFC4Live` visibility checks. The crashing render thread (12564)
has null Havok memory-router TLS (slot 32) and monitor TLS (slot 33).
These slot numbers are observations only; runtime reads the indices at
`32b77e8` and `32c1b68` on each scope entry.

The native scoped worker at `1e6ede0` establishes the matching lifecycle:
128-byte aligned router constructed by `1dfbea0`; memory system `[32c1b60]`
vtable +18 thread initialization with flags 3; `1e02190` thread setup;
work; `1e021e0` thread cleanup; memory system vtable +20 with flags 3;
router destruction by `1dfdf70`.

The runtime now uses this lifecycle around an aiming pass's visibility
queries. Existing complete contexts are reused; incomplete existing
contexts reject the query without an aim write. Owned contexts are cleaned
up on all ordinary returns. The scoped lifecycle regression test and FC4
test-host loading test pass. Actual in-game aiming after this change still
requires verification; this does not establish general physics-thread safety.

## Death / melee crash investigation â€” 2026-09-26

`FarCry4.exe.20300.dmp` shows a separate invalid read at `FC64+9a6dad`.
The saved stack includes `d4bf7b` and Wetsox's No Reload caller: the native
capacity function traversed the player's perk data while the user reported
death with a machete equipped. This establishes the failing capacity path;
it does not establish whether death, melee selection, or their timing was
the sole trigger.

No Reload no longer calls `d4bf00` or `d4fb80`. It preserves a bounded,
observed magazine count using checked process reads/writes, validates the
equipped weapon and owner identities again immediately before writes, and
sets the weapon's ammo-dirty byte. It does not invoke strategy callbacks.
Initially empty magazines wait for a normal reload. Death, invalid state,
and changed identities discard ownership; only the same currently equipped
live weapon can have its owned spare round restored when disabling.
Ammo and FOV overrides and final aim writes now also check player lifetime.

Regression coverage includes death, switching, recycled player/weapon
addresses, empty magazines, single-shot spare-round restoration, pickups,
and invalid counts. In-game death/respawn and ammo behavior still need
verification with the rebuilt development DLL.

## AK-47, wildlife, and animated-camera corrections — 2026-09-26

Captured AK-47 config +1f8 is 3437876748 (a stat lookup key), while
+1fc is 30 and +200 is 1 (ammo per shot). The previous capacity-range guard
therefore rejected every magazine update. The guard now uses the verified
ammo-per-shot field and still preserves only actually observed loaded ammo.

The nearby living ground bird is CEntity with CAnimalAgentFC3 (CRC afec1e66),
not CAnimal. CFCXCountersComponent lookup reports health 15. Its graphic has
12 joints, parent index +4, pose +60, stride 70. Wildlife classification now
uses that component. Wildlife bounds and bone ESP use native joints/parents,
while named Head/Spine/etc remain available for aim hit locations.

Projection validity no longer depends on agreement with the base look angles.
A finite perspective camera near the local eye remains usable through damage,
rope climbing, sliding, and knockdown; aim retains its separate alignment guard.
Camera origin is recovered from clip X=Y=W=0, and the camera's forward vector
is the normalized W column. Tests cover shifted cameras and invalid matrices.

The captured rendered forward and native pose-quaternion forward agree, but
both differ from the old base-look direction by roughly 0.0046 in vector
length. Aim now solves from the rendered eye/forward and applies that rotation
correction to the base look direction, rather than assuming the two are equal.
This targets the observed aim bias; it does not remove random weapon spread.

AK-47 strategy vtable 28037e0, direction virtual +1d8 -> d9fd40. Native firing
at daeb90 multiplies direction by range and submits the complete ray to 4af770.
The verified AK bullet model is hitscan, so its time, lead, and drop corrections
are zero regardless of prediction switches. Physical projectile bindings for
other strategies remain unimplemented; those still report direct-aim fallback.

Live inspection above predates the user's unrelated PC restart. The rebuilt
feature changes need another in-game test, particularly aim accuracy, wildlife
skeletons, no reload under sustained fire, and ESP during the reported actions.

## Shoot-through foliage visibility filter — 2026-09-26

The visibility query used generic interaction mask 0x2dbf. The verified AK
firing path loads mask 0x1fbb at FC64+daed11 and constructs its filter at
+daed39. Visibility now uses 0x1fbb with its existing synchronous query and
physics-thread lifetime. Target confirmation, local-player exclusions and
solid/unknown-hit rejection remain in place. This matches bullet collision
categories; it does not implement weapon/material penetration through hits
that the bullet query still reports. In-game foliage verification is pending.

### Detailed branch visibility query (2026-09-26)

The mask-only change was insufficient. In process 15736, the same eye-to-head
segment for target b55c12e7800e5a00 returned an entity-less type-8 proxy (collider
7ed, vtable RVA 2733040) with query flags 5. Flags 4 and 6 instead returned the
actual target collider d87. FC64+4a8110 selects different collectors with bit 0:
483a10 when set, 4a7a80 when clear. The production query now uses flags 6,
retaining bounds handling and additional geometry while using the detailed
collector. Another sampled segment still returned terrain (collider 28) before
the target; flags 6 also included additional geometry missing from flags 4.
No blanket entity-less/type-8 exception was added. Existing target confirmation
and unknown/solid obstruction rejection remain intact. Diagnostic DLLs in
fc4-research are not distributable game modules. Live aim behavior still needs
confirmation after replacing the loaded module.

Candidate DLL compiled and passed the disposable-host loading/status/stop/restart test, fc5_targeting, and fc4_physics_thread. Replacement is pending game exit.

After game exit, replaced build-Release/cheats/farcry4/WetsoxFC4Live.dll with the verified candidate (SHA256 AFD2CF25349550E02A4F570D15DD06B3090AFA02D6D04FBBFE3EC66222C2D618). The installed DLL passed loading/status/stop/restart in the disposable host. Removed temporary candidate and diagnostic DLLs. User gameplay confirmation remains pending.

### Wildlife aim points (2026-09-26, in progress)
The recorded 12-joint ground bird has Head and Spine but lacks Spine2/Hips.
Several live 31/58/72-joint animal rigs also lack Spine2. Animal-only missing
Chest/Pelvis aim locations now fall back to the validated Spine, preserving
real joints and leaving the ESP skeleton unchanged. New fc4_aim_locations
coverage verifies selection for all four locations, animal filtering, invalid
body rejection and preservation of real joints. Candidate compiled; loading
and targeting tests passed. This candidate is not deployed yet.
The user uses Head, so this fallback alone does not resolve the reported issue.
Live detailed rays hit animal heads and resolve the correct entity IDs; some
sampled rays are blocked by terrain. Awaiting a reproducible failed Head lock
with the bird inside the configured cone. Do not describe Head as fixed.

### Main-loader promotion (2026-09-26)
The user confirmed bird aiming now works and requested main-loader deployment
and a separate Nexus pack. The release target is now WetsoxFC4.dll, installed
with game.json, cover.jpg and dependency licenses into out/bin/cheats/farcry4.
The latest missing-animal-body-point fallback is included. package.ps1 builds
Wetsox-FarCry4.zip separately and refreshes the compatible loader download.
Earlier investigation notes above describe historical snapshots; the later
user confirmation supersedes the pending Head-lock reproduction request.
Physical-projectile prediction remains the previously documented direct-aim
fallback; this promotion does not add unverified ballistics bindings.
