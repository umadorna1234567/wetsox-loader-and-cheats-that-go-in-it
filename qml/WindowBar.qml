import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property QtObject theme
    required property var window
    property string label: "NEXUS"
    property bool showThemeButtons: false
    signal lightTheme()
    signal darkTheme()
    implicitHeight: 54
    MouseArea {
        anchors.fill: parent
        onPressed: root.window.startSystemMove()
        onDoubleClicked: root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized()
    }
    RowLayout {
        anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 10; spacing: 6
        Text { text: root.label; color: theme.muted; font.family: theme.family; font.pixelSize: 10; font.letterSpacing: 1.8; Layout.fillWidth: true }
        Repeater {
            model: root.showThemeButtons ? ["sun", "moon"] : []
            Button {
                required property string modelData
                implicitWidth: 32; implicitHeight: 30; hoverEnabled: true
                contentItem: Glyph { name: modelData; color: theme.muted }
                padding: 7
                background: Rectangle { radius: 7; color: parent.hovered ? theme.tint(theme.accent, 0.13) : "transparent" }
                onClicked: modelData === "sun" ? root.lightTheme() : root.darkTheme()
                Accessible.name: modelData === "sun" ? "Apply light theme" : "Apply dark theme"
            }
        }
        Item { width: 8 }
        Repeater {
            model: ["minimize", "maximize", "close"]
            Button {
                required property string modelData
                implicitWidth: 38; implicitHeight: 32; padding: 8; hoverEnabled: true
                contentItem: Glyph { name: modelData; color: theme.muted }
                background: Rectangle { radius: 7; color: parent.hovered ? (modelData === "close" ? "#803442" : theme.tint(theme.text, 0.08)) : "transparent" }
                onClicked: {
                    if (modelData === "close") root.window.close()
                    else if (modelData === "minimize") root.window.showMinimized()
                    else root.window.visibility === Window.Maximized ? root.window.showNormal() : root.window.showMaximized()
                }
                Accessible.name: modelData + " window"
            }
        }
    }
}
