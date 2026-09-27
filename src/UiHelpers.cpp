#include "UiHelpers.h"
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPainterPath>
#include <cmath>

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
    m_listening = true;
    emit listeningChanged();
}

void InputRecorder::cancel()
{
    if (!m_listening) return;
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
