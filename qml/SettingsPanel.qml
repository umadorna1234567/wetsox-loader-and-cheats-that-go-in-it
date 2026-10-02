import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Nexus

Item {
    id: root
    objectName: "appearancePanel"
    required property QtObject theme
    property var sections: []
    property var controller: null
    property string scopeName: "Loader"
    property string status: ""
    property string category: "Theme"
    property string currentPreset: ""
    property string previewMode: "Main Menu"
    Connections { target:root.theme; function onValuesChanged() {root.currentPreset=""} }
    function show(category) { return root.category === "Theme" || root.category === category }
    function preset(name) { AppState.applyPreset(theme.scope,name); root.currentPreset=name }
    onCategoryChanged: {
        root.status = ""
        Qt.callLater(function() { scroll.contentItem.cancelFlick(); scroll.contentItem.contentY = scroll.contentItem.originY })
    }
    function saveProfile() { if(AppState.saveAppearance(theme.scope,profileName.text)) {root.status="Theme saved.";saveDialog.close()} else root.status=AppState.error }
    MountainBackdrop {anchors.fill:parent;light:theme.accent;opacity:0.28}
    RowLayout {
        anchors.fill:parent;spacing:0
        layoutDirection: theme.values.sidebarSide === "Right" ? Qt.RightToLeft : Qt.LeftToRight
        GlassSurface {
            theme:root.theme;kind:"sidebar";radius:0
            objectName:"appearanceSidebar"
            Layout.preferredWidth: Math.min(root.width * 0.30, Math.max(theme.sidebarWidth, theme.fontSize * 8 + 40)); Layout.fillHeight:true
            fillColor: theme.background
            Rectangle{anchors.right:parent.right;width:1;height:parent.height;color:theme.border}
            ColumnLayout {
                anchors.fill:parent;anchors.margins:12;spacing:7
                Text {text:"Customize";color:theme.text;font.family:theme.family;font.pixelSize:16;font.weight:Font.DemiBold;Layout.leftMargin:9;Layout.topMargin:4;Layout.bottomMargin:7}
                ScrollView {
                    id:categoryScroll;Layout.fillWidth:true;Layout.fillHeight:true;clip:true;contentWidth:availableWidth;ScrollBar.horizontal.policy:ScrollBar.AlwaysOff
                    ColumnLayout {
                        width:categoryScroll.availableWidth;spacing:3
                        Repeater {
                            model:[{name:"Theme",icon:"palette"},{name:"Layout",icon:"layout"},{name:"Colors",icon:"palette"},{name:"Background",icon:"image"},{name:"Effects",icon:"sun"},{name:"Animations",icon:"motion"},{name:"Fonts",icon:"font"},{name:"Widgets",icon:"layout"},{name:"Icons",icon:"layers"},{name:"Sounds",icon:"sound"},{name:"Notifications",icon:"bell"},{name:"Cursor",icon:"cursor"},{name:"Advanced",icon:"settings"}]
                            NavItem {
                                required property var modelData
                                objectName: "category_" + modelData.name
                                theme:root.theme;text:modelData.name;glyph:modelData.icon;selected:root.category===modelData.name
                                implicitHeight:Math.max(root.height<780?33:42,theme.fontSize+14);Layout.fillWidth:true
                                onClicked:root.category=modelData.name
                            }
                        }
                    }
                }
                Rectangle {Layout.fillWidth:true;height:1;color:theme.border}
                Text {text:"Theme Profile";color:theme.text;font.family:theme.family;font.pixelSize:12;Layout.topMargin:3}
                Choice {
                    id:profileChoice;theme:root.theme;implicitHeight:29;Layout.fillWidth:true
                    model:AppState.revision>=0?["Current / Custom"].concat(AppState.appearanceProfiles()):[]
                    onActivated:if(currentIndex>0){AppState.loadAppearance(theme.scope,currentText);root.status="Loaded "+currentText;root.currentPreset=""}
                }
                ActionButton {theme:root.theme;text:"Save Theme";primary:true;implicitHeight:32;Layout.fillWidth:true;onClicked:saveDialog.open()}
                ActionButton {theme:root.theme;text:"Duplicate";implicitHeight:29;Layout.fillWidth:true;onClicked:{profileName.text=(profileChoice.currentIndex>0?profileChoice.currentText:root.scopeName)+" copy";saveDialog.open()}}
                RowLayout {Layout.fillWidth:true;spacing:4
                    ActionButton{theme:root.theme;text:"Import";implicitHeight:28;Layout.fillWidth:true;onClicked:importDialog.open()}
                    ActionButton{theme:root.theme;text:"Export";implicitHeight:28;Layout.fillWidth:true;onClicked:exportDialog.open()}
                }
                ActionButton {theme:root.theme;text:"Reset to Default";implicitHeight:32;Layout.fillWidth:true;onClicked:root.preset("Reference")}
            }
        }
        ScrollView {
            id:scroll;objectName:"appearanceScroll";Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:16
            clip:true;contentWidth:availableWidth;ScrollBar.horizontal.policy:ScrollBar.AlwaysOff
            ColumnLayout {
                width:scroll.availableWidth;spacing:theme.spacing
                GridLayout {
                    visible:["Theme","Colors","Background"].indexOf(root.category)>=0
                    Layout.fillWidth:true;columns:scroll.availableWidth>=920?2:1;columnSpacing:14;rowSpacing:14
                    SettingsCard {
                        theme:root.theme;objectName:"presetCard";title:"Theme Presets";visible:root.show("Theme");Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:620;Layout.row:0;Layout.column:0
                        GridLayout {
                            Layout.fillWidth:true;columns:width<500?4:7;columnSpacing:8;rowSpacing:7
                            Repeater {
                                model:[{label:"Default",name:"Reference"},{label:"Purple",name:"Violet"},{label:"Blue",name:"Blue"},{label:"Red",name:"Crimson"},{label:"Green",name:"Mint"},{label:"Orange",name:"Amber"},{label:"Cyberpunk",name:"Cyberpunk"},{label:"Glass",name:"Liquid glass"},{label:"OLED",name:"OLED black"},{label:"Light",name:"Light"},{label:"Halloween",name:"Halloween"},{label:"Christmas",name:"Christmas"},{label:"Minimal",name:"Minimal"}]
                                ColumnLayout {
                                    required property var modelData
                                    Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:1;spacing:4
                                    ThemePreview {
                                        theme:root.theme;previewPalette:AppState.presetTheme(modelData.name);thumbnail:true
                                        Layout.fillWidth:true;Layout.preferredHeight:root.width<1200?48:56
                                        border.width:root.currentPreset===modelData.name?2:1
                                        border.color:root.currentPreset===modelData.name?theme.accent:theme.border
                                        MouseArea{anchors.fill:parent;onClicked:root.preset(modelData.name)}
                                    }
                                    Text{text:modelData.label;color:theme.text;font.family:theme.family;font.pixelSize:11;Layout.fillWidth:true;Layout.minimumWidth:0;horizontalAlignment:Text.AlignHCenter;elide:Text.ElideRight}
                                }
                            }
                        }
                        RowLayout {Layout.fillWidth:true
                            Text{text:theme.linked?"Shared theme":"Independent theme";color:theme.muted;font.family:theme.family;font.pixelSize:10;Layout.fillWidth:true}
                            Toggle{theme:root.theme;checked:theme.linked;onToggled:AppState.setLinked(theme.scope,checked);Accessible.name:"Use shared customization"}
                            ActionButton{theme:root.theme;text:"Randomize";implicitHeight:25;onClicked:{AppState.randomizeTheme(theme.scope);root.currentPreset=""}}
                        }
                    }
                    SettingsCard {
                        theme:root.theme;objectName:"previewCard";title:"Live Preview";fillContent:true;visible:root.category==="Theme"||root.category==="Colors"||root.category==="Background"
                        Layout.minimumWidth:0;Layout.preferredWidth:scroll.availableWidth*0.40;Layout.fillWidth:true;Layout.fillHeight:true
                        Layout.column:scroll.availableWidth>=920?1:0;Layout.row:scroll.availableWidth>=920?0:(root.category==="Background"?0:2);Layout.rowSpan:scroll.availableWidth>=920&&root.category==="Theme"?2:1
                        RowLayout{Layout.fillWidth:true
                            Text{text:"Show:";color:theme.muted;font.family:theme.family;font.pixelSize:10;Layout.fillWidth:true}
                            Choice{theme:root.theme;implicitHeight:25;Layout.preferredWidth:145;model:["Main Menu","Game Library"];onActivated:root.previewMode=currentText}
                        }
                        ThemePreview {objectName:"liveThemePreview";theme:root.theme;scopeName:root.scopeName;sections:root.sections;view:root.previewMode;Layout.fillWidth:true;Layout.fillHeight:true;Layout.preferredHeight:root.category==="Theme"?390:310}
                    }
                SettingsCard {
                    theme:root.theme;objectName:"colorCard";title:"Color Customization";visible:root.show("Colors");Layout.fillWidth:true;Layout.minimumWidth:0;Layout.preferredWidth:620;Layout.column:0;Layout.row:root.category==="Theme"?1:0
                    ColorWorkbench{theme:root.theme;sections:root.sections;controller:root.controller;Layout.fillWidth:true}
                }
                }
                GridLayout {
                    visible: ["Advanced","Colors"].indexOf(root.category) < 0
                    Layout.fillWidth:true;columns:scroll.availableWidth>=1050?3:scroll.availableWidth>=650?2:1;columnSpacing:14;rowSpacing:14
                    SettingsCard {
                        theme:root.theme;title:"Transparency & Blur";Layout.preferredWidth:300;Layout.minimumWidth:0;Layout.columnSpan:1;visible:root.show("Effects");Layout.fillWidth:true;Layout.fillHeight:true
                        CompactSetting{theme:root.theme;label:"Liquid Glass";setting:"glass";type:"toggle";Layout.fillWidth:true}
                        ColorSetting{theme:root.theme;label:"Glass color";setting:"glassColor";enabled:theme.values.glass;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Glass Tint";setting:"glassTint";to:0.6;enabled:theme.values.glass;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Background Opacity";setting:"windowOpacity";from:0.05;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Panel Opacity";setting:"cardOpacity";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Sidebar Opacity";setting:"sidebarOpacity";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Control Opacity";setting:"controlOpacity";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Blur Strength";enabled:theme.values.glass&&theme.graphicsEffects;setting:"blurStrength";Layout.fillWidth:true}
                        Text {visible:root.category==="Effects";text:"Glass blurs the artwork behind panels. Use an image or animated background for the strongest effect.";color:theme.muted;font.pixelSize:12;Layout.fillWidth:true;wrapMode:Text.WordWrap}
                    }
                    SettingsCard {
                        theme:root.theme;title:"Corners, Borders & Shadows";Layout.preferredWidth:300;Layout.minimumWidth:0;Layout.columnSpan:1;visible:root.show("Effects");Layout.fillWidth:true;Layout.fillHeight:true
                        CompactSetting{theme:root.theme;label:"Corner Radius";enabled:theme.values.widgetStyle==="Rounded";setting:"radius";to:30;stepSize:1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Border Thickness";setting:"borderWidth";to:6;stepSize:1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Border Glow";setting:"glowStrength";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Gradient Border";setting:"gradientBorders";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Shadow Strength";setting:"shadowStrength";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Shadow Softness";setting:"shadowSoftness";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Effects";label:"Animated Borders";setting:"animatedBorders";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Effects";label:"Border Opacity";setting:"borderOpacity";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Effects";label:"Shadow Offset";setting:"shadowSpread";to:24;stepSize:1;Layout.fillWidth:true}
                    }
                    SettingsCard {
                        theme:root.theme;title:"Animations & Transitions";Layout.preferredWidth:300;Layout.minimumWidth:0;Layout.columnSpan:1;visible:root.show("Animations");Layout.fillWidth:true;Layout.fillHeight:true
                        CompactSetting{theme:root.theme;label:"Animation Style";setting:"animationMode";type:"choice";options:["Off","Minimal","Full"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Animation Speed";setting:"animationSpeed";percent:true;from:0.25;to:3;stepSize:0.05;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Page Transition";setting:"pageTransition";type:"choice";options:["Fade","Slide","Zoom","Crossfade"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Toggle Animation";setting:"toggleAnimation";type:"choice";options:["Slide","Fade","Spring"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Hover Effects";setting:"hoverEffect";type:"choice";options:["None","Glow","Brighten","Scale","Border"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Click Effects";setting:"clickEffect";type:"choice";options:["None","Ripple","Pulse","Shrink","Bounce"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Animations";label:"Open Animation";setting:"openAnimation";type:"choice";options:["None","Fade","Scale","Slide","Blur"];Layout.fillWidth:true}
                    }
                    SettingsCard {
                        theme:root.theme;title:"Layout & Window";Layout.preferredWidth:300;Layout.minimumWidth:0;Layout.columnSpan:1;visible:root.show("Layout");Layout.fillWidth:true;Layout.fillHeight:true
                        CompactSetting{theme:root.theme;label:"Window Size";setting:"windowSize";type:"choice";options:["Small","Medium","Large"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Resizable Window";setting:"resizable";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Remember Position / Size";setting:"rememberPosition";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Snap to Screen Edge";setting:"snapToEdge";type:"toggle";Layout.fillWidth:true}
                        Text {text:"Navigation and card layout apply to Home / Library. Sidebar side and width also update this workspace.";color:theme.muted;font.pixelSize:theme.fontSize-1;Layout.fillWidth:true;wrapMode:Text.WordWrap}
                        CompactSetting{theme:root.theme;label:"UI Scale";setting:"uiScale";percent:true;from:0.75;to:1.5;stepSize:0.05;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Density";setting:"density";type:"choice";options:["Compact","Normal","Spacious"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Sidebar Position";setting:"sidebarSide";type:"choice";options:["Left","Right"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Sidebar Width";enabled:!theme.values.compactSidebar&&theme.values.navigationStyle!=="Icon rail";setting:"sidebarWidth";from:150;to:320;stepSize:1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Compact Sidebar";setting:"compactSidebar";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Navigation";setting:"navigationStyle";type:"choice";options:["Sidebar","Top tabs","Floating","Icon rail"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Card Layout";setting:"cardLayout";type:"choice";options:["Grid","Columns","List"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Library Card Height";setting:"cardHeight";from:220;to:380;stepSize:1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Show Game Artwork";setting:"showArtwork";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Artwork Opacity";setting:"artOpacity";enabled:theme.values.showArtwork;from:0.1;to:1;stepSize:0.05;percent:true;Layout.fillWidth:true}
                    }
                    SettingsCard {
                        theme:root.theme;title:"Background";Layout.preferredWidth:300;Layout.minimumWidth:0;Layout.columnSpan:1;visible:root.show("Background");Layout.fillWidth:true;Layout.fillHeight:true
                        Choice{theme:root.theme;implicitHeight:27;Layout.fillWidth:true;model:["Solid","Gradient","Image","Aurora","Particles","Grid","Waves","Halloween","Christmas"];currentIndex:model.indexOf(theme.values.backgroundStyle);onActivated:theme.set("backgroundStyle",currentText)}
                        RowLayout{Layout.fillWidth:true;spacing:5
                            Repeater{model:["Aurora","Grid","Halloween","Christmas"];Rectangle{required property string modelData;Layout.fillWidth:true;implicitHeight:30;radius:5;color:modelData==="Halloween"?"#3d1645":modelData==="Christmas"?"#164537":"#201846";border.color:theme.values.backgroundStyle===modelData?theme.accent:theme.border;MountainBackdrop{anchors.fill:parent;light:modelData==="Halloween"?"#ff8629":modelData==="Christmas"?"#df4471":"#7e3cff";opacity:0.9} MouseArea{anchors.fill:parent;onClicked:theme.set("backgroundStyle",modelData)}}}
                        }
                        CompactSetting{theme:root.theme;label:"Animation Speed";setting:"backgroundMotion";from:0.1;to:3;stepSize:0.1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Gradient Intensity";setting:"gradientIntensity";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Game Theme Sync";setting:"gameTheme";type:"toggle";Layout.fillWidth:true}
                        ActionButton{theme:root.theme;text:"Choose Image";implicitHeight:25;Layout.fillWidth:true;onClicked:imageDialog.open()}
                    }
                    SettingsCard {
                        theme:root.theme;title:"Font & Text";Layout.preferredWidth:300;Layout.minimumWidth:0;Layout.columnSpan:1;visible:root.show("Fonts");Layout.fillWidth:true;Layout.fillHeight:true
                        Choice{objectName:"fontPicker";theme:root.theme;previewFonts:true;implicitHeight:27;Layout.fillWidth:true;model:AppState.fonts;currentIndex:Math.max(0,AppState.fonts.indexOf(theme.family));onActivated:theme.set("fontFamily",currentText);Accessible.name:"Font family"}
                        CompactSetting{theme:root.theme;label:"Font Size";setting:"fontSize";from:12;to:20;stepSize:1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Font Weight";setting:"fontWeight";from:100;to:900;stepSize:100;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Letter Spacing";setting:"letterSpacing";from:-1;to:5;stepSize:0.25;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Control Height";setting:"controlHeight";from:34;to:58;stepSize:1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;label:"Spacing";setting:"spacing";from:8;to:28;stepSize:1;Layout.fillWidth:true}
                    }
                    SettingsCard {
                        theme:root.theme;title:root.category==="Theme"?"Miscellaneous":root.category;Layout.preferredWidth:300;Layout.minimumWidth:0;Layout.columnSpan:1;visible:root.show("Cursor")||root.category==="Sounds"||root.category==="Widgets"||root.category==="Icons"||root.category==="Notifications";Layout.fillWidth:true
                        CompactSetting{theme:root.theme;visible:root.show("Cursor");label:"Custom Cursor";setting:"customCursor";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.show("Cursor");label:"Cursor Glow";setting:"cursorGlow";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Cursor";label:"Shape";setting:"cursorShape";type:"choice";options:["Ring","Dot","Crosshair"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Cursor";label:"Size";setting:"cursorSize";from:8;to:48;stepSize:1;Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Cursor";label:"Trail";setting:"cursorTrail";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.show("Sounds");label:"Sound Effects";setting:"sounds";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.show("Sounds");label:"Volume";setting:"soundVolume";Layout.fillWidth:true}
                        Repeater {model:root.category==="Sounds"?[{label:"Hover",key:"hoverSound"},{label:"Click",key:"clickSound"},{label:"Toggle",key:"toggleSound"},{label:"Open / close",key:"windowSound"}]:[]
                            CompactSetting {required property var modelData;theme:root.theme;label:modelData.label;setting:modelData.key;type:"toggle";Layout.fillWidth:true}
                        }
                        CompactSetting{theme:root.theme;visible:root.show("Widgets");label:"Widget Style";setting:"widgetStyle";type:"choice";options:["Rounded","Square","Pill"];Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.show("Notifications");label:"Show Shortcut Hints";setting:"showHints";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Icons";label:"Show Branding";setting:"showBrand";type:"toggle";Layout.fillWidth:true}
                        CompactSetting{theme:root.theme;visible:root.category==="Notifications";label:"Show Status";setting:"showStatus";type:"toggle";Layout.fillWidth:true}
                    }
                }
                LayoutEditor{theme:root.theme;sections:root.sections;visible:root.category==="Layout"&&root.sections.length>0;Layout.fillWidth:true}
                AdvancedAppearance{objectName:"advancedAppearance";theme:root.theme;visible:root.category==="Advanced";Layout.fillWidth:true}
                Text{text:root.status;visible:text.length>0;color:theme.accent;font.family:theme.family;font.pixelSize:11;Layout.fillWidth:true;wrapMode:Text.WordWrap}
            }
        }
    }
    Dialog{
        id:saveDialog;parent:Overlay.overlay;title:"Save Appearance";modal:true;width:350;anchors.centerIn:parent
        background:Rectangle{color:theme.surface;border.color:theme.border;radius:theme.radius}
        ColumnLayout{anchors.fill:parent
            Input{id:profileName;theme:root.theme;placeholderText:"Theme name";Layout.fillWidth:true;onAccepted:root.saveProfile()}
            RowLayout{Layout.fillWidth:true;ActionButton{theme:root.theme;text:"Save Theme";primary:true;onClicked:root.saveProfile()} ActionButton{theme:root.theme;text:"Cancel";onClicked:saveDialog.close()}}
        }
    }
    FileDialog{id:importDialog;title:"Import Wetsox Theme";nameFilters:["Wetsox themes (*.json)"];onAccepted:root.status=AppState.importTheme(theme.scope,selectedFile)?"Theme imported.":AppState.error}
    FileDialog{id:exportDialog;title:"Export Wetsox Theme";fileMode:FileDialog.SaveFile;defaultSuffix:"json";nameFilters:["Wetsox themes (*.json)"];onAccepted:root.status=AppState.exportTheme(theme.scope,selectedFile)?"Theme exported.":AppState.error}
    FileDialog{id:imageDialog;title:"Background Image";nameFilters:["Images (*.png *.jpg *.jpeg *.jfif *.webp *.bmp)"];onAccepted:{theme.set("backgroundImage",selectedFile.toString());theme.set("backgroundStyle","Image")}}
}
