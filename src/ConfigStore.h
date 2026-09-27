#pragma once
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QVariantMap>
class ConfigStore:public QObject {
 Q_OBJECT
 QML_ELEMENT
 QML_SINGLETON
 Q_PROPERTY(QString error READ error NOTIFY changed)
public:
 explicit ConfigStore(QObject* parent=nullptr);
 explicit ConfigStore(const QString& root,QObject* parent=nullptr);
 QString error()const{return error_;}
 Q_INVOKABLE QStringList names(const QString& game) const;
 Q_INVOKABLE bool save(const QString& game,const QString& name,const QVariantMap& values);
 Q_INVOKABLE QVariantMap load(const QString& game,const QString& name);
 Q_INVOKABLE bool remove(const QString& game,const QString& name);
 Q_INVOKABLE QVariantMap lastValues(const QString& game);
 Q_INVOKABLE void saveLast(const QString& game,const QVariantMap& values);
signals: void changed();
private:
 QString path(const QString& game,const QString& name,bool internal=false)const;
 bool write(const QString& game,const QString& file,const QVariantMap& values);
 QVariantMap read(const QString& game,const QString& file);
 bool fail(const QString& error);
 QString root_,error_;
};
