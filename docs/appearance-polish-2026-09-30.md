# Appearance polish and repair

The Appearance workspace retains the charcoal/violet styling and user settings while refining the supplied reference layout.

Fixed category pages retaining an empty overview-sized gap, width feedback loops in compact settings, large-font sidebar clipping, an unintended feature-only slider, header/window-button overflow, stale HSV picker hue after preset changes, preset toggles retaining violet regardless of palette, the window border ignoring its visibility toggle, and animated backgrounds continuing when animations were disabled.

Added responsive color-editor columns, editable gradient stops and count, native feature-color rainbow speed access, grouped cursor/sound/border/open-animation settings, brighter selected navigation, consistent card spacing, and frosted rounded panel sampling with actual blur and tint. Game Library preview now uses installed game covers; miniature previews keep a consistent aspect ratio and their own palette opacity/radius.

Validation: full CTest suite (16 tests); added preset color regression and appearance checks for gradient editing, HSV synchronization, large-font viewport bounds and master glass toggle. GPU screenshot review covers 1536x1024, 1020x640 at 150% UI scale, and Liquid glass Effects. Final render logs contain no QML warnings or recursive layout warnings. Screenshots use isolated temporary test settings; personal themes/configs are not overwritten.
