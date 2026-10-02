import QtQuick
import Nexus

Item {
    id: root
    objectName: "windowPreferences"
    required property var window
    required property QtObject theme
    property bool ready: false
    property bool applying: false
    property string sizePreset: theme.values.windowSize
    function bounds(x, y) {
        return WindowEnvironment.availableGeometry(x === undefined ? window.x + window.width/2 : x, y === undefined ? window.y + window.height/2 : y)
    }
    function applyGeometry(saved) {
        applying = true
        const area = bounds(saved.x === undefined ? undefined : Number(saved.x)+Number(saved.width||window.width)/2, saved.y === undefined ? undefined : Number(saved.y)+Number(saved.height||window.height)/2)
        const dimensions = sizePreset === "Small" ? [1100,720] : sizePreset === "Medium" ? [1320,860] : [1536,1024]
        if (window.visibility === Window.Maximized) window.showNormal()
        window.width = Math.max(window.minimumWidth, Math.min(area.width, saved.width || dimensions[0]))
        window.height = Math.max(window.minimumHeight, Math.min(area.height, saved.height || dimensions[1]))
        window.x = Math.round(Math.max(area.x, Math.min(area.x + area.width - window.width, saved.x === undefined ? window.x : saved.x)))
        window.y = Math.round(Math.max(area.y, Math.min(area.y + area.height - window.height, saved.y === undefined ? window.y : saved.y)))
        applying = false
    }
    function save() {
        if (!ready || applying || !window.visible || window.visibility !== Window.Windowed) return
        if(WindowEnvironment.dragging()){settle.restart();return}
        if(theme.values.snapToEdge) {
            const b=bounds(), distance=18
            if(Math.abs(window.x-b.x)<distance)window.x=b.x
            else if(Math.abs(window.x+window.width-b.x-b.width)<distance)window.x=b.x+b.width-window.width
            if(Math.abs(window.y-b.y)<distance)window.y=b.y
            else if(Math.abs(window.y+window.height-b.y-b.height)<distance)window.y=b.y+b.height-window.height
        }
        if(theme.values.rememberPosition)AppState.saveWindowGeometry(theme.scope,{x:window.x,y:window.y,width:window.width,height:window.height})
    }
    onSizePresetChanged: if(ready){applyGeometry({});settle.restart()}
    Component.onCompleted: {
        applyGeometry(theme.values.rememberPosition ? AppState.windowGeometry(theme.scope) : {})
        ready=true
    }
    Timer { id:settle; interval:400; onTriggered:root.save() }
    Connections {
        target:root.window
        function onXChanged(){if(root.ready&&!root.applying)settle.restart()}
        function onYChanged(){if(root.ready&&!root.applying)settle.restart()}
        function onWidthChanged(){if(root.ready&&!root.applying)settle.restart()}
        function onHeightChanged(){if(root.ready&&!root.applying)settle.restart()}
        function onClosing(){root.save()}
    }
}
