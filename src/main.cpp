#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QTemporaryDir>
#include <QQuickWindow>
#include <QImage>
#include <QDir>
#include <QIcon>

int main(int argc, char *argv[])
{
#if QT_VERSION == QT_VERSION_CHECK(6, 8, 3)
    // The 6.8.3 JIT path crashes during repeated theme/model changes in KF2 UI tests.
    // Interpreter mode passes the same regression while retaining normal UI rendering.
    qputenv("QV4_FORCE_INTERPRETER", "1");
#endif
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Wetsox");
    app.setApplicationName("Wetsox");
    app.setWindowIcon(QIcon(":/brand/wetsox.ico"));
    QQuickStyle::setStyle("Basic");
    QQuickWindow::setDefaultAlphaBuffer(true);
    const bool smoke = app.arguments().contains("--smoke-test");
    QTemporaryDir testSettings;
    if (smoke) { qputenv("NEXUS_SETTINGS_PATH", testSettings.filePath("settings.json").toUtf8()); qputenv("WETSOX_CONFIG_ROOT",testSettings.filePath("configs").toUtf8()); }
    QQmlApplicationEngine engine;
    bool qmlFailed = false;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
                     [&qmlFailed](const QList<QQmlError> &) { qmlFailed = true; });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    if (smoke) engine.load(QUrl(app.arguments().contains("--reference-preview") ? "qrc:/testing/tests/ReferencePreview.qml" : app.arguments().contains("--appearance-smoke") ? "qrc:/testing/tests/AppearanceSmoke.qml" : "qrc:/testing/tests/Smoke.qml"));
    else engine.loadFromModule("Nexus", "Main");
    const int captureIndex = app.arguments().indexOf("--screenshots");
    if (smoke && captureIndex >= 0 && captureIndex + 1 < app.arguments().size()) {
        const auto dir = app.arguments().at(captureIndex + 1);
        QDir().mkpath(dir);
        for (const int delay : {500, 1500, 2500, 7500, 8500}) {
            QTimer::singleShot(delay, &app, [&, dir, delay] {
                for (auto *window : app.allWindows()) {
                    auto *quick = qobject_cast<QQuickWindow *>(window);
                    if (quick && quick->isVisible())
                        quick->grabWindow().save(dir + "/" + (quick == engine.rootObjects().value(0) ? "loader-" : "menu-") + QString::number(delay) + ".png");
                }
            });
        }
    }
    if (smoke) QTimer::singleShot(20000, &app, [] { QCoreApplication::exit(2); });
    const int result = app.exec();
    return smoke && qmlFailed ? 1 : result;
}
