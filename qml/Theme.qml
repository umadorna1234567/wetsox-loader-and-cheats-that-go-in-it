import QtQuick
import Nexus

QtObject {
    id: root
    property string scope: "loader"
    readonly property var values: AppState.revision >= 0 ? AppState.theme(scope) : ({})
    readonly property bool linked: AppState.revision >= 0 && AppState.linked(scope)
    readonly property color background: values.background
    readonly property color surface: values.surface
    readonly property color text: values.text
    readonly property color muted: values.muted
    readonly property color accent: values.accent
    readonly property color border: values.border
    readonly property string family: values.fontFamily
    readonly property real fontSize: values.fontSize
    readonly property real radius: values.radius
    readonly property real spacing: values.spacing
    readonly property real controlHeight: values.controlHeight
    readonly property real cardHeight: values.cardHeight
    readonly property int duration: values.animations ? 150 : 0
    function set(key, value) { AppState.setThemeValue(scope, key, value) }
    function tint(color, opacity) { return Qt.rgba(color.r, color.g, color.b, opacity) }
}
