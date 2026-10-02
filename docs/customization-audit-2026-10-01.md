# Customization audit ? 2026-10-01

## Changes
- Liquid glass has a saved color, rainbow mode/speed, and tint strength. Its highlights and panel overlays use that color. Sidebar opacity now affects the customization workspace too; panel opacity is no longer applied twice.
- Added working Small/Medium/Large window presets, resize controls, remembered window position/size, and edge snapping. Geometry is stored separately for each loader/game window, independent of shared themes. Monitor work areas exclude taskbars, restore onto the correct monitor when available, and clamp off-screen positions. Snapping waits for mouse release.
- Sidebar side/width now affect the customization workspace. Library Grid, Columns, and List layouts differ correctly, including the Add Game card. Added compact-sidebar, library card-height, artwork visibility, and artwork opacity controls.
- Live preview reacts to navigation, sidebar placement, card layout, density, spacing, type size, artwork, section visibility, and glass tint.
- Fixed animations remaining disabled after changing away from the Minimal preset's Off animation setting. Navigation hover None/Brighten/Glow/Scale/Border now follows the selected effect.
- Fixed Pill styling, Ripple also triggering Shrink, the crosshair cursor looking square, and cursor glow only affecting the ring fill. Navigation tooltips respect Show Shortcut Hints.
- Gradient borders repaint when colors, corners, width, opacity, stops, or direction change. Animated borders respect zero thickness. Static grid backgrounds repaint on accent/intensity changes.
- Renamed Shadow Spread to Shadow Offset to match the existing behavior. Disabled controls that have no effect under their current parent setting, and improved compact navigation spacing.
- Existing shared and independent themes, import/export, profiles, and automatic saving remain supported. New settings receive defaults when older profiles are loaded.

## Validation
- Full 21-test suite passed after the C++/QML changes; affected UI tests were rerun after final QML polish.
- Added persistence/shared-theme/window-geometry regression coverage and animation re-enable coverage.
- Expanded appearance UI smoke checks to exercise actual sidebar selection, resize disabling, size presets, glass settings, and preview columns.
- GPU-rendered Effects page using Liquid glass and Layout page at 1100x720 / 150% UI scale inspected; no QML warnings in those preview runs.
- Blur/glow effects require Qt's graphics backend; the software renderer uses a fallback. Multi-monitor restoration uses Qt's monitor work areas; a physical multi-monitor drag test was not performed.

Gameplay DLL behavior was not changed as part of this appearance audit.

## Deployment
The tested backend executable is deployed to out/bin/backend/WetsoxApp.exe. All five Nexus archives were refreshed. ZIP CRC checks and SHA-256 comparisons passed for all 1,376 packaged files against their deployed sources. Preview captures are in docs/appearance-review.

