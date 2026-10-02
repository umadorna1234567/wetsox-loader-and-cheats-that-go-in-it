import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nexus

Rectangle {
    id: root
    required property QtObject theme
    property var sections: []
    property string scopeName: "Wetsox"
    property string view: "Main Menu"
    property bool thumbnail: false
    property var previewPalette: null
    readonly property bool topNavigation: ["Top tabs","Floating"].indexOf(value("navigationStyle")) >= 0
    readonly property bool iconNavigation: value("navigationStyle") === "Icon rail" || value("compactSidebar")
    readonly property real railWidth: !value("showNavigation") || topNavigation ? 0 : iconNavigation ? 45 : Math.min(175,Number(value("sidebarWidth"))*0.5)
    readonly property real bodyX: value("sidebarSide") === "Right" ? 10 : 15+railWidth
    readonly property real bodyY: (value("showTitleBar") ? 52 : 12)+(topNavigation && value("showNavigation") ? 38 : 0)
    readonly property real bodyWidth: 590-railWidth
    readonly property int previewColumns: value("cardLayout") === "List" ? 1 : value("cardLayout") === "Columns" ? 3 : 2
    readonly property real previewGap: Math.max(4,Number(value("spacing"))*0.6*(value("density")==="Compact"?.7:value("density")==="Spacious"?1.35:1))
    function textSize(size) { return size * (thumbnail ? 1 : theme.fontSize / 13) }
    function ink(key) { return previewPalette ? previewPalette[key] : key === "accent" ? theme.accent : theme.colorFor(key) }
    function value(key) { return previewPalette ? previewPalette[key] : theme.values[key] }
    color: root.ink("background"); radius: 8
    border.color: theme.tint(root.ink("accent"), 0.7)
    clip: true
    MountainBackdrop { anchors.fill: parent; light: root.ink("accent"); opacity: 0.7 }
    Item {
        id: menuCanvas
        width: 610; height: 430
        scale: Math.min((root.width - (root.thumbnail ? 8 : 30)) / width, (root.height - (root.thumbnail ? 8 : 30)) / height)
        anchors.centerIn: parent
        transformOrigin: Item.Center
        Rectangle {
            anchors.fill: parent; radius: root.value("radius"); color: root.thumbnail ? theme.tint(root.ink("background"), 0.92) : "transparent"
            Loader { anchors.fill: parent; active: !root.thumbnail; sourceComponent: Component { AppearanceBackground { theme: root.theme; radius: 12 } } }
            border.color: root.ink("accent"); border.width: 1.4
            Rectangle {
                visible:root.value("showTitleBar");height: 40; width: parent.width; radius: 12; color: theme.tint(root.ink("surface"), 0.75)
                Row {
                    anchors.left: parent.left; anchors.leftMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 9
                    Glyph { visible:root.value("showBrand");name: "logo"; width: 22; height: 22; color: root.ink("accent") }
                    Text { visible:root.value("showBrand");text: "Wetsox"; color: root.ink("text"); font.family: theme.family; font.pixelSize: root.textSize(14); font.weight: Font.DemiBold }
                }
                Row { anchors.right: parent.right; anchors.rightMargin: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 18
                    Repeater { model: ["minimize","maximize","close"]; Glyph { required property string modelData; name: modelData; color: root.ink("muted"); width: 14; height: 14 } }
                }
            }
            Column {
                visible:root.railWidth>0
                x: root.value("sidebarSide")==="Right" ? 600-root.railWidth : 10; y: root.bodyY; width: root.railWidth; spacing: root.previewGap
                Repeater {
                    model: root.view==="Game Library" ? ["Library","Appearance"] : ["Home","Visuals","Player","Weapons","World","Configs"]
                    Rectangle {
                        required property string modelData
                        required property int index
                        width: root.railWidth-4; height: 32; radius: 6; color: index === 0 ? theme.tint(root.ink("accent"),0.24) : "transparent"
                        Row { anchors.verticalCenter: parent.verticalCenter; x: 8; spacing: 9
                            Glyph { name: ["home","eye","player","weapon","world","save"][index]; color: root.ink("text"); width: 17; height: 17 }
                            Text { visible:!root.iconNavigation;text: modelData; color: root.ink("text"); font.family: theme.family; font.pixelSize: 11 }
                        }
                    }
                }
            }
            Flow {
                x:10;y:root.value("showTitleBar")?48:8;width:590;spacing:8
                visible:root.topNavigation&&root.value("showNavigation")
                Repeater {model:["Home","Visuals","Player","Customize"];Text {required property string modelData;text:modelData;color:root.ink("text");font.family:theme.family;font.pixelSize:11}}
            }
            Grid {
                id: libraryCards
                x:root.bodyX;y:root.bodyY;width:root.bodyWidth;columns:root.value("cardLayout")==="List"?1:3;spacing:root.previewGap;visible:root.view==="Game Library"
                Repeater {
                    model:AppState.games.slice(0,3)
                    Rectangle {
                        required property var modelData
                        width:(libraryCards.width-(libraryCards.columns-1)*libraryCards.spacing)/libraryCards.columns;height:root.value("cardLayout")==="List"?100:310;radius:8;color:root.ink("surface");border.color:root.ink("border");clip:true
                        Image {visible:root.value("showArtwork");width:root.value("cardLayout")==="List"?65:parent.width;height:root.value("cardLayout")==="List"?100:250;source:modelData.artwork||"";opacity:Number(root.value("artOpacity"));fillMode:Image.PreserveAspectCrop;asynchronous:true}
                        Text {x:root.value("cardLayout")==="List"?75:9;y:root.value("cardLayout")==="List"?25:265;width:parent.width-18;text:modelData.name;color:root.ink("text");font.family:theme.family;font.pixelSize:12;elide:Text.ElideRight}
                        Text {x:root.value("cardLayout")==="List"?75:9;y:root.value("cardLayout")==="List"?50:286;text:"READY TO PLAY";color:root.ink("accent");font.family:theme.family;font.pixelSize:8}
                    }
                }
            }
            Grid {
                visible:root.view !== "Game Library"
                id: previewGrid; x: root.bodyX; y: root.bodyY; width: root.bodyWidth; columns: root.previewColumns; spacing: root.previewGap
                Repeater {
                    model: root.view === "Game Library" ? [{name:"Your games",icon:"library",controls:[{label:"Far Cry 4"},{label:"Far Cry 5"},{label:"Just Cause 4"}]}] : root.sections.length ? root.sections.slice(0,4) : [{name:"Player",icon:"player",controls:[{label:"God mode"},{label:"Infinite stamina"},{label:"Walk speed",type:"slider"},{label:"Run speed",type:"slider"}]},{name:"Visuals",icon:"eye",controls:[{label:"Enable ESP"},{label:"Name ESP"},{label:"Health ESP"},{label:"Skeleton ESP"}]},{name:"Weapons",icon:"weapon",controls:[{label:"No recoil"},{label:"No spread"},{label:"Infinite ammo"}]}]
                    Rectangle {
                        required property var modelData
                        width: (previewGrid.width-(previewGrid.columns-1)*previewGrid.spacing)/previewGrid.columns; height: Math.max(80, (menuCanvas.height - 72) / 2); radius: root.value("radius") * 0.5
                        color: root.value("showSectionBoxes") ? theme.tint(root.ink("surface"), root.value("cardOpacity")) : "transparent"; border.width:root.value("showSectionBoxes")?1:0; border.color: theme.tint(root.ink("border"), 0.9)
                        Rectangle { visible:root.value("showSectionHeaders");width: parent.width; height: 29; radius: parent.radius; color: theme.tint(root.ink("accent"), 0.12)
                            Row { x: 10; anchors.verticalCenter: parent.verticalCenter; spacing: 7
                                Glyph { name: modelData.icon || "layers"; width: 15; height: 15; color: root.ink("text") }
                                Text { text: modelData.name; color: root.ink("text"); font.family: theme.family; font.pixelSize: root.textSize(12); font.weight: Font.Medium }
                            }
                        }
                        Rectangle {anchors.fill:parent;radius:parent.radius;color:theme.tint(root.ink("glassColor"),Number(root.value("glassTint")));visible:root.value("glass")}
                        Column { x: 10; y: root.value("showSectionHeaders")?39:10; width:parent.width-20;spacing: root.previewGap
                            Repeater {
                                model: (modelData.controls || []).slice(0,4)
                                Row {
                                    required property var modelData
                                    width: parent.width
                                    Text { width: parent.width-36; text: modelData.label; color: root.ink("muted"); font.family: theme.family; font.pixelSize: root.textSize(10); elide: Text.ElideRight }
                                    Rectangle {
                                        width:26; height:modelData.type === "slider" ? 4 : 13; y:modelData.type === "slider" ? 5 : 0; radius:7
                                        color:root.ink(modelData.type === "slider" ? "accent" : "enabledColor")
                                        Rectangle {x:15;y:parent.height/2-height/2;width:9;height:9;radius:5;color:"white"}
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
