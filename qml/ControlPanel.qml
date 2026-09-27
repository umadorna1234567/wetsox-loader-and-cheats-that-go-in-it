import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    required property QtObject theme
    required property var section
    required property var controller
    readonly property bool active: true
    implicitHeight: panelColumn.implicitHeight + 20
    color: theme.tint(theme.surface, 0.9)
    radius: theme.radius
    border.color: theme.tint(theme.accent, 0.27)
    ColumnLayout {
        id: panelColumn
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 1; spacing: 0
        Rectangle {
            Layout.fillWidth: true; implicitHeight: theme.controlHeight + 12
            color: theme.tint(theme.accent, 0.04); radius: theme.radius
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 10
                Glyph { name: root.section.icon; color: Qt.lighter(theme.accent, 1.5); Layout.preferredWidth: 22; Layout.preferredHeight: 22 }
                Text { text: root.section.name; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 1; font.weight: Font.DemiBold; Layout.fillWidth: true }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: theme.tint(theme.accent, 0.12) }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.margins: 15; Layout.bottomMargin: 0
            spacing: Math.max(2, theme.spacing - 10)
            enabled: root.active; opacity: root.active ? 1 : 0.4
            Repeater {
                model: root.section.controls
                CompactControl {
                    required property var modelData
                    theme: root.theme; spec: modelData; Layout.fillWidth: true
                    enabled: modelData.key === "espEnemies" ? root.controller.featureStates.allHumans === false :
                             (modelData.key === "prediction" || modelData.key === "drop") ? root.controller.featureStates.travel !== false : true
                    opacity: enabled ? 1 : 0.4
                    value: root.controller.featureStates[modelData.key] !== undefined ? root.controller.featureStates[modelData.key] : (modelData.defaultValue !== undefined ? modelData.defaultValue : false)
                    onEdited: function(nextValue) { root.controller.toggleFeature(modelData.key, nextValue) }
                }
            }

        }
    }
}