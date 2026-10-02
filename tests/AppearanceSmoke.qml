import QtQuick
import Nexus
import QtTest

Main {
    id: root
    property int step: 0
    property var menu
    property var firstSection
    property var firstControl
    TestCase { id: input; when: false }
    function verify(condition, message) {
        if (!condition) { console.error("APPEARANCE FAILED: " + message); Qt.exit(1) }
    }
    Timer {
        interval: 700; repeat: true; running: true
        onTriggered: {
            switch (root.step++) {
            case 0:
                AppState.applyPreset("loader", "Liquid glass")
                root.openGame(AppState.games[0])
                root.menu = root.windows[AppState.games[0].id]
                root.verify(!!root.menu, "game menu opened")
                root.firstSection = menu.sections.reduce(function(best, s) { return s.controls.length < best.controls.length ? s : best }, menu.sections[0])
                root.firstControl = firstSection.controls[0]
                break
            case 1:
                root.verify(AppState.theme("loader").glass, "glass preset enabled")
                input.mouseClick(input.findChild(menu.contentItem, "editLayout"))
                root.verify(menu.editMode, "edit mode button works")
                menu.page = firstSection.name
                AppState.setThemeValue("loader", "cardOpacity", 0.25)
                AppState.setThemeValue("loader", "blurStrength", 0.7)
                AppState.setThemeValue("loader", "accentRainbow", true)
                AppState.setThemeValue("loader", "animatedBorders", true)
                AppState.setThemeValue("loader", "layoutElements", ({["control:" + firstControl.key]: {hidden:true}, ["section:" + firstSection.name]: {extraHeight:40,color:"#384669"}}))
                break
            case 2:
                const panel = input.findChild(menu.contentItem, "panel_" + firstSection.name)
                root.verify(!!panel && panel.visible, "card remains visible")
                root.verify(!input.findChild(panel, "setting_" + firstControl.key), "hidden control removed")
                const handle = input.findChild(panel, "resizeCard")
                root.verify(!!handle && handle.visible, "card resize handle available")
                input.mousePress(handle, handle.width / 2, handle.height / 2)
                const point = handle.mapToItem(menu.contentItem, handle.width / 2, handle.height / 2)
                input.mouseMove(menu.contentItem, point.x, point.y + 30, 80)
                input.mouseRelease(menu.contentItem, point.x, point.y + 30)
                root.verify(AppState.theme("loader").layoutElements["section:" + firstSection.name].extraHeight > 40, "drag resize saves height")
                AppState.setThemeValue("loader", "sectionOrder", menu.sections.slice().reverse().map(function(s) { return s.name }))
                AppState.saveAppearance("loader", "Glass layout")
                menu.page = "Appearance"
                break
            case 3:
                AppState.applyPreset("loader", "Light")
                root.verify(AppState.loadAppearance("loader", "Glass layout"), "profile reload succeeds")
                root.verify(AppState.theme("loader").glass, "profile restores glass")
                root.verify(AppState.theme("loader").layoutElements["control:" + firstControl.key].hidden, "profile restores hidden settings")
                AppState.setThemeValue("loader", "layoutElements", ({}))
                menu.editMode = false; menu.page = "Home"
                break
            case 4:
                const search = input.findChild(menu.contentItem, "settingSearch")
                root.verify(!!search, "game setting search exists")
                search.text = firstControl.label
                break
            case 5:
                root.verify(!!input.findChild(menu.contentItem, "panel_" + firstSection.name), "search finds related card")
                input.findChild(menu.contentItem, "settingSearch").text = "no-matching-setting-91283"
                break
            case 6:
                root.verify(!input.findChild(menu.contentItem, "panel_" + firstSection.name), "unmatched cards filtered")
                input.findChild(menu.contentItem, "settingSearch").text = ""
                AppState.setThemeValue("loader", "navigationStyle", "Icon rail")
                AppState.setThemeValue("loader", "sidebarSide", "Right")
                AppState.setThemeValue("loader", "uiScale", 1.5)
                AppState.setThemeValue("loader", "density", "Spacious")
                menu.width = 1020; menu.height = 640
                break
            case 7:
                AppState.setThemeValue("loader", "navigationStyle", "Top tabs")
                AppState.setThemeValue("loader", "cardLayout", "List")
                AppState.setThemeValue("loader", "pageTransition", "Crossfade")
                menu.page = "Appearance"
                break
            case 8:
                AppState.applyPreset("loader", "Christmas")
                AppState.setThemeValue("loader", "customCursor", true)
                AppState.setThemeValue("loader", "cursorTrail", true)
                menu.page = "Home"
                break
            case 9:
                AppState.applyPreset("loader", "Halloween")
                AppState.setThemeValue("loader", "navigationStyle", "Floating")
                AppState.setThemeValue("loader", "pageTransition", "Slide")
                menu.page = "Appearance"
                break
            case 10:
                AppState.applyPreset("loader", "Cyberpunk")
                AppState.setThemeValue("loader", "pageTransition", "Zoom")
                AppState.setThemeValue("loader", "showBanner", false)
                AppState.setThemeValue("loader", "showFooter", false)
                AppState.setThemeValue("loader", "showSectionHeaders", false)
                menu.page = "Home"
                break
            case 11:
                AppState.setThemeValue("loader", "showSectionBoxes", false)
                AppState.setThemeValue("loader", "showSearch", false)
                AppState.setThemeValue("loader", "showArtwork", false)
                AppState.setThemeValue("loader", "animationMode", "Off")
                menu.close(); root.page = "Library"
                break
            case 12:
                AppState.applyPreset("loader", "Reference")
                root.showGame(menu.game);menu.width=1536;menu.height=1024;menu.page="Appearance"
                break
            case 13:
                const appearance=input.findChild(menu.contentItem,"appearancePanel")
                appearance.category="Colors"
                break
            case 14:
                const workbench=input.findChild(menu.contentItem,"colorWorkbench")
                workbench.setting="gradient2"
                workbench.applyColor(Qt.rgba(0.2,0.8,0.6,1))
                root.verify(AppState.theme(menu.game.id).gradient2==="#33cc99","gradient swatch edits the correct stored color")
                AppState.applyPreset("loader","Blue")
                root.verify(Math.abs(workbench.hue-workbench.hsv(workbench.selectedColor).h)<0.001,"color picker hue follows preset changes")
                menu.width=1020;menu.height=640
                AppState.setThemeValue("loader","uiScale",1.5)
                input.findChild(menu.contentItem,"appearancePanel").category="Fonts"
                break
            case 15:
                const fontPicker=input.findChild(menu.contentItem,"fontPicker")
                const fontPoint=fontPicker.mapToItem(menu.contentItem,0,0)
                root.verify(fontPicker.width>80&&fontPoint.x+fontPicker.width<=menu.width&&fontPoint.y>=0&&fontPoint.y+fontPicker.height<=menu.height,"font picker fits a narrow window with large UI scale")
                input.findChild(menu.contentItem,"appearancePanel").category="Effects"
                break
            case 16:
                const glass=input.findChild(menu.contentItem,"appearance_glass")
                root.verify(!!glass&&glass.visible,"glass master toggle is reachable in Effects")
                input.mouseClick(glass)
                root.verify(AppState.theme(menu.game.id).glass,"Effects master toggle updates the saved theme")
                AppState.applyPreset("loader", "Reference")
                AppState.setThemeValue("loader", "layoutElements", ({}))
                AppState.setThemeValue("loader", "sectionOrder", ["Camera", "Vehicles", "Weapons", "Aimbot", "Visuals"])
                menu.width=1536;menu.height=1024;menu.page="Home"
                break
            case 17:
                const weapons=input.findChild(menu.contentItem,"panel_Weapons")
                const visuals=input.findChild(menu.contentItem,"panel_Visuals")
                const aim=input.findChild(menu.contentItem,"panel_Aimbot")
                const layout=input.findChild(menu.contentItem,"homeCardLayout")
                root.verify(Math.abs(visuals.y-weapons.y-weapons.height-layout.gap)<1,"Visuals stacks immediately below Weapons")
                root.verify(visuals.y<aim.y+aim.height,"taller neighboring card does not push Visuals down")
                AppState.setThemeValue("loader","layoutElements",({"section:Weapons":{extraHeight:100}}))
                break
            case 18:
                const resized=input.findChild(menu.contentItem,"panel_Weapons")
                const moved=input.findChild(menu.contentItem,"panel_Visuals")
                const compact=input.findChild(menu.contentItem,"homeCardLayout")
                root.verify(Math.abs(moved.y-resized.y-resized.height-compact.gap)<1,"resizing reflows the next card without a gap")
                AppState.setThemeValue("loader","layoutElements",({"section:Visuals":{span:3}}))
                break
            case 19:
                const wide=input.findChild(menu.contentItem,"panel_Visuals")
                const grid=input.findChild(menu.contentItem,"homeCardLayout")
                const neighbor=input.findChild(menu.contentItem,"panel_Aimbot")
                root.verify(Math.abs(wide.width-grid.width)<1,"full-width cards span all columns")
                root.verify(wide.y>=neighbor.y+neighbor.height,"full-width card clears taller neighbors")
                menu.page="Appearance"
                input.findChild(menu.contentItem,"appearancePanel").category="Layout"
                AppState.setThemeValue("loader","sidebarSide","Left")
                AppState.setThemeValue("loader","sidebarWidth",222)
                break
            case 20:
                const sideChoice=input.findChild(menu.contentItem,"appearance_sidebarSide")
                root.verify(!!sideChoice&&sideChoice.visible,"sidebar setting is reachable")
                input.mouseClick(sideChoice)
                input.keyClick(Qt.Key_End)
                input.keyClick(Qt.Key_Return)
                break
            case 21:
                root.verify(AppState.theme(menu.game.id).sidebarSide==="Right","sidebar choice changes the setting through input")
                const sidebar=input.findChild(menu.contentItem,"appearanceSidebar")
                root.verify(sidebar.x>300,"customization sidebar actually moves to the right")
                AppState.setThemeValue("loader","windowSize","Small")
                AppState.setThemeValue("loader","resizable",false)
                AppState.setThemeValue("loader","glassColor","#29b8cc")
                break
            case 22:
                root.verify(menu.width<=1100&&menu.height<=720,"window preset resizes the live menu")
                root.verify(!input.findChild(menu.contentItem.parent,"windowResizeEdges").enabled,"resizing can be disabled")
                AppState.setThemeValue("loader","resizable",true)
                AppState.setThemeValue("loader","cardLayout","List")
                input.findChild(menu.contentItem,"appearancePanel").category="Theme"
                break
            case 23:
                const live=input.findChild(menu.contentItem,"liveThemePreview")
                root.verify(live.previewColumns===1,"preview follows list layout")
                root.verify(live.value("glassColor")==="#29b8cc","preview uses saved glass tint")
                root.verify(input.findChild(menu.contentItem.parent,"windowResizeEdges").enabled,"resizing re-enables")
                const appearanceScroll=input.findChild(menu.contentItem,"appearanceScroll")
                appearanceScroll.contentItem.contentY=500
                input.findChild(menu.contentItem,"appearancePanel").category="Advanced"
                break
            case 24:
                const advanced=input.findChild(menu.contentItem,"advancedAppearance")
                const advancedScroll=input.findChild(menu.contentItem,"appearanceScroll")
                root.verify(advanced.visible && advanced.y<2,"Advanced options start at the top without an empty settings grid")
                root.verify(Math.abs(advancedScroll.contentItem.contentY-advancedScroll.contentItem.originY)<1,"switching to Advanced resets prior scrolling")
                console.log("Appearance smoke passed: live layouts, windows, glass color, profiles, controls, effects, and Advanced page position.")
                Qt.quit()
            }
        }
    }
}
