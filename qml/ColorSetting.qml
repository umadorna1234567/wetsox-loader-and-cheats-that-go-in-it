import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: root
    required property QtObject theme
    required property string label
    required property string setting
    spacing: 6
    RowLayout {
    Layout.fillWidth: true; spacing: 12
    Text { text: root.label; color: theme.text; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; Layout.fillWidth: true }
    Button {
        implicitWidth: 32; implicitHeight: 32
        Accessible.name: root.label + " color picker"
        background: Rectangle { color: theme.colorFor(root.setting); radius: 8; border.color: parent.activeFocus ? theme.accent : theme.border; border.width: 2 }
        onClicked: { picker.selectedColor = theme.values[root.setting]; picker.open() }
    }
    Input {
        id: hex
        theme: root.theme
        Layout.preferredWidth: 108
        text: theme.values[root.setting]
        maximumLength: 7
        validator: RegularExpressionValidator { regularExpression: /#[0-9a-fA-F]{6}/ }
        onEditingFinished: {
            if (acceptableInput) theme.set(root.setting, text)
            text = Qt.binding(function() { return theme.values[root.setting] })
        }
        Accessible.name: root.label + " hex color"
    }
    }
    RowLayout {
        Layout.fillWidth: true
        Text { text: "Rainbow"; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 1 }
        Toggle { theme: root.theme; checked: !!theme.values[root.setting + "Rainbow"]; onToggled: theme.set(root.setting + "Rainbow", checked); Accessible.name: root.label + " rainbow" }
        ThemedSlider { theme: root.theme; Layout.fillWidth: true; from: 0.1; to: 5; stepSize: 0.1; value: theme.values[root.setting + "RainbowSpeed"]; onMoved: theme.set(root.setting + "RainbowSpeed", value); Accessible.name: root.label + " rainbow speed" }
        Text { text: Number(theme.values[root.setting + "RainbowSpeed"]).toFixed(1) + "×"; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 1 }
    }
    ColorDialog {
        id: picker
        title: "Choose " + root.label.toLowerCase()
        onAccepted: theme.set(root.setting, selectedColor.toString())
    }
}
