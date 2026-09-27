import QtQuick
import QtQuick.Controls
import Nexus

Button {
    id: root
    required property QtObject theme
    property string binding: ""
    readonly property bool listening: recorder.listening
    signal edited(string nextValue)
    implicitHeight: Math.max(27, theme.controlHeight - 8)
    implicitWidth: 142
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    text: listening ? "Press input…" : binding || "Unbound"
    onClicked: { forceActiveFocus(Qt.MouseFocusReason); recorder.begin() }
    onVisibleChanged: if (!visible) recorder.cancel()
    onEnabledChanged: if (!enabled) recorder.cancel()
    InputRecorder {
        id: recorder
        objectName: root.objectName + "Recorder"
        window: root.visible ? root.Window.window : null
        onRecorded: function(value) { root.edited(value) }
    }
    contentItem: Text {
        text: root.text; color: root.listening ? theme.accent : theme.text
        font.family: theme.family; font.pixelSize: theme.fontSize
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: Math.min(theme.radius, 8)
        color: root.listening ? theme.tint(theme.accent, 0.1) : theme.background
        border.color: root.listening || root.visualFocus ? theme.accent : theme.border
    }
    ToolTip.visible: listening || hovered
    ToolTip.delay: listening ? 0 : 500
    ToolTip.timeout: -1
    ToolTip.text: listening ? "Press a key or mouse button. Esc cancels; Backspace clears." : "Click to record a key or mouse button"
    Accessible.name: "Key binding: " + (binding || "unbound")
}
