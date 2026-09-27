import QtQuick
import QtQuick.Controls

Button {
    id: root
    required property QtObject theme
    property bool primary: false
    property bool selected: false
    implicitHeight: theme.controlHeight
    implicitWidth: Math.max(implicitContentWidth + 30, 42)
    padding: 10
    hoverEnabled: true
    font.family: theme.family
    font.pixelSize: theme.fontSize
    opacity: enabled ? 1 : 0.45
    contentItem: Text {
        text: root.text
        font: root.font
        color: root.primary ? "#ffffff" : root.selected ? Qt.lighter(theme.accent, 1.4) : theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: Math.min(theme.radius, 8)
        color: root.primary ? (root.hovered ? Qt.lighter(theme.accent, 1.15) : theme.accent) : root.selected || root.hovered ? theme.tint(theme.accent, 0.12) : theme.surface
        border.color: root.activeFocus ? theme.accent : root.selected ? theme.tint(theme.accent, 0.5) : theme.border
        border.width: root.activeFocus ? 2 : 1
        scale: root.down ? 0.98 : 1
        Behavior on color { ColorAnimation { duration: theme.duration } }
        Behavior on scale { NumberAnimation { duration: theme.duration } }
    }
}
