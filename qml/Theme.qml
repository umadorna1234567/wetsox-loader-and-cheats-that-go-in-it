import QtQuick
import Nexus

QtObject {
    id: root
    property string scope: "loader"
    property Item backdrop: null
    readonly property var values: AppState.revision >= 0 ? AppState.theme(scope) : ({})
    readonly property bool graphicsEffects: backdrop !== null && backdrop.GraphicsInfo.api !== GraphicsInfo.Software && backdrop.GraphicsInfo.api !== GraphicsInfo.Unknown
    readonly property bool linked: AppState.revision >= 0 && AppState.linked(scope)
    property real phase: 0
    property Timer clock: Timer {
        interval: 33; repeat: true
        running: root.values.animations && root.values.animationMode !== "Off" && root.backdrop && root.backdrop.Window.window && root.backdrop.Window.window.visible &&
                 (root.values.animatedBorders || ["Aurora","Particles","Grid","Waves","Halloween","Christmas"].indexOf(root.values.backgroundStyle) >= 0 || Object.keys(root.values).some(function(k) { return k.endsWith("Rainbow") && root.values[k] }))
        onTriggered: root.phase = (root.phase + 0.0033) % 10000
    }
    function colorFor(key) {
        return values[key + "Rainbow"] ? Qt.hsva((phase * Number(values[key + "RainbowSpeed"])) % 1, 0.65, 1, 1) : values[key]
    }
    readonly property color background: colorFor("background")
    readonly property color surface: colorFor("surface")
    readonly property color text: colorFor("text")
    readonly property color muted: colorFor("muted")
    readonly property color accent: values.gameTheme && scope !== "loader" && !values.accentRainbow ? (scope === "justcause4" ? "#ff9b3c" : scope === "farcry4" ? "#d1a0ff" : "#69bdd9") : colorFor("accent")
    readonly property color border: colorFor("border")
    readonly property string family: values.fontFamily
    readonly property real fontSize: values.fontSize * values.uiScale
    readonly property real radius: values.widgetStyle === "Square" ? 2 : values.widgetStyle === "Pill" ? 30 : values.radius
    readonly property real spacing: values.spacing * values.uiScale * (values.density === "Compact" ? 0.7 : values.density === "Spacious" ? 1.35 : 1)
    readonly property real controlHeight: values.controlHeight * values.uiScale
    readonly property real cardHeight: values.cardHeight * values.uiScale
    readonly property int duration: !values.animations || values.animationMode === "Off" ? 0 : (values.animationMode === "Minimal" ? 90 : 220) / values.animationSpeed
    readonly property real sidebarWidth: values.compactSidebar || values.navigationStyle === "Icon rail" ? 79 : values.sidebarWidth * values.uiScale
    function set(key, value) { AppState.setThemeValue(scope, key, value) }
    function sound(kind) { if (values.sounds && values[kind + "Sound"]) UiSound.play(kind, values.soundVolume) }
    function tint(value: color, opacity: real): color { return Qt.rgba(value.r, value.g, value.b, opacity) }
    function element(key) { return values.layoutElements[key] || ({}) }
    function setElement(key, property, value) {
        const all = Object.assign({}, values.layoutElements)
        all[key] = Object.assign({}, all[key] || ({}))
        all[key][property] = value
        set("layoutElements", all)
    }
    function hidden(key) { return !!element(key).hidden }
    function surfaceColor(kind) { return tint(kind === "control" ? background : surface, values[kind + "Opacity"]) }
    function moveSection(name, offset, sections) {
        const order = sections.map(function(s) { return s.name })
        const i = order.indexOf(name), j = Math.max(0, Math.min(order.length - 1, i + offset))
        if (i < 0) return
        order.splice(i, 1); order.splice(j, 0, name); set("sectionOrder", order)
    }
    function ordered(sections) {
        const order = values.sectionOrder
        return sections.slice().sort(function(a, b) {
            const ai = order.indexOf(a.name), bi = order.indexOf(b.name)
            return (ai < 0 ? 1000 + sections.indexOf(a) : ai) - (bi < 0 ? 1000 + sections.indexOf(b) : bi)
        })
    }
}
