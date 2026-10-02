#include "AppState.h"
#include "AppPaths.h"
#include "GamePackages.h"
#include <QCoreApplication>
#include <QTimer>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <QRandomGenerator>
#include <QKeySequence>
#include <cmath>

QVariantMap AppState::defaults(const QString &name)
{
    QVariantMap t{{"background", "#090d19"}, {"surface", "#101525"},
        {"text", "#eeeefa"}, {"muted", "#969fbc"}, {"accent", "#9146ff"},
        {"border", "#252b46"}, {"fontFamily", "Segoe UI"}, {"fontSize", 13.0},
        {"radius", 10.0}, {"spacing", 14.0}, {"controlHeight", 36.0},
        {"cardHeight", 320.0}, {"artOpacity", 1.0}, {"animations", true},
        {"showArtwork", true}, {"compactSidebar", false}};
    const QVariantMap extra{
        {"glass", false}, {"glassColor", "#c4a5ff"}, {"glassTint", 0.18}, {"windowSize", "Large"}, {"resizable", true}, {"rememberPosition", true}, {"snapToEdge", false}, {"windowOpacity", 1.0}, {"sidebarOpacity", 0.3}, {"cardOpacity", 0.9}, {"controlOpacity", 1.0},
        {"blurStrength", 0.45}, {"glowStrength", 0.15}, {"borderWidth", 1.0}, {"borderOpacity", 0.7},
        {"gradientBorders", false}, {"animatedBorders", false}, {"shadowStrength", 0.25}, {"shadowSoftness", 0.5}, {"shadowSpread", 4.0},
        {"enabledColor", "#9146ff"}, {"disabledColor", "#252b46"}, {"gradient1", "#9146ff"}, {"gradient2", "#46cfff"}, {"gradient3", "#ff709d"},
        {"gradientIntensity", 0.35}, {"gradientDirection", "Diagonal"}, {"gradientStops", "3"},
        {"animationMode", "Full"}, {"animationSpeed", 1.0}, {"pageTransition", "Fade"}, {"toggleAnimation", "Slide"},
        {"hoverEffect", "Brighten"}, {"clickEffect", "Shrink"}, {"openAnimation", "Fade"},
        {"fontWeight", 400.0}, {"letterSpacing", 0.0}, {"uiScale", 1.0}, {"density", "Normal"},
        {"sidebarSide", "Left"}, {"sidebarWidth", 222.0}, {"navigationStyle", "Sidebar"}, {"cardLayout", "Grid"},
        {"backgroundStyle", "Solid"}, {"backgroundImage", ""}, {"backgroundMotion", 1.0}, {"gameTheme", false},
        {"customCursor", false}, {"cursorShape", "Ring"}, {"cursorSize", 18.0}, {"cursorGlow", 0.3}, {"cursorTrail", false},
        {"sounds", false}, {"soundVolume", 0.3}, {"hoverSound", false}, {"clickSound", true}, {"toggleSound", true}, {"windowSound", true},
        {"menuKey", "F6"}, {"showHints", true}, {"widgetStyle", "Rounded"}, {"showSearch", true}, {"showBanner", true},
        {"showStatus", true}, {"showFooter", true}, {"showSectionHeaders", true}, {"showSectionBoxes", true},
        {"showTitleBar", true}, {"showWindowBorder", true}, {"showAddGame", true}, {"showNavigation", true}, {"showRecent", true}, {"showBrand", true}, {"showArtwork", true}, {"layoutElements", QVariantMap{}},
        {"sectionOrder", QStringList{}}, {"recentSettings", QStringList{}}
    };
    for (auto it = extra.cbegin(); it != extra.cend(); ++it) t[it.key()] = it.value();
    for (const auto &key : {"accent", "background", "surface", "text", "muted", "border", "enabledColor", "disabledColor", "gradient1", "gradient2", "gradient3", "glassColor"}) {
        t[QString(key) + "Rainbow"] = false;
        t[QString(key) + "RainbowSpeed"] = 1.0;
    }
    if (name == "Reference") { t["background"]="#060b1b"; t["surface"]="#0b1329"; t["accent"]="#8b35ff"; t["border"]="#262d4d"; t["muted"]="#adb6d8"; t["text"]="#f2f1ff"; t["radius"]=8.0; t["spacing"]=10.0; t["glowStrength"]=0.4; }
    if (name == "Blue") t["accent"]="#2f89ff";
    if (name == "OLED black") { t["background"] = "#000000"; t["surface"] = "#080808"; t["border"] = "#242424"; }
    if (name == "Cyberpunk") { t["accent"] = "#ff36ce"; t["gradient1"] = "#ff36ce"; t["gradient2"] = "#00e5ff"; t["backgroundStyle"] = "Grid"; t["glowStrength"] = 0.65; t["gradientBorders"] = true; }
    if (name == "Minimal") { t["radius"] = 4.0; t["accent"] = "#a4b2c6"; t["animations"] = false; t["animationMode"] = "Off"; t["shadowStrength"] = 0.0; }
    if (name == "Liquid glass") { t["glass"] = true; t["windowOpacity"] = 0.78; t["cardOpacity"] = 0.38; t["sidebarOpacity"] = 0.25; t["controlOpacity"] = 0.5; t["backgroundStyle"] = "Aurora"; t["radius"] = 22.0; t["borderOpacity"] = 0.5; }
    if (name == "Halloween") { t["accent"] = "#ff8a24"; t["gradient1"] = "#ff8a24"; t["gradient2"] = "#6422a8"; t["backgroundStyle"] = "Halloween"; }
    if (name == "Christmas") { t["accent"] = "#ef5363"; t["gradient1"] = "#177d55"; t["gradient2"] = "#a9183b"; t["backgroundStyle"] = "Christmas"; }
    if (name == "Crimson") t["accent"] = "#ff627e";
    if (name == "Mint") t["accent"] = "#70e3bf";
    if (name == "Amber") t["accent"] = "#edc779";
    if (name == "Daylight" || name == "Light") {
        t["background"] = "#eef1f6"; t["surface"] = "#ffffff";
        t["text"] = "#202537"; t["muted"] = "#606b82";
        t["accent"] = "#5364d9"; t["border"] = "#d3d9e5";
    }
    t["enabledColor"] = t["accent"];
    t["disabledColor"] = t["border"];
    if (name != "Cyberpunk" && name != "Halloween" && name != "Christmas") t["gradient1"] = t["accent"];
    return t;
}

QVariantMap AppState::sanitized(const QVariantMap &input, const QVariantMap &base)
{
    QVariantMap result = base;
    const QStringList colors{"background", "surface", "text", "muted", "accent", "border", "enabledColor", "disabledColor", "gradient1", "gradient2", "gradient3", "glassColor"};
    for (const auto &key : colors) {
        const QColor c(input.value(key).toString());
        if (c.isValid()) result[key] = c.name();
    }
    const QMap<QString, QPair<double, double>> ranges{
        {"fontSize", {12, 20}}, {"radius", {0, 30}}, {"spacing", {8, 28}},
        {"windowOpacity", {0.05, 1}}, {"sidebarOpacity", {0, 1}}, {"cardOpacity", {0, 1}}, {"controlOpacity", {0, 1}},
        {"glassTint", {0, 0.6}}, {"blurStrength", {0, 1}}, {"glowStrength", {0, 1}}, {"borderWidth", {0, 6}}, {"borderOpacity", {0, 1}},
        {"shadowStrength", {0, 1}}, {"shadowSoftness", {0, 1}}, {"shadowSpread", {0, 24}},
        {"gradientIntensity", {0, 1}}, {"animationSpeed", {0.25, 3}}, {"fontWeight", {100, 900}}, {"letterSpacing", {-1, 5}},
        {"uiScale", {0.75, 1.5}}, {"sidebarWidth", {150, 320}}, {"backgroundMotion", {0.1, 3}}, {"cursorSize", {8, 48}}, {"cursorGlow", {0, 1}}, {"soundVolume", {0, 1}},
        {"controlHeight", {34, 58}}, {"cardHeight", {220, 380}}, {"artOpacity", {0.1, 1.0}}};
    for (auto it = ranges.cbegin(); it != ranges.cend(); ++it) {
        bool ok = false;
        const double value = input.value(it.key()).toDouble(&ok);
        if (ok && std::isfinite(value)) result[it.key()] = qBound(it.value().first, value, it.value().second);
    }
    for (const auto &key : colors) {
        const auto toggle = key + "Rainbow", speed = key + "RainbowSpeed";
        if (input.value(toggle).metaType().id() == QMetaType::Bool) result[toggle] = input.value(toggle);
        bool ok = false; const auto v = input.value(speed).toDouble(&ok);
        if (ok && std::isfinite(v)) result[speed] = qBound(0.1, v, 5.0);
    }
    const QMap<QString, QStringList> enums{
        {"gradientDirection", {"Horizontal", "Vertical", "Diagonal"}}, {"gradientStops", {"2", "3"}},
        {"animationMode", {"Off", "Minimal", "Full"}}, {"pageTransition", {"Fade", "Slide", "Zoom", "Crossfade"}},
        {"toggleAnimation", {"Slide", "Fade", "Spring"}}, {"hoverEffect", {"None", "Glow", "Brighten", "Scale", "Border"}},
        {"clickEffect", {"None", "Ripple", "Pulse", "Shrink", "Bounce"}}, {"openAnimation", {"None", "Fade", "Scale", "Slide", "Blur"}},
        {"windowSize", {"Small", "Medium", "Large"}}, {"density", {"Compact", "Normal", "Spacious"}}, {"sidebarSide", {"Left", "Right"}},
        {"navigationStyle", {"Sidebar", "Top tabs", "Floating", "Icon rail"}}, {"cardLayout", {"Grid", "Columns", "List"}},
        {"backgroundStyle", {"Solid", "Gradient", "Image", "Aurora", "Particles", "Grid", "Waves", "Halloween", "Christmas"}},
        {"cursorShape", {"Ring", "Dot", "Crosshair"}}, {"widgetStyle", {"Rounded", "Square", "Pill"}}
    };
    for (auto it = enums.cbegin(); it != enums.cend(); ++it)
        if (it.value().contains(input.value(it.key()).toString())) result[it.key()] = input.value(it.key());
    for (const auto &key : {"menuKey", "backgroundImage"}) {
        const auto v = input.value(key).toString();
        if (input.contains(key) && v.size() <= 2048 && (QString(key) != "menuKey" || (QKeySequence::fromString(v, QKeySequence::PortableText).count() == 1 && QKeySequence::fromString(v, QKeySequence::PortableText)[0].key() != Qt::Key_unknown))) result[key] = v;
    }
    for (const auto &key : {"sectionOrder", "recentSettings"}) {
        if (!input.contains(key)) continue;
        QStringList list;
        for (const auto &v : input.value(key).toStringList()) if (!v.isEmpty() && v.size() <= 120 && !list.contains(v)) list.append(v);
        result[key] = list.mid(0, 200);
    }
    if (input.contains("layoutElements")) {
        QVariantMap elements;
        const auto entries = input.value("layoutElements").toMap();
        for (auto it = entries.cbegin(); it != entries.cend() && elements.size() < 300; ++it) {
            if (it.key().size() > 120) continue;
            const auto entry = it.value().toMap(); QVariantMap valid;
            for (const auto &k : {"hidden", "homeHidden", "rainbow"}) if (entry.value(k).metaType().id() == QMetaType::Bool) valid[k] = entry.value(k);
            const QColor color(entry.value("color").toString()); if (color.isValid()) valid["color"] = color.name();
            bool ok = false; auto height = entry.value("extraHeight").toDouble(&ok);
            if (ok && std::isfinite(height)) valid["extraHeight"] = qBound(0.0, height, 600.0);
            auto speed = entry.value("rainbowSpeed").toDouble(&ok);
            if (ok && std::isfinite(speed)) valid["rainbowSpeed"] = qBound(0.1, speed, 5.0);
            auto span = entry.value("span").toInt(&ok);
            if (ok) valid["span"] = qBound(1, span, 3);
            elements[it.key()] = valid;
        }
        result["layoutElements"] = elements;
    }
    for (const auto &key : {"resizable", "rememberPosition", "snapToEdge", "animations", "showArtwork", "compactSidebar", "glass", "gradientBorders", "animatedBorders", "gameTheme",
                           "customCursor", "cursorTrail", "sounds", "hoverSound", "clickSound", "toggleSound", "windowSound", "showHints", "showSearch", "showBanner", "showStatus", "showFooter", "showSectionHeaders", "showSectionBoxes", "showTitleBar", "showWindowBorder", "showAddGame", "showNavigation", "showRecent", "showBrand"})
        if (input.contains(key) && input.value(key).metaType().id() == QMetaType::Bool)
            result[key] = input.value(key);
    const QString font = input.value("fontFamily").toString().trimmed();
    if (!font.isEmpty() && font.size() <= 120) result["fontFamily"] = font;
    const auto mode = input.value("_colorMode").toString();
    if (mode == "light" || mode == "dark") result["_colorMode"] = mode;
    for (const auto &key : {"_lightPalette", "_darkPalette"}) {
        if (!input.contains(key)) continue;
        QVariantMap palette;
        const auto saved = input.value(key).toMap();
        for (const auto &colorKey : {"background", "surface", "text", "muted", "border"}) {
            const QColor color(saved.value(colorKey).toString());
            if (color.isValid()) palette[colorKey] = color.name();
        }
        result[key] = palette;
    }
    return result;
}

namespace {
QString portableSettings() {
    if(qEnvironmentVariableIsSet("NEXUS_SETTINGS_PATH"))return qEnvironmentVariable("NEXUS_SETTINGS_PATH");
    const auto target=wetsoxRoot()+"/configs/settings.json";
    QDir().mkpath(wetsoxRoot()+"/configs");
    const auto portableOld=wetsoxRoot()+"/settings.json";
    if(!QFile::exists(target)&&QFile::exists(portableOld))QFile::rename(portableOld,target);
    // Keep existing customization when moving from Nexus to Wetsox.
    if(!QFile::exists(target)) {
        const auto old=qEnvironmentVariable("APPDATA")+"/NexusDesktop/Nexus/settings.json";
        if(QFile::exists(old))QFile::copy(old,target);
    }
    return target;
}
}
AppState::AppState(QObject *parent) : AppState(portableSettings(),parent) {
    m_packageRoot=wetsoxCheats();
    refreshPackages();
    auto timer=new QTimer(this);timer->setInterval(1000);
    connect(timer,&QTimer::timeout,this,&AppState::refreshPackages);timer->start();
}
QVariantList AppState::games() const {
    if(m_packageRoot.isEmpty())return m_games;
    auto result=m_installed;
    for(const auto& value:m_games) {
        const auto custom=value.toMap();const auto id=custom.value("id").toString();bool installed=false;
        for(auto& entry:result) {
            auto game=entry.toMap();if(game.value("id").toString()!=id)continue;
            installed=true;
            // Legacy seeded FC5 entry has no artwork and must not override a pack.
            if(id!="farcry5"||!custom.value("artwork").toString().isEmpty())
                for(const auto& key:{"name","subtitle","artwork"})if(!custom.value(key).toString().isEmpty())game[key]=custom.value(key);
            entry=game;break;
        }
        if(!installed&&id!="farcry5")result.append(custom);
    }
    return result;
}
void AppState::refreshPackages() {
    if(m_packageRoot.isEmpty())return;
    const auto found=scanGamePackages(m_packageRoot);
    if(found==m_installed)return;
    m_installed=found;++m_revision;emit changed();
}

AppState::AppState(const QString &path, QObject *parent)
    : QObject(parent), m_path(path), m_global(defaults())
{
    m_games = {QVariantMap{{"id", "farcry5"}, {"name", "Far Cry 5"},
        {"subtitle", "Hope County, Montana"}, {"artwork", ""}, {"palette", 0}}};
    QFile file(m_path);
    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly)) { fail("Could not read saved settings: " + file.errorString()); return; }
    QJsonParseError parse;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
    const auto root = doc.object();
    if (parse.error != QJsonParseError::NoError || root.value("version").toInt() != 1) {
        fail("Saved settings are invalid or unsupported. Defaults loaded."); return;
    }
    m_windows = root.value("windows").toObject().toVariantMap();
    m_global = sanitized(root.value("global").toObject().toVariantMap(), defaults());
    const auto profiles = root.value("appearanceProfiles").toObject();
    for (auto it = profiles.begin(); it != profiles.end(); ++it)
        if (it.key().size() <= 80 && it.value().isObject()) m_profiles[it.key()] = sanitized(it.value().toObject().toVariantMap(), defaults());
    const auto local = root.value("local").toObject();
    for (auto it = local.begin(); it != local.end(); ++it)
        m_local[it.key()] = sanitized(it.value().toObject().toVariantMap(), defaults());
    const auto links = root.value("links").toObject();
    for (auto it = links.begin(); it != links.end(); ++it)
        if (it.value().isBool()) m_links[it.key()] = it.value().toBool();
    const auto games = root.value("games").toVariant().toList();
    QVariantList valid;
    QStringList ids;
    for (const auto &value : games) {
        auto game = value.toMap();
        const auto id = game.value("id").toString();
        if (id.isEmpty() || id == "loader" || ids.contains(id) || game.value("name").toString().trimmed().isEmpty()) continue;
        ids.append(id);
        game["palette"] = qBound(0, game.value("palette").toInt(), 2);
        valid.append(game);
    }
    if (!valid.isEmpty()) m_games = valid;
}

QStringList AppState::fonts() const { return QFontDatabase::families(); }
bool AppState::linked(const QString &scope) const { return m_links.value(scope, true); }
QVariantMap AppState::theme(const QString &scope) const
{
    return linked(scope) ? m_global : m_local.value(scope, m_global);
}
void AppState::setLinked(const QString &scope, bool value)
{
    if (linked(scope) == value) return;
    if (!value && !m_local.contains(scope)) m_local[scope] = m_global;
    m_links[scope] = value;
    commit();
}
QVariantMap &AppState::editableTheme(const QString &scope)
{
    if (linked(scope)) return m_global;
    if (!m_local.contains(scope)) m_local[scope] = m_global;
    return m_local[scope];
}
void AppState::setThemeValue(const QString &scope, const QString &key, const QVariant &value)
{
    auto &target = editableTheme(scope);
    auto next = sanitized({{key, value}}, target);
    if (key == "animationMode") next["animations"] = next.value("animationMode").toString() != "Off";
    if (key == "animations") next["animationMode"] = next.value("animations").toBool() ? "Full" : "Off";
    if (next == target) return;
    target = next;
    if (key != "recentSettings") {
        auto recent = target.value("recentSettings").toStringList(); recent.removeAll(key); recent.prepend(key); target["recentSettings"] = recent.mid(0, 12);
    }
    commit();
}
QVariantMap AppState::presetTheme(const QString &name) const { return defaults(name); }
void AppState::applyPreset(const QString &scope, const QString &name)
{
    if (!QStringList{"Reference", "Blue", "Violet", "Crimson", "Mint", "Amber", "Daylight", "Dark", "OLED black", "Light", "Cyberpunk", "Minimal", "Liquid glass", "Halloween", "Christmas"}.contains(name)) return;
    editableTheme(scope) = defaults(name);
    commit();
}
QStringList AppState::appearanceProfiles() const { return m_profiles.keys(); }
bool AppState::saveAppearance(const QString &scope, const QString &name) {
    const auto clean = name.trimmed();
    if (clean.isEmpty() || clean.size() > 80) { fail("Enter a theme name with 1–80 characters."); return false; }
    m_profiles[clean] = theme(scope); commit(); return true;
}
bool AppState::loadAppearance(const QString &scope, const QString &name) {
    if (!m_profiles.contains(name)) { fail("Theme profile was not found."); return false; }
    editableTheme(scope) = m_profiles.value(name); commit(); return true;
}
void AppState::deleteAppearance(const QString &name) { if (m_profiles.remove(name)) commit(); }
void AppState::randomizeTheme(const QString &scope) {
    auto &target = editableTheme(scope); auto *rng = QRandomGenerator::global();
    const double hue = rng->generateDouble();
    target["accent"] = QColor::fromHsvF(hue, 0.65, 1).name();
    target["enabledColor"] = target["accent"];
    target["background"] = QColor::fromHsvF(hue, 0.4, 0.07).name();
    target["surface"] = QColor::fromHsvF(hue, 0.3, 0.13).name();
    target["text"] = "#f1f3fa"; target["muted"] = "#a7b3c9";
    target["gradient1"] = target["accent"];
    target["gradient2"] = QColor::fromHsvF(std::fmod(hue + 0.3, 1.0), 0.65, 0.9).name();
    target["gradient3"] = QColor::fromHsvF(std::fmod(hue + 0.6, 1.0), 0.65, 0.9).name();
    commit();
}
void AppState::setColorMode(const QString &scope, bool light)
{
    auto &target = editableTheme(scope);
    const QString requested = light ? "light" : "dark";
    // Older saved themes have no mode metadata; infer their initial appearance.
    const QString current = target.value("_colorMode",
        QColor(target.value("background").toString()).lightnessF() > 0.5 ? "light" : "dark").toString();
    if (current == requested) return;
    const QStringList colors{"background", "surface", "text", "muted", "border"};
    QVariantMap previous;
    for (const auto &key : colors) previous[key] = target.value(key);
    target["_" + current + "Palette"] = previous;
    const auto saved = target.value("_" + requested + "Palette").toMap();
    const auto fallback = defaults(light ? "Daylight" : "Violet");
    for (const auto &key : colors) target[key] = saved.value(key, fallback.value(key));
    target["_colorMode"] = requested;
    commit();
}
bool AppState::exportTheme(const QString &scope, const QUrl &url)
{
    if (!url.isLocalFile()) { fail("Choose a local JSON file."); return false; }
    QSaveFile file(url.toLocalFile());
    const auto data = QJsonDocument(QJsonObject{{"version", 1}, {"theme", QJsonObject::fromVariantMap(theme(scope))}}).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        fail("Could not export theme: " + file.errorString()); return false;
    }
    return true;
}
bool AppState::importTheme(const QString &scope, const QUrl &url)
{
    QFile file(url.toLocalFile());
    if (!url.isLocalFile() || !file.open(QIODevice::ReadOnly)) { fail("Could not open theme file."); return false; }
    if (file.size() > 1024 * 1024) { fail("Theme file is too large."); return false; }
    QJsonParseError parse;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &parse);
    const auto root = doc.object();
    if (parse.error != QJsonParseError::NoError || root.value("version").toInt() != 1 || !root.value("theme").isObject()) {
        fail("Choose a valid Wetsox v1 theme JSON file."); return false;
    }
    auto &target = editableTheme(scope);
    target = sanitized(root.value("theme").toObject().toVariantMap(), target);
    commit();
    return true;
}
QString AppState::saveGame(const QString &id, const QString &name, const QString &subtitle, const QUrl &artwork)
{
    if (name.trimmed().isEmpty()) { fail("Enter a game name."); return {}; }
    if (!artwork.isEmpty() && !artwork.isLocalFile()) { fail("Select an image from your computer."); return {}; }
    const QString gameId = id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id;
    int index = -1;
    for (int i = 0; i < m_games.size(); ++i)
        if (m_games[i].toMap().value("id").toString() == gameId) { index = i; break; }
    if (!id.isEmpty() && index < 0) { fail("Game could not be found."); return {}; }
    const auto game = QVariantMap{{"id", gameId}, {"name", name.trimmed().left(80)},
        {"subtitle", subtitle.trimmed().left(120)}, {"artwork", artwork.toString()},
        {"palette", index < 0 ? m_games.size() % 3 : m_games[index].toMap().value("palette").toInt()}};
    if (index < 0) m_games.append(game); else m_games[index] = game;
    commit();
    return gameId;
}
void AppState::fail(const QString &message) { m_error = message; emit errorChanged(); }
void AppState::clearError() { m_error.clear(); emit errorChanged(); }
void AppState::saveWindowGeometry(const QString &scope, const QVariantMap &geometry)
{
    if(scope.isEmpty() || scope.size()>80)return;
    QVariantMap valid;
    for(const auto &key : {"x","y","width","height"}) {
        bool ok=false;double n=geometry.value(key).toDouble(&ok);
        if(!ok||!std::isfinite(n))return;
        valid[key]=int(qBound(QString(key)=="x"||QString(key)=="y"?-100000.0:100.0,n,100000.0));
    }
    if(m_windows.value(scope).toMap()==valid)return;
    m_windows[scope]=valid;commit();
}

void AppState::commit()
{
    QJsonObject local, links, profiles;
    for (auto it = m_profiles.cbegin(); it != m_profiles.cend(); ++it) profiles[it.key()] = QJsonObject::fromVariantMap(it.value());
    for (auto it = m_local.cbegin(); it != m_local.cend(); ++it) local[it.key()] = QJsonObject::fromVariantMap(it.value());
    for (auto it = m_links.cbegin(); it != m_links.cend(); ++it) links[it.key()] = it.value();
    const auto data = QJsonDocument(QJsonObject{{"version", 1}, {"global", QJsonObject::fromVariantMap(m_global)},
        {"windows", QJsonObject::fromVariantMap(m_windows)}, {"local", local}, {"links", links}, {"appearanceProfiles", profiles}, {"games", QJsonValue::fromVariant(m_games)}}).toJson();
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        fail("Changes work for this session but could not be saved: " + file.errorString());
    ++m_revision;
    emit changed();
}
