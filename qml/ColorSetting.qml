import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

RowLayout {
    id: root
    required property QtObject theme
    required property string label
    required property string setting
    spacing: 12
    Text { text: root.label; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize; Layout.fillWidth: true }
    Button {
        implicitWidth: 32; implicitHeight: 32
        Accessible.name: root.label + " color picker"
        background: Rectangle { color: theme.values[root.setting]; radius: 8; border.color: parent.activeFocus ? theme.accent : theme.border; border.width: 2 }
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
    ColorDialog {
        id: picker
        title: "Choose " + root.label.toLowerCase()
        onAccepted: theme.set(root.setting, selectedColor.toString())
    }
}
