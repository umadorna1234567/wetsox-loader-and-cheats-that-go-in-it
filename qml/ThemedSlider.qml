import QtQuick
import QtQuick.Controls

Slider {
    id: control
    required property QtObject theme
    implicitWidth: 92
    implicitHeight: 24
    padding: 0
    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + (control.availableHeight - height) / 2
        width: control.availableWidth
        height: 4
        radius: 2
        color: control.theme.border
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            color: control.theme.accent
        }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + (control.availableHeight - height) / 2
        implicitWidth: 12
        implicitHeight: 12
        radius: width / 2
        color: Qt.lighter(control.theme.accent, 1.8)
        border.color: control.visualFocus ? control.theme.text : "transparent"
    }
}
