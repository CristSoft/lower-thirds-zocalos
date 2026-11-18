#include "model.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>

namespace jcp {
QStringList designNames()
{
	return {"Barra y emblema",
		"Clásico · aparición suave",
		"Banda · esquinas rectas",
		"Estela dorada · círculo y revelado",
		"Línea superior",
		"Línea inferior",
		"Doble línea",
		"Tarjeta lateral",
		"Marco fino",
		"Franjas diagonales · clásico",
		"Franjas + logo",
		"Recipiente llenado",
		"Moderno",
		"Cinta 3D"};
}
Caption Caption::create()
{
	return {QUuid::createUuid().toString(QUuid::WithoutBraces), "Nuevo zócalo", {"", "", ""}};
}
QJsonObject Caption::json() const
{
	return {{"id", id}, {"name", name}, {"lines", QJsonArray::fromStringList(lines)}};
}
QJsonObject Style::json() const
{
	return {{"design", design},      {"plate", plate.name()},        {"accent", accent.name()},
		{"title", title.name()}, {"detail", detail.name()},      {"font", font},
		{"icon", icon}, {"logo", logo},          {"showIcon", showIcon},         {"iconSize", iconSize},
		{"radius", radius},      {"transitionMs", transitionMs}, {"holdSeconds", holdSeconds},
		{"maxWidth", maxWidth}};
}
QJsonObject Library::json() const
{
	QJsonArray rows;
	for (const auto &c : captions)
		rows.append(c.json());
	return {{"version", 1}, {"captions", rows}, {"style", style.json()}};
}
std::optional<Library> Library::parse(const QByteArray &bytes, QString &error)
{
	QJsonParseError parseError;
	const auto doc = QJsonDocument::fromJson(bytes, &parseError);
	auto fail = [&](const QString &why) -> std::optional<Library> {
		error = why;
		return std::nullopt;
	};
	if (parseError.error != QJsonParseError::NoError || !doc.isObject())
		return fail("El archivo no contiene una biblioteca JSON válida.");
	const auto root = doc.object();
	if (root.value("version").toInt() != 1 || !root.value("captions").isArray() || !root.value("style").isObject())
		return fail("La versión o la estructura de la biblioteca no es compatible.");
	Library result;
	QSet<QString> ids;
	for (const auto value : root.value("captions").toArray()) {
		const auto o = value.toObject();
		if (!o.value("name").isString() || !o.value("lines").isArray())
			return fail("Hay un zócalo sin nombre o sin líneas válidas.");
		auto c = Caption::create();
		c.name = o.value("name").toString();
		c.id = o.value("id").toString();
		if (c.id.isEmpty() || ids.contains(c.id))
			c.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
		ids.insert(c.id);
		c.lines.clear();
		const auto lines = o.value("lines").toArray();
		if (lines.size() > 3)
			return fail("Cada zócalo admite hasta tres líneas.");
		for (const auto line : lines) {
			if (!line.isString() || line.toString().size() > 500)
				return fail("Una línea no es válida o supera los 500 caracteres.");
			c.lines.append(line.toString());
		}
		while (c.lines.size() < 3)
			c.lines.append("");
		result.captions.append(c);
	}
	const auto s = root.value("style").toObject();
	auto &t = result.style;
	t.design = std::clamp(s.value("design").toInt(), 0, int(DesignCount) - 1);
	auto color = [&](const char *key, QColor &out) {
		if (s.contains(key)) {
			QColor v(s.value(key).toString());
			if (!v.isValid())
				return false;
			out = v;
		}
		return true;
	};
	if (!color("plate", t.plate) || !color("accent", t.accent) || !color("title", t.title) ||
	    !color("detail", t.detail))
		return fail("La biblioteca contiene un color inválido.");
	t.font = s.value("font").toString(t.font);
	if (t.font.compare("Adven Sans", Qt::CaseInsensitive) == 0)
		t.font = "Advent Sans";
	t.icon = s.value("icon").toString();
	const int savedLogo = s.value("logo").toInt(t.icon.isEmpty() ? DefaultLogo : ImportedLogo);
	t.logo = savedLogo >= DefaultLogo && savedLogo <= ImportedLogo ? savedLogo : DefaultLogo;
	if (t.logo == ImportedLogo && t.icon.isEmpty())
		t.logo = DefaultLogo;
	t.showIcon = s.value("showIcon").toBool(true);
	t.iconSize = std::clamp(s.value("iconSize").toInt(t.iconSize), 32, 196);
	t.radius = std::clamp(s.value("radius").toInt(t.radius), 0, 98);
	t.transitionMs = std::clamp(s.value("transitionMs").toInt(2200), 150, 5000);
	t.holdSeconds = std::clamp(s.value("holdSeconds").toInt(t.holdSeconds), 0, 3600);
	t.maxWidth = std::clamp(s.value("maxWidth").toInt(1440), 64, 16384);
	return result;
}
bool saveLibrary(const QString &path, const Library &library, QString &error)
{
	if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
		error = "No se pudo crear la carpeta de la biblioteca.";
		return false;
	}
	QSaveFile f(path);
	const auto bytes = QJsonDocument(library.json()).toJson();
	if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit()) {
		error = "No se pudo guardar: " + f.errorString();
		return false;
	}
	return true;
}
bool loadLibrary(const QString &path, Library &library, QString &error)
{
	QFile f(path);
	if (!f.open(QIODevice::ReadOnly)) {
		error = f.errorString();
		return false;
	}
	const auto parsed = Library::parse(f.readAll(), error);
	if (!parsed)
		return false;
	library = *parsed;
	return true;
}
void PreviewPlayback::start(const Style &style, bool repeat)
{
	transition = std::clamp(style.transitionMs, 150, 5000) / 1000.0;
	hold = style.holdSeconds > 0 ? std::clamp(style.holdSeconds, 1, 3600) : 2;
	elapsed = 0.0;
	phase = 0.0;
	playing = true;
	loop = repeat;
}
void PreviewPlayback::stop()
{
	playing = false;
	loop = false;
	elapsed = 0.0;
	phase = 1.0;
}
void PreviewPlayback::tick(double seconds)
{
	if (!playing)
		return;
	elapsed += std::max(0.0, seconds);
	const double duration = 2.0 * transition + hold;
	if (elapsed >= duration) {
		if (loop)
			elapsed = std::fmod(elapsed, duration);
		else {
			elapsed = duration;
			phase = 0.0;
			playing = false;
			return;
		}
	}
	if (elapsed < transition)
		phase = elapsed / transition;
	else if (elapsed < transition + hold)
		phase = 1.0;
	else
		phase = (duration - elapsed) / transition;
}
void Playout::launch(const Caption &caption, const Style &nextStyle)
{
	if (visible()) {
		pending = caption;
		pendingStyle = nextStyle;
		target = 0.0;
		return;
	}
	active = caption;
	++generation;
	style = nextStyle;
	phase = 0.0;
	target = 1.0;
	held = 0.0;
}
void Playout::hide()
{
	pending.reset();
	target = 0.0;
}
void Playout::stop()
{
	pending.reset();
	target = phase = held = 0.0;
}
bool Playout::controls(const QString &id) const
{
	return (visible() && active.id == id) || (pending && pending->id == id);
}
void Playout::tick(double seconds)
{
	seconds = std::max(0.0, seconds);
	for (int boundary = 0; boundary < 6; ++boundary) {
		if (phase != target) {
			const double duration = style.transitionMs / 1000.0;
			const double needed = std::abs(target - phase) * duration;
			const double used = std::min(seconds, needed);
			phase += (target > phase ? 1.0 : -1.0) * used / duration;
			seconds -= used;
			if (used >= needed)
				phase = target;
			else
				break;
		}
		if (phase == 0.0 && pending) {
			const auto next = *pending;
			const auto nextStyle = pendingStyle;
			pending.reset();
			launch(next, nextStyle);
		} else if (phase == 1.0 && target == 1.0 && style.holdSeconds > 0) {
			const double used = std::min(seconds, style.holdSeconds - held);
			held += used;
			seconds -= used;
			if (held >= style.holdSeconds)
				target = 0.0;
			else
				break;
		} else
			break;
		if (seconds <= 0.0)
			break;
	}
}
} // namespace jcp
