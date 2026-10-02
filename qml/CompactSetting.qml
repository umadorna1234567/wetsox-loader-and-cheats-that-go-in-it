import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    required property QtObject theme
    required property string label
    required property string setting
    property string type: "number"
    property real controlWidth:92
    property var options: []
    property real from: 0
    property real to: 1
    property real stepSize: 0.01
    property bool percent: to <= 1.5 && stepSize < 1
    opacity: enabled ? 1 : 0.45
    implicitWidth:260
    spacing: 8
    implicitHeight: Math.max(29, theme.fontSize + 12)
    Text { text: root.label; color: theme.values.glass ? theme.text : theme.muted; font.family: theme.family; font.weight:theme.values.fontWeight; font.letterSpacing:theme.values.letterSpacing; font.pixelSize: Math.max(11, theme.fontSize - 1); Layout.fillWidth: true; Layout.minimumWidth: 0; elide: Text.ElideRight }
    Toggle { objectName: root.type === "toggle" ? "appearance_" + root.setting : ""; theme: root.theme; visible: root.type === "toggle"; checked: !!theme.values[root.setting]; onToggled: theme.set(root.setting, checked); Accessible.name: root.label }
    Choice {
        objectName: root.type === "choice" ? "appearance_" + root.setting : ""
        theme: root.theme; visible: root.type === "choice"; implicitHeight: 26
        Layout.preferredWidth: 140
        model: root.options; currentIndex: Math.max(0, root.options.indexOf(theme.values[root.setting])); onActivated: theme.set(root.setting, currentText)
        Accessible.name: root.label
    }
    Slider {
        id: slider
        objectName: root.type === "number" ? "appearance_" + root.setting : ""
        visible: root.type === "number"; Layout.preferredWidth: root.controlWidth; implicitHeight: 24
        from: root.from; to: root.to; stepSize: root.stepSize; value: Number(theme.values[root.setting])
        onMoved: theme.set(root.setting, value)
        background: Rectangle {
            x: slider.leftPadding; y: slider.topPadding + slider.availableHeight / 2 - 2
            width: slider.availableWidth; height: 4; radius: 2; color: theme.border
            Rectangle { width: slider.visualPosition * parent.width; height: 4; radius: 2; color: theme.accent }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 10; height: 10; radius: 5; color: Qt.lighter(theme.accent, 1.8)
            border.color: slider.visualFocus ? theme.text : theme.accent
        }
        Accessible.name: root.label
    }
    Text {
        visible: root.type === "number"; Layout.preferredWidth: 43
        text: root.percent ? Math.round(slider.value * 100) + "%" : Number(slider.value).toFixed(root.stepSize < 1 ? 1 : 0)
        color: theme.muted; font.family: theme.family; font.pixelSize: Math.max(10, theme.fontSize - 2); horizontalAlignment: Text.AlignRight
    }
}
