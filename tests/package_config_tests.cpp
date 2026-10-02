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
  data["module"]="WetsoxFC5.dll";put("game.json",QJsonDocument(data).toJson());QFile::remove(folder+"/cover.jpg");QCOMPARE(scanGamePackages(root.path()).size(),1);
  QDir().mkpath(root.filePath("download/wrapper"));
  QVERIFY(QDir().rename(folder,root.filePath("download/wrapper/farcry5")));
  games=scanGamePackages(root.path());QCOMPARE(games.size(),1);
  QVERIFY(games[0].toMap()["ready"].toBool());
  QVERIFY(games[0].toMap()["modulePath"].toString().contains("download/wrapper/farcry5/"));
 }
 void nestedPacksAndBackendIdentity() {
  QTemporaryDir root;
  auto make=[&](const QString& location,const QString& id,const QString& backend){
   const auto folder=root.filePath(location);QDir().mkpath(folder);
   QFile module(folder+"/module.dll");QVERIFY(module.open(QIODevice::WriteOnly));module.write("fixture");module.close();
   QFile manifest(folder+"/game.json");QVERIFY(manifest.open(QIODevice::WriteOnly));
   manifest.write(QJsonDocument(QJsonObject{{"formatVersion",1},{"id",id},{"name",id},{"backend",backend},{"module","module.dll"},
    {"artwork","missing.jpg"},{"sections",QJsonArray{QJsonObject{{"name","Player"},{"controls",QJsonArray{QJsonObject{{"key","test"}}}}}}}}).toJson());
  };
  for(const auto& id:{QString("farcry4"),QString("farcry5"),QString("justcause4")})make("download-"+id+"/wrapper/"+id,id,id);
  make("bad-direct","farcry4","farcry5");
  auto games=scanGamePackages(root.path());QCOMPARE(games.size(),3);
  for(const auto& entry:games){const auto pack=entry.toMap();QVERIFY(pack["installed"].toBool());QVERIFY(pack["ready"].toBool());QCOMPARE(pack["id"],pack["backend"]);QVERIFY(pack["artwork"].toString().isEmpty());}
  make("custom-pack","custom-game","farcry4");
  games=scanGamePackages(root.path());QCOMPARE(games.size(),4);
  for(const auto& entry:games)if(entry.toMap()["id"]=="custom-game")QVERIFY(!entry.toMap()["ready"].toBool());
  make("farcry4","farcry4","farcry4");
  games=scanGamePackages(root.path());QCOMPARE(games.size(),4);
  for(const auto& entry:games)if(entry.toMap()["id"]=="farcry4")QVERIFY(entry.toMap()["modulePath"].toString().endsWith("/farcry4/module.dll")&&!entry.toMap()["modulePath"].toString().contains("wrapper"));
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
