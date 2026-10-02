import QtQuick

Canvas {
    id: root
    property color skyTop: "#30215c"
    property color skyBottom: "#121830"
    property color light: "#8145c2"
    onSkyTopChanged: requestPaint()
    onLightChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const c = getContext("2d"); c.clearRect(0, 0, width, height)
        const sky = c.createLinearGradient(0, 0, 0, height); sky.addColorStop(0, skyTop); sky.addColorStop(1, skyBottom)
        c.fillStyle = sky; c.fillRect(0, 0, width, height)
        const sun = c.createRadialGradient(width * 0.62, height * 0.37, 0, width * 0.62, height * 0.37, width * 0.48)
        sun.addColorStop(0, light); sun.addColorStop(1, "transparent"); c.globalAlpha = 0.38; c.fillStyle = sun; c.fillRect(0,0,width,height); c.globalAlpha = 1
        for (let layer = 0; layer < 4; ++layer) {
            c.fillStyle = ["#29284b", "#201d3b", "#13192c", "#090f20"][layer]
            c.beginPath(); c.moveTo(0,height)
            for (let i = 0; i <= 30; ++i) {
                const x = width * i / 30
                const ridge = 0.28 + layer * 0.13 + 0.11 * Math.sin(i * 1.07 + layer * 3) + 0.055 * Math.cos(i * 2.7)
                c.lineTo(x, height * ridge)
            }
            c.lineTo(width,height); c.closePath(); c.fill()
        }
        c.fillStyle = "#070d19"
        for (let i=0;i<35;i++) {
            const x = (i * 47.13) % width, y = height * (0.67 + (i % 5) * 0.04), h = height * (0.11 + (i % 4) * 0.02)
            c.beginPath(); c.moveTo(x,y-h); c.lineTo(x-h*0.25,y); c.lineTo(x+h*0.25,y); c.closePath(); c.fill()
        }
    }
}
