import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nexus

ColumnLayout {
    id: root
    required property QtObject theme
    required property var controller
    property var configNames: []
    property string status: ""
    spacing: theme.spacing
    function refresh() { const previous = saved.currentText; configNames = ConfigStore.names(controller.game.id); saved.currentIndex = configNames.length ? Math.max(0, configNames.indexOf(previous)) : -1 }
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()
    Connections { target: ConfigStore; function onChanged() { root.refresh() } }
    Text { text: "Configs"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 13 }
    Text { text: "Save and switch feature settings and keybinds. Appearance saves automatically."; color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Rectangle {
        Layout.fillWidth: true; implicitHeight: fields.implicitHeight + 36
        radius: theme.radius; color: theme.surface; border.color: theme.border
        ColumnLayout {
            id: fields; anchors.fill: parent; anchors.margins: 18; spacing: 14
            Text { text: "Saved configs"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize }
            Choice { id: saved; objectName: "savedConfigs"; theme: root.theme; model: root.configNames; Layout.fillWidth: true; onActivated: configName.text = currentText }
            RowLayout {
                ActionButton { theme: root.theme; text: "Load"; enabled: saved.currentIndex >= 0; onClicked: {
                    const result = ConfigStore.load(root.controller.game.id, saved.currentText)
                    if (result.ok) { root.controller.featureStates = result.values; root.controller.saveFeatures(); root.status = "Loaded " + saved.currentText }
                } }
                ActionButton { theme: root.theme; text: "Delete"; enabled: saved.currentIndex >= 0; onClicked: removeDialog.open() }
            }
            Text { text: "Config name"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize }
            RowLayout {
                Input { id: configName; objectName: "configName"; theme: root.theme; placeholderText: "My config"; maximumLength: 64; Layout.fillWidth: true }
                ActionButton { objectName: "saveConfig"; theme: root.theme; text: "Save config"; primary: true; enabled: configName.text.trim().length > 0; onClicked: {
                    if (ConfigStore.save(root.controller.game.id, configName.text.trim(), root.controller.featureStates)) root.status = "Saved " + configName.text.trim()
                } }
            }
        }
    }
    Text { text: "Folder: configs/" + controller.game.id + "/ beside Wetsox.exe"; color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize - 1; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Text { text: ConfigStore.error || root.status; color: theme.accent; font.family: theme.family; font.pixelSize: theme.fontSize; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    Item { Layout.fillHeight: true }
    Dialog { id: removeDialog; title: "Delete " + saved.currentText + "?"; modal: true; anchors.centerIn: parent; standardButtons: Dialog.Yes | Dialog.No; onAccepted: {
        if (ConfigStore.remove(root.controller.game.id, saved.currentText)) root.status = "Config deleted."
    } }
}
