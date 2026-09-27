import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    required property QtObject theme
    required property string label
    required property string setting
    property real from: 0
    property real to: 100
    property real stepSize: 1
    property string suffix: " px"
    spacing: 3
    RowLayout {
        Layout.fillWidth: true
        Text { text: root.label; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize; Layout.fillWidth: true }
        Text {
            text: root.stepSize < 1 ? Math.round(slider.value * 100) + "%" : Math.round(slider.value) + root.suffix
            color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize - 1
        }
    }
    Slider {
        id: slider
        Layout.fillWidth: true
        from: root.from; to: root.to; stepSize: root.stepSize
        value: theme.values[root.setting]
        onMoved: theme.set(root.setting, value)
        background: Rectangle {
            x: slider.leftPadding; y: slider.topPadding + slider.availableHeight / 2 - 2
            width: slider.availableWidth; height: 4; radius: 2; color: theme.border
            Rectangle { width: slider.visualPosition * parent.width; height: 4; radius: 2; color: theme.accent }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 16; height: 16; radius: 8; color: theme.accent
            border.width: slider.activeFocus ? 2 : 0; border.color: theme.text
        }
    }
}
