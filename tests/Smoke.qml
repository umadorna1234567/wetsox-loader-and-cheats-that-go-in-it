import QtQuick
import Nexus
import QtTest

Main {
    id: root
    property int step: 0
    property var fontPicker
    TestCase { id: input; when: false }
    function verify(condition, message) {
        if (!condition) { console.error("SMOKE FAILED: " + message); Qt.exit(1); throw new Error(message) }
    }
    Timer {
        id: stepTimer
        interval: 1000; running: true; repeat: false
        onTriggered: {
            const gameIndex = Qt.application.arguments.indexOf("--smoke-game")
            const game = gameIndex < 0 ? AppState.games[0] : AppState.games.find(function(g) { return g.id === Qt.application.arguments[gameIndex + 1] })
            root.verify(!!game, "requested game pack is discovered")
            const jc4 = game.id === "justcause4"
            const kf2 = game.id === "killingfloor2"
            console.log("Smoke step",root.step)
            switch (root.step++) {
            case 0:
                if (Qt.application.arguments.indexOf("--reference-preview") >= 0) AppState.applyPreset("loader", "Reference")
                root.openGame(game)
                if (kf2) root.windows[game.id].page = "Aimbot"
                root.verify(!!root.windows[game.id], "game window opens")
                root.verify(!root.visible && root.windows[game.id].visible, "opening the menu hides the loader")
                Qt.callLater(function() {
                    const menu = root.windows[game.id]
                    const toggle = input.findChild(menu.contentItem, "aimToggle")
                    root.verify(!!toggle, "aim toggle exists")
                    input.mouseClick(toggle)
                    root.verify(menu.featureStates.aim === true, "mouse click changes toggle")
                    root.verify(!toggle.visualFocus, "mouse click leaves no focus outline")
                    root.verify(!input.findChild(menu.contentItem,"aimKeyKeybind"),"inline aim key picker removed")
                    input.mouseClick(input.findChild(menu.contentItem,"aimHotkeyLink"))
                    input.wait(100)
                    root.verify(menu.page === "Hotkeys","feature shortcut opens Hotkeys")
                    const keybind = input.findChild(menu.contentItem, "hotkeyBinding_aim")
                    input.mouseClick(keybind)
                    root.verify(keybind.listening, "keybind click starts recording")
                    input.keyClick(Qt.Key_K, Qt.ControlModifier)
                    root.verify(menu.hotkey("aim").binding === "Ctrl+K", "recorded key reaches menu state")
                    input.mouseClick(keybind)
                    input.keyClick(Qt.Key_Escape)
                    root.verify(menu.hotkey("aim").binding === "Ctrl+K" && !keybind.listening, "Escape preserves binding")
                    stepTimer.restart()
                })
                break
            case 1:
                root.page = "Appearance"
                root.windows[game.id].page = "Appearance"
                root.fontPicker = input.findChild(root.windows[game.id].contentItem, "fontPicker")
                root.verify(!!root.fontPicker && root.fontPicker.previewFonts, "font preview is enabled")
                const preview = input.findChild(root.windows[game.id].contentItem, "liveThemePreview")
                root.verify(!!preview, "live appearance preview exists")
                input.wait(100)
                const square = input.findChild(root.windows[game.id].contentItem, "colorSaturation")
                const previous = AppState.theme(game.id).accent
                input.mouseClick(square, square.width * 0.4, square.height * 0.3)
                root.verify(AppState.theme(game.id).accent !== previous, "inline color picker changes theme")
                root.verify(preview.theme.values.accent === AppState.theme(game.id).accent, "live preview follows edited palette")
                root.fontPicker.popup.open()
                break
            case 2:
                root.fontPicker.popup.close()
                AppState.setThemeValue("loader", "background", "#202030")
                root.verify(root.windows[game.id].backgroundColor.toString() === "#202030", "shared menu updates")
                AppState.setLinked(game.id, false)
                AppState.setThemeValue(game.id, "background", "#302020")
                root.verify(root.backgroundColor.toString() === "#202030", "loader isolated")
                break
            case 3:
                AppState.setThemeValue("loader", "background", "#203020")
                root.verify(root.windows[game.id].backgroundColor.toString() === "#302020", "independent theme retained")
                AppState.setLinked(game.id, true)
                root.verify(root.windows[game.id].backgroundColor.toString() === "#203020", "menu rejoins shared theme")
                AppState.setLinked(game.id, false)
                root.verify(root.windows[game.id].backgroundColor.toString() === "#302020", "local theme restored")
                root.windows[game.id].page = jc4 ? "Teleport" : "Weapons"
                break
            case 4:
                if (jc4) {
                    const action = input.findChild(root.windows[game.id].contentItem, "teleportWaypointAction")
                    root.verify(!!action && action.visible, "JC4 teleport action is in the Wetsox menu")
                    input.mouseClick(action)
                }
                root.windows[game.id].page = jc4 ? "Grapple" : kf2 ? "Movement" : "Vehicles"
                root.windows[game.id].toggleFeature(kf2 ? "playerFov" : "vehicleFov", true)
                root.verify(root.windows[game.id].featureStates[kf2 ? "playerFov" : "vehicleFov"] === true, "preview state changes")
                root.windows[game.id].close()
                root.verify(root.visible && !root.windows[game.id].visible, "closing menu restores loader")
                root.openGame(game)
                root.verify(root.windows[game.id].visible, "menu reopens")
                root.verify(root.windows[game.id].featureStates[kf2 ? "playerFov" : "vehicleFov"] === true, "preview state retained on reopen")
                const shortcut = input.findChild(root.windows[game.id], "gameMenuShortcut")
                root.verify(!!shortcut,"menu shortcut exists")
                shortcut.activated()
                input.wait(50)
                root.verify(root.windows[game.id].visibility === Window.Minimized && !root.visible,"shortcut minimizes game menu without showing loader")
                shortcut.activated()
                input.wait(50)
                root.verify(root.windows[game.id].visibility === Window.Windowed && !root.visible,"shortcut restores menu without showing loader")

                break
            case 5:
                if (jc4) {
                    const menu = root.windows[game.id]
                    root.verify(!game.sections.some(s => s.name === "Movement"), "JC4 Movement tab removed")
                    const grapple = input.findChild(menu.contentItem, "grappleRangeToggle")
                    root.verify(!!grapple && grapple.visible, "Unlimited grapple range appears in Grapple")
                    input.mouseClick(grapple)
                    root.verify(menu.featureStates.grappleRange === true, "Grapple range control updates state")
                    menu.page = "Hoverboard"
                    Qt.callLater(function() {
                        const speed = input.findChild(menu.contentItem, "hoverboardSpeedSlider")
                        root.verify(!!speed && speed.visible, "Hoverboard speed appears in Hoverboard")
                    })
                } else root.windows[game.id].page = "Home"
                root.windows[game.id].width = 1020
                root.windows[game.id].height = 640
                AppState.setThemeValue(game.id, "fontSize", 20)
                AppState.applyPreset("loader", "Daylight")
                AppState.setThemeValue("loader", "fontSize", 20)
                AppState.setThemeValue("loader", "spacing", 28)
                AppState.setThemeValue("loader", "compactSidebar", true)
                root.width = 930; root.height = 640
                break
            case 6:
                root.windows[game.id].page = "Configs"
                root.page = "Library"
                break
            case 7:
                root.windows[game.id].close()
                root.verify(root.visible, "loader returns for library editing")
                root.editGame({})
                root.verify(root.gameEditor.visible, "add game editor opens")
                break
            case 8:
                root.windows[game.id].page = "Configs"
                root.windows[game.id].toggleFeature("bone", "Chest")
                root.verify(root.windows[game.id].featureStates.bone === "Chest", "choice state survives section switching")
                const menu = root.windows[game.id]
                const name = input.findChild(menu.contentItem, "configName")
                root.verify(!!name, "config name input exists")
                name.text = "Smoke config"
                input.mouseClick(input.findChild(menu.contentItem, "saveConfig"))
                root.verify(ConfigStore.names(game.id).indexOf("Smoke config") >= 0, "config saved from menu")
                root.verify(ConfigStore.load(game.id,"Smoke config").values._hotkeys.aim.binding === "Ctrl+K","config preserves hotkeys")
                root.gameEditor.close()
                root.editGame(game)
                root.verify(root.gameEditor.game.id === game.id, "edit game loads selected card")
                break
            case 9:
                root.gameEditor.close()
                const targetMenu=root.windows[game.id]
                root.showGame(game)
                targetMenu.openHotkey(jc4 ? "teleportObjective" : kf2 ? "playerFov" : "vehicleFov")
                input.wait(100)
                const list=input.findChild(targetMenu.contentItem,"hotkeyList")
                root.verify(list.contentY>0,"hotkey jump scrolls to offscreen entry")
                const targetCard=input.findChild(targetMenu.contentItem,"hotkey_"+(jc4?"teleportObjective":kf2?"playerFov":"vehicleFov"))
                root.verify(!!targetCard&&targetCard.border.width===2,"destination hotkey is highlighted")
                console.log("UI smoke passed: windows, themes, tabs, preview controls, and resizing.")
                Qt.quit()
            }
            if (root.step > 1 && root.step < 10) stepTimer.restart()
        }
    }
}
