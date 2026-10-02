import QtQuick
import QtQuick.Controls

Button {
    id: root
    required property QtObject theme
    property bool primary: false
    property bool selected: false
    implicitHeight: theme.controlHeight
    implicitWidth: Math.max(implicitContentWidth + 30, 42)
    padding: 10
    hoverEnabled: true
    font.family: theme.family
    font.pixelSize: theme.fontSize
    onHoveredChanged: if (hovered) theme.sound("hover")
    onClicked: { theme.sound("click"); ripple.restart() }
    font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing
    scale: hovered && theme.values.hoverEffect === "Scale" ? 1.03 : 1
    Behavior on scale { NumberAnimation { duration: theme.duration } }
    opacity: enabled ? 1 : 0.45
    contentItem: Text {
        text: root.text
        font: root.font
        color: root.primary ? "#ffffff" : root.selected ? Qt.lighter(theme.accent, 1.4) : theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: GlassSurface {
        theme: root.theme; kind: "control"; effectGlow: (root.selected || root.hovered && theme.values.hoverEffect === "Glow") ? theme.values.glowStrength : 0;
        radius: Math.min(theme.radius, height / 2)
        color: root.primary ? (root.hovered && theme.values.hoverEffect === "Brighten" ? Qt.lighter(theme.accent, 1.15) : theme.accent) : root.selected || root.hovered && ["Brighten", "Glow"].indexOf(theme.values.hoverEffect) >= 0 ? theme.tint(theme.accent, 0.12) : theme.surfaceColor("control")
        border.color: (root.visualFocus || root.hovered && theme.values.hoverEffect === "Border") ? theme.accent : root.selected ? theme.tint(theme.accent, 0.5) : theme.border
        border.width: root.visualFocus ? 2 : 1
        scale: root.down && ["Pulse","Shrink","Bounce"].indexOf(theme.values.clickEffect) >= 0 ? (theme.values.clickEffect === "Pulse" ? 1.04 : 0.96) : 1
        Behavior on color { ColorAnimation { duration: theme.duration } }
        Behavior on scale { NumberAnimation { duration: theme.duration; easing.type: theme.values.clickEffect === "Bounce" ? Easing.OutBack : Easing.OutCubic } }
        Rectangle {
            id: splash; anchors.centerIn: parent; width: parent.width * 0.9; height: parent.height * 0.9; radius: parent.radius
            color: theme.tint(theme.accent, 0.3); opacity: 0; scale: 0.1
        }
    }
    ParallelAnimation {
        id: ripple
        NumberAnimation { target: splash; property: "scale"; from: 0.1; to: 1; duration: theme.values.clickEffect === "Ripple" ? theme.duration * 2 : 0 }
        NumberAnimation { target: splash; property: "opacity"; from: theme.values.clickEffect === "Ripple" ? 1 : 0; to: 0; duration: theme.duration * 2 }
    }
}
