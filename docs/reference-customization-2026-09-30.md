# Reference-style customization screen

The Appearance page now follows the supplied image: top navigation, a dedicated customization sidebar, twelve miniature theme previews, inline color controls, a large live preview, and compact effects/layout/background/font panels.

Open `out/bin/Wetsox.exe`, go to **Appearance / Customize**, and click **Default** under Theme Presets to use the matching dark navy and purple palette. Existing saved palettes continue to load until you select a preset. Wetsox branding and actual game categories are retained.

**Theme** shows the full dashboard. The other sidebar entries focus its settings; **Advanced** retains the complete appearance controls and search. **Layout** also contains the per-game card editor and hidden-element recovery. Named profiles, duplicate, import/export, shared/local linking and automatic persistence remain connected to the original settings store.

The color square and hue strip edit the selected palette entry immediately. Tabs select text, panels, controls or game feature colors. The live preview follows the edited palette, typography, panel opacity and corner shape without changing gameplay. Its view selector switches between a sample menu and a library view. Preset thumbnails use each preset's palette.

The mountain scenery is a procedural background inspired by the reference. It does not embed the reference image or its VoidCheat branding. The application's existing glass, animated background, opacity and layout options remain available.

Validation includes all 16 project tests, plus an inline color-picker interaction that verifies the live preview follows the saved color. Native Direct3D screenshots are saved in `docs/reference-preview`; the final build log is `build-reference.log`.
