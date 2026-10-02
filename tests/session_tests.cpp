#include <QtTest>
#include "GameSession.h"
#include "fc5/controller.hpp"
class SessionTests : public QObject {
 Q_OBJECT
private slots:
 void kf2Settings(){
  GameSession session;session.game_="killingfloor2";
  session.update({{"aim",true},{"fov",500},{"smooth",.25},{"priority","Both"},{"zedEsp",true},{"playerEsp",true},{"zedLineOrigin","Top"},{"playerLineOrigin","Middle"},{"zedHealthPosition","Left"},{"playerHealthPosition","Bottom"},{"zedSkeletonColor","#123456"},{"doshAmount",3000},{"_hotkeys",QVariantMap{{"aim",QVariantMap{{"enabled",false}}}}}},false);
  auto value=session.effectiveSettings().kf2;
  QVERIFY(value.aim);QCOMPARE(value.fov,360.f);QCOMPARE(value.smooth,.25f);QCOMPARE(value.priority,2);
  QVERIFY(value.zed.enabled&&value.player.enabled);QCOMPARE(value.zed.origin,0);QCOMPARE(value.player.origin,1);QCOMPARE(value.zed.healthPosition,0);QCOMPARE(value.player.healthPosition,3);
  QCOMPARE(value.zed.skeletonColor.r,static_cast<unsigned char>(0x12));QCOMPARE(value.doshAmount,3000);
  QVERIFY(value.visibility);QVERIFY(value.player.skeleton);QVERIFY(!value.silent&&!value.wallShots);
  session.update({},true);value=session.effectiveSettings().kf2;
  QVERIFY(!value.aim&&!value.zed.enabled&&!value.player.enabled);QCOMPARE(value.fov,30.f);
  QVERIFY(nexus::sessionName(42,false,false,true)!=nexus::sessionName(42,false,false,false));
 }
 void endToEnd_data(){QTest::addColumn<QString>("game");QTest::newRow("farcry5")<<QString("farcry5");QTest::newRow("farcry4")<<QString("farcry4");}
 void endToEnd(){
  QFETCH(QString,game);
  GameSession session;
  session.launch("unknown");QVERIFY(!session.busy());QVERIFY(!session.connected());
  session.launch(game);QTRY_VERIFY_WITH_TIMEOUT(!session.busy(),35000);
  QVERIFY(!session.connected());QVERIFY(session.message().contains("not running"));
  QProcess host;
  host.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a){a->flags|=CREATE_NO_WINDOW;});
  host.start(QCoreApplication::applicationDirPath()+"/NexusFC5TestHost.exe",{"--serve"});
  QVERIFY(host.waitForStarted());QTest::qWait(500);
  session.launch(game);QTRY_VERIFY_WITH_TIMEOUT(session.connected(),35000);
  QCOMPARE(session.gameId(),game);
  auto pid=static_cast<DWORD>(host.processId());
  HANDLE mutex=OpenMutexW(SYNCHRONIZE|MUTEX_MODIFY_STATE,FALSE,nexus::mutexName(pid,game=="farcry4").c_str());
  HANDLE mapping=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,nexus::sessionName(pid,game=="farcry4").c_str());
  QVERIFY(mutex&&mapping);
  auto* data=static_cast<nexus::Session*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(nexus::Session)));QVERIFY(data);
  session.update({{"aim",true},{"aimKey","Ctrl+K"},{"bone","Pelvis"},{"fov",1000},{"smooth",.42},{"ammo",true},{"reload",true},{"esp",true},{"espOutline",false},{"allHumans",false},{"espAnimals",true},{"boneEsp",true},{"boxEsp",true},{"showFov",true},{"playerFov",true},{"playerDegrees",105},{"enemyColor","#123456"},{"vehicleFov",true},{"vehicleDegrees",110}},false);
  QCOMPARE(WaitForSingleObject(mutex,1000),DWORD(WAIT_OBJECT_0));
  nexus::Session snapshot=*data;ReleaseMutex(mutex);
  QVERIFY(!snapshot.settings.aim.enabled); // Aim waits for the input while the test host is not foreground.
  QVERIFY(snapshot.settings.esp);QVERIFY(!snapshot.settings.espOutline);QVERIFY(snapshot.settings.noReload);QVERIFY(snapshot.settings.unlimitedAmmo);
  QCOMPARE(snapshot.settings.aim.fovDegrees,180.);QCOMPARE(snapshot.settings.aim.smoothingSeconds,.42);
  QCOMPARE(snapshot.settings.aim.hitLocation,fc5::HitLocation::Pelvis);QCOMPARE(session.hotkeys_[0].key,unsigned('K'));QCOMPARE(session.hotkeys_[0].modifiers,1u);
  QCOMPARE(snapshot.aimKey,0u);
  QVERIFY(!snapshot.menuActive);QVERIFY(!snapshot.allHumans);QVERIFY(snapshot.settings.boneEsp&&snapshot.settings.boxEsp&&snapshot.settings.aim.showFov);QVERIFY(snapshot.settings.espTargets.animals);
  QCOMPARE(snapshot.settings.colors[0].r,uint8_t(0x12));QCOMPARE(snapshot.settings.vehicleFovDegrees,110.);QVERIFY(snapshot.settings.playerFovOverride);QCOMPARE(snapshot.settings.playerFovDegrees,105.);
  session.update({{"aimKey","Pad LT"}},false);
  QCOMPARE(WaitForSingleObject(mutex,1000),DWORD(WAIT_OBJECT_0));snapshot=*data;ReleaseMutex(mutex);
  QCOMPARE(session.hotkeys_[0].key,0x10010u);
  for(unsigned index=0;index<wetsox::padNames.size();++index) {
   const auto name=wetsox::padNames[index];if(name.empty())continue;
   session.update({{"aimKey",QString::fromUtf8(name.data(),name.size())}},false);
   QCOMPARE(WaitForSingleObject(mutex,1000),DWORD(WAIT_OBJECT_0));snapshot=*data;ReleaseMutex(mutex);
   QCOMPARE(session.hotkeys_[0].key,wetsox::padBase+index);QCOMPARE(session.hotkeys_[0].modifiers,0u);
  }
  session.update({{"aimKey","Win"}},false);
  QCOMPARE(WaitForSingleObject(mutex,1000),DWORD(WAIT_OBJECT_0));snapshot=*data;ReleaseMutex(mutex);
  QCOMPARE(session.hotkeys_[0].key,unsigned(VK_LWIN));
  session.update({{"aim",true},{"_hotkeys",QVariantMap{{"aim",QVariantMap{{"enabled",false},{"binding","Pad LT"},{"mode","Toggle"}}}}}},true);
  QCOMPARE(WaitForSingleObject(mutex,1000),DWORD(WAIT_OBJECT_0));snapshot=*data;ReleaseMutex(mutex);
  QVERIFY(snapshot.settings.aim.enabled); // Disabling hotkey restores the normal feature switch.
  QVERIFY(!session.hotkeys_[0].enabled);QVERIFY(session.hotkeys_[0].toggle);
  session.update({},true);
  QCOMPARE(WaitForSingleObject(mutex,1000),DWORD(WAIT_OBJECT_0));snapshot=*data;ReleaseMutex(mutex);
  QVERIFY(!snapshot.settings.aim.enabled&&!snapshot.settings.esp&&!snapshot.settings.noReload&&!snapshot.settings.unlimitedAmmo);
  QVERIFY(snapshot.settings.espOutline);QCOMPARE(snapshot.aimKey,0u);QVERIFY(snapshot.menuActive);
  QTRY_VERIFY_WITH_TIMEOUT(session.message().contains("entities"),5000);
  session.detach();QVERIFY(!session.connected());
  UnmapViewOfFile(data);CloseHandle(mapping);CloseHandle(mutex);
  host.kill();QVERIFY(host.waitForFinished());
 }
};
QTEST_MAIN(SessionTests)
#include "session_tests.moc"
