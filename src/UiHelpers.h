#pragma once
#include <QObject>
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
    explicit InputRecorder(QObject *parent = nullptr) : QObject(parent) {}
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
private:
    void finish(const QString &binding);
    static QString modifierName(int key);
    static QString modifierPrefix(Qt::KeyboardModifiers modifiers);
    QPointer<QWindow> m_window;
    bool m_listening = false;
    int m_modifier = 0;
};
