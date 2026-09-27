import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root
    required property QtObject theme
    property string glyph: "home"
    property bool selected: false
    property bool compact: false
    implicitHeight: theme.controlHeight + 8
    hoverEnabled: true
    padding: 12
    contentItem: RowLayout {
        spacing: 14
        Glyph { name: root.glyph; color: root.selected ? Qt.lighter(theme.accent, 1.5) : theme.muted; Layout.preferredWidth: 21; Layout.preferredHeight: 21 }
        Text { visible: !root.compact; text: root.text; color: root.selected ? theme.text : theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize; Layout.fillWidth: true; elide: Text.ElideRight }
    }
    background: Rectangle {
        radius: Math.min(theme.radius, 9)
        color: root.selected ? theme.tint(theme.accent, 0.13) : root.hovered ? theme.tint(theme.text, 0.04) : "transparent"
        border.color: root.activeFocus ? theme.accent : "transparent"
        Rectangle { visible: root.selected; x: 0; y: 10; width: 2; height: parent.height - 20; radius: 1; color: theme.accent }
    }
    ToolTip.visible: hovered && compact
    ToolTip.text: text
}
