import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Nexus

ColumnLayout {
    id: root
    required property QtObject theme
    property string query: ""
    property var groups: [
        {title:"Liquid glass & lighting", options:[
            {label:"Liquid glass",key:"glass",type:"toggle"},
            {label:"Glass color",key:"glassColor",type:"color"},
            {label:"Glass tint",key:"glassTint",min:0,max:0.6,step:0.01},
            {label:"Window background opacity",key:"windowOpacity",min:0.05,max:1,step:0.05},
            {label:"Section / card opacity",key:"cardOpacity",min:0,max:1,step:0.05},
            {label:"Sidebar opacity",key:"sidebarOpacity",min:0,max:1,step:0.05},
            {label:"Control opacity",key:"controlOpacity",min:0,max:1,step:0.05},
            {label:"Backdrop blur strength",key:"blurStrength",min:0,max:1,step:0.05},
            {label:"Glow / bloom",key:"glowStrength",min:0,max:1,step:0.05},
            {label:"Border thickness",key:"borderWidth",min:0,max:6,step:1},
            {label:"Border opacity",key:"borderOpacity",min:0,max:1,step:0.05},
            {label:"Gradient borders",key:"gradientBorders",type:"toggle"},
            {label:"Animated borders",key:"animatedBorders",type:"toggle"},
            {label:"Shadow strength",key:"shadowStrength",min:0,max:1,step:0.05},
            {label:"Shadow softness",key:"shadowSoftness",min:0,max:1,step:0.05},
            {label:"Shadow offset",key:"shadowSpread",min:0,max:24,step:1}]},
        {title:"Gradients & backgrounds", options:[
            {label:"Enabled controls",key:"enabledColor",type:"color"},{label:"Disabled controls",key:"disabledColor",type:"color"},
            {label:"Gradient color 1",key:"gradient1",type:"color"},{label:"Gradient color 2",key:"gradient2",type:"color"},{label:"Gradient color 3",key:"gradient3",type:"color"},
            {label:"Gradient colors",key:"gradientStops",type:"choice",items:["2","3"]},
            {label:"Gradient direction",key:"gradientDirection",type:"choice",items:["Horizontal","Vertical","Diagonal"]},
            {label:"Gradient intensity",key:"gradientIntensity",min:0,max:1,step:0.05},
            {label:"Background",key:"backgroundStyle",type:"choice",items:["Solid","Gradient","Image","Aurora","Particles","Grid","Waves","Halloween","Christmas"]},
            {label:"Background motion speed",key:"backgroundMotion",min:0.1,max:3,step:0.1},
            {label:"Automatic game colors",key:"gameTheme",type:"toggle"}]},
        {title:"Animations & interactions",options:[
            {label:"Animation level",key:"animationMode",type:"choice",items:["Off","Minimal","Full"]},
            {label:"Animation speed",key:"animationSpeed",min:0.25,max:3,step:0.05},
            {label:"Page transition",key:"pageTransition",type:"choice",items:["Fade","Slide","Zoom","Crossfade"]},
            {label:"Toggle animation",key:"toggleAnimation",type:"choice",items:["Slide","Fade","Spring"]},
            {label:"Hover effect",key:"hoverEffect",type:"choice",items:["None","Glow","Brighten","Scale","Border"]},
            {label:"Click effect",key:"clickEffect",type:"choice",items:["None","Ripple","Pulse","Shrink","Bounce"]},
            {label:"Menu open animation",key:"openAnimation",type:"choice",items:["None","Fade","Scale","Slide","Blur"]},
            {label:"Custom cursor",key:"customCursor",type:"toggle"},
            {label:"Cursor shape",key:"cursorShape",type:"choice",items:["Ring","Dot","Crosshair"]},
            {label:"Cursor size",key:"cursorSize",min:8,max:48,step:1},
            {label:"Cursor glow",key:"cursorGlow",min:0,max:1,step:0.05},
            {label:"Cursor trail",key:"cursorTrail",type:"toggle"},
            {label:"Sound effects",key:"sounds",type:"toggle"},
            {label:"Sound volume",key:"soundVolume",min:0,max:1,step:0.05},
            {label:"Hover sound",key:"hoverSound",type:"toggle"},{label:"Click sound",key:"clickSound",type:"toggle"},
            {label:"Toggle sound",key:"toggleSound",type:"toggle"},{label:"Open / close sound",key:"windowSound",type:"toggle"}]},
        {title:"Typography, navigation & visibility",options:[
            {label:"Font weight",key:"fontWeight",min:100,max:900,step:100},
            {label:"Letter spacing",key:"letterSpacing",min:-1,max:5,step:0.25},
            {label:"UI scale",key:"uiScale",min:0.75,max:1.5,step:0.05},
            {label:"Density",key:"density",type:"choice",items:["Compact","Normal","Spacious"]},
            {label:"Sidebar side",key:"sidebarSide",type:"choice",items:["Left","Right"]},
            {label:"Sidebar width",key:"sidebarWidth",min:150,max:320,step:1},
            {label:"Navigation style",key:"navigationStyle",type:"choice",items:["Sidebar","Top tabs","Floating","Icon rail"]},
            {label:"Card layout",key:"cardLayout",type:"choice",items:["Grid","Columns","List"]},
            {label:"Widget style",key:"widgetStyle",type:"choice",items:["Rounded","Square","Pill"]},
            {label:"Show search",key:"showSearch",type:"toggle"},{label:"Show banner",key:"showBanner",type:"toggle"},
            {label:"Show session status",key:"showStatus",type:"toggle"},{label:"Show footer",key:"showFooter",type:"toggle"},
            {label:"Show section headers",key:"showSectionHeaders",type:"toggle"},{label:"Show section boxes",key:"showSectionBoxes",type:"toggle"},
            {label:"Recently changed settings",key:"showRecent",type:"toggle"},{label:"Show branding",key:"showBrand",type:"toggle"},
            {label:"Show navigation",key:"showNavigation",type:"toggle"},{label:"Show title bar",key:"showTitleBar",type:"toggle"},
            {label:"Show window border",key:"showWindowBorder",type:"toggle"},{label:"Show add game",key:"showAddGame",type:"toggle"},
            {label:"Show shortcut hints",key:"showHints",type:"toggle"}]}]
    spacing: theme.spacing
    Input { theme: root.theme; Layout.fillWidth: true; placeholderText: "Search appearance: glass, opacity, speed, cursor…"; onTextChanged: root.query = text.toLowerCase(); Accessible.name: "Search appearance settings" }
    Repeater {
        model: root.groups
        GlassSurface {
            required property var modelData
            property var matches: modelData.options.filter(function(o) { return (o.label + " " + o.key + " " + modelData.title).toLowerCase().indexOf(root.query) >= 0 })
            theme: root.theme; Layout.fillWidth: true; visible: matches.length > 0
            implicitHeight: groupColumn.implicitHeight + 32
            ColumnLayout {
                id: groupColumn
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 16; spacing: 12
                Text { text: modelData.title; color: theme.text; font.family: theme.family; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize + 2; font.weight: Font.DemiBold; Layout.fillWidth: true }
                Repeater {
                    model: matches
                    ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        RowLayout {
                            visible: modelData.type === "toggle" || modelData.type === "choice"
                            Layout.fillWidth: true
                            Text { text: modelData.label; color: theme.text; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            Toggle { theme: root.theme; visible: modelData.type === "toggle"; checked: !!theme.values[modelData.key]; onToggled: theme.set(modelData.key, checked); Accessible.name: modelData.label }
                            Choice { theme: root.theme; visible: modelData.type === "choice"; Layout.preferredWidth: 165; model: modelData.items || []; currentIndex: Math.max(0, (modelData.items || []).indexOf(theme.values[modelData.key])); onActivated: theme.set(modelData.key, currentText); Accessible.name: modelData.label }
                        }
                        Loader {
                            Layout.fillWidth: true
                            active: modelData.type === "color" || !modelData.type
                            sourceComponent: modelData.type === "color" ? colorControl : numberControl
                            Component { id: colorControl; ColorSetting { theme: root.theme; label: modelData.label; setting: modelData.key } }
                            Component { id: numberControl; SliderSetting { theme: root.theme; label: modelData.label; setting: modelData.key; from: modelData.min; to: modelData.max; stepSize: modelData.step; suffix: "" } }
                        }
                    }
                }
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        ActionButton { theme: root.theme; text: "Choose background image"; onClicked: imageDialog.open() }
        Text { text: theme.values.backgroundImage ? String(theme.values.backgroundImage).split("/").pop() : "No image selected"; color: theme.muted; elide: Text.ElideMiddle; Layout.fillWidth: true }
    }
    FileDialog { id: imageDialog; title: "Background image"; nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp)"]; onAccepted: { theme.set("backgroundImage", selectedFile.toString()); theme.set("backgroundStyle", "Image") } }
    RowLayout {
        visible: root.theme.scope === "loader"
        Layout.fillWidth: true
        Text { text: "Menu shortcut (F1–F24, letter, digit or navigation key)"; color: theme.text; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize; Layout.fillWidth: true }
        Input { theme: root.theme; text: theme.values.menuKey; Layout.preferredWidth: 165; onEditingFinished: theme.set("menuKey", text); Accessible.name: "Menu shortcut" }
    }
    Text { visible: theme.values.showRecent; text: "Recently changed appearance: " + theme.values.recentSettings.join(" · "); color: theme.muted; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize - 1; Layout.fillWidth: true; wrapMode: Text.WordWrap }
    Text { text: "Named appearance profiles"; color: theme.text; font.family: theme.family; font.weight: theme.values.fontWeight; font.letterSpacing: theme.values.letterSpacing; font.pixelSize: theme.fontSize + 2 }
    RowLayout {
        Layout.fillWidth: true
        Input { id: profileName; theme: root.theme; placeholderText: "Custom theme name"; Layout.fillWidth: true }
        ActionButton { theme: root.theme; text: "Save theme"; onClicked: AppState.saveAppearance(theme.scope, profileName.text) }
    }
    Flow {
        Layout.fillWidth: true; spacing: 8
        Repeater {
            model: AppState.revision >= 0 ? AppState.appearanceProfiles() : []
            Row {
                required property string modelData
                spacing: 4
                ActionButton { theme: root.theme; text: modelData; onClicked: { AppState.loadAppearance(theme.scope, modelData); profileName.text = modelData } }
                ActionButton { theme: root.theme; text: "×"; Accessible.name: "Delete theme " + modelData; onClicked: AppState.deleteAppearance(modelData) }
            }
        }
    }
}
