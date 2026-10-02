import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nexus

ColumnLayout {
    id: root
    objectName: "colorWorkbench"
    implicitWidth:600
    required property QtObject theme
    property var sections: []
    property var controller: null
    property string group: "General"
    property string setting: "accent"
    property real hue: 0.75
    Component.onCompleted: hue=hsv(selectedColor).h
    property var recent: ["#8b35ff","#247aff","#26c9e8","#fa4a71","#ff962f","#28c99a"]
    readonly property var allFields: [{label:"Primary Accent",key:"accent"},{label:"Liquid Glass",key:"glassColor"},{label:"Background",key:"background"},{label:"Panel Background",key:"surface"},{label:"Border Color",key:"border"},{label:"Text Primary",key:"text"},{label:"Text Secondary",key:"muted"},{label:"Enabled",key:"enabledColor"},{label:"Disabled",key:"disabledColor"}]
    readonly property var fields: group === "Feature Colors" && controller ? sections.reduce(function(a,s) { return a.concat(s.controls.filter(function(c) { return c.type === "color" }).map(function(c) { return {label:c.label,key:c.key,feature:true,defaultValue:c.defaultValue} })) }, []) : allFields.filter(function(f) { return group === "Text" ? ["text","muted"].indexOf(f.key) >= 0 : group === "Controls" || group === "Status" ? ["enabledColor","disabledColor","accent"].indexOf(f.key) >= 0 : group === "Panels" || group === "Sidebar" ? ["surface","background","border","glassColor"].indexOf(f.key) >= 0 : true })
    readonly property var entry: fields.find(function(f) { return f.key === root.setting }) || (root.setting.startsWith("gradient") ? {key:root.setting,label:"Gradient color"} : null) || fields[0] || allFields[0]
    readonly property color selectedColor: entry.feature && controller ? (controller.featureStates[entry.key] || entry.defaultValue || "#9146ff") : theme.colorFor(entry.key)
    readonly property bool rainbow: entry.feature ? !!theme.element("control:" + entry.key).rainbow : !!theme.values[entry.key + "Rainbow"]
    function hsv(color) {
        const r=color.r,g=color.g,b=color.b,hi=Math.max(r,g,b),lo=Math.min(r,g,b),d=hi-lo
        let h=0
        if (d) h=hi===r ? ((g-b)/d+6)%6 : hi===g ? (b-r)/d+2 : (r-g)/d+4
        return {h:h/6,s:hi?d/hi:0,v:hi}
    }
    function applyColor(color) {
        const hex=color.toString()
        if (entry.feature && controller) controller.toggleFeature(entry.key,hex); else theme.set(entry.key,hex)
    }
    onSelectedColorChanged: {
        if (!rainbow) { hue=hsv(selectedColor).h; const hex=selectedColor.toString(); recent=[hex].concat(recent.filter(function(c){return c!==hex})).slice(0,6) }
    }
    onSettingChanged: hue=hsv(selectedColor).h
    onGroupChanged: hue=hsv(selectedColor).h
    spacing: 8
    Flow {
        Layout.fillWidth: true; spacing: 5
        Repeater {
            model: ["General","Sidebar","Panels","Text","Controls","Status","Feature Colors"]
            ActionButton { required property string modelData; theme: root.theme; text: modelData; implicitHeight: 26; implicitWidth: Math.max(62, implicitContentWidth + 30); selected: root.group === modelData; onClicked: { root.group=modelData; root.setting = root.fields.length ? root.fields[0].key : "accent" } }
        }
    }
    GridLayout {
        Layout.fillWidth: true; columns: width < 430 ? 1 : width < 580 ? 2 : 3; columnSpacing: 14; rowSpacing: 14
        ColumnLayout {
            Layout.minimumWidth:0; Layout.preferredWidth: 170; Layout.fillWidth: true; spacing: 3
            Repeater {
                model: root.fields
                RowLayout {
                    required property var modelData
                    Layout.fillWidth: true; implicitHeight: 23
                    Text { text: modelData.label; color: root.entry.key === modelData.key ? theme.text : theme.muted; font.family: theme.family; font.pixelSize: Math.max(11,theme.fontSize-1); Layout.fillWidth: true; Layout.minimumWidth:0; elide: Text.ElideRight }
                    Rectangle {
                        width: 34; height: 21; radius: 3; color: modelData.feature && root.controller ? (root.controller.featureStates[modelData.key] || modelData.defaultValue || "#9146ff") : theme.colorFor(modelData.key)
                        border.color: root.entry.key === modelData.key ? theme.accent : theme.border
                        MouseArea { anchors.fill: parent; onClicked: root.setting=modelData.key }
                    }
                }
            }
        }
        ColumnLayout {
            Layout.minimumWidth:0; Layout.preferredWidth: 220; Layout.fillWidth: true; spacing: 6
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Rectangle {
                    id: square
                    objectName: "colorSaturation"
                    Layout.fillWidth: true; Layout.preferredHeight: 148
                    color: Qt.hsva(root.hue,1,1,1); radius: 3
                    Rectangle { anchors.fill: parent; gradient: Gradient { orientation: Gradient.Horizontal; GradientStop {position:0;color:"white"} GradientStop {position:1;color:"transparent"} } }
                    Rectangle { anchors.fill: parent; gradient: Gradient { GradientStop {position:0;color:"transparent"} GradientStop {position:1;color:"black"} } }
                    Rectangle { width:9;height:9;radius:5;color:"transparent";border.color:"white"; x:root.hsv(root.selectedColor).s * (square.width-9); y:(1-root.hsv(root.selectedColor).v) * (square.height-9) }
                    MouseArea {
                        anchors.fill: parent; preventStealing: true
                        function pick(mouse) { root.applyColor(Qt.hsva(root.hue,Math.max(0,Math.min(1,mouse.x/width)),Math.max(0,Math.min(1,1-mouse.y/height)),1)) }
                        onPressed: function(mouse) {pick(mouse)}
                        onPositionChanged: function(mouse) {if(pressed)pick(mouse)}
                    }
                }
                Rectangle {
                    width: 16; Layout.preferredHeight: 148; radius: 3
                    gradient: Gradient { GradientStop{position:0;color:"#ff0000"} GradientStop{position:0.167;color:"#ffff00"} GradientStop{position:0.333;color:"#00ff00"} GradientStop{position:0.5;color:"#00ffff"} GradientStop{position:0.667;color:"#0000ff"} GradientStop{position:0.833;color:"#ff00ff"} GradientStop{position:1;color:"#ff0000"} }
                    Rectangle { x:-2;y:root.hue*(parent.height-4);width:20;height:4;color:"transparent";border.color:"white" }
                    MouseArea { anchors.fill: parent; preventStealing:true; function pick(m) {root.hue=Math.max(0,Math.min(1,m.y/height));const hsv=root.hsv(root.selectedColor);root.applyColor(Qt.hsva(root.hue,hsv.s || 1,hsv.v || 1,1))} onPressed:function(m){pick(m)} onPositionChanged:function(m){if(pressed)pick(m)} }
                }
            }
            RowLayout {
                Input { id: hex; theme: root.theme; implicitHeight:26; Layout.fillWidth:true; text:root.selectedColor.toString(); maximumLength:7; validator: RegularExpressionValidator{regularExpression:/#[0-9a-fA-F]{6}/} onEditingFinished:{if(acceptableInput)root.applyColor(Qt.rgba(parseInt(text.substr(1,2),16)/255,parseInt(text.substr(3,2),16)/255,parseInt(text.substr(5,2),16)/255,1));text=Qt.binding(function(){return root.selectedColor.toString()})} Accessible.name:"Selected color hex" }
                ActionButton { theme:root.theme;text:"Copy";implicitHeight:26;implicitWidth:58;onClicked:{copyHelper.text=root.selectedColor.toString();copyHelper.selectAll();copyHelper.copy()} }
            }
            Text {text:"Recent Colors";color:theme.muted;font.family:theme.family;font.pixelSize:10}
            Row { spacing:5; Repeater{model:root.recent;Rectangle{required property string modelData;width:19;height:19;radius:4;color:modelData;MouseArea{anchors.fill:parent;onClicked:root.applyColor(modelData)}}} }
        }
        ColumnLayout {
            Layout.minimumWidth:0; Layout.preferredWidth: 170; Layout.fillWidth:true; spacing:8
            Text {text:"Color behavior";color:theme.muted;font.family:theme.family;font.pixelSize:11}
            Choice {theme:root.theme;implicitHeight:26;Layout.fillWidth:true;model:["Solid","Rainbow"];currentIndex:root.rainbow?1:0;onActivated:{if(root.entry.feature)theme.setElement("control:"+root.entry.key,"rainbow",currentIndex===1);else theme.set(root.entry.key+"Rainbow",currentIndex===1)} }
            RowLayout {
                Layout.fillWidth:true
                Text{text:"Rainbow Mode";color:theme.muted;font.family:theme.family;font.pixelSize:11;Layout.fillWidth:true}
                Toggle{theme:root.theme;checked:root.rainbow;onToggled:{if(root.entry.feature)theme.setElement("control:"+root.entry.key,"rainbow",checked);else theme.set(root.entry.key+"Rainbow",checked)}}
            }
            CompactSetting {theme:root.theme;label:"Speed";controlWidth:62;setting:root.entry.key+"RainbowSpeed";from:0.1;to:5;stepSize:0.1;visible:!root.entry.feature;Layout.fillWidth:true}
            ThemedSlider {
                theme: root.theme
                visible: !!root.entry.feature; Layout.fillWidth:true
                from:0.1;to:5;stepSize:0.1
                value:Number(theme.element("control:"+root.entry.key).rainbowSpeed || 1)
                onMoved:theme.setElement("control:"+root.entry.key,"rainbowSpeed",value)
                Accessible.name:"Feature rainbow speed"
            }
            Text {visible:root.group==="Feature Colors"&&!root.fields.length;text:"No feature colors in this menu.";color:theme.muted;Layout.fillWidth:true;wrapMode:Text.WordWrap}
            RowLayout {Layout.fillWidth:true;Text{text:"Gradient";color:theme.muted;font.family:theme.family;font.pixelSize:11;Layout.fillWidth:true} Toggle{theme:root.theme;checked:theme.values.backgroundStyle==="Gradient";onToggled:theme.set("backgroundStyle",checked?"Gradient":"Solid")} }
            RowLayout {
                Layout.fillWidth:true
                Repeater { model:["gradient1","gradient2","gradient3"]
                    Button { required property string modelData; implicitWidth:32;implicitHeight:24;visible:modelData!=="gradient3"||theme.values.gradientStops==="3"
                        background:Rectangle {radius:5;color:theme.colorFor(modelData);border.color:theme.border}
                        onClicked:root.setting=modelData
                        Accessible.name:"Edit "+modelData
                    }
                }
                Choice {theme:root.theme;Layout.fillWidth:true;implicitHeight:26;model:["2","3"];currentIndex:theme.values.gradientStops==="3"?1:0;onActivated:theme.set("gradientStops",currentText)}
            }
            Choice {theme:root.theme;Layout.fillWidth:true;implicitHeight:26;model:["Horizontal","Vertical","Diagonal"];currentIndex:model.indexOf(theme.values.gradientDirection);onActivated:theme.set("gradientDirection",currentText)}
            CompactSetting{theme:root.theme;label:"Intensity";controlWidth:62;setting:"gradientIntensity";Layout.fillWidth:true}
        }
    }
    TextEdit {id:copyHelper;visible:false}
}
