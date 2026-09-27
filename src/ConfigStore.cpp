#include "ConfigStore.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
ConfigStore::ConfigStore(QObject* p):ConfigStore(qEnvironmentVariableIsSet("WETSOX_CONFIG_ROOT")?qEnvironmentVariable("WETSOX_CONFIG_ROOT"):QCoreApplication::applicationDirPath()+"/configs",p){}
ConfigStore::ConfigStore(const QString& root,QObject* p):QObject(p),root_(QDir(root).absolutePath()){}
bool ConfigStore::fail(const QString& e){error_=e;emit changed();return false;}
QString ConfigStore::path(const QString& game,const QString& name,bool internal)const {
 if(!QRegularExpression("^[a-z0-9][a-z0-9_-]{0,63}$").match(game).hasMatch())return {};
 if(!internal&&(!QRegularExpression("^[A-Za-z0-9][A-Za-z0-9 _-]{0,63}$").match(name).hasMatch()||
   QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$",QRegularExpression::CaseInsensitiveOption).match(name).hasMatch()||name.endsWith(' ')))return {};
 auto dir=QDir(root_).filePath(game);QFileInfo folder(dir);
 if(folder.isSymLink())return {};
 if(folder.exists()&&!folder.canonicalFilePath().startsWith(QFileInfo(root_).canonicalFilePath()+"/",Qt::CaseInsensitive))return {};
 auto file=QDir(dir).filePath(name+".json");if(QFileInfo(file).isSymLink())return {};
 return file;
}
QStringList ConfigStore::names(const QString& game)const {
 auto example=path(game,"example");if(example.isEmpty())return {};
 QStringList result;
 for(const auto& file:QDir(QFileInfo(example).absolutePath()).entryInfoList({"*.json"},QDir::Files,QDir::Name)) {
  auto name=file.completeBaseName();if(!path(game,name).isEmpty())result<<name;
 }
 return result;
}
bool ConfigStore::write(const QString& game,const QString& file,const QVariantMap& values) {
 if(file.isEmpty())return fail("Use a name with letters, numbers, spaces, underscores or dashes (up to 64 characters).");
 auto data=QJsonDocument(QJsonObject{{"version",1},{"game",game},{"values",QJsonObject::fromVariantMap(values)}}).toJson();
 if(data.size()>1024*1024)return fail("Config is too large.");
 if(!QDir().mkpath(QFileInfo(file).absolutePath()))return fail("Could not create the config folder beside Wetsox.exe.");
 QSaveFile output(file);
 if(!output.open(QIODevice::WriteOnly)||output.write(data)!=data.size()||!output.commit())return fail("Could not save config: "+output.errorString());
 error_.clear();emit changed();return true;
}
QVariantMap ConfigStore::read(const QString& game,const QString& file) {
 QFile input(file);if(file.isEmpty()||!input.open(QIODevice::ReadOnly)){fail("Could not open config.");return {{"ok",false}};}
 if(input.size()>1024*1024){fail("Config is too large.");return {{"ok",false}};}
 QJsonParseError parse;const auto doc=QJsonDocument::fromJson(input.readAll(),&parse);const auto obj=doc.object();
 if(parse.error!=QJsonParseError::NoError||obj.value("version").toInt()!=1||obj.value("game").toString()!=game||!obj.value("values").isObject()) {
  fail("Invalid config or config belongs to another game.");return {{"ok",false}};
 }
 error_.clear();return {{"ok",true},{"values",obj.value("values").toObject().toVariantMap()}};
}
bool ConfigStore::save(const QString& game,const QString& name,const QVariantMap& values){return write(game,path(game,name),values);}
QVariantMap ConfigStore::load(const QString& game,const QString& name){return read(game,path(game,name));}
bool ConfigStore::remove(const QString& game,const QString& name){auto file=path(game,name);if(file.isEmpty()||!QFile::remove(file))return fail("Could not delete config.");error_.clear();emit changed();return true;}
QVariantMap ConfigStore::lastValues(const QString& game){auto file=path(game,".last-session",true);if(!QFile::exists(file))return {};return read(game,file).value("values").toMap();}
void ConfigStore::saveLast(const QString& game,const QVariantMap& values){write(game,path(game,".last-session",true),values);}
