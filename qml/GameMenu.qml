import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nexus

ApplicationWindow {
    id: root
    required property var game
    property string page: "Home"
    property var featureStates: ({})
    property string feedback: ""
    property bool editMode: false
    property var recentFeatures: []
    property real rainbowTime: 0
    property bool pinned: false
    property bool restoreMaximized: false
    signal returnToLoader()
    signal requestMenu()
    readonly property color backgroundColor: t.background
    readonly property real cornerRadius: visibility === Window.Maximized || visibility === Window.FullScreen ? 0 : t.radius
    readonly property real bannerHeight: Math.max(106, t.fontSize * 3.8 + 50)
    width: 1536; height: 1024
    minimumWidth: 1020; minimumHeight: 640
    title: game.name + " · Wetsox"
    color: "transparent"
    transientParent: null
    background: AppearanceBackground { gameArtwork: root.game.artwork || ""; theme: t; radius: root.cornerRadius; antialiasing: true }
    WindowShape { window: root; radius: root.cornerRadius }
    flags: Qt.Window | Qt.FramelessWindowHint | (pinned ? Qt.WindowStaysOnTopHint : 0)
    font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize
    Theme { id: t; scope: root.game.id; backdrop: root.background }
    Component.onCompleted: { featureStates = ConfigStore.lastValues(game.id); syncFeatures() }
    function saveFeatures() { ConfigStore.saveLast(game.id, featureStates); syncFeatures() }
    function toggleFeature(key, value) { const next = Object.assign({}, featureStates); next[key] = value; featureStates = next; const recent = recentFeatures.filter(function(k) { return k !== key }); recent.unshift(key); recentFeatures = recent.slice(0, 10); saveFeatures() }
    function syncFeatures() { if (GameSession.preview || GameSession.gameId === game.id) GameSession.update(animatedFeatures(), visible && active && visibility !== Window.Minimized) }
    function animatedFeatures() {
        const next = Object.assign({}, featureStates)
        for (const section of sections) for (const control of section.controls) {
            const options = t.element("control:" + control.key)
            if (control.type === "color" && options.rainbow) next[control.key] = Qt.hsva((rainbowTime * (options.rainbowSpeed || 1)) % 1, 0.75, 1, 1).toString()
        }
        return next
    }
    readonly property bool hasRainbowFeatures: sections.some(function(s) { return s.controls.some(function(c) { return c.type === "color" && t.element("control:" + c.key).rainbow }) })
    Timer { interval: 80; repeat: true; running: root.hasRainbowFeatures && (root.visible || GameSession.connected); onTriggered: { root.rainbowTime += 0.008; root.syncFeatures() } }
    onActiveChanged: syncFeatures()
    onVisibleChanged: syncFeatures()
    onVisibilityChanged: function(nextVisibility) {
        if (nextVisibility === Window.Maximized) restoreMaximized = true
        else if (nextVisibility === Window.Windowed) restoreMaximized = false
    }
    function restoreMenu() {
        if (visibility === Window.Minimized) {
            if (restoreMaximized) showMaximized()
            else showNormal()
        } else show()
        raise(); requestActivate()
    }
    function resetControls() { featureStates = ({}); saveFeatures(); feedback = "Controls reset." }
    onClosing: function(event) {
        event.accepted = false
        root.hide()
        root.returnToLoader()
    }
    readonly property var sections: game.sections || []
    function hotkey(key) {
        const saved = (featureStates._hotkeys || {})[key]
        if (saved) return saved
        if (key === "aim") return {enabled:true, mode:"Hold", binding:featureStates.aimKey !== undefined ? featureStates.aimKey : "RMB"}
        const legacy = key === "teleportWaypoint" ? "waypointKey" : key === "teleportObjective" ? "objectiveKey" : ""
        return {enabled:!!(legacy && featureStates[legacy]), mode:"Hold", binding:legacy ? (featureStates[legacy] || "") : ""}
    }
    function setHotkey(key, field, value) {
        const rules = Object.assign({}, featureStates._hotkeys || {})
        rules[key] = Object.assign({}, hotkey(key), {[field]:value})
        toggleFeature("_hotkeys", rules)
    }
    function openHotkey(key) { page = "Hotkeys"; Qt.callLater(function() { hotkeysPanel.reveal(key) }) }


    header: Rectangle {
        visible: t.values.showTitleBar; height: visible ? 58 : 0; color: "transparent"
        RowLayout {
            anchors.fill: parent; spacing: 0
            RowLayout {
                Layout.preferredWidth: 200; Layout.leftMargin: 22; spacing: 12
                Layout.maximumWidth: 200
                Glyph { visible: t.values.showBrand; name: "logo"; color: t.accent; Layout.preferredWidth: 30; Layout.preferredHeight: 32 }
                Text { visible: t.values.showBrand; text: "Wetsox"; color: t.text; font.family: t.family; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 19; font.weight: Font.DemiBold }
                Rectangle { width: 38; height: 20; radius: 5; color: t.tint(t.accent, 0.1); border.color: t.tint(t.accent, 0.5)
                    Text { anchors.centerIn: parent; text: game.id === "killingfloor2" ? "KF2" : game.id === "justcause4" ? "JC4" : game.id === "farcry4" ? "FC4" : "FC5"; color: Qt.lighter(t.accent, 1.5); font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 10 }
                }
                Item { Layout.fillWidth: true }
            }
            Flickable {
                id: appearanceNavigation
                visible: root.page === "Appearance"; Layout.fillWidth:true; Layout.minimumWidth:0; Layout.preferredHeight:44
                contentWidth:appearanceTabs.width; contentHeight:height; clip:true; boundsBehavior:Flickable.StopAtBounds
                Row {
                    id:appearanceTabs;spacing:4;height:parent.height
                    Repeater {
                        model: [{name:"Home",label:"Home",icon:"home"}].concat(root.sections.map(function(s){return {name:s.name,label:s.name,icon:s.icon}})).concat([{name:"Hotkeys",label:"Hotkeys",icon:"aim"},{name:"Appearance",label:"Customize",icon:"brush"},{name:"Configs",label:"Configs",icon:"save"}])
                        NavItem {
                            required property var modelData
                            theme:t;text:modelData.label;glyph:modelData.icon;selected:root.page===modelData.name
                            compact:root.width<1200;height:40;width:compact?42:Math.max(108,text.length*8+72)
                            onClicked:root.page=modelData.name
                        }
                    }
                }
                ScrollBar.horizontal:ScrollBar {policy:ScrollBar.AsNeeded}
            }
            WindowBar {
                theme: t; window: root; label: root.page === "Appearance" ? "" : root.game.name.toUpperCase() + "  /  " + root.page.toUpperCase()
                showThemeButtons: root.page !== "Appearance"; Layout.fillWidth: root.page !== "Appearance"; Layout.preferredWidth: root.page === "Appearance" ? 245 : 700
                onLightTheme: AppState.setColorMode(t.scope, true)
                onDarkTheme: AppState.setColorMode(t.scope, false)
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Flow {
            visible: root.page !== "Appearance" && t.values.showNavigation && (t.values.navigationStyle === "Top tabs" || t.values.navigationStyle === "Floating")
            Layout.fillWidth: true; Layout.margins: t.values.navigationStyle === "Floating" ? 16 : 4; spacing: 8
            Repeater {
                model: ["Home"].concat(t.ordered(root.sections).filter(function(s) { return !t.hidden("section:" + s.name) }).map(function(s) { return s.name })).concat(["Hotkeys", "Configs", "Appearance"])
                ActionButton { required property string modelData; theme: t; text: modelData; selected: root.page === modelData; onClicked: root.page = modelData }
            }
        }
        RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
        layoutDirection: t.values.sidebarSide === "Right" ? Qt.RightToLeft : Qt.LeftToRight
        GlassSurface {
            theme: t; kind: "sidebar"
            visible: root.page !== "Appearance" && t.values.showNavigation && (t.values.navigationStyle === "Sidebar" || t.values.navigationStyle === "Icon rail")
            Layout.preferredWidth: t.sidebarWidth; Layout.fillHeight: true; radius: root.cornerRadius
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: t.tint(t.border, 0.7) }
            ScrollView {
                id: sidebarScroll
                anchors.fill: parent; anchors.margins: 14; clip: true; contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ColumnLayout {
                width: sidebarScroll.availableWidth
                height: Math.max(sidebarScroll.availableHeight, implicitHeight)
                spacing: 5
                Repeater {
                    model: [{name:"Home",icon:"home"}].concat(t.ordered(root.sections).filter(function(s) { return !t.hidden("section:" + s.name) }).map(function(s) { return {name:s.name,icon:s.icon} })).concat([{name:"Hotkeys",icon:"aim"},{name:"Configs",icon:"save"},{name:"Appearance",icon:"layers"}])
                    NavItem {
                        required property var modelData
                        theme: t; text: modelData.name; glyph: modelData.icon; compact: t.values.compactSidebar || t.values.navigationStyle === "Icon rail"
                        selected: root.page === modelData.name; Layout.fillWidth: true
                        onClicked: root.page = modelData.name
                    }
                }
                Item { Layout.fillHeight: true; Layout.minimumHeight: 8 }
                Rectangle { Layout.fillWidth: true; height: 1; color: t.border }
                Text { text: "CURRENT GAME"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.pixelSize: 9; font.letterSpacing: 1.5; Layout.leftMargin: 10; Layout.topMargin: 14; Layout.bottomMargin: 6 }
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: 64; color: t.tint(t.surface,0.6); radius: t.radius; border.color: t.border
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10; spacing: 11
                        CoverArt { game: root.game; Layout.preferredWidth: 30; Layout.preferredHeight: 44; radius: 4; outside: t.surface }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 4
                            Text { text: root.game.name; color: t.text; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "Single player"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 10 }
                        }
                    }
                }
                NavItem { theme: t; text: "Reset controls"; glyph: "reset"; Layout.fillWidth: true; onClicked: root.resetControls() }
                NavItem { theme: t; text: root.pinned ? "Unpin window" : "Keep on top"; glyph: "layers"; selected: root.pinned; Layout.fillWidth: true; onClicked: { root.pinned = !root.pinned; root.show() } }
                NavItem { theme: t; text: "Close menu"; glyph: "close"; Layout.fillWidth: true; onClicked: root.close() }
                }
            }
        }
        ColumnLayout {
            id: pageBody
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: root.page === "Appearance" ? 0 : 20; spacing: t.spacing
            RowLayout {
                visible: t.values.showBanner && root.page !== "Appearance" && root.page !== "Configs" && root.page !== "Hotkeys"; Layout.fillWidth: true; spacing: 14
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: root.bannerHeight
                    color: t.tint(t.surface, 0.8); radius: t.radius; border.color: t.tint(t.accent, 0.55)
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 13; spacing: 17
                        CoverArt { game: root.game; Layout.preferredWidth: 54; Layout.preferredHeight: 80; radius: 6; outside: t.surface }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 7
                            Text { text: root.game.name; color: t.text; font.family: t.family; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize + 10; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: root.game.name + " module"; color: "#48d7ac"; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1 }
                            Text { text: root.game.subtitle + "   /   Single player"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 2; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                        ActionButton { theme: t; text: "Appearance"; primary: true; visible: root.width > 1150; onClicked: root.page = "Appearance" }
                    }
                }
                Rectangle {
                    visible: t.values.showStatus; Layout.preferredWidth: 230; implicitHeight: root.bannerHeight
                    color: t.tint(t.surface, 0.8); radius: t.radius; border.color: t.border
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 14; spacing: 8
                        Text { text: "Session status"; color: t.text; font.family: t.family; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize; font.weight: Font.Medium }
                        RowLayout {
                            Text { text: "●"; color: "#48d7ac"; font.pixelSize: 10 }
                            Text { text: "Interface"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1; Layout.fillWidth: true }
                            Text { text: GameSession.connected ? "LOADED" : "PREVIEW"; color: t.text; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 9 }
                        }
                        RowLayout {
                            Text { text: "●"; color: "#b69764"; font.pixelSize: 10 }
                            Text { text: "Game connection"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1; Layout.fillWidth: true }
                            Text { text: GameSession.connected ? "CONNECTED" : "OFFLINE"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 9 }
                        }
                    }
                }
            }
            Text { visible: root.page === "Aimbot" || root.page === "Visuals"; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1
                text: root.game.id === "killingfloor2" ? "Aimbot targets Zeds. Configure hold or toggle bindings in Hotkeys; the game must have focus." : root.page === "Aimbot" ? "Handheld aiming ignores vehicle cover. Walls still block aiming. Projectile compensation is experimental; unsupported weapons use direct aim. Hold your aim key while the game has focus." : "Human filters use faction, not current hostility. Boxes and skeletons use resolved body joints. Missing joints are skipped." }
            HotkeysPanel { id: hotkeysPanel; theme: t; controller: root; visible: root.page === "Hotkeys"; Layout.fillWidth: true; Layout.fillHeight: true }
            ConfigsPanel { theme: t; controller: root; visible: root.page === "Configs"; Layout.fillWidth: true; Layout.fillHeight: true }
            SettingsPanel { controller: root; sections: root.sections; theme: t; scopeName: root.game.name; visible: root.page === "Appearance"; Layout.fillWidth: true; Layout.fillHeight: true }
            RowLayout {
                visible: root.page !== "Appearance" && root.page !== "Configs" && root.page !== "Hotkeys"; Layout.fillWidth: true
                Input { id: settingSearch; objectName: "settingSearch"; theme: t; visible: t.values.showSearch; placeholderText: "Find any setting: speed, color, key…"; Layout.fillWidth: true; Accessible.name: "Search all game settings" }
                ActionButton { objectName: "editLayout"; theme: t; text: root.editMode ? "Done editing" : "Edit layout"; selected: root.editMode; onClicked: root.editMode = !root.editMode }
                ActionButton { theme: t; text: "Restore / properties"; visible: root.editMode; onClicked: root.page = "Appearance" }
            }
            Text { visible: root.editMode; text: "Drag card headings to reorder. Drag the bottom edge to resize. Right-click a card or setting for properties."; color: t.accent; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Text {
                visible: t.values.showRecent && root.recentFeatures.length > 0 && root.page === "Home"
                text: "Recently changed: " + root.recentFeatures.map(function(key) { for (const s of root.sections) { const c = s.controls.find(function(c) { return c.key === key }); if (c) return c.label } return key }).join(" · ")
                color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1; Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            ScrollView {
                id: dashboard; visible: root.page !== "Appearance" && root.page !== "Configs" && root.page !== "Hotkeys"
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
                Item {
                    id: dashboardGrid
                    objectName: "homeCardLayout"
                    width: dashboard.availableWidth
                    property int columns: root.page !== "Home" || t.values.cardLayout === "List" ? 1 : Math.min(t.values.cardLayout === "Columns" ? 3 : 2, Math.max(1, Math.floor((width + t.spacing) / (330 + Math.max(0, t.fontSize - 13) * 16))))
                    property real gap: t.spacing
                    property bool layoutPending: false
                    function scheduleLayout() {
                        if (layoutPending) return
                        layoutPending = true
                        Qt.callLater(function() { dashboardGrid.layoutPending = false; dashboardGrid.arrange() })
                    }
                    function arrange() {
                        const bottoms = new Array(columns).fill(0)
                        const columnWidth = Math.max(0, (width - (columns - 1) * gap) / columns)
                        for (let index = 0; index < dashboardCards.count; ++index) {
                            const card = dashboardCards.itemAt(index)
                            if (!card) continue
                            const span = card.cardSpan
                            let bestColumn = 0, bestY = Infinity
                            for (let column = 0; column <= columns - span; ++column) {
                                let y = 0
                                for (let offset = 0; offset < span; ++offset) y = Math.max(y, bottoms[column + offset])
                                if (y < bestY) { bestY = y; bestColumn = column }
                            }
                            card.x = bestColumn * (columnWidth + gap)
                            card.y = bestY
                            for (let offset = 0; offset < span; ++offset) bottoms[bestColumn + offset] = bestY + card.height + gap
                        }
                        implicitHeight = Math.max(0, Math.max.apply(null, bottoms) - gap)
                    }
                    onWidthChanged: scheduleLayout()
                    onColumnsChanged: scheduleLayout()
                    onGapChanged: scheduleLayout()
                    Repeater {
                        id: dashboardCards
                        onItemAdded: dashboardGrid.scheduleLayout()
                        onItemRemoved: dashboardGrid.scheduleLayout()
                        onCountChanged: dashboardGrid.scheduleLayout()
                        model: t.ordered(root.sections).filter(function(s) {
                            if (t.hidden("section:" + s.name)) return false
                            const searching = t.values.showSearch && settingSearch.text.trim().length > 0
                            if (searching) return s.controls.some(function(c) { return c.type !== "keybind" && (c.label + " " + c.key + " " + s.name).toLowerCase().indexOf(settingSearch.text.toLowerCase()) >= 0 })
                            return root.page === "Home" ? !t.element("section:" + s.name).homeHidden : s.name === root.page
                        })
                        ControlPanel {
                            required property var modelData
                            objectName: "panel_" + modelData.name
                            theme: t; section: modelData; controller: root; query: t.values.showSearch ? settingSearch.text : ""; editing: root.editMode
                            enabled: root.editMode || GameSession.connected || GameSession.preview
                            readonly property int cardSpan: Math.min(dashboardGrid.columns, t.element("section:" + modelData.name).span || 1)
                            width: Math.min(root.page === "Home" ? Infinity : 720,
                                Math.max(0, (dashboardGrid.width - (dashboardGrid.columns - 1) * dashboardGrid.gap) / dashboardGrid.columns) * cardSpan + (cardSpan - 1) * dashboardGrid.gap)
                            height: implicitHeight
                            onHeightChanged: dashboardGrid.scheduleLayout()
                            onCardSpanChanged: dashboardGrid.scheduleLayout()
                        }
                    }
                }
            }
            RowLayout {
                visible: t.values.showFooter && root.page !== "Appearance"; Layout.fillWidth: true
                Text { text: "●  " + (t.linked ? "Shared appearance" : "Independent appearance"); color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 10; Layout.fillWidth: true }
                Text { text: GameSession.preview ? "UI TEST" : GameSession.message; Layout.maximumWidth: root.width * 0.58; elide: Text.ElideRight; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.pixelSize: 9; font.letterSpacing: 1 }
            }
            Text { text: AppState.error || ConfigStore.error || root.feedback; visible: text.length > 0; color: t.accent; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }
    }
    Rectangle { visible: t.values.showWindowBorder; parent: root.contentItem.parent; anchors.fill: parent; color: "transparent"; border.color: t.tint(t.accent, 0.4); radius: root.cornerRadius; z: 90 }
    InterfaceEffects { parent: root.contentItem.parent; theme: t; window: root }
    PageMotion { parent: root.contentItem; theme: t; target: pageBody; page: root.page }
    MenuShortcut {
        objectName: "gameMenuShortcut"
        sequence: t.values.menuKey
        enabled: !hotkeysPanel.recording && (root.visible || GameSession.gameId === root.game.id)
        onActivated: { if (root.visible && root.visibility !== Window.Minimized) root.showMinimized(); else root.requestMenu() }
    }
    ActionButton {
        parent: root.contentItem; theme: t
        visible: !t.values.showNavigation || !t.values.showTitleBar
        text: root.page === "Appearance" ? "Restore navigation" : "Appearance"
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; z: 99
        onClicked: { if (root.page === "Appearance") { t.set("showNavigation", true); t.set("showTitleBar", true) } else root.page = "Appearance" }
    }
    WindowPreferences { window: root; theme: t }
    WindowEdges { parent: root.contentItem.parent; window: root; enabled: t.values.resizable }
    Timer { interval: 4000; running: root.feedback.length > 0; onTriggered: root.feedback = "" }
}
