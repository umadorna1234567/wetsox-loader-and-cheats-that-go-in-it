#include "AppState.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <limits>

class ThemeTests : public QObject
{
    Q_OBJECT
private slots:
    void glassAndWindowPreferencesPersist() {
        QTemporaryDir dir; const auto path=dir.filePath("settings.json");
        {
            AppState state(path);
            state.setThemeValue("loader","glassColor","#29b8cc");
            state.setThemeValue("loader","glassTint",0.4);
            state.setThemeValue("loader","windowSize","Small");
            state.setThemeValue("loader","resizable",false);
            state.setLinked("farcry4",false);
            state.setThemeValue("loader","glassColor","#ee4488");
            QCOMPARE(state.theme("farcry4")["glassColor"].toString(),QString("#29b8cc"));
            state.saveWindowGeometry("loader",{{"x",40},{"y",50},{"width",1100},{"height",720}});
            state.saveWindowGeometry("farcry4",{{"x",140},{"y",150},{"width",1320},{"height",860}});
        }
        AppState loaded(path);
        QCOMPARE(loaded.theme("loader")["glassColor"].toString(),QString("#ee4488"));
        QCOMPARE(loaded.theme("loader")["windowSize"].toString(),QString("Small"));
        QCOMPARE(loaded.theme("loader")["resizable"].toBool(),false);
        QCOMPARE(loaded.windowGeometry("loader")["x"].toInt(),40);
        QCOMPARE(loaded.windowGeometry("farcry4")["x"].toInt(),140);
        loaded.setThemeValue("loader","glassTint",10.0);
        QCOMPARE(loaded.theme("loader")["glassTint"].toDouble(),0.6);
    }

    void presetControlsMatchPalette() {
        QTemporaryDir dir; AppState state(dir.filePath("settings.json"));
        for (const auto& name : {"Reference", "Blue", "Mint", "Light", "Cyberpunk", "Halloween", "Christmas"}) {
            state.applyPreset("loader", name);
            const auto theme=state.theme("loader");
            QCOMPARE(theme["enabledColor"], theme["accent"]);
            QCOMPARE(theme["disabledColor"], theme["border"]);
        }
    }

    void appearanceProfilesSurviveRestart() {
        QTemporaryDir dir; const auto path = dir.filePath("settings.json");
        QVariantMap saved;
        {
            AppState state(path);
            state.applyPreset("loader", "Liquid glass");
            state.setThemeValue("loader", "cardOpacity", 0.35);
            state.setThemeValue("loader", "accentRainbow", true);
            state.setThemeValue("loader", "accentRainbowSpeed", 2.0);
            state.setThemeValue("loader", "sectionOrder", QStringList{"Visuals", "Aimbot"});
            state.setThemeValue("loader", "layoutElements", QVariantMap{{"section:Aimbot", QVariantMap{{"hidden", true}, {"extraHeight", 60.0}}}});
            saved = state.theme("loader");
            QVERIFY(state.saveAppearance("loader", "Glass desk"));
            QVERIFY(!state.saveAppearance("loader", "  "));
            state.setLinked("farcry5", false);
            state.applyPreset("loader", "Light");
            QCOMPARE(state.theme("farcry5"), saved);
        }
        AppState loaded(path);
        QCOMPARE(loaded.appearanceProfiles(), QStringList{"Glass desk"});
        QVERIFY(loaded.loadAppearance("loader", "Glass desk"));
        QCOMPARE(loaded.theme("loader"), saved);
        const auto url = QUrl::fromLocalFile(dir.filePath("appearance.json"));
        QVERIFY(loaded.exportTheme("loader", url));
        loaded.applyPreset("farcry5", "Halloween");
        QVERIFY(loaded.importTheme("farcry5", url));
        QCOMPARE(loaded.theme("farcry5"), saved);
        loaded.deleteAppearance("Glass desk");
        QVERIFY(!loaded.loadAppearance("loader", "Glass desk"));
        QCOMPARE(loaded.theme("loader"), saved);
    }
    void appearanceValidationAndLegacyMigration() {
        QTemporaryDir dir; const auto path = dir.filePath("settings.json");
        QFile old(path); QVERIFY(old.open(QIODevice::WriteOnly));
        old.write("{\"version\":1,\"global\":{\"accent\":\"#123456\",\"fontSize\":16},\"local\":{},\"links\":{}}"); old.close();
        AppState state(path);
        QCOMPARE(state.theme("loader")["accent"].toString(), QString("#123456"));
        QCOMPARE(state.theme("loader")["windowOpacity"].toDouble(), 1.0);
        state.setThemeValue("loader", "windowOpacity", -10);
        QCOMPARE(state.theme("loader")["windowOpacity"].toDouble(), 0.05);
        state.setThemeValue("loader", "uiScale", 99);
        QCOMPARE(state.theme("loader")["uiScale"].toDouble(), 1.5);
        const auto before = state.theme("loader");
        state.setThemeValue("loader", "navigationStyle", "invalid");
        state.setThemeValue("loader", "glass", "false");
        state.setThemeValue("loader", "blurStrength", std::numeric_limits<double>::infinity());
        QCOMPARE(state.theme("loader"), before);
        state.setThemeValue("loader", "layoutElements", QVariantMap{{"control:color", QVariantMap{{"rainbow", true}, {"rainbowSpeed", 99.0}, {"extraHeight", -50}, {"color", "invalid"}}}});
        const auto entry = state.theme("loader")["layoutElements"].toMap()["control:color"].toMap();
        QCOMPARE(entry["rainbowSpeed"].toDouble(), 5.0);
        QCOMPARE(entry["extraHeight"].toDouble(), 0.0);
        QVERIFY(!entry.contains("color"));
    }

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
                if (it.key() != "recentSettings") QCOMPARE(state.theme("loader").value(it.key()), it.value());
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
    void animationsCanResumeAfterMinimalPreset() {
        QTemporaryDir dir;
        AppState state(dir.filePath("settings.json"));
        state.applyPreset("loader", "Minimal");
        QVERIFY(!state.theme("loader")["animations"].toBool());
        state.setThemeValue("loader", "animationMode", "Full");
        QVERIFY(state.theme("loader")["animations"].toBool());
        state.setThemeValue("loader", "animationMode", "Off");
        QVERIFY(!state.theme("loader")["animations"].toBool());
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
