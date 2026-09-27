#include "AppState.h"
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
#include <cmath>

QVariantMap AppState::defaults(const QString &name)
{
    QVariantMap t{{"background", "#090d19"}, {"surface", "#101525"},
        {"text", "#eeeefa"}, {"muted", "#969fbc"}, {"accent", "#9146ff"},
        {"border", "#252b46"}, {"fontFamily", "Segoe UI"}, {"fontSize", 13.0},
        {"radius", 10.0}, {"spacing", 14.0}, {"controlHeight", 36.0},
        {"cardHeight", 320.0}, {"artOpacity", 1.0}, {"animations", true},
        {"showArtwork", true}, {"compactSidebar", false}};
    if (name == "Crimson") t["accent"] = "#ff627e";
    if (name == "Mint") t["accent"] = "#70e3bf";
    if (name == "Amber") t["accent"] = "#edc779";
    if (name == "Daylight") {
        t["background"] = "#eef1f6"; t["surface"] = "#ffffff";
        t["text"] = "#202537"; t["muted"] = "#606b82";
        t["accent"] = "#5364d9"; t["border"] = "#d3d9e5";
    }
    return t;
}

QVariantMap AppState::sanitized(const QVariantMap &input, const QVariantMap &base)
{
    QVariantMap result = base;
    const QStringList colors{"background", "surface", "text", "muted", "accent", "border"};
    for (const auto &key : colors) {
        const QColor c(input.value(key).toString());
        if (c.isValid()) result[key] = c.name();
    }
    const QMap<QString, QPair<double, double>> ranges{
        {"fontSize", {12, 20}}, {"radius", {0, 30}}, {"spacing", {8, 28}},
        {"controlHeight", {34, 58}}, {"cardHeight", {220, 380}}, {"artOpacity", {0.1, 1.0}}};
    for (auto it = ranges.cbegin(); it != ranges.cend(); ++it) {
        bool ok = false;
        const double value = input.value(it.key()).toDouble(&ok);
        if (ok && std::isfinite(value)) result[it.key()] = qBound(it.value().first, value, it.value().second);
    }
    for (const auto &key : {"animations", "showArtwork", "compactSidebar"})
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
    const auto target=QCoreApplication::applicationDirPath()+"/settings.json";
    // Keep existing customization when moving from Nexus to Wetsox.
    if(!QFile::exists(target)) {
        const auto old=qEnvironmentVariable("APPDATA")+"/NexusDesktop/Nexus/settings.json";
        if(QFile::exists(old))QFile::copy(old,target);
    }
    return target;
}
}
AppState::AppState(QObject *parent) : AppState(portableSettings(),parent) {
    m_packageRoot=qEnvironmentVariableIsSet("WETSOX_CHEATS_ROOT")?qEnvironmentVariable("WETSOX_CHEATS_ROOT"):QCoreApplication::applicationDirPath()+"/cheats";
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
    m_global = sanitized(root.value("global").toObject().toVariantMap(), defaults());
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
    const auto next = sanitized({{key, value}}, target);
    if (next == target) return;
    target = next;
    commit();
}
void AppState::applyPreset(const QString &scope, const QString &name)
{
    if (!QStringList{"Violet", "Crimson", "Mint", "Amber", "Daylight"}.contains(name)) return;
    editableTheme(scope) = defaults(name);
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
void AppState::commit()
{
    QJsonObject local, links;
    for (auto it = m_local.cbegin(); it != m_local.cend(); ++it) local[it.key()] = QJsonObject::fromVariantMap(it.value());
    for (auto it = m_links.cbegin(); it != m_links.cend(); ++it) links[it.key()] = it.value();
    const auto data = QJsonDocument(QJsonObject{{"version", 1}, {"global", QJsonObject::fromVariantMap(m_global)},
        {"local", local}, {"links", links}, {"games", QJsonValue::fromVariant(m_games)}}).toJson();
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit())
        fail("Changes work for this session but could not be saved: " + file.errorString());
    ++m_revision;
    emit changed();
}
