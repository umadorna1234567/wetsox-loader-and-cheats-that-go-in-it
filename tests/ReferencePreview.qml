import QtQuick
import Nexus
import QtTest

Main {
    id: root
    TestCase {id:input;when:false}
    function option(name,fallback) { const i=Qt.application.arguments.indexOf(name);return i>=0?Qt.application.arguments[i+1]:fallback }
    Component.onCompleted: {
        AppState.applyPreset("loader",option("--preset","Reference"))
        const game = AppState.games.find(g => g.id === option("--review-game",AppState.games[0].id))
        root.showGame(game)
        root.windows[game.id].page = option("--review-page","Appearance")
        root.windows[game.id].width=Number(option("--review-width","1536"))
        root.windows[game.id].height=Number(option("--review-height","1024"))
        AppState.setThemeValue("loader","uiScale",Number(option("--review-scale","1")))
        Qt.callLater(function(){input.findChild(root.windows[game.id].contentItem,"appearancePanel").category=option("--category","Theme")})
    }
    Timer {interval:2000;running:Qt.application.arguments.indexOf("--inspect-layout")>=0;onTriggered:{
        const menu=root.windows[option("--review-game",AppState.games[0].id)]
        function inspect(item) {
            if((item.title || String(item.objectName).startsWith("panel_")) && item.visible){const pos=item.mapToItem(menu.contentItem,0,0);console.log("LAYOUT",item.title || item.objectName,pos.x,pos.y,item.width,item.height)}
            for(const child of item.children||[])inspect(child)
        }
        inspect(menu.contentItem)
    }}
    Timer { interval: 4500; running: Qt.application.arguments.indexOf("--screenshot") >= 0; onTriggered: {
        const menu=root.windows[option("--review-game",AppState.games[0].id)]
        input.findChild(menu.contentItem,"appearancePanel").grabToImage(function(result){ console.log("Screenshot saved:",result.saveToFile(option("--screenshot","appearance.png"))) })
    }}
    Timer { interval: 9000; running: true; onTriggered: { console.log("Reference preview rendered."); Qt.quit() } }
}
