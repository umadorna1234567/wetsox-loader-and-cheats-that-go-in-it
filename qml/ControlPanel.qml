import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

GlassSurface {
    id: root
    required property var section
    required property var controller
    property bool editing: false
    property string query: ""
    property real previewHeight: -1
    property real startHeight: 0
    readonly property bool active: true
    implicitHeight: panelColumn.implicitHeight + 20 + (previewHeight >= 0 ? previewHeight : (theme.element("section:" + section.name).extraHeight || 0))
    decorated: theme.values.showSectionBoxes
    fillColor: theme.element("section:" + section.name).color || theme.surface
    radius: theme.radius
    ColumnLayout {
        id: panelColumn
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 1; spacing: 0
        Rectangle {
            visible: root.editing || theme.values.showSectionHeaders
            Layout.fillWidth: true; implicitHeight: theme.controlHeight + 12
            color: theme.tint(theme.accent, 0.04); radius: theme.radius
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 10
                Glyph { name: root.section.icon; color: Qt.lighter(theme.accent, 1.5); Layout.preferredWidth: 22; Layout.preferredHeight: 22 }
                Text { text: root.section.name; color: theme.text; font.family: theme.family; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize + 1; font.weight: Font.DemiBold; Layout.fillWidth: true }
            }
            MouseArea {
                id: heading
                anchors.fill: parent; enabled: root.editing; acceptedButtons: Qt.LeftButton | Qt.RightButton
                preventStealing: true; cursorShape: Qt.OpenHandCursor
                drag.target: dragToken
                onPressed: function(mouse) { if (mouse.button === Qt.RightButton) properties.popup() }
                onReleased: function(mouse) { if (mouse.button === Qt.LeftButton) { dragToken.Drag.drop(); dragToken.x = 0; dragToken.y = 0 } }
            }
            Item {
                id: dragToken; width: parent.width; height: parent.height
                Drag.active: heading.drag.active; Drag.source: root; Drag.keys: ["section"]
                Drag.hotSpot.x: width / 2; Drag.hotSpot.y: height / 2
                Rectangle { anchors.fill: parent; visible: heading.drag.active; color: theme.tint(theme.accent, 0.2); border.color: theme.accent; radius: theme.radius }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: theme.tint(theme.accent, 0.12) }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.margins: 15; Layout.bottomMargin: 0
            spacing: Math.max(2, theme.spacing - 10)
            enabled: root.active; opacity: root.active ? 1 : 0.4
            Repeater {
                model: root.section.controls.filter(function(c) { return c.type !== "keybind" && !theme.hidden("control:" + c.key) && (c.label + " " + c.key + " " + root.section.name).toLowerCase().indexOf(root.query.toLowerCase()) >= 0 })
                CompactControl {
                    required property var modelData
                    theme: root.theme; spec: modelData; editing: root.editing; rainbowPhase: root.controller.rainbowTime; Layout.fillWidth: true
                    featureEnabled: modelData.key === "espEnemies" ? root.controller.featureStates.allHumans === false :
                             (modelData.key === "prediction" || modelData.key === "drop") ? root.controller.featureStates.travel !== false : true
                    opacity: featureEnabled ? 1 : 0.7
                    value: root.controller.featureStates[modelData.key] !== undefined ? root.controller.featureStates[modelData.key] : (modelData.defaultValue !== undefined ? modelData.defaultValue : false)
                    onHotkeyRequested: root.controller.openHotkey(modelData.key)
                    onEdited: function(nextValue) { root.controller.toggleFeature(modelData.key, nextValue) }
                }
            }

        }
    }
    DropArea {
        anchors.fill: parent; keys: ["section"]; enabled: root.editing
        onDropped: function(drop) {
            if (!drop.source || drop.source === root) return
            const sections = theme.ordered(root.controller.sections)
            const source = sections.findIndex(function(s) { return s.name === drop.source.section.name })
            const destination = sections.findIndex(function(s) { return s.name === root.section.name })
            theme.moveSection(drop.source.section.name, destination - source, sections)
            drop.accept()
        }
    }
    Rectangle {
        visible: root.editing; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter
        width: 48; height: 6; radius: 3; color: theme.accent
        MouseArea {
            objectName: "resizeCard"; anchors.fill: parent; anchors.margins: -6; preventStealing: true; cursorShape: Qt.SizeVerCursor
            property real startY: 0
            onPressed: function(mouse) { startY = mapToItem(null, 0, mouse.y).y; root.startHeight = theme.element("section:" + root.section.name).extraHeight || 0 }
            onPositionChanged: function(mouse) { if (pressed) root.previewHeight = Math.max(0, Math.min(600, root.startHeight + mapToItem(null, 0, mouse.y).y - startY)) }
            onReleased: { theme.setElement("section:" + root.section.name, "extraHeight", root.previewHeight); root.previewHeight = -1 }
        }
    }
    Menu {
        id: properties
        MenuItem { text: "Full width card"; onTriggered: theme.setElement("section:" + root.section.name, "span", 3) }
        MenuItem { text: "Single column card"; onTriggered: theme.setElement("section:" + root.section.name, "span", 1) }
        MenuItem { text: "Hide card"; onTriggered: theme.setElement("section:" + root.section.name, "hidden", true) }
        MenuItem { text: "Show / hide on Home"; onTriggered: theme.setElement("section:" + root.section.name, "homeHidden", !theme.element("section:" + root.section.name).homeHidden) }
        MenuItem { text: "Card color…"; onTriggered: palette.open() }
        MenuItem { text: "Reset card properties"; onTriggered: { theme.setElement("section:" + root.section.name, "color", theme.surface.toString()); theme.setElement("section:" + root.section.name, "extraHeight", 0) } }
    }
    ColorDialog { id: palette; onAccepted: theme.setElement("section:" + root.section.name, "color", selectedColor.toString()) }
}
