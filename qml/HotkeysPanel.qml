import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    required property QtObject theme
    required property var controller
    property string highlightedKey: ""
    property bool recording: false
    readonly property var entries: {
        const result = []
        for (const section of controller.sections) for (const control of section.controls) {
            if (!control.type || control.type === "toggle" || control.type === "action")
                result.push({key:control.key, label:control.label, section:section.name, action:control.type === "action"})
        }
        return result
    }
    function reveal(key) {
        const index = entries.findIndex(function(e) { return e.key === key })
        if (index < 0) return
        highlightedKey = key
        list.currentIndex = index
        list.positionViewAtIndex(index, ListView.Center)
        flash.restart()
    }
    Timer { id: flash; interval: 2600; onTriggered: root.highlightedKey = "" }
    spacing: theme.spacing
    Text { text: "Hotkeys"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 13; font.weight: Font.DemiBold }
    Text {
        text: "Enable a feature in its menu first. With its hotkey off, it stays enabled. Hold activates it while pressed; Toggle switches it on or off with each press."
        color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize
        wrapMode: Text.WordWrap; Layout.fillWidth: true
    }
    RowLayout {
        Layout.fillWidth: true
        Text { text: "Show / hide menu (keyboard shortcut)"; color: root.theme.text; font.family: root.theme.family; font.pixelSize: root.theme.fontSize; Layout.fillWidth: true }
        Input { theme: root.theme; text: root.theme.values.menuKey; Layout.preferredWidth: 180; onEditingFinished: root.theme.set("menuKey",text); Accessible.name: "Menu shortcut" }
    }
    ListView {
        id: list; objectName: "hotkeyList"
        Layout.fillWidth: true; Layout.fillHeight: true
        clip: true; spacing: root.theme.spacing
        model: root.entries
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        delegate: Rectangle {
            id: card
            required property var modelData
            required property int index
            readonly property var rule: root.controller.hotkey(modelData.key)
            objectName: "hotkey_" + modelData.key
            width: list.width
            height: contents.implicitHeight + 28
            radius: root.theme.radius
            color: root.highlightedKey === modelData.key ? root.theme.tint(root.theme.accent,0.18) : root.theme.surface
            border.color: root.highlightedKey === modelData.key ? root.theme.accent : root.theme.border
            border.width: root.highlightedKey === modelData.key ? 2 : 1
            ColumnLayout {
                id: contents
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 14
                spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: card.modelData.section + " / " + card.modelData.label; color: root.theme.text; font.family: root.theme.family; font.pixelSize: root.theme.fontSize; font.weight: Font.DemiBold; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Text { text: "Use hotkey"; color: root.theme.muted; font.family: root.theme.family; font.pixelSize: root.theme.fontSize - 1 }
                    Toggle { objectName: "hotkeyEnabled_" + card.modelData.key; theme: root.theme; checked: !!card.rule.enabled; Accessible.name: "Enable hotkey for " + card.modelData.label; onToggled: root.controller.setHotkey(card.modelData.key,"enabled",checked) }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: card.modelData.action ? "Press to activate" : "Activation"; color: root.theme.muted; font.family: root.theme.family; font.pixelSize: root.theme.fontSize; Layout.fillWidth: true }
                    Choice { objectName: "hotkeyMode_" + card.modelData.key; theme: root.theme; visible: !card.modelData.action; enabled: !!card.rule.enabled; model: ["Hold","Toggle"]; currentIndex: card.rule.mode === "Toggle" ? 1 : 0; Layout.preferredWidth: 130; onActivated: root.controller.setHotkey(card.modelData.key,"mode",currentText) }
                    KeybindPicker { objectName: "hotkeyBinding_" + card.modelData.key; theme: root.theme; enabled: !!card.rule.enabled; binding: card.rule.binding || ""; onListeningChanged: root.recording = listening; Layout.preferredWidth: 180; onEdited: function(value) { root.controller.setHotkey(card.modelData.key,"binding",value) } }
                }
                Text { visible: !!card.rule.enabled && !card.rule.binding; text: "Choose a binding to activate this hotkey."; color: root.theme.accent; font.family: root.theme.family; font.pixelSize: root.theme.fontSize - 1 }
            }
        }
    }
}
