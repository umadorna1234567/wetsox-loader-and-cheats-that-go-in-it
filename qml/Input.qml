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
    font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing
    background: Rectangle {
        color: theme.surfaceColor("control")
        radius: Math.min(theme.radius, height / 2)
        border.color: root.activeFocus ? theme.accent : theme.border
    }
}
