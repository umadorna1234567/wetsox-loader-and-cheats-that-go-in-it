#pragma once
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
inline QString wetsoxRoot() {
 QDir dir(QCoreApplication::applicationDirPath());
 if(dir.dirName().compare("backend",Qt::CaseInsensitive)==0)dir.cdUp();
 return dir.absolutePath();
}
inline QString wetsoxCheats() {
 return qEnvironmentVariableIsSet("WETSOX_CHEATS_ROOT")?qEnvironmentVariable("WETSOX_CHEATS_ROOT"):wetsoxRoot()+"/cheats";
}
inline QString wetsoxHelper() {
 auto path=wetsoxRoot()+"/backend/WetsoxGameLoader.exe";
 return QFileInfo::exists(path)?path:wetsoxRoot()+"/WetsoxGameLoader.exe";
}
