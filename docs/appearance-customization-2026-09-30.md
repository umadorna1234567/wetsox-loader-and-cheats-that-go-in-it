# Appearance and layout customization — 2026-09-30

Launch `out/bin/Wetsox.exe`, open **Appearance**, and select **Liquid glass** to try the new material. Shared and independent themes retain the previous linking behavior. All appearance changes save automatically; presets replace the current appearance, so save a named profile before switching if you want to return to it.

Implemented controls:

- Glass master toggle; independent window background, sidebar, section/card, and control opacity; adjustable blur of the menu background; layered highlights, gradients, shadows, glow, and static or animated gradient borders.
- Dark, OLED black, Light, Cyberpunk, Minimal, Liquid glass, Halloween, Christmas, and the previous color presets, with palette previews and randomization.
- Full palette, enabled/disabled colors, two/three gradient colors, direction and intensity. Every appearance color and game-menu color has an independent rainbow toggle and speed. Gameplay colors animate through the existing session interface without writing a config on each animation frame.
- Animation level/speed, page fade/slide/zoom/crossfade, toggle slide/fade/spring, hover and click effects, and menu entrance effects.
- Fonts, weight, size and letter spacing; 75–150% scaling; density; left/right sidebar, width and icon rail; top or floating navigation; grid, columns and list card layouts.
- Solid, gradient, image, aurora, particles, grid, waves, Halloween and Christmas backgrounds. Automatic game colors and artwork. Custom cursor shapes/trails and optional Windows sound effects with volume and individual event toggles.
- Named appearance profiles, deletion, and existing JSON import/export. Older appearance files receive defaults for the new settings. Imported values are validated and bounded.
- Game-setting search across categories, recently changed settings, configurable Home cards, and visibility switches for navigation, title bar, branding, search, banner, status, footer, artwork, section headers/boxes, window border and adding games. A recovery Appearance button remains available if navigation/title-bar controls are hidden.

In a game menu, press **Edit layout**. Drag card headings to reorder categories; drag a card's bottom handle to add height. Right-click a card for visibility, individual color and column-width properties. Right-click settings for visibility and label colors. **Appearance → Customize layout** restores hidden elements, changes Home visibility, chooses one/two/three column widths and resets layout. Hiding a gameplay setting preserves its value; it does not disable the underlying feature.

The menu shortcut defaults to **F6**. In a game session it closes/reopens the menu, including while the game has focus. In the standalone library it switches between Appearance and Library. Supported shortcuts use letters, digits, F1–F24 or navigation keys, optionally with modifiers. The shortcut hint can be hidden.

The glass treatment is Apple-inspired. Blur samples the application's artwork and animated background; it is not Apple's proprietary Liquid Glass implementation or a native desktop acrylic blur. Software rendering retains the visible UI, transparency and highlights while omitting GPU blur/shadows. Layout editing uses a responsive grid with card ordering, column spans and added height, rather than arbitrary placement of every individual widget.

Validation: theme migration, bounds and invalid imports, shared/local isolation, named-profile persistence/export/import, existing loader/game-menu interaction tests, and the new appearance smoke test. The appearance test exercises actual pointer resizing, hiding/restoring controls, profile restoration, search, navigation, scaling, backgrounds and transitions. The full CTest suite includes 16 tests; screenshots and the final build log are stored beside this report and in `build-appearance.log`.

For a fresh local build, run `powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_appearance.ps1`. It builds in `build-appearance`, deploys the Qt runtime, runs all tests and installs to `out` only after tests pass. `package.ps1` rebuilds the local ZIP downloads.

Rendering implementation references: [Qt MultiEffect](https://doc.qt.io/qt-6.8/qml-qtquick-effects-multieffect.html), [Qt HoverHandler](https://doc.qt.io/qt-6/qml-qtquick-hoverhandler.html), and [Windows PlaySound buffer lifetime](https://learn.microsoft.com/it-it/previous-versions/dd743680%28v%3Dvs.85%29).

Final verification: all 16 CTest tests passed with no QML warnings. The installed executable also passed the appearance smoke test on the native Windows Direct3D 11 renderer (AMD Radeon RX 6600), exit code 0. Inspected screenshots are in docs/appearance-preview-gpu; menu-1500.png shows the glass dashboard and menu-2500.png shows editing a card. Live gameplay and subjective sound feedback were not part of these UI checks.

