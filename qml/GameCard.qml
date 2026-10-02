import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property QtObject theme
    required property var game
    signal openMenu()
    signal editGame()
    readonly property bool listMode: theme.values.cardLayout === "List"
    implicitWidth: theme.cardHeight * 2 / 3
    implicitHeight: listMode ? 112 : theme.cardHeight + 74
    CoverArt {
        id: cover
        width: root.listMode ? 70 : root.width; height: root.listMode ? 105 : theme.cardHeight
        game: root.game; radius: theme.radius; outside: theme.background
        showArtwork: theme.values.showArtwork; artOpacity: theme.values.artOpacity
    }
    Rectangle {
        anchors.fill: cover; radius: theme.radius; color: "transparent"
        border.width: launch.hovered || launch.activeFocus ? 2 : 1
        border.color: launch.hovered || launch.activeFocus ? theme.accent : theme.tint(theme.text, 0.08)
    }
    Rectangle {
        visible: !root.listMode; x: 10; y: theme.cardHeight - height - 10; width: parent.width - 20; height: 36
        radius: 6; color: "#dc090d19"
        opacity: launch.hovered || launch.activeFocus ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: theme.duration } }
        Text { anchors.centerIn: parent; text: "Open menu   →"; color: "#f4edff"; font.family: theme.family; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; font.weight: Font.Medium }
    }
    ColumnLayout {
        x: root.listMode ? 90 : 1; y: root.listMode ? 26 : theme.cardHeight + 13; width: root.width - x - 2; spacing: 5
        Text { text: root.game.name; color: theme.text; font.family: theme.family; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize + 1; font.weight: Font.DemiBold; Layout.fillWidth: true; elide: Text.ElideRight }
        RowLayout {
            Layout.fillWidth: true
            Text { text: "Single player"; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 2; Layout.fillWidth: true }
            Rectangle { implicitWidth: 57; implicitHeight: 19; radius: 4; color: theme.tint(theme.accent, 0.12)
                Text { anchors.centerIn: parent; text: root.game.ready ? "READY" : root.game.installed ? "UPDATE" : "NO MOD"; color: Qt.lighter(theme.accent, 1.6); font.family: theme.family; font.weight: theme.values.fontWeight; font.pixelSize: 8; font.letterSpacing: 0.7 }
            }
        }
    }
    Button {
        id: launch; anchors.fill: parent; hoverEnabled: true
        background: Item {} contentItem: Item {}
        Accessible.name: "Open " + root.game.name + " menu"
        onClicked: root.openMenu()
    }
    Button {
        anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 10
        width: 30; height: 28; hoverEnabled: true
        contentItem: Text { text: "•••"; color: "#ffffff"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: 6; color: "#cf090d19"; border.color: parent.activeFocus ? theme.accent : "#303449" }
        onClicked: root.editGame()
        Accessible.name: "Edit " + root.game.name
        ToolTip.visible: hovered; ToolTip.text: "Edit game & artwork"
    }
}
