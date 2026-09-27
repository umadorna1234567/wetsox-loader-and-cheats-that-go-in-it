#include <QtTest>
#include "GameSession.h"
class SessionTests : public QObject {
 Q_OBJECT
private slots:
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
  QVERIFY(snapshot.settings.aim.enabled);QVERIFY(snapshot.settings.esp);QVERIFY(!snapshot.settings.espOutline);QVERIFY(snapshot.settings.noReload);QVERIFY(snapshot.settings.unlimitedAmmo);
  QCOMPARE(snapshot.settings.aim.fovDegrees,180.);QCOMPARE(snapshot.settings.aim.smoothingSeconds,.42);
  QCOMPARE(snapshot.settings.aim.hitLocation,fc5::HitLocation::Pelvis);QCOMPARE(snapshot.aimKey,unsigned('K'));QCOMPARE(snapshot.modifiers,1u);
  QVERIFY(!snapshot.menuActive);QVERIFY(!snapshot.allHumans);QVERIFY(snapshot.settings.boneEsp&&snapshot.settings.boxEsp&&snapshot.settings.aim.showFov);QVERIFY(snapshot.settings.espTargets.animals);
  QCOMPARE(snapshot.settings.colors[0].r,uint8_t(0x12));QCOMPARE(snapshot.settings.vehicleFovDegrees,110.);QVERIFY(snapshot.settings.playerFovOverride);QCOMPARE(snapshot.settings.playerFovDegrees,105.);
  session.update({},true);
  QCOMPARE(WaitForSingleObject(mutex,1000),DWORD(WAIT_OBJECT_0));snapshot=*data;ReleaseMutex(mutex);
  QVERIFY(!snapshot.settings.aim.enabled&&!snapshot.settings.esp&&!snapshot.settings.noReload&&!snapshot.settings.unlimitedAmmo);
  QVERIFY(snapshot.settings.espOutline);QCOMPARE(snapshot.aimKey,unsigned(VK_RBUTTON));QVERIFY(snapshot.menuActive);
  QTRY_VERIFY_WITH_TIMEOUT(session.message().contains("entities"),5000);
  session.detach();QVERIFY(!session.connected());
  UnmapViewOfFile(data);CloseHandle(mapping);CloseHandle(mutex);
  host.kill();QVERIFY(host.waitForFinished());
 }
};
QTEST_MAIN(SessionTests)
#include "session_tests.moc"
