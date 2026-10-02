import QtQuick
import QtQuick.Controls
import QtQuick.Effects

Item {
    id: root
    required property QtObject theme
    required property var window
    anchors.fill: parent; z: 95
    property var trail: []
    HoverHandler { id: hover; cursorShape: theme.values.customCursor ? Qt.BlankCursor : Qt.ArrowCursor }
    Timer {
        interval: 40; repeat: true
        running: theme.values.customCursor && theme.values.cursorTrail && hover.hovered && window.visible
        onTriggered: { const list = root.trail.slice(-7); list.push({x:hover.point.position.x,y:hover.point.position.y}); root.trail = list }
    }
    Repeater {
        model: theme.values.customCursor && theme.values.cursorTrail && hover.hovered ? root.trail : []
        Rectangle {
            required property var modelData
            required property int index
            width: theme.values.cursorSize / 2; height: width; radius: width / 2
            x: modelData.x - width / 2; y: modelData.y - height / 2
            color: theme.accent; opacity: (index + 1) / 20
        }
    }
    Rectangle {
        visible: theme.values.customCursor && hover.hovered
        layer.enabled: theme.graphicsEffects && theme.values.cursorGlow > 0
        layer.effect: MultiEffect { shadowEnabled:true; shadowColor:theme.accent; shadowOpacity:theme.values.cursorGlow; shadowBlur:0.6; shadowVerticalOffset:0; blurMax:16 }

        x: hover.point.position.x - width / 2; y: hover.point.position.y - height / 2
        width: theme.values.cursorSize; height: width
        radius: theme.values.cursorShape === "Crosshair" ? 0 : width / 2
        color: theme.values.cursorShape === "Crosshair" ? "transparent" : theme.values.cursorShape === "Dot" ? theme.accent : theme.tint(theme.accent, theme.values.cursorGlow * 0.2)
        border.color: theme.accent; border.width: theme.values.cursorShape === "Ring" ? 2 : 0
        Rectangle {anchors.centerIn:parent;width:parent.width;height:2;color:theme.accent;visible:theme.values.cursorShape === "Crosshair"}
        Rectangle {anchors.centerIn:parent;width:2;height:parent.height;color:theme.accent;visible:theme.values.cursorShape === "Crosshair"}
    }
    ParallelAnimation {
        id: opening
        NumberAnimation { target: root.window.contentItem; property: "opacity"; from: root.theme.values.openAnimation === "None" ? 1 : 0; to: 1; duration: root.theme.duration * 1.5 }
        NumberAnimation { target: root.window.contentItem; property: "scale"; from: root.theme.values.openAnimation === "Scale" ? 0.96 : 1; to: 1; duration: root.theme.duration * 1.5; easing.type: Easing.OutCubic }
        NumberAnimation { target: shift; property: "y"; from: root.theme.values.openAnimation === "Slide" ? 24 : 0; to: 0; duration: root.theme.duration * 1.5 }
        NumberAnimation { target: root.window.background; property: "openingBlur"; from: root.theme.values.openAnimation === "Blur" ? 1 : 0; to: 0; duration: root.theme.duration * 1.5 }
    }
    Translate { id: shift }
    Component.onCompleted: window.contentItem.transform.push(shift)
    Connections {
        target: root.window
        function onVisibleChanged() {
            if (root.window.visible) opening.restart()
            root.theme.sound("window")
        }
    }
}
