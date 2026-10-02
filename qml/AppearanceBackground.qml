import QtQuick
import QtQuick.Effects

Rectangle {
    id: root
    required property QtObject theme
    property url gameArtwork: ""
    property real openingBlur: 0
    color: theme.tint(theme.background, theme.values.windowOpacity)
    radius: theme.radius; clip: true
    Item {
        id: art
        anchors.fill: parent
        visible: !theme.graphicsEffects
        Rectangle {
            anchors.fill: parent
            visible: theme.values.backgroundStyle !== "Solid" && theme.values.backgroundStyle !== "Image"
            opacity: theme.values.gradientIntensity
            gradient: Gradient {
                orientation: theme.values.gradientDirection === "Horizontal" ? Gradient.Horizontal : Gradient.Vertical
                GradientStop { position: 0; color: theme.colorFor("gradient1") }
                GradientStop { position: theme.values.gradientStops === "3" ? 0.5 : 1; color: theme.colorFor("gradient2") }
                GradientStop { position: 1; color: theme.colorFor(theme.values.gradientStops === "3" ? "gradient3" : "gradient2") }
            }
            rotation: theme.values.gradientDirection === "Diagonal" ? 15 : 0
            scale: theme.values.gradientDirection === "Diagonal" ? 1.5 : 1
        }
        Image {
            anchors.fill: parent; source: theme.values.backgroundStyle === "Image" ? theme.values.backgroundImage : theme.values.gameTheme ? root.gameArtwork : ""
            fillMode: Image.PreserveAspectCrop; opacity: theme.values.artOpacity * (theme.values.backgroundStyle === "Image" ? 1 : 0.25)
        }
        Repeater {
            model: ["Aurora", "Waves"].indexOf(theme.values.backgroundStyle) >= 0 ? 5 : 0
            Rectangle {
                required property int index
                width: art.width * 0.65; height: art.height * 0.65; radius: width / 2
                x: art.width * (0.2 + 0.35 * Math.sin(theme.phase * theme.values.backgroundMotion + index * 1.9)) - width / 3
                y: art.height * (0.3 + 0.4 * Math.cos(theme.phase * theme.values.backgroundMotion + index)) - height / 3
                color: theme.colorFor(index % 2 ? "gradient1" : "gradient2"); opacity: theme.values.gradientIntensity * 0.32
            }
        }
        Canvas {
            anchors.fill: parent
            visible: theme.values.backgroundStyle === "Grid"
            property color lineColor: theme.accent
            property real intensity: theme.values.gradientIntensity
            onLineColorChanged: requestPaint()
            onIntensityChanged: requestPaint()
            onVisibleChanged: requestPaint()
            property real motion: theme.phase * theme.values.backgroundMotion
            onMotionChanged: if (visible) requestPaint()
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                const c = getContext("2d"); c.clearRect(0, 0, width, height)
                c.strokeStyle = theme.accent; c.globalAlpha = theme.values.gradientIntensity * 0.3; c.lineWidth = 1
                c.beginPath()
                const offset = (motion * 20) % 40
                for (let x = offset; x < width; x += 40) { c.moveTo(x, 0); c.lineTo(x, height) }
                for (let y = offset; y < height; y += 40) { c.moveTo(0, y); c.lineTo(width, y) }
                c.stroke()
            }
        }
        Repeater {
            model: ["Particles", "Halloween", "Christmas"].indexOf(theme.values.backgroundStyle) >= 0 ? 35 : 0
            Text {
                required property int index
                text: theme.values.backgroundStyle === "Halloween" ? (index % 2 ? "✦" : "🎃") : theme.values.backgroundStyle === "Christmas" ? "❄" : "·"
                font.pixelSize: theme.values.backgroundStyle === "Particles" ? 26 : 15 + index % 12
                color: theme.values.backgroundStyle === "Halloween" ? "#ff8a24" : "#e9f5ff"
                opacity: 0.2 + index % 5 * 0.1
                x: ((index * 127.3 + Math.sin(theme.phase + index) * 22) % Math.max(1, art.width))
                y: ((index * 97 + theme.phase * theme.values.backgroundMotion * 35) % Math.max(1, art.height))
            }
        }
    }
    MultiEffect {
        anchors.fill: art; source: art; visible: theme.graphicsEffects; autoPaddingEnabled: false
        blurEnabled: theme.values.glass || root.openingBlur > 0
        blurMax: 64; blur: Math.max(root.openingBlur, theme.values.glass ? theme.values.blurStrength : 0)
    }
    Rectangle { anchors.fill: parent; color: "transparent"; radius: root.radius; border.color: theme.tint(theme.accent, theme.values.borderOpacity); border.width: theme.values.showWindowBorder ? theme.values.borderWidth : 0 }
}
