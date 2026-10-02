import QtQuick

Item {
    id: root
    objectName: "windowResizeEdges"
    required property var window
    anchors.fill: parent
    z: 100
    Repeater {
        model: [Qt.TopEdge, Qt.BottomEdge, Qt.LeftEdge, Qt.RightEdge,
                Qt.TopEdge | Qt.LeftEdge, Qt.TopEdge | Qt.RightEdge,
                Qt.BottomEdge | Qt.LeftEdge, Qt.BottomEdge | Qt.RightEdge]
        MouseArea {
            required property int modelData
            readonly property bool leftEdge: (modelData & Qt.LeftEdge) !== 0
            readonly property bool rightEdge: (modelData & Qt.RightEdge) !== 0
            readonly property bool topEdge: (modelData & Qt.TopEdge) !== 0
            readonly property bool bottomEdge: (modelData & Qt.BottomEdge) !== 0
            readonly property bool corner: (leftEdge || rightEdge) && (topEdge || bottomEdge)
            width: corner ? 10 : leftEdge || rightEdge ? 5 : root.width - 20
            height: corner ? 10 : topEdge || bottomEdge ? 5 : root.height - 20
            x: leftEdge ? 0 : rightEdge ? root.width - width : 10
            y: topEdge ? 0 : bottomEdge ? root.height - height : 10
            enabled: root.window.visibility !== Window.Maximized
            cursorShape: corner ? (leftEdge === topEdge ? Qt.SizeFDiagCursor : Qt.SizeBDiagCursor) : leftEdge || rightEdge ? Qt.SizeHorCursor : Qt.SizeVerCursor
            onPressed: root.window.startSystemResize(modelData)
        }
    }
}
