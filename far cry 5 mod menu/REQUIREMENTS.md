# Far Cry 5 mod menu

Status: live overlay, human/animal ESP, named body parts, direct on-foot aiming, cover checking, magazine preservation and unlimited reserve ammo verified by the user. AR-C travel-time acquisition is verified; long-range accuracy and moving-target prediction need live tests. Vehicle FOV is connected and awaits restoration testing. Mounted aiming and vehicle speed remain pending. Target environment: Steam offline single-player.

## Targeting

- Aimbot enable/disable and configurable activation binding.
- Optional sticky aim: preserve the same eligible target while holding the aim key; release on key-up or failed eligibility/visibility.
- Independent target filters for enemy humans, other humans, and animals.
- Selectable hit location: head, chest, abdomen, and pelvis where the model provides a verified mapping. Animal skeletons require species-specific mappings; unavailable hit locations must be reported instead of silently selecting a different body part.
- Adjustable aim FOV, expressed as the full angular diameter of the acquisition cone, separate from camera FOV.
- Adjustable smoothing. Instant aiming and smoothed aiming must be distinguished; smoothing introduces convergence time.
- Require an unobstructed line of sight to the selected body part. Never acquire through walls or objects; release a target as it moves behind cover. Missing or unconfirmed visibility blocks aiming. This applies to humans, animals and eventually mounted weapons.
- Vehicle-occupant exception: ignore the target's own vehicle body/glass while retaining unrelated walls/objects as blockers. The user explicitly chose this over ignoring all cover for occupants.
- Separate controls for bullet-drop compensation, motion prediction, and projectile travel-time compensation. Their dependencies must be clear in the UI: disabling travel time also disables time-based lead and drop compensation.
- Targeting from handheld weapons and vehicle-mounted guns, using the active weapon's muzzle origin, launch speed, gravity, and aiming constraints.
- Preserve weapon spread. Do not change spread, manipulate random spread, redirect bullets after launch, or enlarge hitboxes to force a hit.
- Aim at the calculated interception point. Do not advertise guaranteed hits: future target movement, animation, spread, and model inaccuracies can prevent them.

## ESP

- Enable/disable ESP independently of targeting.
- Enemy-human-only or all-human display, with animals controlled separately.
- Independent colors for enemy humans, friendly/neutral humans, and animals.
- Define entity classification from verified game data; avoid identifying friendly entities as enemies by default.

## Weapons and vehicles

- Unlimited reserve ammunition toggle.
- Separate no-reload toggle, maintaining ammunition in the current magazine.
- Vehicle camera FOV override with a reset to the original value.
- Vehicle speed multiplier, default 1.0, with reset on disable.
- Verify mounted weapons individually, including magazine-based and heat-based weapons where applicable; unsupported behavior must be shown as unavailable.

## Integration prerequisites

The in-game menu must release the mouse and capture mouse/keyboard gameplay input while open, allowing use without pausing and preventing click-through firing. Closing it must restore normal controls without forwarding the closing click.

Before selecting a runtime integration, establish the PC storefront, executable version, installation path, intended game mode, and existing mod loader/SDK.

Live functionality requires verified access to entity classification, skeleton transforms, camera projection, visibility, active weapon ballistics, input/view control, ammunition state, vehicle controls, and camera settings. Do not invent offsets or expose disconnected controls as working features.

Restore modified game values when disabling a feature or unloading. Keep unsupported features unavailable and explain which data or capability is missing.

## Verification

- Test the interception solver against known stationary, moving, gravity-affected, and unreachable trajectories.
- Verify head/chest mappings for humans and supported animal species.
- Test stationary and moving shooters, including vehicle-mounted weapons.
- Check scoped and unscoped FOV calculations and ESP alignment.
- Confirm spread remains unchanged while aim compensation is enabled.
- Confirm ammunition and reload toggles operate independently.
- Confirm camera FOV and vehicle speed return to their original values on disable.

## Reference

The Resistance mod documents existing ammunition, reload, FOV, and scripting-related packages. That does not establish an API for this project's runtime targeting or ESP.

https://downloads.fcmodding.com/fc5/resistance-mod/
