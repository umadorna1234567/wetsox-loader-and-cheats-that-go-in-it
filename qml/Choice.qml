import QtQuick
import QtQuick.Controls

ComboBox {
    id: root
    required property QtObject theme
    property bool previewFonts: false
    implicitHeight: theme.controlHeight
    font.family: theme.family
    font.pixelSize: theme.fontSize
    font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing
    palette.text: theme.text
    palette.buttonText: theme.text
    palette.base: theme.surface
    palette.window: theme.surface
    palette.highlight: theme.accent
    palette.highlightedText: theme.background
    contentItem: Text {
        leftPadding: 12; rightPadding: 28
        text: root.displayText
        font.family: root.previewFonts ? root.displayText : theme.family
        font.pixelSize: theme.fontSize
        color: theme.text
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Canvas {
        width: 12; height: 8
        x: root.width - width - 12; y: (root.height - height) / 2
        property color arrowColor: theme.muted
        onArrowColorChanged: requestPaint()
        onPaint: {
            const context = getContext("2d")
            context.clearRect(0, 0, width, height)
            context.strokeStyle = arrowColor
            context.lineWidth = 1.5
            context.lineCap = "round"
            context.lineJoin = "round"
            context.beginPath()
            context.moveTo(2, 2)
            context.lineTo(6, 6)
            context.lineTo(10, 2)
            context.stroke()
        }
    }
    background: Rectangle {
        color: theme.surfaceColor("control"); radius: Math.min(theme.radius, height / 2)
        border.color: root.activeFocus ? theme.accent : theme.border
    }
    delegate: ItemDelegate {
        id: option
        required property var modelData
        required property int index
        width: root.width
        implicitHeight: root.previewFonts ? 40 : theme.controlHeight
        text: modelData
        font: root.font
        highlighted: root.highlightedIndex === index
        contentItem: Text {
            text: option.text
            font.family: root.previewFonts ? String(option.modelData) : theme.family
            font.pixelSize: root.previewFonts ? Math.max(16, theme.fontSize) : theme.fontSize
            color: theme.text; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle { color: parent.highlighted ? theme.tint(theme.accent, 0.2) : theme.surface }
    }
    popup: Popup {
        y: root.height + 4
        width: root.width
        implicitHeight: Math.min(contentItem.implicitHeight + 8, 300)
        padding: 4
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle { color: theme.surface; border.color: theme.border; radius: 8 }
    }
}
