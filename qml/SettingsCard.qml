import QtQuick
import QtQuick.Layouts

GlassSurface {
    id: root
    property string title: ""
    default property alias contents: body.data
    property real padding: 14
    property bool fillContent: false
    implicitWidth:320
    implicitHeight: body.implicitHeight + 44 + padding
    radius: Math.min(theme.radius + 2, 16)
    color: theme.tint(theme.surface, theme.values.cardOpacity)
    border.color: Qt.tint(theme.tint(theme.border, theme.values.borderOpacity), theme.tint(theme.accent, 0.12))
    Rectangle { x: root.padding; y: 0; width: Math.min(64, parent.width / 3); height: 2; radius: 1; color: theme.tint(theme.accent, 0.8) }
    Text {
        x: root.padding; y: 13
        text: root.title; color: theme.text
        font.family: theme.family; font.pixelSize: Math.max(12, theme.fontSize + 1); font.weight: Font.DemiBold
    }
    ColumnLayout {
        id: body
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.bottom: root.fillContent ? parent.bottom : undefined
        anchors.bottomMargin: root.padding
        anchors.leftMargin: root.padding; anchors.rightMargin: root.padding; anchors.topMargin: 44
        spacing: 6
    }
}
