import QtQuick

Item {
    id: root
    required property QtObject theme
    required property Item target
    property string page: ""
    property var previousGrab: null
    property bool ready: false
    x: target.mapToItem(parent, 0, 0).x; y: target.mapToItem(parent, 0, 0).y; width: target.width; height: target.height; z: 30
    onPageChanged: {
        if (!ready || !target || theme.duration === 0) return
        ghost.source = theme.values.pageTransition === "Crossfade" && previousGrab ? previousGrab.url : ""
        ghost.opacity = theme.values.pageTransition === "Crossfade" ? 1 : 0
        ghostFade.restart()
        target.opacity = 0
        target.scale = theme.values.pageTransition === "Zoom" ? 0.96 : 1
        slide.x = theme.values.pageTransition === "Slide" ? 24 : 0
        entrance.restart()
    }
    Image { id: ghost; anchors.fill: parent; opacity: 0; fillMode: Image.Stretch }
    NumberAnimation { id: ghostFade; target: ghost; property: "opacity"; to: 0; duration: theme.duration; onStopped: ghost.source = "" }
    Translate { id: slide }
    Component.onCompleted: {
        target.transform.push(slide)
        ready = true; capture.restart()
    }
    ParallelAnimation {
        id: entrance
        NumberAnimation { target: root.target; property: "opacity"; to: 1; duration: root.theme.duration }
        NumberAnimation { target: root.target; property: "scale"; to: 1; duration: root.theme.duration; easing.type: Easing.OutCubic }
        NumberAnimation { target: slide; property: "x"; to: 0; duration: root.theme.duration; easing.type: Easing.OutCubic }
        onStopped: capture.restart()
    }
    Timer { id: capture; interval: 250; onTriggered: if (root.theme.values.pageTransition === "Crossfade" && root.target.visible && root.target.Window.window && root.target.Window.window.visible && root.target.width > 0 && root.target.height > 0) root.target.grabToImage(function(result) { root.previousGrab = result }) }
}
