import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property QtObject theme
    required property string title
    required property string description
    property string shortcut: ""
    property bool checked: false
    signal toggled(bool value)
    implicitHeight: content.implicitHeight + 28
    color: checked ? theme.tint(theme.accent, 0.05) : theme.surface
    radius: theme.radius
    border.color: checked ? theme.tint(theme.accent, 0.35) : theme.border
    RowLayout {
        id: content
        anchors.fill: parent; anchors.margins: 14; spacing: 14
        ColumnLayout {
            Layout.fillWidth: true; spacing: 5
            Text { text: root.title; color: theme.text; font.family: theme.family; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; font.weight: Font.Medium; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Text { text: root.description; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 2; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        }
        Text { text: root.shortcut; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 2; visible: text.length > 0 }
        Toggle {
            theme: root.theme; checked: root.checked
            onToggled: root.toggled(checked)
            Accessible.name: root.title + " preview"
        }
    }
}
