import QtQuick
import QtQuick.Controls

Switch {
    id: root
    required property QtObject theme
    implicitWidth: 36
    implicitHeight: 24
    onToggled: theme.sound("toggle")
    padding: 0
    spacing: 0
    indicator: Rectangle {
        x: 0; y: (root.height - height) / 2
        width: 36; height: 20; radius: theme.values.widgetStyle === "Square" ? 2 : 10
        color: root.checked ? theme.colorFor("enabledColor") : theme.colorFor("disabledColor")
        border.color: root.visualFocus ? theme.text : "transparent"
        border.width: 2
        Rectangle {
            x: root.checked ? 19 : 3; y: 3
            width: 14; height: 14; radius: 7
            color: root.checked ? "#f5f0ff" : theme.muted
            opacity: theme.values.toggleAnimation === "Fade" && !root.checked ? 0.55 : 1
            Behavior on opacity { NumberAnimation { duration: theme.duration } }
            Behavior on x { NumberAnimation { duration: theme.duration; easing.type: theme.values.toggleAnimation === "Spring" ? Easing.OutBack : Easing.OutCubic } }
        }
        Behavior on color { ColorAnimation { duration: theme.duration } }
    }
    contentItem: Item {}
}
