#pragma once
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class AppState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QVariantList games READ games NOTIFY changed)
    Q_PROPERTY(QStringList fonts READ fonts CONSTANT)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit AppState(QObject *parent = nullptr);
    explicit AppState(const QString &path, QObject *parent = nullptr);
    int revision() const { return m_revision; }
    QVariantList games() const;
    Q_INVOKABLE void refreshPackages();
    QStringList fonts() const;
    QString error() const { return m_error; }
    Q_INVOKABLE QVariantMap theme(const QString &scope) const;
    Q_INVOKABLE bool linked(const QString &scope) const;
    Q_INVOKABLE void setLinked(const QString &scope, bool value);
    Q_INVOKABLE void setThemeValue(const QString &scope, const QString &key, const QVariant &value);
    Q_INVOKABLE void applyPreset(const QString &scope, const QString &name);
    Q_INVOKABLE void setColorMode(const QString &scope, bool light);
    Q_INVOKABLE bool exportTheme(const QString &scope, const QUrl &url);
    Q_INVOKABLE bool importTheme(const QString &scope, const QUrl &url);
    Q_INVOKABLE QString saveGame(const QString &id, const QString &name, const QString &subtitle, const QUrl &artwork);
    Q_INVOKABLE void clearError();
signals:
    void changed();
    void errorChanged();
private:
    static QVariantMap defaults(const QString &name = "Violet");
    static QVariantMap sanitized(const QVariantMap &input, const QVariantMap &base);
    QVariantMap &editableTheme(const QString &scope);
    void commit();
    void fail(const QString &message);
    QString m_path, m_error;
    int m_revision = 0;
    QVariantMap m_global;
    QMap<QString, QVariantMap> m_local;
    QMap<QString, bool> m_links;
    QVariantList m_games, m_installed;
    QString m_packageRoot;
};
