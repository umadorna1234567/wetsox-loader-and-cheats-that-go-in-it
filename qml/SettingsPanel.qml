import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Nexus

ScrollView {
    id: root
    required property QtObject theme
    property string scopeName: "Loader"
    property string status: ""
    clip: true; contentWidth: availableWidth
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ColumnLayout {
        width: root.availableWidth; spacing: theme.spacing
        Text { text: "Appearance"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 13; font.weight: Font.DemiBold }
        Text { text: "Fine-tune your workspace. Every change saves automatically."; color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize; wrapMode: Text.WordWrap; Layout.fillWidth: true; Layout.bottomMargin: 8 }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: syncRow.implicitHeight + 32
            color: theme.tint(theme.accent, 0.07); radius: theme.radius; border.color: theme.tint(theme.accent, 0.3)
            RowLayout {
                id: syncRow
                anchors.fill: parent; anchors.margins: 16; spacing: 14
                Glyph { name: "layers"; color: Qt.lighter(theme.accent, 1.5); Layout.preferredWidth: 23; Layout.preferredHeight: 23 }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Text { text: "Use shared customization"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 1; font.weight: Font.Medium; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    Text {
                        text: theme.linked ? "Apply changes to the loader and every linked menu." : root.scopeName + " has its own appearance. Other windows stay unchanged."
                        color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize - 1; Layout.fillWidth: true; wrapMode: Text.WordWrap
                    }
                }
                Toggle { theme: root.theme; checked: theme.linked; onToggled: AppState.setLinked(theme.scope, checked); Accessible.name: "Use shared customization" }
            }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.topMargin: 4
            Text { text: "PRESETS"; color: theme.muted; font.family: theme.family; font.pixelSize: 9; font.letterSpacing: 1.5; Layout.fillWidth: true }
            Text { text: theme.linked ? "EDITING SHARED THEME" : "EDITING " + root.scopeName.toUpperCase(); color: theme.accent; font.family: theme.family; font.pixelSize: 9; font.letterSpacing: 1 }
        }
        Flow {
            Layout.fillWidth: true; spacing: 8
            Repeater {
                model: ["Violet", "Crimson", "Mint", "Amber", "Daylight"]
                ActionButton { required property string modelData; theme: root.theme; text: modelData; onClicked: AppState.applyPreset(theme.scope, modelData) }
            }
        }
        GridLayout {
            Layout.fillWidth: true
            columns: root.availableWidth >= 750 ? 2 : 1
            columnSpacing: theme.spacing; rowSpacing: theme.spacing
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 370
                implicitHeight: colorColumn.implicitHeight + 32
                color: theme.surface; radius: theme.radius; border.color: theme.border
                ColumnLayout {
                    id: colorColumn
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16; spacing: 12
                    Text { text: "Color palette"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 2; font.weight: Font.DemiBold; Layout.bottomMargin: 4 }
                    Repeater {
                        model: [{label:"Accent",key:"accent"},{label:"Background",key:"background"},{label:"Surface",key:"surface"},{label:"Primary text",key:"text"},{label:"Secondary text",key:"muted"},{label:"Borders",key:"border"}]
                        ColorSetting { required property var modelData; theme: root.theme; label: modelData.label; setting: modelData.key; Layout.fillWidth: true }
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 370
                implicitHeight: typeColumn.implicitHeight + 32
                color: theme.surface; radius: theme.radius; border.color: theme.border
                ColumnLayout {
                    id: typeColumn
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16; spacing: 9
                    Text { text: "Typography & shape"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 2; font.weight: Font.DemiBold; Layout.bottomMargin: 4 }
                    Choice { objectName: "fontPicker"; theme: root.theme; previewFonts: true; Layout.fillWidth: true; model: AppState.fonts; currentIndex: Math.max(0, AppState.fonts.indexOf(theme.family)); onActivated: theme.set("fontFamily", currentText); Accessible.name: "Font family" }
                    SliderSetting { theme: root.theme; label: "Text size"; setting: "fontSize"; from: 12; to: 20; Layout.fillWidth: true }
                    SliderSetting { theme: root.theme; label: "Corner radius"; setting: "radius"; from: 0; to: 30; Layout.fillWidth: true }
                    SliderSetting { theme: root.theme; label: "Spacing"; setting: "spacing"; from: 8; to: 28; Layout.fillWidth: true }
                    SliderSetting { theme: root.theme; label: "Control height"; setting: "controlHeight"; from: 34; to: 58; Layout.fillWidth: true }
                }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 370
                implicitHeight: artColumn.implicitHeight + 32
                color: theme.surface; radius: theme.radius; border.color: theme.border
                ColumnLayout {
                    id: artColumn
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16; spacing: 10
                    Text { text: "Library artwork"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 2; font.weight: Font.DemiBold }
                    SliderSetting { theme: root.theme; label: "Cover height"; setting: "cardHeight"; from: 220; to: 380; Layout.fillWidth: true }
                    SliderSetting { theme: root.theme; label: "Artwork opacity"; setting: "artOpacity"; from: 0.1; to: 1; stepSize: 0.05; Layout.fillWidth: true }
                }
            }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; Layout.preferredWidth: 370
                implicitHeight: behaviorColumn.implicitHeight + 32
                color: theme.surface; radius: theme.radius; border.color: theme.border
                ColumnLayout {
                    id: behaviorColumn
                    anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16; spacing: 13
                    Text { text: "Interface"; color: theme.text; font.family: theme.family; font.pixelSize: theme.fontSize + 2; font.weight: Font.DemiBold; Layout.bottomMargin: 4 }
                    Repeater {
                        model: [{label:"Smooth animations",key:"animations"},{label:"Show game artwork",key:"showArtwork"},{label:"Compact loader sidebar",key:"compactSidebar"}]
                        RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Text { text: modelData.label; color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            Toggle { theme: root.theme; checked: theme.values[modelData.key]; onToggled: theme.set(modelData.key, checked); Accessible.name: modelData.label }
                        }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            ActionButton { theme: root.theme; text: "Import theme"; onClicked: importDialog.open() }
            ActionButton { theme: root.theme; text: "Export theme"; onClicked: exportDialog.open() }
            Item { Layout.fillWidth: true }
        }
        Text { text: root.status || "Independent themes are kept when you rejoin the shared theme. Presets replace the current palette and sizing."; color: theme.muted; font.family: theme.family; font.pixelSize: theme.fontSize - 2; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Item { height: 10 }
    }
    FileDialog { id: importDialog; title: "Import Nexus theme"; nameFilters: ["Nexus themes (*.json)"]; onAccepted: root.status = AppState.importTheme(theme.scope, selectedFile) ? "Theme imported." : AppState.error }
    FileDialog { id: exportDialog; title: "Export Nexus theme"; fileMode: FileDialog.SaveFile; defaultSuffix: "json"; nameFilters: ["Nexus themes (*.json)"]; onAccepted: root.status = AppState.exportTheme(theme.scope, selectedFile) ? "Theme exported." : AppState.error }
}
