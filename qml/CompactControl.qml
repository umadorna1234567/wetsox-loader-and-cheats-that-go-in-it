import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

RowLayout {
    id: root
    required property QtObject theme
    required property var spec
    required property var value
    signal edited(var nextValue)
    spacing: 10
    implicitHeight: Math.max(28, theme.controlHeight - 7)
    Text {
        text: root.spec.label; color: theme.muted
        font.family: theme.family; font.pixelSize: theme.fontSize
        Layout.fillWidth: true; Layout.minimumWidth: 70; elide: Text.ElideRight
    }
    Toggle {
        objectName: root.spec.key + "Toggle"
        theme: root.theme
        visible: !root.spec.type || root.spec.type === "toggle"
        checked: !!root.value
        onToggled: root.edited(checked)
        Accessible.name: root.spec.label
    }
    KeybindPicker {
        objectName: root.spec.key + "Keybind"
        theme: root.theme
        visible: root.spec.type === "keybind"
        Layout.preferredWidth: 142
        binding: String(root.value || "")
        onEdited: function(nextValue) { root.edited(nextValue) }
    }
    Choice {
        theme: root.theme
        visible: root.spec.type === "choice"
        Layout.preferredWidth: 142
        implicitHeight: Math.max(27, theme.controlHeight - 8)
        model: root.spec.options || []
        currentIndex: Math.max(0, (root.spec.options || []).indexOf(root.value))
        onActivated: root.edited(currentText)
        Accessible.name: root.spec.label
    }
    Button {
        visible: root.spec.type === "color"
        implicitWidth: 58; implicitHeight: 25
        background: Rectangle { color: root.spec.type === "color" ? String(root.value) : "transparent"; radius: theme.radius; border.color: theme.border }
        onClicked: { picker.selectedColor = String(root.value); picker.open() }
        Accessible.name: root.spec.label
    }
    ColorDialog { id: picker; title: root.spec.label; onAccepted: root.edited(selectedColor.toString()) }
    Slider {
        id: slider
        visible: root.spec.type === "slider"
        Layout.preferredWidth: 92
        implicitHeight: 24
        from: root.spec.min || 0
        to: root.spec.max || 3
        stepSize: root.spec.step || 0.1
        value: Number(root.value) || 0
        onMoved: root.edited(value)
        background: Rectangle {
            x: slider.leftPadding; y: slider.topPadding + slider.availableHeight / 2 - 2
            width: slider.availableWidth; height: 4; radius: 2; color: theme.border
            Rectangle { width: slider.visualPosition * parent.width; height: 4; radius: 2; color: theme.accent }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 12; height: 12; radius: 6; color: Qt.lighter(theme.accent, 1.8)
            border.color: slider.activeFocus ? theme.text : "transparent"
        }
        Accessible.name: root.spec.label
    }
    Text {
        visible: root.spec.type === "slider"
        text: Number(root.value).toFixed(root.spec.step === 1 ? 0 : (root.spec.step === 0.01 ? 2 : 1)) + (root.spec.unit || "")
        color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize - 1
        Layout.preferredWidth: 42; horizontalAlignment: Text.AlignRight
    }
}
