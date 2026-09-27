import QtQuick
import QtQuick.Controls

TextField {
    id: root
    required property QtObject theme
    implicitHeight: theme.controlHeight
    leftPadding: 14; rightPadding: 14
    color: theme.text
    placeholderTextColor: theme.muted
    selectionColor: theme.accent
    selectedTextColor: theme.background
    font.family: theme.family
    font.pixelSize: theme.fontSize
    background: Rectangle {
        color: theme.background
        radius: Math.min(theme.radius, 10)
        border.color: root.activeFocus ? theme.accent : theme.border
    }
}
