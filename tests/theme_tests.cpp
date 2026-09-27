#include "AppState.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ThemeTests : public QObject
{
    Q_OBJECT
private slots:
    void modeSwitchPreservesCustomization() {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.json");
        QVariantMap custom;
        {
            AppState state(path);
            state.setThemeValue("loader", "fontFamily", "Comic Sans MS");
            state.setThemeValue("loader", "fontSize", 18);
            state.setThemeValue("loader", "radius", 26);
            state.setThemeValue("loader", "accent", "#ea7299");
            state.setThemeValue("loader", "background", "#151525");
            state.setThemeValue("loader", "animations", false);
            custom = state.theme("loader");
            state.setLinked("farcry5", false);
            state.setColorMode("loader", false);
            QCOMPARE(state.theme("loader"), custom);
            state.setColorMode("loader", true);
            const auto light = state.theme("loader");
            for (auto it = custom.cbegin(); it != custom.cend(); ++it) {
                if (!QStringList{"background", "surface", "text", "muted", "border"}.contains(it.key()))
                    QCOMPARE(light.value(it.key()), it.value());
            }
            QCOMPARE(state.theme("farcry5"), custom);
            QCOMPARE(state.theme("another-linked-menu"), light);
            state.setThemeValue("loader", "background", "#eee8e0");
            state.setColorMode("loader", false);
            for (auto it = custom.cbegin(); it != custom.cend(); ++it)
                QCOMPARE(state.theme("loader").value(it.key()), it.value());
        }
        AppState reloaded(path);
        reloaded.setColorMode("loader", true);
        QCOMPARE(reloaded.theme("loader")["background"].toString(), QString("#eee8e0"));
        QCOMPARE(reloaded.theme("loader")["accent"], custom["accent"]);
        QCOMPARE(reloaded.theme("loader")["fontFamily"], custom["fontFamily"]);
        const auto before = reloaded.theme("loader");
        reloaded.setColorMode("loader", true);
        QCOMPARE(reloaded.theme("loader"), before);
    }
    void sharedAndIndependentThemes() {
        QTemporaryDir dir;
        AppState state(dir.filePath("settings.json"));
        state.setThemeValue("loader", "accent", "#112233");
        QCOMPARE(state.theme("farcry5")["accent"].toString(), QString("#112233"));
        state.setLinked("farcry5", false);
        QCOMPARE(state.theme("farcry5"), state.theme("loader"));
        state.setThemeValue("farcry5", "accent", "#abcdef");
        QCOMPARE(state.theme("loader")["accent"].toString(), QString("#112233"));
        state.setThemeValue("another-game", "accent", "#334455");
        QCOMPARE(state.theme("loader")["accent"].toString(), QString("#334455"));
        QCOMPARE(state.theme("farcry5")["accent"].toString(), QString("#abcdef"));
        state.setLinked("farcry5", true);
        QCOMPARE(state.theme("farcry5"), state.theme("loader"));
        state.setLinked("farcry5", false);
        QCOMPARE(state.theme("farcry5")["accent"].toString(), QString("#abcdef"));
        state.setLinked("loader", false);
        state.setThemeValue("loader", "accent", "#fedcba");
        QCOMPARE(state.theme("another-game")["accent"].toString(), QString("#334455"));
    }
    void persistsAndReloads() {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.json");
        {
            AppState state(path);
            state.applyPreset("loader", "Mint");
            state.setLinked("farcry5", false);
            state.setThemeValue("farcry5", "radius", 24);
            QVERIFY(!state.saveGame({}, "Test Game", "Test edition", {}).isEmpty());
            QVERIFY(state.error().isEmpty());
        }
        AppState loaded(path);
        QVERIFY(!loaded.linked("farcry5"));
        QCOMPARE(loaded.theme("farcry5")["radius"].toInt(), 24);
        QCOMPARE(loaded.theme("loader")["accent"].toString(), QString("#70e3bf"));
        QCOMPARE(loaded.games().size(), 2);
    }
    void importsExportsAndValidates() {
        QTemporaryDir dir;
        AppState state(dir.filePath("settings.json"));
        state.setThemeValue("loader", "fontSize", 1000);
        QCOMPARE(state.theme("loader")["fontSize"].toInt(), 20);
        const auto before = state.theme("loader");
        state.setThemeValue("loader", "accent", "not-a-color");
        state.setThemeValue("loader", "unknown", true);
        QCOMPARE(state.theme("loader"), before);
        const auto url = QUrl::fromLocalFile(dir.filePath("theme.json"));
        QVERIFY(state.exportTheme("loader", url));
        state.setLinked("farcry5", false);
        state.applyPreset("farcry5", "Crimson");
        QVERIFY(state.importTheme("farcry5", url));
        QCOMPARE(state.theme("farcry5"), before);
        QFile bad(dir.filePath("bad.json"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("{broken json"); bad.close();
        QVERIFY(!state.importTheme("loader", QUrl::fromLocalFile(bad.fileName())));
        QCOMPARE(state.theme("loader"), before);
    }
};
QTEST_MAIN(ThemeTests)
#include "theme_tests.moc"
