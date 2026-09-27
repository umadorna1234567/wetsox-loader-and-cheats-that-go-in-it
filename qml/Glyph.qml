import QtQuick

Canvas {
    id: root
    property string name: "home"
    property color color: "#a6a9c4"
    implicitWidth: 22; implicitHeight: 22
    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const c = getContext("2d")
        c.clearRect(0, 0, width, height)
        c.save(); c.scale(width / 24, height / 24)
        c.strokeStyle = color; c.fillStyle = color; c.lineWidth = 1.6
        c.lineCap = "round"; c.lineJoin = "round"
        function line(points, close) {
            c.beginPath(); c.moveTo(points[0][0], points[0][1])
            for (let i = 1; i < points.length; ++i) c.lineTo(points[i][0], points[i][1])
            if (close) c.closePath()
            c.stroke()
        }
        function circle(x, y, r) { c.beginPath(); c.arc(x, y, r, 0, Math.PI * 2); c.stroke() }
        switch (name) {
        case "logo":
            line([[2,4],[6,20],[12,9],[18,20],[22,4]],false)
            line([[5,4],[7,13],[12,4],[17,13],[19,4]],false); break
        case "home": line([[2,11],[12,3],[22,11]],false); line([[5,10],[5,21],[10,21],[10,15],[14,15],[14,21],[19,21],[19,10]],false); break
        case "aim": circle(12,12,7); circle(12,12,2); line([[12,1],[12,6]],false); line([[12,18],[12,23]],false); line([[1,12],[6,12]],false); line([[18,12],[23,12]],false); break
        case "eye":
            c.beginPath(); c.moveTo(2,12); c.bezierCurveTo(7,3,17,3,22,12); c.bezierCurveTo(17,21,7,21,2,12); c.stroke(); circle(12,12,3); break
        case "player": circle(12,7,4); c.beginPath(); c.moveTo(3,22); c.lineTo(3,19); c.bezierCurveTo(3,12,21,12,21,19); c.lineTo(21,22); c.closePath(); c.stroke(); break
        case "weapon": line([[3,5],[22,5],[22,10],[14,10],[12,14],[8,14],[6,22],[2,22],[5,9],[3,9]],true); line([[10,10],[9,13]],false); break
        case "world": circle(12,12,10); c.beginPath(); c.ellipse(7,2,10,20); c.stroke(); line([[2,12],[22,12]],false); line([[4,6],[20,6]],false); line([[4,18],[20,18]],false); break
        case "settings":
            for (let i=0;i<8;i++) { const a=i*Math.PI/4; line([[12+8*Math.cos(a),12+8*Math.sin(a)],[12+10*Math.cos(a),12+10*Math.sin(a)]],false) }
            circle(12,12,7); circle(12,12,3); break
        case "layers": line([[2,7],[12,2],[22,7],[12,12]],true); line([[2,12],[12,17],[22,12]],false); line([[2,17],[12,22],[22,17]],false); break
        case "library": line([[3,3],[9,3],[9,21],[3,21]],true); line([[13,3],[19,2],[22,20],[16,21]],true); break
        case "plus": line([[12,5],[12,19]],false); line([[5,12],[19,12]],false); break
        case "search": circle(10,10,6); line([[15,15],[21,21]],false); break
        case "arrow": line([[5,12],[19,12],[14,7]],false); line([[19,12],[14,17]],false); break
        case "reset": c.beginPath(); c.arc(12,13,8,-Math.PI/2,Math.PI*1.1); c.stroke(); line([[12,1],[12,7],[7,5]],false); break
        case "save": line([[4,3],[17,3],[21,7],[21,21],[3,21],[3,3]],true); line([[8,3],[8,10],[17,10],[17,3]],false); line([[7,21],[7,15],[17,15],[17,21]],false); break
        case "close": line([[6,6],[18,18]],false); line([[18,6],[6,18]],false); break
        case "minimize": line([[5,12],[19,12]],false); break
        case "maximize": line([[5,5],[19,5],[19,19],[5,19]],true); break
        case "sun": circle(12,12,4); for(let i=0;i<8;i++){const a=i*Math.PI/4;line([[12+7*Math.cos(a),12+7*Math.sin(a)],[12+10*Math.cos(a),12+10*Math.sin(a)]],false)} break
        case "moon": c.beginPath(); c.moveTo(15,3); c.bezierCurveTo(2,0,0,22,15,21); c.bezierCurveTo(20,21,22,18,22,15); c.bezierCurveTo(12,19,8,8,15,3); c.stroke(); break
        default: circle(12,12,8)
        }
        c.restore()
    }
}
