import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: root
    required property QtObject theme
    property var sections: []
    spacing: theme.spacing
    Text { text: "Customize layout"; color: theme.text; font.family: theme.family; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize + 2; font.weight: Font.DemiBold }
    Text { text: "Use Edit layout on your dashboard to drag, resize, or right-click cards. Restore hidden cards and controls here. Hiding controls keeps their current values."; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    ActionButton { theme: root.theme; text: "Restore default layout"; onClicked: { theme.set("layoutElements", ({})); theme.set("sectionOrder", []) } }
    Repeater {
        model: theme.ordered(root.sections)
        GlassSurface {
            required property var modelData
            theme: root.theme; Layout.fillWidth: true; implicitHeight: controls.implicitHeight + 24
            ColumnLayout {
                id: controls
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 12; spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: modelData.name; color: theme.text; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; Layout.fillWidth: true }
                    ActionButton { theme: root.theme; text: "↑"; onClicked: theme.moveSection(modelData.name, -1, theme.ordered(root.sections)) }
                    ActionButton { theme: root.theme; text: "↓"; onClicked: theme.moveSection(modelData.name, 1, theme.ordered(root.sections)) }
                    ActionButton { theme: root.theme; text: "Color"; onClicked: { root.colorKey = "section:" + modelData.name; colorDialog.open() } }
                    Toggle { theme: root.theme; checked: !theme.hidden("section:" + modelData.name); onToggled: theme.setElement("section:" + modelData.name, "hidden", !checked); Accessible.name: "Show " + modelData.name }
                }
                RowLayout {
                    Text { text: "Show on Home dashboard"; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; Layout.fillWidth: true }
                    Toggle { theme: root.theme; checked: !theme.element("section:" + modelData.name).homeHidden; onToggled: theme.setElement("section:" + modelData.name, "homeHidden", !checked) }
                }
                RowLayout {
                    Text { text: "Card width"; color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize; Layout.fillWidth: true }
                    Choice { theme: root.theme; model: ["1 column", "2 columns", "3 columns"]; currentIndex: (theme.element("section:" + modelData.name).span || 1) - 1; onActivated: theme.setElement("section:" + modelData.name, "span", currentIndex + 1) }
                }
                Flow {
                    Layout.fillWidth: true; spacing: 6
                    Repeater {
                        model: modelData.controls
                        ActionButton {
                            required property var modelData
                            theme: root.theme; text: (theme.hidden("control:" + modelData.key) ? "+ " : "✓ ") + modelData.label
                            selected: !theme.hidden("control:" + modelData.key)
                            onClicked: theme.setElement("control:" + modelData.key, "hidden", !theme.hidden("control:" + modelData.key))
                        }
                    }
                }
            }
        }
    }
    property string colorKey: ""
    ColorDialog { id: colorDialog; onAccepted: theme.setElement(root.colorKey, "color", selectedColor.toString()) }
}
