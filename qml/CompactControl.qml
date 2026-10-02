import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Nexus

RowLayout {
    id: root
    required property QtObject theme
    required property var spec
    objectName: "setting_" + root.spec.key
    property real rainbowPhase: 0
    property bool editing: false
    property bool featureEnabled: true
    required property var value
    signal edited(var nextValue)
    signal hotkeyRequested()
    spacing: 10
    implicitHeight: Math.max(28, theme.controlHeight - 7)
    Text {
        text: root.spec.label; color: theme.element("control:" + root.spec.key).color || theme.muted
        font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize
        Layout.fillWidth: true; Layout.minimumWidth: 40; Layout.maximumWidth: implicitWidth; elide: Text.ElideRight
    }
    ActionButton {
        objectName: root.spec.key + "HotkeyLink"
        theme: root.theme; text: "Key"
        visible: !root.spec.type || root.spec.type === "toggle"
        implicitWidth: 36; implicitHeight: 24; padding: 3
        Accessible.name: "Configure hotkey for " + root.spec.label
        onClicked: root.hotkeyRequested()
    }
    Item { Layout.fillWidth: true; Layout.minimumWidth: 0 }
    ActionButton {
        objectName: root.spec.key + "Action"
        theme: root.theme
        visible: root.spec.type === "action"
        text: "Apply"
        onClicked: GameSession.action(root.spec.key)
    }
    Toggle {
        objectName: root.spec.key + "Toggle"
        theme: root.theme
        visible: !root.spec.type || root.spec.type === "toggle"
        enabled: root.featureEnabled
        checked: !!root.value
        onToggled: root.edited(checked)
        Accessible.name: root.spec.label
    }
    Choice {
        theme: root.theme
        visible: root.spec.type === "choice"
        Layout.preferredWidth: 142
        implicitHeight: Math.max(27, theme.controlHeight - 8)
        model: root.spec.options || []
        currentIndex: Math.max(0, (root.spec.options || []).indexOf(root.value))
        onActivated: root.edited(currentText)
        Accessible.name: root.spec.label
    }
    Button {
        visible: root.spec.type === "color"
        implicitWidth: 58; implicitHeight: 25
        background: Rectangle { color: root.spec.type === "color" ? (theme.element("control:" + root.spec.key).rainbow ? Qt.hsva((root.rainbowPhase * (theme.element("control:" + root.spec.key).rainbowSpeed || 1)) % 1, 0.75, 1, 1) : String(root.value)) : "transparent"; radius: theme.radius; border.color: theme.border }
        onClicked: { picker.selectedColor = String(root.value); picker.open() }
        Accessible.name: root.spec.label
    }
    Toggle { visible: root.spec.type === "color"; theme: root.theme; checked: !!theme.element("control:" + root.spec.key).rainbow; onToggled: theme.setElement("control:" + root.spec.key, "rainbow", checked); Accessible.name: root.spec.label + " rainbow" }
    ThemedSlider { theme: root.theme; visible: root.spec.type === "color"; Layout.preferredWidth: 60; from: 0.1; to: 5; stepSize: 0.1; value: theme.element("control:" + root.spec.key).rainbowSpeed || 1; onMoved: theme.setElement("control:" + root.spec.key, "rainbowSpeed", value); Accessible.name: root.spec.label + " rainbow speed" }
    ColorDialog { id: picker; title: root.spec.label; onAccepted: root.edited(selectedColor.toString()) }
    Slider {
        id: slider
        objectName: root.spec.key + "Slider"
        visible: root.spec.type === "slider"
        Layout.preferredWidth: 92
        implicitHeight: 24
        from: root.spec.min || 0
        to: root.spec.max || 3
        stepSize: root.spec.step || 0.1
        value: Number(root.value) || 0
        onMoved: root.edited(value)
        background: Rectangle {
            x: slider.leftPadding; y: slider.topPadding + slider.availableHeight / 2 - 2
            width: slider.availableWidth; height: 4; radius: 2; color: theme.border
            Rectangle { width: slider.visualPosition * parent.width; height: 4; radius: 2; color: theme.accent }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 12; height: 12; radius: 6; color: Qt.lighter(theme.accent, 1.8)
            border.color: slider.activeFocus ? theme.text : "transparent"
        }
        Accessible.name: root.spec.label
    }
    Text {
        visible: root.spec.type === "slider"
        text: Number(root.value).toFixed(root.spec.step === 1 ? 0 : (root.spec.step === 0.01 ? 2 : 1)) + (root.spec.unit || "")
        color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 1
        Layout.preferredWidth: 42; horizontalAlignment: Text.AlignRight
    }
    TapHandler {
        enabled: root.editing; acceptedButtons: Qt.RightButton
        onTapped: options.popup()
    }
    Menu { id: options
        MenuItem { text: "Hide setting"; onTriggered: theme.setElement("control:" + root.spec.key, "hidden", true) }
        MenuItem { text: "Label color…"; onTriggered: labelColor.open() }
    }
    ColorDialog { id: labelColor; onAccepted: theme.setElement("control:" + root.spec.key, "color", selectedColor.toString()) }
}
