import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Nexus

Popup {
    id: root
    required property QtObject theme
    property var game: ({})
    property url artwork
    width: Math.min(parent.width - 32, 500)
    height: Math.min(parent.height - 32, editor.implicitHeight + 48)
    anchors.centerIn: parent
    modal: true; focus: true
    padding: 24
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { color: theme.surface; radius: theme.radius; border.color: theme.border }
    onOpened: {
        nameField.text = game.name || ""
        subtitleField.text = game.subtitle || ""
        artwork = game.artwork || ""
        nameField.forceActiveFocus()
    }
    contentItem: ScrollView {
        id: editorScroll
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            id: editor
            width: editorScroll.availableWidth
            spacing: 16
            Text { text: root.game.id ? "Edit game" : "Add a game"; color: theme.text; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize + 8; font.bold: true }
            Text { text: "Your library, your artwork."; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize }
            Input { id: nameField; theme: root.theme; placeholderText: "Game name"; maximumLength: 80; Layout.fillWidth: true; Accessible.name: "Game name" }
            Input { id: subtitleField; theme: root.theme; placeholderText: "Subtitle or edition"; maximumLength: 120; Layout.fillWidth: true; Accessible.name: "Game subtitle" }
            Artwork { Layout.fillWidth: true; implicitHeight: 130; source: root.artwork; palette: root.game.palette || 0 }
            RowLayout {
                ActionButton { theme: root.theme; text: "Choose artwork"; onClicked: artworkDialog.open() }
                ActionButton { theme: root.theme; text: "Clear"; enabled: root.artwork.toString().length > 0; onClicked: root.artwork = "" }
            }
            Text { text: "Images stay on your computer. Keep the selected file in place."; color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 2; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                ActionButton { theme: root.theme; text: "Cancel"; onClicked: root.close() }
                ActionButton {
                    theme: root.theme; text: "Save game"; primary: true; enabled: nameField.text.trim().length > 0
                    onClicked: {
                        if (AppState.saveGame(root.game.id || "", nameField.text, subtitleField.text, root.artwork)) root.close()
                    }
                }
            }
        }
    }
    FileDialog {
        id: artworkDialog
        title: "Choose game artwork"
        nameFilters: ["Images (*.png *.jpg *.jpeg *.jfif *.webp *.bmp)"]
        onAccepted: root.artwork = selectedFile
    }
}
