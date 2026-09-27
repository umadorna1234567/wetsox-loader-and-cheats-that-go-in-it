import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nexus

ApplicationWindow {
    id: root
    width: 1220; height: 800
    minimumWidth: 930; minimumHeight: 640
    visible: true
    title: "Wetsox · Game library"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    readonly property color backgroundColor: t.background
    readonly property real cornerRadius: visibility === Window.Maximized || visibility === Window.FullScreen ? 0 : t.radius
    background: Rectangle { color: t.background; radius: root.cornerRadius; antialiasing: true }
    WindowShape { window: root; radius: root.cornerRadius }
    font.family: t.family; font.pixelSize: t.fontSize
    property string page: "Library"
    property var windows: ({})
    property alias gameEditor: editor
    readonly property var filteredGames: AppState.games.filter(function(game) {
        return (game.name + " " + game.subtitle).toLowerCase().indexOf(search.text.toLowerCase()) !== -1
    })
    Theme { id: t; scope: "loader" }
    Component { id: menuComponent; GameMenu {} }
    function editGame(game) { editor.game = game; editor.open() }
    function restoreLoader() {
        for (let id in windows) if (windows[id].visible) return
        root.show(); root.raise(); root.requestActivate()
    }
    function openGame(game) { GameSession.launch(game.id) }
    Connections {
        target: GameSession
        function onReady(id) {
            const game = AppState.games.find(function(g) { return g.id === id })
            if (game) root.showGame(game)
        }
    }
    function showGame(game) {
        if (!windows[game.id]) {
            const window = menuComponent.createObject(root, { game: game })
            if (!window) return
            windows[game.id] = window
            window.returnToLoader.connect(root.restoreLoader)
        }
        for (let id in windows) if (id !== game.id) windows[id].hide()
        windows[game.id].show(); windows[game.id].raise(); windows[game.id].requestActivate()
        root.hide()
    }
    Connections {
        target: AppState
        function onChanged() {
            for (let game of AppState.games)
                if (root.windows[game.id]) root.windows[game.id].game = game
        }
    }
    onClosing: { GameSession.detach(); Qt.quit() }
    header: Rectangle {
        height: 62; color: "transparent"
        RowLayout {
            anchors.fill: parent; spacing: 0
            RowLayout {
                Layout.preferredWidth: t.values.compactSidebar ? 55 : 181; Layout.leftMargin: 24; spacing: 12
                Layout.maximumWidth: Layout.preferredWidth
                Glyph { name: "logo"; color: t.accent; Layout.preferredWidth: 30; Layout.preferredHeight: 32 }
                Text { visible: !t.values.compactSidebar; text: "Wetsox"; color: t.text; font.family: t.family; font.pixelSize: 21; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
            }
            WindowBar {
                theme: t; window: root; label: "WORKSPACE  /  " + root.page.toUpperCase()
                Layout.fillWidth: true; Layout.preferredWidth: 700; showThemeButtons: true
                onLightTheme: AppState.setColorMode(t.scope, true)
                onDarkTheme: AppState.setColorMode(t.scope, false)
            }
        }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.fillHeight: true; Layout.preferredWidth: t.values.compactSidebar ? 79 : 205
            color: t.tint(t.surface, 0.3); radius: root.cornerRadius
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: t.border }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 6
                Text { visible: !t.values.compactSidebar; text: "EXPLORE"; color: t.muted; font.family: t.family; font.pixelSize: 9; font.letterSpacing: 1.8; Layout.leftMargin: 12; Layout.topMargin: 12; Layout.bottomMargin: 10 }
                NavItem { theme: t; text: "Game library"; glyph: "library"; compact: t.values.compactSidebar; selected: root.page === "Library"; Layout.fillWidth: true; onClicked: root.page = "Library"; Accessible.name: "Game library" }
                NavItem { theme: t; text: "Appearance"; glyph: "layers"; compact: t.values.compactSidebar; selected: root.page === "Appearance"; Layout.fillWidth: true; onClicked: root.page = "Appearance"; Accessible.name: "Appearance settings" }
                Item { Layout.fillHeight: true }
                Rectangle { Layout.fillWidth: true; height: 1; color: t.border }
                NavItem { theme: t; text: "Add game"; glyph: "plus"; compact: t.values.compactSidebar; Layout.fillWidth: true; onClicked: root.editGame({}) }
                RowLayout {
                    Layout.leftMargin: 12; Layout.topMargin: 8; Layout.bottomMargin: 7; spacing: 8
                    Rectangle { width: 5; height: 5; radius: 3; color: "#48d7ac" }
                    Text { text: t.values.compactSidebar ? "UI" : "LOCAL WORKSPACE"; color: t.muted; font.family: t.family; font.pixelSize: 9; font.letterSpacing: 1 }
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 28; spacing: t.spacing
            SettingsPanel { theme: t; scopeName: "Loader"; visible: root.page === "Appearance"; Layout.fillWidth: true; Layout.fillHeight: true }
            ScrollView {
                id: libraryScroll; visible: root.page === "Library"
                Layout.fillWidth: true; Layout.fillHeight: true; contentWidth: availableWidth; clip: true
                ColumnLayout {
                    width: libraryScroll.availableWidth; spacing: 24
                    RowLayout {
                        Layout.fillWidth: true
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 7
                            Text { text: "Game library"; color: t.text; font.family: t.family; font.pixelSize: t.fontSize + 15; font.weight: Font.DemiBold; Layout.fillWidth: true }
                            Text { text: "Your games. Your setup."; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize; Layout.fillWidth: true }
                        }
                        ActionButton { theme: t; text: "+  Add game"; primary: true; onClicked: root.editGame({}) }
                    }
                    Rectangle {
                        Layout.fillWidth: true; implicitHeight: 58; radius: t.radius
                        color: t.tint(t.accent, 0.05); border.color: t.tint(t.accent, 0.22)
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 14; spacing: 12
                            Glyph { name: "layers"; color: Qt.lighter(t.accent, 1.5); Layout.preferredWidth: 22; Layout.preferredHeight: 22 }
                            Text { text: t.linked ? "One appearance. Every linked menu." : "Your loader has its own appearance."; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize; Layout.fillWidth: true; elide: Text.ElideRight }
                            ActionButton { theme: t; text: "Customize"; implicitHeight: 30; onClicked: root.page = "Appearance" }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 12
                        ActionButton { theme: t; text: "All games  ·  " + AppState.games.length; selected: true; implicitHeight: 32; onClicked: search.clear() }
                        Item { Layout.fillWidth: true }
                        Input { id: search; objectName: "gameSearch"; theme: t; placeholderText: "Search your library"; Layout.preferredWidth: 215; Accessible.name: "Search games" }
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: 22
                        Repeater {
                            model: root.filteredGames
                            GameCard {
                                required property var modelData
                                theme: t; game: modelData
                                width: implicitWidth; height: implicitHeight
                                onOpenMenu: root.openGame(modelData)
                                onEditGame: root.editGame(modelData)
                            }
                        }
                        Item {
                            visible: search.text.length === 0
                            width: t.cardHeight * 2 / 3; height: t.cardHeight + 74
                            Rectangle {
                                width: parent.width; height: t.cardHeight; radius: t.radius
                                color: addButton.hovered ? t.tint(t.accent, 0.05) : t.tint(t.surface, 0.35)
                                border.color: addButton.hovered || addButton.activeFocus ? t.accent : t.border
                                ColumnLayout {
                                    anchors.centerIn: parent; width: parent.width - 30; spacing: 13
                                    Rectangle {
                                        Layout.alignment: Qt.AlignHCenter; width: 42; height: 42; radius: 12
                                        color: t.tint(t.accent, 0.1)
                                        Glyph { anchors.centerIn: parent; name: "plus"; color: Qt.lighter(t.accent, 1.5); width: 23; height: 23 }
                                    }
                                    Text { text: "Add a game"; color: t.text; font.family: t.family; font.pixelSize: t.fontSize; Layout.alignment: Qt.AlignHCenter }
                                    Text { text: "Build your collection"; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize - 2; Layout.alignment: Qt.AlignHCenter }
                                }
                            }
                            Button { id: addButton; anchors.fill: parent; hoverEnabled: true; contentItem: Item {} background: Item {} Accessible.name: "Add another game"; onClicked: root.editGame({}) }
                        }
                    }
                    Text { visible: root.filteredGames.length === 0; text: "No games match your search."; color: t.muted; font.family: t.family; font.pixelSize: t.fontSize }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: t.border }
            RowLayout {
                Layout.fillWidth: true
                Text { text: "●  " + (t.linked ? "Shared appearance" : "Independent appearance"); color: t.muted; font.family: t.family; font.pixelSize: 10; Layout.fillWidth: true }
                Text { text: "NEXUS  /  UI PREVIEW"; color: t.muted; font.family: t.family; font.pixelSize: 9; font.letterSpacing: 1.4 }
            }
            RowLayout {
                visible: AppState.error.length > 0 || GameSession.message.length > 0; Layout.fillWidth: true
                Text { text: AppState.error || GameSession.message; color: t.accent; font.family: t.family; font.pixelSize: t.fontSize - 1; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                ActionButton { theme: t; text: "Dismiss"; onClicked: AppState.clearError() }
            }
        }
    }
    GameEditor { id: editor; theme: t; parent: Overlay.overlay }
    Rectangle { parent: root.contentItem.parent; anchors.fill: parent; color: "transparent"; border.color: t.tint(t.accent, 0.4); radius: root.cornerRadius; z: 90 }
    WindowEdges { parent: root.contentItem.parent; window: root }
}
