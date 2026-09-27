import QtQuick

Item {
    id: root
    property int palette: 0
    property url source
    property color accent: "#a78bfa"
    clip: true
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: ["#606776", "#45405c", "#48625e"][root.palette] }
            GradientStop { position: 0.55; color: "#303a4c" }
            GradientStop { position: 1; color: "#111922" }
        }
    }
    Rectangle {
        width: parent.width * 0.22; height: width; radius: width / 2
        x: parent.width * 0.67; y: parent.height * 0.16
        color: "#e4d9bf"; opacity: 0.7
    }
    Canvas {
        id: landscape
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            for (let layer = 0; layer < 3; ++layer) {
                ctx.fillStyle = ["#394755", "#25353f", "#16272e"][layer]
                ctx.beginPath(); ctx.moveTo(0, height)
                for (let i = 0; i <= 10; ++i) {
                    let y = (0.43 + layer * 0.14 + Math.sin(i * 2.2 + layer) * 0.14) * height
                    ctx.lineTo(i * width / 10, y)
                }
                ctx.lineTo(width, height); ctx.closePath(); ctx.fill()
            }
            ctx.fillStyle = "#101c24"
            for (let i = 0; i < 26; ++i) {
                const x = width * i / 25
                const h = height * (0.12 + (Math.sin(i * 6.5) + 1) * 0.09)
                const y = height * 0.88
                ctx.fillRect(x - 1, y - h, 2, h + 25)
                ctx.beginPath(); ctx.moveTo(x, y - h)
                ctx.lineTo(x + h * 0.25, y); ctx.lineTo(x - h * 0.25, y)
                ctx.closePath(); ctx.fill()
            }
        }
    }
    Image { anchors.fill: parent; source: root.source; fillMode: Image.PreserveAspectCrop; asynchronous: true }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#00101117" }
            GradientStop { position: 0.45; color: "#10101117" }
            GradientStop { position: 1; color: "#ee101117" }
        }
    }
}
