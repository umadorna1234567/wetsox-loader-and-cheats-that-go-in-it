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
    property bool pinned: false
    signal returnToLoader()
    readonly property color backgroundColor: t.background
    readonly property real cornerRadius: visibility === Window.Maximized || visibility === Window.FullScreen ? 0 : t.radius
    readonly property real bannerHeight: Math.max(106, t.fontSize * 3.8 + 50)
    width: 1420; height: 940
    minimumWidth: 1020; minimumHeight: 640
    title: game.name + " · Wetsox"
    color: "transparent"
    transientParent: null
    background: Rectangle { color: t.background; radius: root.cornerRadius; antialiasing: true }
    WindowShape { window: root; radius: root.cornerRadius }
    flags: Qt.Window | Qt.FramelessWindowHint | (pinned ? Qt.WindowStaysOnTopHint : 0)
    font.family: t.family; font.pixelSize: t.fontSize
    Theme { id: t; scope: root.game.id }
    Component.onCompleted: { featureStates = ConfigStore.lastValues(game.id); syncFeatures() }
    function saveFeatures() { ConfigStore.saveLast(game.id, featureStates); syncFeatures() }
    function toggleFeature(key, value) { const next = Object.assign({}, featureStates); next[key] = value; featureStates = next; saveFeatures() }
    function syncFeatures() { if (GameSession.preview || GameSession.gameId === game.id) GameSession.update(featureStates, visible && active) }
    onActiveChanged: syncFeatures()
    onVisibleChanged: syncFeatures()
    function resetControls() { featureStates = ({}); saveFeatures(); feedback = "Controls reset." }
    onClosing: function(event) {
        event.accepted = false
        root.hide()
        root.returnToLoader()
    }
    readonly property var sections: game.sections || []

    header: Rectangle {
        height: 58; color: "transparent"
        RowLayout {
            anchors.fill: parent; spacing: 0
            RowLayout {
                Layout.preferredWidth: 200; Layout.leftMargin: 22; spacing: 12
                Layout.maximumWidth: 200
                Glyph { name: "logo"; color: t.accent; Layout.preferredWidth: 30; Layout.preferredHeight: 32 }
                Text { text: "Wetsox"; color: t.text; font.family: t.family; font.pixelSize: 19; font.weight: Font.DemiBold }
                Rectangle { width: 38; height: 20; radius: 5; color: t.tint(t.accent, 0.1); border.color: t.tint(t.accent, 0.5)
                    Text { anchors.centerIn: parent; text: game.id === "farcry4" ? "FC4" : "FC5"; color: Qt.lighter(t.accent, 1.5); font.family: t.family; font.pixelSize: 10 }
                }
                Item { Layout.fillWidth: true }
            }
            WindowBar {
                theme: t; window: root; label: root.game.name.toUpperCase() + "  /  " + root.page.toUpperCase()
                showThemeButtons: true; Layout.fillWidth: true; Layout.preferredWidth: 700
                onLightTheme: AppState.setColorMode(t.scope, true)
                onDarkTheme: AppState.setColorMode(t.scope, false)
            }
        }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: 222; Layout.fillHeight: true; color: t.tint(t.surface, 0.3); radius: root.cornerRadius
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
                    model: [{name:"Home",icon:"home"}].concat(root.sections.map(function(s) { return {name:s.name,icon:s.icon} })).concat([{name:"Configs",icon:"save"},{name:"Appearance",icon:"layers"}])
                    NavItem {
                        required property var modelData
                        theme: t; text: modelData.name; glyph: modelData.icon
                        selected: root.page === modelData.name; Layout.fillWidth: true
                        onClicked: root.page = modelData.name
                    }
                }
                Item { Layout.fillHeight: true; Layout.minimumHeight: 8 }
                Rectangle { Layout.fillWidth: true; height: 1; color: t.border }
                Text { text: "CURRENT GAME"; color: t.muted; font.family: t.family; font.pixelSize: 9; font.letterSpacing: 1.5; Layout.leftMargin: 10; Layout.topMargin: 14; Layout.bottomMargin: 6 }
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: 64; color: t.tint(t.surface,0.6); radius: t.radius; border.color: t.border
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 10; spacing: 11
                        CoverArt { game: root.game; Layout.preferredWidth: 30; Layout.preferredHeight: 44; radius: 4; outside: t.surface }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 4
                            Text { text: root.game.name; color: t.text; font.family: t.family; font.pixelSize: t.fontSize; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "Single player"; color: t.muted; font.family: t.family; font.pixelSize: 10 }
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
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 20; spacing: t.spacing
            RowLayout {
                visible: root.page !== "Appearance" && root.page !== "Configs"; Layout.fillWidth: true; spacing: 14
                Rectangle {
                    Layout.fillWidth: true; implicitHeight: root.bannerHeight
                    color: t.tint(t.surface, 0.8); radius: t.radius; border.color: t.tint(t.accent, 0.55)
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 13; spacing: 17
                        CoverArt { game: root.game; Layout.preferredWidth: 54; Layout.preferredHeight: 80; radius: 6; outside: t.surface }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 7
                            Text { text: root.game.name; color: t.text; font.family: t.family; font.pixelSize: t.fontSize + 10; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "●  Far Cry 5 module"; color: "#48d7ac"; font.family: t.family; font.pixelSize: t.fontSize - 1 }
                            Text { text: root.game.subtitle + "   /   Single player"; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize - 2; Layout.fillWidth: true; elide: Text.ElideRight }
                        }
                        ActionButton { theme: t; text: "Appearance"; primary: true; visible: root.width > 1150; onClicked: root.page = "Appearance" }
                    }
                }
                Rectangle {
                    Layout.preferredWidth: 230; implicitHeight: root.bannerHeight
                    color: t.tint(t.surface, 0.8); radius: t.radius; border.color: t.border
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 14; spacing: 8
                        Text { text: "Session status"; color: t.text; font.family: t.family; font.pixelSize: t.fontSize; font.weight: Font.Medium }
                        RowLayout {
                            Text { text: "●"; color: "#48d7ac"; font.pixelSize: 10 }
                            Text { text: "Interface"; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize - 1; Layout.fillWidth: true }
                            Text { text: GameSession.connected ? "LOADED" : "PREVIEW"; color: t.text; font.family: t.family; font.pixelSize: 9 }
                        }
                        RowLayout {
                            Text { text: "●"; color: "#b69764"; font.pixelSize: 10 }
                            Text { text: "Game connection"; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize - 1; Layout.fillWidth: true }
                            Text { text: GameSession.connected ? "CONNECTED" : "OFFLINE"; color: t.muted; font.family: t.family; font.pixelSize: 9 }
                        }
                    }
                }
            }
            Text { visible: root.page === "Aimbot" || root.page === "Visuals"; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize - 1
                text: root.page === "Aimbot" ? "Handheld aiming ignores vehicle cover. Walls still block aiming. Projectile compensation is experimental; unsupported weapons use direct aim. Hold your aim key while the game has focus." : "Human filters use faction, not current hostility. Boxes and skeletons use resolved body joints. Missing joints are skipped." }
            ConfigsPanel { theme: t; controller: root; visible: root.page === "Configs"; Layout.fillWidth: true; Layout.fillHeight: true }
            SettingsPanel { theme: t; scopeName: root.game.name; visible: root.page === "Appearance"; Layout.fillWidth: true; Layout.fillHeight: true }
            ScrollView {
                id: dashboard; visible: root.page !== "Appearance" && root.page !== "Configs"
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; contentWidth: availableWidth
                GridLayout {
                    width: dashboard.availableWidth
                    columns: root.page !== "Home" ? 1 : Math.min(2, Math.max(1, Math.floor((width + t.spacing) / (330 + Math.max(0, t.fontSize - 13) * 16))))
                    columnSpacing: t.spacing; rowSpacing: t.spacing
                    Repeater {
                        model: root.page === "Home" ? root.sections : root.sections.filter(function(s) { return s.name === root.page })
                        ControlPanel {
                            required property var modelData
                            theme: t; section: modelData; controller: root
                            enabled: GameSession.connected || GameSession.preview
                            Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 320
                            Layout.maximumWidth: root.page === "Home" ? Infinity : 720
                            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        }
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: "●  " + (t.linked ? "Shared appearance" : "Independent appearance"); color: t.muted; font.family: t.family; font.pixelSize: 10; Layout.fillWidth: true }
                Text { text: GameSession.preview ? "UI TEST" : GameSession.message; Layout.maximumWidth: root.width * 0.58; elide: Text.ElideRight; color: t.muted; font.family: t.family; font.pixelSize: 9; font.letterSpacing: 1 }
            }
            Text { text: AppState.error || ConfigStore.error || root.feedback; visible: text.length > 0; color: t.accent; font.family: t.family; font.pixelSize: t.fontSize - 1; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }
    Rectangle { parent: root.contentItem.parent; anchors.fill: parent; color: "transparent"; border.color: t.tint(t.accent, 0.4); radius: root.cornerRadius; z: 90 }
    WindowEdges { parent: root.contentItem.parent; window: root }
    Timer { interval: 4000; running: root.feedback.length > 0; onTriggered: root.feedback = "" }
}
