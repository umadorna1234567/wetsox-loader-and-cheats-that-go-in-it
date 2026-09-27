import QtQuick

Item {
    id: root
    required property var game
    property real radius: 8
    property color outside: "#090d19"
    property bool showArtwork: true
    property real artOpacity: 1
    readonly property url coverSource: game.artwork || ""
    clip: true
    Rectangle { anchors.fill: parent; color: "#141c2d"; radius: root.radius }
    Artwork { anchors.fill: parent; visible: root.showArtwork && cover.status !== Image.Ready; palette: root.game.palette || 0 }
    Image {
        id: cover
        objectName: "coverImage"
        anchors.fill: parent; source: root.showArtwork ? root.coverSource : ""
        fillMode: Image.PreserveAspectCrop; asynchronous: true; opacity: root.artOpacity
    }
    Canvas {
        anchors.fill: parent
        property real corner: root.radius
        property color fill: root.outside
        onCornerChanged: requestPaint()
        onFillChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const c = getContext("2d"); c.clearRect(0,0,width,height); c.fillStyle = fill
            for (let i=0;i<4;i++) {
                c.save(); c.translate(i%2 ? width : 0,i>1 ? height : 0); c.scale(i%2 ? -1 : 1,i>1 ? -1 : 1)
                c.beginPath(); c.moveTo(0,0); c.lineTo(corner,0)
                if (corner>0) c.arc(corner,corner,corner,-Math.PI/2,-Math.PI,true)
                c.closePath(); c.fill(); c.restore()
            }
        }
    }
}
