#pragma once
#include <QVariantList>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QUrl>
inline QVariantList scanGamePackages(const QString& directory) {
    QVariantList result;QStringList ids;
    for(const auto& folder:QDir(directory).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name)) {
        if(folder.isSymLink())continue;
        const QDir dir(folder.absoluteFilePath());
        QFile file(dir.filePath("game.json"));
        if(!file.open(QIODevice::ReadOnly)||file.size()>1024*1024)continue;
        QJsonParseError error;auto doc=QJsonDocument::fromJson(file.readAll(),&error);auto o=doc.object();
        const QString id=o.value("id").toString();
        if(error.error!=QJsonParseError::NoError||o.value("formatVersion").toInt()!=1||
           !QRegularExpression("^[a-z0-9][a-z0-9_-]{0,63}$").match(id).hasMatch()||id=="loader"||ids.contains(id)||
           o.value("name").toString().trimmed().isEmpty())continue;
        auto localFile=[&](const QString& key) {
            const auto name=o.value(key).toString();
            if(name.isEmpty()||QFileInfo(name).fileName()!=name)return QString{};
            QFileInfo info(dir.filePath(name));const auto path=info.canonicalFilePath();
            return info.isFile()&&path.startsWith(folder.canonicalFilePath()+"/",Qt::CaseInsensitive)?path:QString{};
        };
        auto module=localFile("module"),art=localFile("artwork");
        if(module.isEmpty()||!module.endsWith(".dll",Qt::CaseInsensitive)||art.isEmpty())continue;
        auto sections=o.value("sections").toArray();
        if(sections.isEmpty()||sections.size()>24)continue;
        bool valid=true;QStringList keys;
        for(auto section:sections) {
            const auto controls=section.toObject().value("controls").toArray();
            if(section.toObject().value("name").toString().isEmpty()||controls.size()>128)valid=false;
            for(auto control:controls) {
                const auto c=control.toObject();const auto key=c.value("key").toString();
                if(key.isEmpty()||keys.contains(key)||!QStringList{"","toggle","slider","choice","keybind","color"}.contains(c.value("type").toString()))valid=false;
                keys.append(key);
            }
        }
        if(!valid)continue;
        auto game=o.toVariantMap();game["artwork"]=QUrl::fromLocalFile(art).toString();game["modulePath"]=module;
        game["folder"]=dir.absolutePath();game["palette"]=0;game["installed"]=true;
        result.append(game);ids.append(id);
    }
    return result;
}
