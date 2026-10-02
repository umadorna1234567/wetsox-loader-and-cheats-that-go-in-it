import QtQuick
import QtQuick.Effects

Rectangle {
    id: root
    required property QtObject theme
    property string kind: "card"
    property color fillColor: theme.surface
    property real effectGlow: 0
    property bool decorated: true
    radius: theme.radius
    color: decorated ? theme.tint(fillColor, theme.values[kind + "Opacity"]) : "transparent"
    border.width: decorated ? theme.values.borderWidth : 0
    border.color: theme.tint(theme.values.gradientBorders ? theme.colorFor("gradient1") : theme.border, theme.values.borderOpacity)
    // Sample only the window artwork, avoiding feedback from the panel's own controls.
    ShaderEffectSource {
        id: sample
        anchors.fill: parent; visible: false
        sourceItem: root.theme.graphicsEffects && root.decorated && root.kind !== "control" && root.theme.values.glass ? root.theme.backdrop : null
        sourceRect: {
            let item = root
            let px = 0, py = 0
            while (item && item !== root.theme.backdrop.parent) { px += item.x; py += item.y; item = item.parent }
            return Qt.rect(px, py, root.width, root.height)
        }
        live: true; recursive: false
        z: -1
    }
    Rectangle {
        id: frostMask; anchors.fill:parent; radius:root.radius; color:"white"; visible:false; layer.enabled:true
    }
    MultiEffect {
        anchors.fill:parent; z:-1; source:sample
        visible:sample.sourceItem !== null
        blurEnabled:true; blurMax:64; blur:theme.values.blurStrength
        maskEnabled:true; maskSource:frostMask; autoPaddingEnabled:false
    }
    Rectangle {
        anchors.fill:parent;radius:root.radius
        visible:root.decorated && theme.values.glass && root.kind !== "control"
        color:theme.tint(theme.colorFor("glassColor"), theme.values.glassTint)
    }
    Rectangle {
        anchors.fill: parent; radius: parent.radius
        visible: root.decorated && theme.values.glass
        gradient: Gradient {
            GradientStop { position: 0; color: theme.tint(theme.colorFor("glassColor"), 0.17) }
            GradientStop { position: 0.45; color: theme.tint(theme.colorFor("glassColor"), 0.025) }
            GradientStop { position: 1; color: theme.tint(theme.colorFor("glassColor"), 0.08) }
        }
        border.width: 1; border.color: theme.tint(theme.colorFor("glassColor"), 0.22)
    }
    Rectangle {
        anchors.fill: parent; anchors.margins: 2; radius: Math.max(0, root.radius - 2)
        color: "transparent"; border.color: theme.tint(theme.colorFor("glassColor"), 0.08)
        visible: root.decorated && theme.values.glass
    }
    Rectangle {
        anchors.fill: parent; radius: parent.radius; color: "transparent"
        border.width: theme.values.borderWidth
        border.color: theme.tint(Qt.hsva(theme.phase % 1, 0.6, 1, 1), theme.values.glowStrength)
        visible: root.decorated && theme.values.animatedBorders
    }
    Canvas {
        anchors.fill: parent
        visible: root.decorated && (theme.values.gradientBorders || theme.values.animatedBorders)
        property real phase: theme.values.animatedBorders ? theme.phase : 0
        property color first: theme.colorFor("gradient1")
        property color second: theme.colorFor("gradient2")
        property color third: theme.colorFor("gradient3")
        property real strokeWidth: theme.values.borderWidth
        property real strokeOpacity: theme.values.borderOpacity
        property string direction: theme.values.gradientDirection
        property string stops: theme.values.gradientStops
        onDirectionChanged: requestPaint()
        onStopsChanged: requestPaint()
        property real corners: root.radius
        onThirdChanged: requestPaint()
        onStrokeWidthChanged: requestPaint()
        onStrokeOpacityChanged: requestPaint()
        onCornersChanged: requestPaint()
        onPhaseChanged: if (visible) requestPaint()
        onFirstChanged: requestPaint()
        onSecondChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onVisibleChanged: requestPaint()
        onPaint: {
            const c = getContext("2d"); c.clearRect(0, 0, width, height); if(strokeWidth<=0)return
            const inset = Math.max(0.5, theme.values.borderWidth / 2), r = Math.min(root.radius, width / 2, height / 2)
            const angle = phase * 6.28318 + (direction === "Vertical" ? Math.PI/2 : direction === "Diagonal" ? Math.PI/4 : 0)
            const grad = c.createLinearGradient(width * (0.5 - 0.5 * Math.cos(angle)), height * (0.5 - 0.5 * Math.sin(angle)), width * (0.5 + 0.5 * Math.cos(angle)), height * (0.5 + 0.5 * Math.sin(angle)))
            grad.addColorStop(0, first); grad.addColorStop(stops === "2" ? 1 : 0.5, second); if(stops !== "2") grad.addColorStop(1, third)
            c.strokeStyle = grad; c.globalAlpha = theme.values.borderOpacity; c.lineWidth = theme.values.borderWidth
            c.beginPath(); c.moveTo(r, inset); c.lineTo(width-r, inset); c.quadraticCurveTo(width-inset,inset,width-inset,r)
            c.lineTo(width-inset,height-r); c.quadraticCurveTo(width-inset,height-inset,width-r,height-inset)
            c.lineTo(r,height-inset); c.quadraticCurveTo(inset,height-inset,inset,height-r)
            c.lineTo(inset,r); c.quadraticCurveTo(inset,inset,r,inset); c.closePath(); c.stroke()
        }
    }
    layer.enabled: theme.graphicsEffects && root.decorated && ((root.kind !== "control" && (theme.values.shadowStrength > 0 || theme.values.glowStrength > 0.2)) || root.effectGlow > 0)
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: root.effectGlow > 0 || theme.values.glowStrength > 0.2 ? theme.accent : "#000000"
        shadowOpacity: root.kind === "control" ? root.effectGlow : Math.max(theme.values.shadowStrength, theme.values.glowStrength * 0.5)
        shadowBlur: theme.values.shadowSoftness
        shadowVerticalOffset: theme.values.shadowSpread
        blurMax: 32
    }
}
