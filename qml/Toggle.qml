import QtQuick
import QtQuick.Controls

Switch {
    id: root
    required property QtObject theme
    implicitWidth: 36
    implicitHeight: 24
    padding: 0
    spacing: 0
    indicator: Rectangle {
        x: 0; y: (root.height - height) / 2
        width: 36; height: 20; radius: 10
        color: root.checked ? theme.accent : theme.border
        border.color: root.visualFocus ? theme.text : "transparent"
        border.width: 2
        Rectangle {
            x: root.checked ? 19 : 3; y: 3
            width: 14; height: 14; radius: 7
            color: root.checked ? "#f5f0ff" : theme.muted
            Behavior on x { NumberAnimation { duration: theme.duration } }
        }
        Behavior on color { ColorAnimation { duration: theme.duration } }
    }
    contentItem: Item {}
}
