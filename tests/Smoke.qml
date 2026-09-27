import QtQuick
import Nexus
import QtTest

Main {
    id: root
    property int step: 0
    property var fontPicker
    TestCase { id: input; when: false }
    function verify(condition, message) {
        if (!condition) { console.error("SMOKE FAILED: " + message); Qt.exit(1) }
    }
    Timer {
        interval: 1000; running: true; repeat: true
        onTriggered: {
            const game = AppState.games[0]
            switch (root.step++) {
            case 0:
                root.openGame(game)
                root.verify(!!root.windows[game.id], "game window opens")
                root.verify(!root.visible && root.windows[game.id].visible, "opening the menu hides the loader")
                Qt.callLater(function() {
                    const menu = root.windows[game.id]
                    const toggle = input.findChild(menu.contentItem, "aimToggle")
                    root.verify(!!toggle, "aim toggle exists")
                    input.mouseClick(toggle)
                    root.verify(menu.featureStates.aim === true, "mouse click changes toggle")
                    root.verify(!toggle.visualFocus, "mouse click leaves no focus outline")
                    const keybind = input.findChild(menu.contentItem, "aimKeyKeybind")
                    input.mouseClick(keybind)
                    root.verify(keybind.listening, "keybind click starts recording")
                    input.keyClick(Qt.Key_K, Qt.ControlModifier)
                    root.verify(menu.featureStates.aimKey === "Ctrl+K", "recorded key reaches menu state")
                    input.mouseClick(keybind)
                    input.keyClick(Qt.Key_Escape)
                    root.verify(menu.featureStates.aimKey === "Ctrl+K" && !keybind.listening, "Escape preserves binding")
                })
                break
            case 1:
                root.page = "Appearance"
                root.windows[game.id].page = "Appearance"
                root.fontPicker = input.findChild(root.windows[game.id].contentItem, "fontPicker")
                root.verify(!!root.fontPicker && root.fontPicker.previewFonts, "font preview is enabled")
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
                root.windows[game.id].page = "Weapons"
                break
            case 4:
                root.windows[game.id].page = "Vehicles"
                root.windows[game.id].toggleFeature("vehicleFov", true)
                root.verify(root.windows[game.id].featureStates.vehicleFov === true, "preview state changes")
                root.windows[game.id].close()
                root.verify(root.visible && !root.windows[game.id].visible, "closing menu restores loader")
                root.openGame(game)
                root.verify(root.windows[game.id].visible, "menu reopens")
                root.verify(root.windows[game.id].featureStates.vehicleFov === true, "preview state retained on reopen")
                break
            case 5:
                root.windows[game.id].page = "Home"
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
                root.gameEditor.close()
                root.editGame(game)
                root.verify(root.gameEditor.game.id === game.id, "edit game loads selected card")
                break
            case 9:
                root.gameEditor.close()
                console.log("UI smoke passed: windows, themes, tabs, preview controls, and resizing.")
                Qt.quit()
            }
        }
    }
}
