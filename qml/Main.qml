import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nexus

ApplicationWindow {
    id: root
    width: 1536; height: 1024
    minimumWidth: 930; minimumHeight: 640
    visible: true
    title: "Wetsox · Game library"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    readonly property color backgroundColor: t.background
    readonly property real cornerRadius: visibility === Window.Maximized || visibility === Window.FullScreen ? 0 : t.radius
    background: AppearanceBackground { theme: t; radius: root.cornerRadius; antialiasing: true }
    WindowShape { window: root; radius: root.cornerRadius }
    font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize
    property string page: "Library"
    property var windows: ({})
    property alias gameEditor: editor
    readonly property var filteredGames: AppState.games.filter(function(game) {
        return !t.values.showSearch || (game.name + " " + game.subtitle).toLowerCase().indexOf(search.text.toLowerCase()) !== -1
    })
    Theme { id: t; scope: "loader"; backdrop: root.background }
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
            window.requestMenu.connect(function() { root.showGame(window.game) })
        }
        for (let id in windows) if (id !== game.id) windows[id].hide()
        windows[game.id].restoreMenu()
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
        visible: t.values.showTitleBar; height: visible ? 62 : 0; color: "transparent"
        RowLayout {
            anchors.fill: parent; spacing: 0
            RowLayout {
                Layout.preferredWidth: (t.values.compactSidebar || t.values.navigationStyle === "Icon rail") ? 55 : 181; Layout.leftMargin: 24; spacing: 12
                Layout.maximumWidth: Layout.preferredWidth
                Glyph { visible: t.values.showBrand; name: "logo"; color: t.accent; Layout.preferredWidth: 30; Layout.preferredHeight: 32 }
                Text { visible: t.values.showBrand && !(t.values.compactSidebar || t.values.navigationStyle === "Icon rail"); text: "Wetsox"; color: t.text; font.family: t.family; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 21; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
            }
            RowLayout {
                visible: root.page === "Appearance"; Layout.fillWidth: true; spacing: 6
                Repeater {
                    model: [{name:"Library",label:"Home",icon:"home"},{name:"Appearance",label:"Customize",icon:"brush"}]
                    NavItem {
                        required property var modelData
                        theme: t; text: modelData.label; glyph: modelData.icon; selected: root.page === modelData.name
                        compact: root.width < 1450; implicitHeight: 40
                        Layout.preferredWidth: compact ? 42 : Math.max(98, text.length * 8 + 66)
                        onClicked: root.page = modelData.name
                    }
                }
                Item { Layout.fillWidth: true }
            }
            WindowBar {
                theme: t; window: root; label: root.page === "Appearance" ? "" : "WORKSPACE  /  " + root.page.toUpperCase()
                Layout.fillWidth: root.page !== "Appearance"; Layout.preferredWidth: root.page === "Appearance" ? 245 : 700; showThemeButtons: root.page !== "Appearance"
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
                model: ["Library", "Appearance"]
                ActionButton { required property string modelData; theme: t; text: modelData; selected: root.page === modelData; onClicked: root.page = modelData }
            }
        }
        RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
        layoutDirection: t.values.sidebarSide === "Right" ? Qt.RightToLeft : Qt.LeftToRight
        GlassSurface {
            theme: t; kind: "sidebar"
            visible: root.page !== "Appearance" && t.values.showNavigation && (t.values.navigationStyle === "Sidebar" || t.values.navigationStyle === "Icon rail")
            Layout.fillHeight: true; Layout.preferredWidth: t.sidebarWidth
            radius: root.cornerRadius
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: t.border }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 6
                Text { visible: !(t.values.compactSidebar || t.values.navigationStyle === "Icon rail"); text: "EXPLORE"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.pixelSize: 9; font.letterSpacing: 1.8; Layout.leftMargin: 12; Layout.topMargin: 12; Layout.bottomMargin: 10 }
                NavItem { theme: t; text: "Game library"; glyph: "library"; compact: (t.values.compactSidebar || t.values.navigationStyle === "Icon rail"); selected: root.page === "Library"; Layout.fillWidth: true; onClicked: root.page = "Library"; Accessible.name: "Game library" }
                NavItem { theme: t; text: "Appearance"; glyph: "layers"; compact: (t.values.compactSidebar || t.values.navigationStyle === "Icon rail"); selected: root.page === "Appearance"; Layout.fillWidth: true; onClicked: root.page = "Appearance"; Accessible.name: "Appearance settings" }
                Item { Layout.fillHeight: true }
                Rectangle { Layout.fillWidth: true; height: 1; color: t.border }
                NavItem { theme: t; visible:t.values.showAddGame; text: "Add game"; glyph: "plus"; compact: (t.values.compactSidebar || t.values.navigationStyle === "Icon rail"); Layout.fillWidth: true; onClicked: root.editGame({}) }
                RowLayout {
                    visible:t.values.showStatus; Layout.leftMargin: 12; Layout.topMargin: 8; Layout.bottomMargin: 7; spacing: 8
                    Rectangle { width: 5; height: 5; radius: 3; color: "#48d7ac" }
                    Text { text: (t.values.compactSidebar || t.values.navigationStyle === "Icon rail") ? "UI" : "LOCAL WORKSPACE"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.pixelSize: 9; font.letterSpacing: 1 }
                }
            }
        }
        ColumnLayout {
            id: pageBody
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: root.page === "Appearance" ? 0 : 28; spacing: t.spacing
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
                            Text { text: "Game library"; color: t.text; font.family: t.family; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize + 15; font.weight: Font.DemiBold; Layout.fillWidth: true }
                            Text { text: "Your games. Your setup."; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize; Layout.fillWidth: true }
                        }
                        ActionButton { theme: t; visible: t.values.showAddGame; text: "+  Add game"; primary: true; onClicked: root.editGame({}) }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 12
                        ActionButton { theme: t; text: "All games  ·  " + AppState.games.length; selected: true; implicitHeight: 32; onClicked: search.clear() }
                        Item { Layout.fillWidth: true }
                        Input { visible: t.values.showSearch; id: search; objectName: "gameSearch"; theme: t; placeholderText: "Search your library"; Layout.preferredWidth: 215; Accessible.name: "Search games" }
                    }
                    Flow {
                        Layout.fillWidth: true; spacing: t.spacing
                        Repeater {
                            model: root.filteredGames
                            GameCard {
                                required property var modelData
                                theme: t; game: modelData
                                width: t.values.cardLayout === "List" ? libraryScroll.availableWidth : t.values.cardLayout === "Columns" ? (libraryScroll.availableWidth - 2*t.spacing)/3 : implicitWidth; height: implicitHeight
                                onOpenMenu: root.openGame(modelData)
                                onEditGame: root.editGame(modelData)
                            }
                        }
                        Item {
                            visible: t.values.showAddGame && search.text.length === 0
                            width: t.values.cardLayout === "List" ? libraryScroll.availableWidth : t.values.cardLayout === "Columns" ? (libraryScroll.availableWidth - 2*t.spacing)/3 : t.cardHeight * 2 / 3; height: t.values.cardLayout === "List" ? 112 : t.cardHeight + 74
                            Rectangle {
                                width: parent.width; height: t.values.cardLayout === "List" ? 105 : t.cardHeight; radius: t.radius
                                color: addButton.hovered ? t.tint(t.accent, 0.05) : t.tint(t.surface, 0.35)
                                border.color: addButton.hovered || addButton.activeFocus ? t.accent : t.border
                                ColumnLayout {
                                    anchors.centerIn: parent; width: parent.width - 30; spacing: t.values.cardLayout === "List" ? 3 : 13
                                    Rectangle {
                                        Layout.alignment: Qt.AlignHCenter; width: 42; height: 42; radius: 12
                                        color: t.tint(t.accent, 0.1)
                                        Glyph { anchors.centerIn: parent; name: "plus"; color: Qt.lighter(t.accent, 1.5); width: 23; height: 23 }
                                    }
                                    Text { text: "Add a game"; color: t.text; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize; Layout.alignment: Qt.AlignHCenter }
                                    Text { visible:t.values.cardLayout !== "List";text: "Build your collection"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 2; Layout.alignment: Qt.AlignHCenter }
                                }
                            }
                            Button { id: addButton; anchors.fill: parent; hoverEnabled: true; contentItem: Item {} background: Item {} Accessible.name: "Add another game"; onClicked: root.editGame({}) }
                        }
                    }
                    Text { visible: root.filteredGames.length === 0; text: "No games match your search."; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize }
                }
            }
            Rectangle { visible: t.values.showFooter && root.page !== "Appearance"; Layout.fillWidth: true; height: 1; color: t.border }
            RowLayout {
                visible: t.values.showFooter && root.page !== "Appearance"
                Layout.fillWidth: true
                Text { text: "●  " + (t.linked ? "Shared appearance" : "Independent appearance"); color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: 10; Layout.fillWidth: true }
                Text { text: "WETSOX  /  GAME LIBRARY"; color: t.muted; font.family: t.family; font.weight: t.values.fontWeight; font.pixelSize: 9; font.letterSpacing: 1.4 }
            }
            RowLayout {
                visible: AppState.error.length > 0 || GameSession.message.length > 0; Layout.fillWidth: true
                Text { text: AppState.error || GameSession.message; color: t.accent; font.family: t.family; font.weight: t.values.fontWeight; font.letterSpacing: t.values.letterSpacing; font.pixelSize: t.fontSize - 1; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                ActionButton { theme: t; text: "Dismiss"; onClicked: AppState.clearError() }
            }
        }
    }
    }
    GameEditor { id: editor; theme: t; parent: Overlay.overlay }
    Rectangle { visible: t.values.showWindowBorder; parent: root.contentItem.parent; anchors.fill: parent; color: "transparent"; border.color: t.tint(t.accent, 0.4); radius: root.cornerRadius; z: 90 }
    InterfaceEffects { parent: root.contentItem.parent; theme: t; window: root }
    PageMotion { parent: root.contentItem; theme: t; target: pageBody; page: root.page }
    MenuShortcut { sequence: t.values.menuKey; enabled: root.visible && GameSession.gameId.length === 0; onActivated: root.page = root.page === "Appearance" ? "Library" : "Appearance" }
    ActionButton {
        parent: root.contentItem; theme: t
        visible: !t.values.showNavigation || !t.values.showTitleBar
        text: root.page === "Appearance" ? "Restore navigation" : "Appearance"
        anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 8; z: 99
        onClicked: { if (root.page === "Appearance") { t.set("showNavigation", true); t.set("showTitleBar", true) } else root.page = "Appearance" }
    }
    WindowPreferences { window: root; theme: t }
    WindowEdges { parent: root.contentItem.parent; window: root; enabled: t.values.resizable }
}
