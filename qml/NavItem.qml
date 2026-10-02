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
    onHoveredChanged: if (hovered) theme.sound("hover")
    onClicked: theme.sound("click")
    scale: hovered && theme.values.hoverEffect === "Scale" ? 1.03 : 1
    Behavior on scale { NumberAnimation { duration: theme.duration } }
    padding: 12
    topPadding: 6; bottomPadding: 6
    contentItem: RowLayout {
        spacing: 14
        Glyph { name: root.glyph; color: root.selected ? Qt.lighter(theme.accent, 1.5) : theme.muted; Layout.preferredWidth: 21; Layout.preferredHeight: 21 }
        Text { visible: !root.compact; text: root.text; color: root.selected ? theme.text : theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; Layout.fillWidth: true; elide: Text.ElideRight }
    }
    background: GlassSurface {
        theme: root.theme; kind: "control"; effectGlow: root.selected || root.hovered && theme.values.hoverEffect === "Glow" ? theme.values.glowStrength : 0; decorated: root.selected || root.hovered && theme.values.hoverEffect === "Glow"
        radius: Math.min(theme.radius, height / 2)
        color: root.selected ? theme.tint(theme.accent, 0.32) : root.hovered && ["Brighten","Glow"].indexOf(theme.values.hoverEffect) >= 0 ? theme.tint(theme.text, 0.04) : "transparent"
        border.color: root.visualFocus || root.selected || root.hovered && theme.values.hoverEffect === "Border" ? theme.tint(theme.accent, 0.85) : "transparent"
        Rectangle {
            anchors.fill:parent;radius:parent.radius;visible:root.selected
            gradient:Gradient {orientation:Gradient.Horizontal;GradientStop {position:0;color:theme.tint(theme.accent,0.42)} GradientStop {position:1;color:theme.tint(theme.accent,0.08)}}
        }
    }
    ToolTip.visible: hovered && compact && theme.values.showHints
    ToolTip.text: text
}
