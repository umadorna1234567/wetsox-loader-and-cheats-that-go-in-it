#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>
#include "fc5/session.hpp"
class GameSession : public QObject {
 Q_OBJECT
 QML_ELEMENT
 QML_SINGLETON
 Q_PROPERTY(bool busy READ busy NOTIFY changed)
 Q_PROPERTY(bool connected READ connected NOTIFY changed)
 Q_PROPERTY(QString message READ message NOTIFY changed)
 Q_PROPERTY(bool preview READ preview CONSTANT)
 Q_PROPERTY(QString gameId READ gameId NOTIFY changed)
public:
 explicit GameSession(QObject* parent=nullptr);
 ~GameSession();
 bool busy() const {return busy_;}
 bool connected() const {return connected_;}
 bool preview() const;
 QString message() const {return message_;}
 QString gameId() const {return game_;}
 Q_INVOKABLE void launch(const QString& game);
 Q_INVOKABLE void update(const QVariantMap& values, bool menuActive);
 Q_INVOKABLE void detach();
signals:
 void changed();
 void ready(QString game);
private:
 void poll();
 void release();
 bool busy_{},connected_{};
 QString message_;
 QString game_;
 QProcess loader_;
 QTimer timer_;
 HANDLE mapping_{},mutex_{},process_{};
 nexus::Session* session_{};
 nexus::Session desired_;
};
