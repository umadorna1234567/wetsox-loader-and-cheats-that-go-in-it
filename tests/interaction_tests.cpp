#include "UiHelpers.h"
#include <QtTest>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

class InteractionTests : public QObject
{
    Q_OBJECT
private slots:
    void keyboardAndMouseRecording() {
        QWindow window;
        InputRecorder recorder;
        recorder.setWindow(&window);
        QSignalSpy recorded(&recorder, &InputRecorder::recorded);
        recorder.begin();
        QTest::keyClick(&window, Qt::Key_K, Qt::ControlModifier);
        QCOMPARE(recorded.takeFirst().at(0).toString(), QString("Ctrl+K"));
        QVERIFY(!recorder.listening());
        recorder.begin();
        QTest::keyClick(&window, Qt::Key_Alt);
        QCOMPARE(recorded.takeFirst().at(0).toString(), QString("Alt"));
        recorder.begin();
        QTest::mouseClick(&window, Qt::RightButton, Qt::ShiftModifier);
        QCOMPARE(recorded.takeFirst().at(0).toString(), QString("Shift+RMB"));
        recorder.begin();
        QTest::mouseClick(&window, Qt::BackButton);
        QCOMPARE(recorded.takeFirst().at(0).toString(), QString("Mouse 4"));
        QTest::keyClick(&window, Qt::Key_X);
        QVERIFY(recorded.isEmpty());
    }
    void cancellationClearingAndIsolation() {
        QWindow target, other;
        InputRecorder recorder;
        recorder.setWindow(&target);
        QSignalSpy recorded(&recorder, &InputRecorder::recorded);
        recorder.begin();
        QTest::keyClick(&other, Qt::Key_A);
        QVERIFY(recorder.listening());
        QVERIFY(recorded.isEmpty());
        QTest::keyClick(&target, Qt::Key_Escape);
        QVERIFY(!recorder.listening());
        QVERIFY(recorded.isEmpty());
        recorder.begin();
        QTest::keyClick(&target, Qt::Key_Backspace);
        QCOMPARE(recorded.size(), 1);
        QVERIFY(recorded.takeFirst().at(0).toString().isEmpty());
        recorder.begin();
        QEvent deactivate(QEvent::WindowDeactivate);
        QCoreApplication::sendEvent(&target, &deactivate);
        QVERIFY(!recorder.listening());
        QVERIFY(recorded.isEmpty());
    }
    void actualWindowShape() {
        const auto round = WindowShape::roundedRegion({500, 300}, 30);
        QVERIFY(!round.contains(QPoint(0, 0)));
        QVERIFY(!round.contains(QPoint(499, 299)));
        QVERIFY(round.contains(QPoint(30, 1)));
        QVERIFY(round.contains(QPoint(250, 150)));
        QVERIFY(WindowShape::roundedRegion({500, 300}, 0).contains(QPoint(0, 0)));
        QWindow window;
        window.setFlags(Qt::Window | Qt::FramelessWindowHint);
        window.resize(500, 300);
        WindowShape shape;
        shape.setWindow(&window);
        shape.setRadius(30);
        QCOMPARE(window.mask(), round);
#ifdef Q_OS_WIN
        if (QGuiApplication::platformName() == "windows") {
            // A hidden native window is sufficient to inspect the real OS shape.
            const auto handle = reinterpret_cast<HWND>(window.winId());
            const auto nativeRegion = CreateRectRgn(0, 0, 0, 0);
            const int regionType = GetWindowRgn(handle, nativeRegion);
            const bool excludesCorner = !PtInRegion(nativeRegion, 0, 0);
            const bool includesCenter = PtInRegion(nativeRegion,
                qRound(250 * window.devicePixelRatio()), qRound(150 * window.devicePixelRatio()));
            DeleteObject(nativeRegion);
            QVERIFY(regionType != ERROR);
            QVERIFY(excludesCorner);
            QVERIFY(includesCenter);
        }
#endif
        window.resize(600, 400);
        QTRY_COMPARE(window.mask(), WindowShape::roundedRegion({600, 400}, 30));
        shape.setRadius(0);
        QVERIFY(window.mask().isEmpty());
    }
};
QTEST_MAIN(InteractionTests)
#include "interaction_tests.moc"
