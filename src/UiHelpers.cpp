#include "../games/farcry5/include/fc5/controller.hpp"
#include "UiHelpers.h"
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainterPath>
#include <cmath>
#include <QDataStream>
#include <QGuiApplication>
#include <QScreen>
#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

void WindowShape::setWindow(QWindow *window)
{
    if (m_window == window) return;
    if (m_window) {
        disconnect(m_window, nullptr, this, nullptr);
        m_window->setMask({});
    }
    m_window = window;
    if (window) {
        connect(window, &QWindow::widthChanged, this, &WindowShape::updateMask);
        connect(window, &QWindow::heightChanged, this, &WindowShape::updateMask);
        connect(window, &QWindow::visibleChanged, this, &WindowShape::updateMask);
        connect(window, &QWindow::windowStateChanged, this, &WindowShape::updateMask);
        connect(window, &QWindow::screenChanged, this, &WindowShape::updateMask);
    }
    updateMask();
    emit windowChanged();
}

void WindowShape::setRadius(qreal radius)
{
    radius = std::isfinite(radius) ? qMax(0.0, radius) : 0;
    if (qFuzzyCompare(m_radius, radius)) return;
    m_radius = radius;
    updateMask();
    emit radiusChanged();
}

QRegion WindowShape::roundedRegion(const QSize &size, qreal radius)
{
    if (size.isEmpty()) return {};
    radius = qBound(0.0, radius, qMin(size.width(), size.height()) / 2.0);
    QPainterPath outline;
    outline.addRoundedRect(QRectF(QPointF(0, 0), QSizeF(size)), radius, radius);
    return QRegion(outline.toFillPolygon().toPolygon());
}

void WindowShape::updateMask()
{
    if (!m_window) return;
    const bool square = m_radius <= 0 || m_window->windowState() == Qt::WindowMaximized
        || m_window->windowState() == Qt::WindowFullScreen;
    m_window->setMask(square ? QRegion{} : roundedRegion(m_window->size(), m_radius));
}

InputRecorder::InputRecorder(QObject *parent):QObject(parent) {
    m_controllerTimer.setInterval(25);
    connect(&m_controllerTimer,&QTimer::timeout,this,[this] {
        if(!m_listening||!m_window)return;
        for(unsigned i=0;i<wetsox::padCount;++i) {
            auto current=controllerState(i),pressed=current&~m_previousPads[i];m_previousPads[i]=current;
            for(unsigned bit=0;bit<wetsox::padNames.size();++bit)
                if((pressed&(1u<<bit))&&!wetsox::padNames[bit].empty()) {
                    finish(QString::fromUtf8(wetsox::padNames[bit].data(),wetsox::padNames[bit].size()));return;
                }
        }
    });
}
unsigned InputRecorder::controllerState(unsigned index) const
{
    static_assert(wetsox::padCount == std::tuple_size_v<decltype(m_previousPads)>);
    return wetsox::readPad(index);
}

void InputRecorder::setWindow(QWindow *window)
{
    if (m_window == window) return;
    cancel();
    if (m_window) m_window->removeEventFilter(this);
    m_window = window;
    if (window) window->installEventFilter(this);
    emit windowChanged();
}

void InputRecorder::begin()
{
    if (!m_window || m_listening) return;
    m_modifier = 0;
    for(unsigned i=0;i<wetsox::padCount;++i)m_previousPads[i]=controllerState(i);
    m_controllerTimer.start();
    m_listening = true;
    emit listeningChanged();
}

void InputRecorder::cancel()
{
    if (!m_listening) return;
    m_controllerTimer.stop();
    m_listening = false;
    m_modifier = 0;
    emit listeningChanged();
}

void InputRecorder::finish(const QString &binding)
{
    cancel();
    emit recorded(binding);
}

QString InputRecorder::modifierName(int key)
{
    switch (key) {
    case Qt::Key_Control: return "Ctrl";
    case Qt::Key_Shift: return "Shift";
    case Qt::Key_Alt: return "Alt";
    case Qt::Key_Meta: return "Win";
    default: return {};
    }
}

QString InputRecorder::modifierPrefix(Qt::KeyboardModifiers modifiers)
{
    QString prefix;
    if (modifiers & Qt::ControlModifier) prefix += "Ctrl+";
    if (modifiers & Qt::AltModifier) prefix += "Alt+";
    if (modifiers & Qt::ShiftModifier) prefix += "Shift+";
    if (modifiers & Qt::MetaModifier) prefix += "Win+";
    return prefix;
}

bool InputRecorder::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_listening || watched != m_window) return false;
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide || event->type() == QEvent::Close) {
        cancel();
        return false;
    }
    if (event->type() == QEvent::ShortcutOverride) {
        event->accept();
        return true;
    }
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->isAutoRepeat()) return true;
        if (event->type() == QEvent::KeyRelease) {
            if (key->key() == m_modifier) finish(modifierName(m_modifier));
            return true;
        }
        if (key->key() == Qt::Key_Escape) cancel();
        else if (key->key() == Qt::Key_Backspace || key->key() == Qt::Key_Delete) finish({});
        else if (!modifierName(key->key()).isEmpty()) m_modifier = key->key();
        else if (key->key() != Qt::Key_unknown && key->key() != 0) {
            const auto modifiers = key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);
            finish(QKeySequence(QKeyCombination(modifiers, static_cast<Qt::Key>(key->key()))).toString(QKeySequence::PortableText));
        }
        return true;
    }
    if (event->type() == QEvent::MouseButtonPress) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        QString name;
        switch (mouse->button()) {
        case Qt::LeftButton: name = "LMB"; break;
        case Qt::RightButton: name = "RMB"; break;
        case Qt::MiddleButton: name = "MMB"; break;
        case Qt::BackButton: name = "Mouse 4"; break;
        case Qt::ForwardButton: name = "Mouse 5"; break;
        default: break;
        }
        if (!name.isEmpty()) finish(modifierPrefix(mouse->modifiers()) + name);
        return true;
    }
    return false;
}

UiSound::~UiSound() {
#ifdef Q_OS_WIN
    PlaySoundW(nullptr, nullptr, 0);
#endif
}
void UiSound::play(const QString &kind, double volume) {
#ifdef Q_OS_WIN
    if (!std::isfinite(volume) || volume <= 0) return;
    // Stop the previous sound before replacing its asynchronously read buffer.
    PlaySoundW(nullptr, nullptr, 0);
    m_wave.clear();
    const int rate = 22050, samples = kind == "window" ? 2205 : 882;
    const double frequency = kind == "hover" ? 1100 : kind == "toggle" ? 780 : kind == "window" ? 440 : 620;
    QDataStream stream(&m_wave, QIODevice::WriteOnly); stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData("RIFF", 4); stream << quint32(36 + samples * 2); stream.writeRawData("WAVEfmt ", 8);
    stream << quint32(16) << quint16(1) << quint16(1) << quint32(rate) << quint32(rate * 2) << quint16(2) << quint16(16);
    stream.writeRawData("data", 4); stream << quint32(samples * 2);
    for (int i = 0; i < samples; ++i) {
        const double envelope = std::sin(3.141592653589793 * i / samples) * std::exp(-4.0 * i / samples);
        stream << qint16(std::sin(6.283185307179586 * frequency * i / rate) * envelope * qBound(0.0, volume, 1.0) * 12000);
    }
    PlaySoundW(reinterpret_cast<LPCWSTR>(m_wave.constData()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
#else
    Q_UNUSED(kind); Q_UNUSED(volume);
#endif
}

MenuShortcut::MenuShortcut(QObject *parent) : QObject(parent) {
    m_timer.setInterval(35);
    connect(&m_timer, &QTimer::timeout, this, [this] {
#ifdef Q_OS_WIN
        const auto down = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
        const bool pressed = m_key && down(m_key) &&
            down(VK_CONTROL) == m_modifiers.testFlag(Qt::ControlModifier) &&
            down(VK_MENU) == m_modifiers.testFlag(Qt::AltModifier) &&
            down(VK_SHIFT) == m_modifiers.testFlag(Qt::ShiftModifier) &&
            (down(VK_LWIN) || down(VK_RWIN)) == m_modifiers.testFlag(Qt::MetaModifier);
        if (pressed && !m_wasDown && m_enabled) emit activated();
        m_wasDown = pressed;
#endif
    });
    m_timer.start();
}
void MenuShortcut::setSequence(const QString &sequence) {
    if (m_sequence == sequence) return;
    m_sequence = sequence; m_key = 0; m_wasDown = true;
    const QKeySequence keys = QKeySequence::fromString(sequence, QKeySequence::PortableText);
    if (keys.count() == 1) {
        const auto combination = keys[0]; const int key = combination.key(); m_modifiers = combination.keyboardModifiers();
#ifdef Q_OS_WIN
        if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9)) m_key = key;
        else if (key >= Qt::Key_F1 && key <= Qt::Key_F24) m_key = VK_F1 + key - Qt::Key_F1;
        else {
            const QMap<int, int> mapping{{Qt::Key_Insert,VK_INSERT},{Qt::Key_Home,VK_HOME},{Qt::Key_End,VK_END},
                {Qt::Key_Delete,VK_DELETE},{Qt::Key_PageUp,VK_PRIOR},{Qt::Key_PageDown,VK_NEXT},{Qt::Key_Space,VK_SPACE},
                {Qt::Key_Tab,VK_TAB},{Qt::Key_Escape,VK_ESCAPE},{Qt::Key_Return,VK_RETURN},
                {Qt::Key_Left,VK_LEFT},{Qt::Key_Right,VK_RIGHT},{Qt::Key_Up,VK_UP},{Qt::Key_Down,VK_DOWN}};
            m_key = mapping.value(key);
        }
#endif
    }
    emit sequenceChanged();
}

QRect WindowEnvironment::availableGeometry(int x, int y) const {
    auto *screen = QGuiApplication::screenAt(QPoint(x, y));
    if (!screen) screen = QGuiApplication::primaryScreen();
    return screen ? screen->availableGeometry() : QRect(0, 0, 1536, 1024);
}
bool WindowEnvironment::dragging() const {
#ifdef Q_OS_WIN
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
#else
    return QGuiApplication::mouseButtons().testFlag(Qt::LeftButton);
#endif
}
