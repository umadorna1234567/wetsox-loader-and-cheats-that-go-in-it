#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "GamePackages.h"
#include "ConfigStore.h"
#include "AppState.h"
class PackageConfigTests:public QObject {
 Q_OBJECT
private slots:
 void packages() {
  QTemporaryDir root;QDir().mkpath(root.filePath("farcry5"));auto folder=root.filePath("farcry5");
  auto put=[&](const QString& name,const QByteArray& content){QFile f(folder+"/"+name);QVERIFY(f.open(QIODevice::WriteOnly));f.write(content);};
  QJsonObject data{{"formatVersion",1},{"id","farcry5"},{"name","Far Cry 5"},{"backend","farcry5"},{"module","WetsoxFC5.dll"},{"artwork","cover.jpg"},
   {"sections",QJsonArray{QJsonObject{{"name","Aimbot"},{"controls",QJsonArray{QJsonObject{{"key","aim"},{"label","Aim"}}}}}}}};
  put("game.json",QJsonDocument(data).toJson());QVERIFY(scanGamePackages(root.path()).isEmpty());
  put("WetsoxFC5.dll","fixture");put("cover.jpg","fixture");
  auto games=scanGamePackages(root.path());QCOMPARE(games.size(),1);
  QCOMPARE(games[0].toMap().value("id").toString(),QString("farcry5"));
  QVERIFY(games[0].toMap().value("artwork").toString().startsWith("file:"));
  QVERIFY(games[0].toMap().value("modulePath").toString().endsWith("/farcry5/WetsoxFC5.dll"));
  data["module"]="../WetsoxFC5.dll";put("game.json",QJsonDocument(data).toJson());QVERIFY(scanGamePackages(root.path()).isEmpty());
  data["module"]="WetsoxFC5.dll";put("game.json",QJsonDocument(data).toJson());QFile::remove(folder+"/cover.jpg");QVERIFY(scanGamePackages(root.path()).isEmpty());
 }
 void liveDiscovery() {
  QTemporaryDir root;
  qputenv("WETSOX_CHEATS_ROOT",root.filePath("cheats").toUtf8());
  qputenv("NEXUS_SETTINGS_PATH",root.filePath("settings.json").toUtf8());
  AppState state;QVERIFY(state.games().isEmpty());
  const auto source=QCoreApplication::applicationDirPath()+"/cheats/farcry5/";
  const auto target=root.filePath("cheats/farcry5/");QDir().mkpath(target);
  for(const auto& name:{"game.json","cover.jpg","WetsoxFC5.dll"})QVERIFY(QFile::copy(source+name,target+name));
  QTRY_COMPARE_WITH_TIMEOUT(state.games().size(),1,4000);
  QCOMPARE(state.games()[0].toMap()["name"].toString(),QString("Far Cry 5"));
  QFile::remove(target+"game.json");QTRY_VERIFY_WITH_TIMEOUT(state.games().isEmpty(),4000);
  qunsetenv("WETSOX_CHEATS_ROOT");qunsetenv("NEXUS_SETTINGS_PATH");
 }
 void configPersistence() {
  QTemporaryDir dir;QVariantMap values{{"aim",true},{"aimKey","Ctrl+K"},{"fov",24.5},{"enemyColor","#ff0022"}};
  {
   ConfigStore store(dir.path());QVERIFY(store.save("farcry5","Config name 1",values));store.saveLast("farcry5",values);
   QCOMPARE(store.names("farcry5"),QStringList{"Config name 1"});
   QVERIFY(!store.save("../outside","bad",values));QVERIFY(!store.save("farcry5","../outside",values));
   QVERIFY(!store.save("farcry5","CON",values));QVERIFY(!store.save("farcry5","bad/name",values));
  }
  ConfigStore restored(dir.path());auto loaded=restored.load("farcry5","Config name 1");QVERIFY(loaded["ok"].toBool());QCOMPARE(loaded["values"].toMap(),values);
  QCOMPARE(restored.lastValues("farcry5"),values);
  QFile bad(dir.filePath("farcry5/Broken.json"));QVERIFY(bad.open(QIODevice::WriteOnly));bad.write("{broken");bad.close();QVERIFY(!restored.load("farcry5","Broken")["ok"].toBool());
  QVERIFY(restored.remove("farcry5","Config name 1"));QVERIFY(!QFile::exists(dir.filePath("farcry5/Config name 1.json")));
  QVERIFY(!restored.load("skyrim","Config name 1")["ok"].toBool());
 }
};
QTEST_MAIN(PackageConfigTests)
#include "package_config_tests.moc"
