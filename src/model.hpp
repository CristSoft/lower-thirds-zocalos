#pragma once
#include <QColor>
#include <QImage>
#include <QJsonObject>
#include <QStringList>
#include <QVector>
#include <optional>
#include <cstdint>

namespace jcp {
enum Design { Liquid = 0, Classic, Band, GoldenTrail, TopBar, BottomBar, DoubleBar, SideCard, Outline, Diagonal, Broadcast, Communion, Modern, Ribbon3D, DesignCount };
QStringList designNames();
enum Logo { DefaultLogo, ChurchLogo, CircularLogo, MusicLogo, CrownLogo, BibleLogo, ImportedLogo };
struct Caption {
	QString id, name;
	QStringList lines;
	QJsonObject json() const;
	static Caption create();
};
struct Style {
	int design = 0;
	QColor plate{"#17120b"}, accent{"#f7a813"}, title{"#f5f6fa"}, detail{"#f7a813"};
	QString font = "Advent Sans", icon;
	bool showIcon = true;
	int logo = DefaultLogo;
	int radius = 90, transitionMs = 2200, holdSeconds = 15, iconSize = 170, maxWidth = 1440;
	QJsonObject json() const;
};
struct Library {
	QVector<Caption> captions;
	Style style;
	QJsonObject json() const;
	static std::optional<Library> parse(const QByteArray &bytes, QString &error);
};
bool saveLibrary(const QString &path, const Library &library, QString &error);
bool loadLibrary(const QString &path, Library &library, QString &error);

// Preview timing is independent of both scene playout and the saved library.
class PreviewPlayback {
public:
	void start(const Style &style, bool repeat = false);
	void stop();
	void tick(double seconds);
	bool playing = false, loop = false;
	double phase = 1.0;

private:
	double elapsed = 0.0, transition = 2.2, hold = 2.0;
};

// Deterministic state machine: edits to the library never change the on-air snapshot.
class Playout {
public:
	void launch(const Caption &caption, const Style &style);
	void hide();
	void stop();
	bool controls(const QString &id) const;
	void tick(double seconds);
	bool visible() const { return phase > 0.0 || target > 0.0; }
	bool exiting() const { return target == 0.0 && phase > 0.0; }
	double phase = 0.0;
	std::uint64_t generation = 0;
	Caption active;
	Style style;

private:
	double target = 0.0, held = 0.0;
	std::optional<Caption> pending;
	Style pendingStyle;
};
} // namespace jcp
