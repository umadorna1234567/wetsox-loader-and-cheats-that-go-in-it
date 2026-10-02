#pragma once
#include <QObject>
#include <QTimer>
#include <array>
#include <QPointer>
#include <QRegion>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

class WindowShape : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)
public:
    explicit WindowShape(QObject *parent = nullptr) : QObject(parent) {}
    QWindow *window() const { return m_window; }
    qreal radius() const { return m_radius; }
    void setWindow(QWindow *window);
    void setRadius(qreal radius);
    static QRegion roundedRegion(const QSize &size, qreal radius);
signals:
    void windowChanged();
    void radiusChanged();
private:
    void updateMask();
    QPointer<QWindow> m_window;
    qreal m_radius = 0;
};

// Records input only while explicitly armed, inside the selected app window.
class InputRecorder : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)
public:
    explicit InputRecorder(QObject *parent = nullptr);
    QWindow *window() const { return m_window; }
    bool listening() const { return m_listening; }
    void setWindow(QWindow *window);
    Q_INVOKABLE void begin();
    Q_INVOKABLE void cancel();
signals:
    void windowChanged();
    void listeningChanged();
    void recorded(const QString &binding);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    virtual unsigned controllerState(unsigned index) const;
private:
    void finish(const QString &binding);
    static QString modifierName(int key);
    static QString modifierPrefix(Qt::KeyboardModifiers modifiers);
    QPointer<QWindow> m_window;
    bool m_listening = false;
    int m_modifier = 0;
    QTimer m_controllerTimer;
    std::array<unsigned,8> m_previousPads{};
};

class UiSound : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit UiSound(QObject *parent = nullptr) : QObject(parent) {}
    ~UiSound() override;
    Q_INVOKABLE void play(const QString &kind, double volume);
private:
    QByteArray m_wave;
};

// Polls a user-selected shortcut so a menu can reopen while its game has focus.
class MenuShortcut : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString sequence READ sequence WRITE setSequence NOTIFY sequenceChanged)
    Q_PROPERTY(bool enabled MEMBER m_enabled)
public:
    explicit MenuShortcut(QObject *parent = nullptr);
    QString sequence() const { return m_sequence; }
    void setSequence(const QString &sequence);
signals:
    void activated();
    void sequenceChanged();
private:
    QString m_sequence;
    int m_key = 0;
    Qt::KeyboardModifiers m_modifiers;
    bool m_enabled = true, m_wasDown = false;
    QTimer m_timer;
};

// Work-area geometry includes taskbars and supports restoring onto a second monitor.
class WindowEnvironment : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit WindowEnvironment(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE QRect availableGeometry(int x, int y) const;
    Q_INVOKABLE bool dragging() const;
};
