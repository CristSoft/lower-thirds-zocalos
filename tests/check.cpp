#include "model.hpp"
#include "renderer.hpp"
#include "panels.hpp"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QPushButton>
#include <QLineEdit>
#include <QFrame>
#include <QAbstractButton>
#include <QMenu>
#include <QAction>
#include <QScrollArea>
#include <QScrollBar>
#include <QDialog>
#include <QColorDialog>
#include <QSpinBox>
#include <QSlider>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QEventLoop>
#include <QToolButton>
#include <QDockWidget>
#include <QMainWindow>
#include <QComboBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QCryptographicHash>
#include <QSet>
#include <iostream>
#include <cstdlib>
#include <cmath>

static void check(bool pass, const char *message)
{
	if (!pass) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}
static int opaquePixels(const QImage &image)
{
	int count = 0;
	for (int y = 0; y < image.height(); ++y)
		for (int x = 0; x < image.width(); ++x)
			if (image.pixelColor(x, y).alpha() > 0)
				++count;
	return count;
}
int main(int argc, char **argv)
{
	QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
	QApplication app(argc, argv);
	check(jcp::loadBundledFonts(), "bundled Advent font registers without system installation");
	check(QFontInfo(QFont("Advent Sans")).family().contains("Advent"),
	      "default font resolves to bundled Advent Sans");
	jcp::Library library;
	check(library.style.font == "Advent Sans" && library.style.title == QColor("#F5F6FA") &&
		      library.style.iconSize == 170 && library.style.radius == 90 && library.style.holdSeconds == 15 &&
		      library.style.transitionMs == 2200,
	      "new library uses requested screenshot defaults");
	auto first = jcp::Caption::create();
	first.name = "Presentación";
	first.lines = {"Nombre y apellido", "Cargo o descripción", "Información adicional"};
	library.captions.append(first);
	library.style.maxWidth = 1700;
	library.style.iconSize = 120;
	QString error;
	QTemporaryDir dir;
	check(jcp::saveLibrary(dir.path() + "/library.json", library, error), "atomic save");
	jcp::Library restored;
	check(jcp::loadLibrary(dir.path() + "/library.json", restored, error), "reload");
	check(restored.captions[0].lines == first.lines, "three lines roundtrip");
	check(restored.style.iconSize == 120, "emblem size persists in library");
	check(restored.style.maxWidth == 1700, "maximum width persists");
	library.style.maxWidth = 900;
	library.style.iconSize = 196;
	auto legacy = library.json();
	auto legacyStyle = legacy["style"].toObject();
	legacyStyle.remove("iconSize");
	legacy["style"] = legacyStyle;
	check(jcp::Library::parse(QJsonDocument(legacy).toJson(), error)->style.iconSize == 170,
	      "missing emblem setting uses new default while saved settings retain their value");
	check(!jcp::Library::parse("{oops", error), "reject malformed JSON");
	check(!jcp::Library::parse("{\"version\":99,\"captions\":[],\"style\":{}}", error),
	      "reject unsupported version");
	auto invalid = library.json();
	auto style = invalid["style"].toObject();
	style["plate"] = "garbage";
	invalid["style"] = style;
	check(!jcp::Library::parse(QJsonDocument(invalid).toJson(), error), "reject invalid color");
	for (int i = 0; i < 1500; ++i)
		library.captions.append(jcp::Caption::create());
	check(jcp::Library::parse(QJsonDocument(library.json()).toJson(), error)->captions.size() == 1501,
	      "no fixed caption count");
	jcp::Playout p;
	p.launch(first, library.style);
	p.tick(2.2);
	check(p.phase == 1.0, "entry completes");
	auto second = first;
	second.id = "second";
	second.lines[0] = "Segundo";
	p.launch(second, library.style);
	check(p.active.id == first.id && p.exiting(), "switch exits old caption first");
	p.tick(2.2);
	check(p.active.id == second.id && p.phase == 0.0, "new snapshot starts after exit");
	p.tick(2.2);
	check(p.phase == 1.0, "replacement enters");
	p.hide();
	p.tick(2.2);
	check(!p.visible(), "manual hide completes");
	auto timed = library.style;
	timed.holdSeconds = 2;
	p.launch(first, timed);
	p.tick(2.2);
	check(!p.exiting(), "hold begins after entry");
	p.tick(1.9);
	check(!p.exiting(), "full hold duration");
	p.tick(.1);
	p.tick(2.2);
	check(!p.visible(), "automatic hide");
	p.launch(first, library.style);
	p.tick(.3);
	p.launch(second, library.style);
	p.hide();
	p.tick(3.0);
	check(!p.visible(), "hide cancels pending caption");
	p.launch(first, library.style);
	p.tick(2.2);
	const auto generation = p.generation;
	auto revised = first;
	revised.lines[0] = "Actualizado";
	p.launch(revised, library.style);
	p.tick(4.4);
	check(p.generation > generation && p.active.lines[0] == "Actualizado",
	      "same caption refresh survives a delayed UI tick");
	p.stop();
	p.launch(first, library.style);
	p.tick(.2);
	p.launch(second, library.style);
	check(p.controls(first.id) && p.controls(second.id), "stop controls include active and queued captions");
	p.stop();
	p.tick(100);
	check(!p.visible() && !p.controls(first.id) && !p.controls(second.id),
	      "emergency stop cancels queued caption immediately");
	{
		jcp::PreviewPlayback playback;
		auto timing = library.style;
		timing.transitionMs = 200;
		timing.holdSeconds = 1;
		playback.start(timing);
		check(playback.playing && !playback.loop && playback.phase == 0.0, "preview once starts at entry");
		playback.tick(.1);
		check(std::abs(playback.phase - .5) < .00001, "preview advances through entry");
		playback.tick(.2);
		check(playback.phase == 1.0, "preview holds full graphic after entry");
		playback.tick(.95);
		check(playback.phase > 0 && playback.phase < 1, "preview reads exit animation backwards");
		playback.tick(.3);
		check(!playback.playing && playback.phase == 0.0, "preview once finishes after exit");
		playback.start(timing, true);
		playback.tick(1.5);
		check(playback.playing && playback.loop && std::abs(playback.phase - .5) < .00001,
		      "loop wraps and preserves elapsed overshoot");
		playback.stop();
		check(!playback.playing && !playback.loop && playback.phase == 1.0,
		      "stop restores complete still preview");
		timing.holdSeconds = 0;
		playback.start(timing);
		playback.tick(1.0);
		check(playback.phase == 1.0, "indefinite live hold uses a short finite preview hold");
		playback.tick(2.0);
		check(!playback.playing, "indefinite live settings still produce a finite preview");
	}
	jcp::Renderer renderer;
	auto hidden = renderer.render(first, library.style, 0);
	check(opaquePixels(hidden) == 0, "transparent hidden frame");
	auto full = renderer.render(first, library.style, 1);
	check(full.size() == QSize(900, 196), "native dimensions");
	check(full.pixelColor(0, 0).alpha() == 0, "transparent rounded corner");
	check(opaquePixels(full) > 140000, "visible plate");
	auto smallStyle = library.style;
	smallStyle.iconSize = 96;
	const auto small = renderer.render(first, smallStyle, 1.0);
	int largeInk = 0, smallInk = 0;
	for (int y = 0; y < 196; ++y)
		for (int x = 0; x < 190; ++x) {
			if (full.pixelColor(x, y) == library.style.accent)
				++largeInk;
			if (small.pixelColor(x, y) == library.style.accent)
				++smallInk;
		}
	check(smallInk > 0 && smallInk < largeInk / 2, "emblem size changes rendered logo without changing plate size");
	check(small.pixelColor(198, 90) == full.pixelColor(198, 90), "emblem resizing preserves separator bar");
	if (argc > 1) {
		QDir().mkpath(argv[1]);
		full.save(QString(argv[1]) + "/zocalo-jcp-sur.png");
		for (int i = 0; i <= 10; ++i)
			renderer.render(first, library.style, i / 10.0)
				.save(QString(argv[1]) + QString("/entrada-%1.png").arg(i, 2, 10, QChar('0')));
	}
	for (int design = 0; design < 3; ++design) {
		auto s = library.style;
		s.design = design;
		auto frame = renderer.render(first, s, 1);
		check(opaquePixels(frame) > 140000, "all designs render");
	}
	auto noIcon = library.style;
	noIcon.showIcon = false;
	auto longCaption = first;
	longCaption.lines[0] = QString(300, QChar('W'));
	check(!renderer.render(longCaption, noIcon, 1).isNull(), "long text and hidden icon");
	{
		auto text = first;
		text.lines = {"Congreso de Comunicación y Esperanza", "", ""};
		auto wider = library.style;
		wider.maxWidth = 1920;
		QFont font(wider.font);
		font.setPixelSize(64);
		font.setBold(true);
		const int natural = int(std::ceil(225 + QFontMetricsF(font).horizontalAdvance(text.lines[0]) + 28));
		check(natural > 900 && natural <= 1920, "long-title fixture exceeds original width at full font size");
		const auto wide = renderer.render(text, wider, 1);
		check(wide.width() == 1920, "plate uses full configured width for long text");
		auto shortText = text;
		shortText.lines = {"Ana", "", ""};
		check(renderer.render(shortText, wider, 1).width() == wide.width(),
		      "short and long captions retain the same configured width");
		for (int design = 0; design < jcp::DesignCount; ++design) {
			auto fixedStyle = wider;
			fixedStyle.design = design;
			for (int count = 1; count <= 3; ++count) {
				shortText.lines = {"Ana", count >= 2 ? "Cargo" : "",
						   count == 3 ? "Información adicional" : ""};
				for (double phase : {0.0, .15, .5, 1.0})
					check(renderer.render(shortText, fixedStyle, phase).size() == QSize(1920, 196),
					      "fixed width survives line count, design and animation changes");
			}
		}
		if (argc > 1)
			renderer.render(shortText, wider, 1).save(QString(argv[1]) + "/zocalo-ancho-fijo-corto.png");
		wider.maxWidth = 1000;
		const auto capped = renderer.render(text, wider, 1);
		check(capped.width() == 1000, "changing fixed width changes plate size");
		const auto inkHeight = [&](const QImage &image) {
			int top = image.height(), bottom = -1;
			for (int y = 0; y < image.height(); ++y)
				for (int x = 225; x < image.width(); ++x)
					if (image.pixelColor(x, y) == wider.title) {
						top = std::min(top, y);
						bottom = std::max(bottom, y);
					}
			return bottom - top + 1;
		};
		check(inkHeight(wide) > inkHeight(capped), "long text fits the usable area of the fixed plate");
		if (argc > 1) {
			wide.save(QString(argv[1]) + "/zocalo-ancho.png");
			capped.save(QString(argv[1]) + "/zocalo-limite.png");
		}
	}
	{
		auto rounded = library.style;
		rounded.maxWidth = 900;
		rounded.radius = 90;
		rounded.showIcon = false;
		auto text = first;
		text.lines = {"Texto", "", ""};
		const auto noSymbol = renderer.render(text, rounded, 1);
		check(noSymbol.pixelColor(93, 98) == rounded.accent && noSymbol.pixelColor(16, 98) == rounded.plate,
		      "without symbol the separator stops where the left corner curve begins");
		const auto titleBounds = [&](const QImage &image) {
			int left = image.width(), right = -1;
			for (int y = 0; y < image.height(); ++y)
				for (int x = 0; x < image.width(); ++x)
					if (image.pixelColor(x, y) == rounded.title) {
						left = std::min(left, x);
						right = std::max(right, x);
					}
			return std::make_pair(left, right);
		};
		check(titleBounds(noSymbol).first >= 110,
		      "hidden symbol leaves text padding after curved-corner separator");
		text.lines[0] = QString(40, QChar('M'));
		for (bool showIcon : {false, true}) {
			rounded.showIcon = showIcon;
			const auto longText = renderer.render(text, rounded, 1);
			check(titleBounds(longText).second <= 900 - 78,
			      "long text reserves right padding beyond 40px corner radius");
			if (argc > 1)
				longText.save(QString(argv[1]) + (showIcon ? "/zocalo-redondeado-con-simbolo.png"
									   : "/zocalo-redondeado-sin-simbolo.png"));
		}
		rounded.showIcon = false;
		auto empty = first;
		empty.lines = {"", "", ""};
		for (int radius : {0, 40, 41, 90, 98}) {
			rounded.radius = radius;
			const auto mask = renderer.render(empty, rounded, .05);
			for (int frame = 1; frame <= 20; ++frame) {
				const auto animated = renderer.render(empty, rounded, frame / 20.0);
				bool contained = true;
				for (int y = 0; y < 196 && contained; ++y)
					for (int x = 0; x < 900; ++x)
						if (mask.pixelColor(x, y).alpha() == 0 &&
						    animated.pixelColor(x, y).alpha() > 2) {
							contained = false;
							break;
						}
				check(contained,
				      "travelling separator never protrudes outside rounded plate at any animation phase");
			}
		}
		if (argc > 1) {
			noSymbol.save(QString(argv[1]) + "/zocalo-corto-sin-simbolo.png");
			renderer.render(text, rounded, .25).save(QString(argv[1]) + "/barra-en-movimiento.png");
		}
	}
	{
		check(jcp::designNames().size() == jcp::DesignCount, "all native styles have selector names");
		QSet<QByteArray> variants;
		QImage gallery(1200, ((jcp::DesignCount + 1) / 2) * 172, QImage::Format_RGB32);
		gallery.fill(QColor("#303944"));
		QPainter sheet(&gallery);
		sheet.setRenderHint(QPainter::SmoothPixmapTransform);
		sheet.setRenderHint(QPainter::TextAntialiasing);
		sheet.setPen(Qt::white);
		QFont labelFont;
		labelFont.setPixelSize(14);
		sheet.setFont(labelFont);
		for (int design = 0; design < jcp::DesignCount; ++design) {
			jcp::Style style;
			style.design = design;
			style.maxWidth = 900;
			const auto fullStyle = renderer.render(first, style, 1);
			const auto motion = renderer.render(first, style, .25);
			variants.insert(QCryptographicHash::hash(
				QByteArray(reinterpret_cast<const char *>(fullStyle.constBits()),
					   fullStyle.sizeInBytes()) +
					QByteArray(reinterpret_cast<const char *>(motion.constBits()),
						   motion.sizeInBytes()),
				QCryptographicHash::Sha256));
			check(opaquePixels(fullStyle) > 70000 && opaquePixels(renderer.render(first, style, 0)) == 0,
			      "each design has a visible hold and transparent endpoints");
			auto roundTrip = library.json();
			roundTrip["style"] = style.json();
			check(jcp::Library::parse(QJsonDocument(roundTrip).toJson(), error)->style.design == design,
			      "new style ID survives persistence");
			auto noLogo = style;
			noLogo.showIcon = false;
			check(fullStyle != renderer.render(first, noLogo, 1), "all styles honor icon visibility");
			auto recolored = style;
			recolored.accent = QColor("#12CCFF");
			check(fullStyle != renderer.render(first, recolored, 1), "all styles honor accent color");
			if (argc > 1) {
				fullStyle.save(QString(argv[1]) + QString("/estilo-%1.png").arg(design));
				const int x = (design % 2) * 600, y = (design / 2) * 172;
				sheet.drawText(QRectF(x + 10, y, 580, 28), Qt::AlignLeft | Qt::AlignVCenter,
					       jcp::designNames()[design]);
				sheet.drawImage(QRectF(x + 5, y + 30, 590, 128.5), fullStyle);
				if (design == jcp::GoldenTrail)
					for (int frame = 0; frame <= 4; ++frame)
						renderer.render(first, style, frame / 4.0)
							.save(QString(argv[1]) +
							      QString("/estela-entrada-%1.png").arg(frame));
			}
		}
		sheet.end();
		check(variants.size() == jcp::DesignCount, "styles have distinct artwork or animation");
		for (const int design : {jcp::Broadcast, jcp::Communion, jcp::Modern}) {
			jcp::Style fit;
			fit.design = design;
			fit.detail = QColor("#ff19d4");
			auto longText = first;
			longText.lines[1] = QString("Reflexión y participación para toda la comunidad ").repeated(2).trimmed();
			longText.lines[2] = "";
			const auto inkHeight = [&](int fixedWidth) {
				fit.maxWidth = fixedWidth;
				const auto frame = renderer.render(longText, fit, 1);
				// Compare two subtitle colors to locate the glyphs without depending on
				// platform-specific font antialiasing or a minimum pixel opacity.
				auto comparison = fit;
				comparison.detail = QColor("#23ff31");
				const auto other = renderer.render(longText, comparison, 1);
				int firstRow = 196, lastRow = -1;
				for (int y = 0; y < frame.height(); ++y) {
					for (int x = 0; x < frame.width(); ++x) {
						if (frame.pixel(x, y) != other.pixel(x, y)) {
							firstRow = std::min(firstRow, y);
							lastRow = y;
							break;
						}
					}
				}
				return std::max(0, lastRow - firstRow + 1);
			};
			const int narrow = inkHeight(640);
			check(narrow > 0 && inkHeight(1440) > narrow,
			      "long subtitles fit and grow with the actual width of each web plate");
		}

		if (argc > 1)
			gallery.save(QString(argv[1]) + "/galeria-estilos.png");
	}
	{
		auto text = first;
		text.lines = {QString(40, QChar('M')), QString(40, QChar('M')), QString(40, QChar('M'))};
		auto empty = first;
		empty.lines = {"", "", ""};
		const auto inkBounds = [&](const QImage &frame, const QImage &background) {
			QRect bounds;
			for (int y = 0; y < 196; ++y)
				for (int x = 150; x < 850; ++x)
					if (frame.pixelColor(x, y) != background.pixelColor(x, y))
						bounds = bounds.united(QRect(x, y, 1, 1));
			return bounds;
		};
		const auto barRows = [&](const QImage &frame, const jcp::Style &style) {
			QVector<int> rows;
			for (int y = 0; y < 196; ++y)
				if (frame.pixelColor(450, y).red() > style.plate.red() + 40)
					rows.append(y);
			return rows;
		};
		for (int design : {jcp::TopBar, jcp::BottomBar, jcp::DoubleBar}) {
			jcp::Style style;
			style.design = design;
			style.showIcon = false;
			style.maxWidth = 900;
			const auto half = renderer.render(text, style, .5),
				   halfEmpty = renderer.render(empty, style, .5);
			const auto ink = inkBounds(half, halfEmpty);
			check(!ink.isEmpty() && ink.right() > 700,
			      "vertical reveal spans text width rather than clipping horizontally");
			if (design == jcp::TopBar)
				check(ink.top() >= 103, "upper bar reveals text only below the moving bar");
			if (design == jcp::BottomBar)
				check(ink.bottom() <= 97, "lower bar reveals text only above the moving bar");
			if (design == jcp::DoubleBar)
				check(ink.top() >= 60 && ink.bottom() <= 137,
				      "double bar reveals text between centre-outward boundaries");
			const auto early = barRows(renderer.render(empty, style, .25), style),
				   late = barRows(renderer.render(empty, style, .75), style);
			check(!early.isEmpty() && !late.isEmpty(), "moving bars remain visible throughout sweep");
			if (design == jcp::TopBar)
				check(early.first() > late.last(), "upper bar sweeps upward to its resting position");
			if (design == jcp::BottomBar)
				check(early.last() < late.first(), "lower bar sweeps downward to its resting position");
			if (design == jcp::DoubleBar)
				check(early.first() > late.first() && early.last() < late.last(),
				      "double bars separate from the centre in opposite directions");
			if (argc > 1)
				for (int i = 0; i <= 4; ++i)
					renderer.render(first, style, i / 4.0)
						.save(QString(argv[1]) +
						      QString("/barrido-%1-%2.png").arg(design).arg(i));
		}
	}
	jcp::Controller controller(nullptr, dir.path() + "/ui.json");
	int attachments = 0, scenePreviews = 0, clears = 0;
	jcp::Caption shownCaption;
	jcp::Style shownStyle;
	controller.showScenePreview = [&](const jcp::Caption &caption, const jcp::Style &style, QString &) {
		++scenePreviews;
		shownCaption = caption;
		shownStyle = style;
		return true;
	};
	controller.hideScenePreviews = [&] {
		++clears;
	};
	controller.ensureSource = [&](QString &) {
		++attachments;
		return true;
	};
	auto *controls = controller.makeControlPanel();
	auto *settings = controller.makeDesignWindow();
	controls->setAttribute(Qt::WA_DontShowOnScreen);
	settings->setAttribute(Qt::WA_DontShowOnScreen);
	controls->resize(440, 900);
	settings->resize(500, 900);
	controls->show();
	settings->show();
	app.processEvents();
	if (argc > 1)
		settings->grab().save(QString(argv[1]) + "/panel-diseno-predeterminados.png");
	check(!controls->findChild<QToolButton *>("stop-all")->property("onAir").toBool() &&
		      !controls->findChild<QToolButton *>("stop-all")->styleSheet().contains("#c62828"),
	      "global stop starts neutral when no caption is on air");
	check(settings->isWindow() && !settings->isModal() && !qobject_cast<QDockWidget *>(settings),
	      "design is an independent modeless settings window");
	settings->hide();
	auto *gear = controls->findChild<QToolButton *>("open-design");
	check(gear && !gear->icon().isNull() && !gear->menu(), "settings gear replaces placement menu");
	gear->click();
	app.processEvents();
	check(settings->isVisible() && controller.makeDesignWindow() == settings,
	      "gear opens the existing settings window");
	settings->close();
	gear->click();
	app.processEvents();
	check(settings->isVisible(), "settings window can be closed and reopened");
	auto *designChoice = settings->findChild<QComboBox *>("design-choice");
	check(designChoice && designChoice->count() == jcp::DesignCount, "settings exposes all native designs");
	for (int i = 0; i < jcp::DesignCount; ++i) {
		designChoice->setCurrentIndex(i);
		check(controller.library.style.design == i, "style selection updates model");
	}
	designChoice->setCurrentIndex(jcp::Liquid);
	auto *logoChoice = settings->findChild<QComboBox *>("logo-choice");
	check(logoChoice && logoChoice->count() == 7, "settings provides all built-in logo choices");
	QSet<QByteArray> logos;
	for (const int logo : {jcp::ChurchLogo, jcp::CircularLogo, jcp::MusicLogo, jcp::CrownLogo, jcp::BibleLogo}) {
		logoChoice->setCurrentIndex(logoChoice->findData(logo));
		auto style = controller.library.style;
		style.design = jcp::Ribbon3D;
		check(style.logo == logo && style.showIcon, "logo selector updates the model");
		const auto result = renderer.render(first, style, 1);
		logos.insert(QCryptographicHash::hash(
			QByteArray(reinterpret_cast<const char *>(result.constBits()), result.sizeInBytes()),
			QCryptographicHash::Sha256));
		const auto reopened = jcp::Library::parse(QJsonDocument(controller.library.json()).toJson(), error);
		check(reopened && reopened->style.logo == logo, "selected logo survives reopening");
	}
	check(logos.size() == 5, "five distinct symbols render over the 3D ribbon");
	{
		const QColor initial("#b129d1"), replacement("#36dabc");
		for (const auto [design, logo] : {std::pair{jcp::Classic, jcp::ChurchLogo},
						 std::pair{jcp::Broadcast, jcp::DefaultLogo},
						 std::pair{jcp::Modern, jcp::DefaultLogo}}) {
			jcp::Style style;
			style.design = design;
			style.logo = logo;
			style.maxWidth = 900;
			style.accent = initial;
			const auto before = renderer.render(first, style, 1);
			style.accent = replacement;
			const auto after = renderer.render(first, style, 1);
			style.showIcon = false;
			const auto hidden = renderer.render(first, style, 1);
			int recolored = 0;
			for (int y = 65; y < 130; ++y)
				for (int x = 75; x < 140; ++x)
					if (before.pixelColor(x, y) == initial && after.pixelColor(x, y) == replacement &&
					    hidden.pixelColor(x, y) != replacement)
						++recolored;
			check(recolored > 25, "the official emblem retains its shape when the accent changes");
		}
	}

	logoChoice->setCurrentIndex(logoChoice->findData(-1));
	check(!controller.library.style.showIcon, "none hides the logo");
	logoChoice->setCurrentIndex(logoChoice->findData(jcp::DefaultLogo));
	check(controller.library.style.showIcon && controller.library.style.logo == jcp::DefaultLogo,
	      "default restores the design logo");
	auto oldLogoLibrary = controller.library.json();
	auto oldLogoSettings = oldLogoLibrary["style"].toObject();
	oldLogoSettings.remove("logo");
	oldLogoSettings["icon"] = "legacy-logo.png";
	oldLogoLibrary["style"] = oldLogoSettings;
	const auto migrated = jcp::Library::parse(QJsonDocument(oldLogoLibrary).toJson(), error);
	check(migrated && migrated->style.logo == jcp::ImportedLogo && migrated->style.icon == "legacy-logo.png",
	      "old libraries retain their imported logo");

	auto *titleColor = settings->findChild<QLineEdit *>("color-title");
	auto *titlePicker = settings->findChild<QToolButton *>("color-title-picker");
	check(titleColor && titlePicker, "text color has a direct input and separate color swatch");
	const auto previewBeforeColor = controller.preview->grab().toImage();
	titleColor->setText("12A0FF");
	app.processEvents();
	check(controller.library.style.title == QColor("#12A0FF") &&
		      titlePicker->property("color").toString() == "#12A0FF",
	      "typing hex updates model and swatch immediately");
	check(controller.preview->grab().toImage() != previewBeforeColor, "typing a text color refreshes the preview");
	QMetaObject::invokeMethod(titleColor, "editingFinished", Qt::DirectConnection);
	check(titleColor->text() == "#12A0FF", "hex without hash is normalized");
	titleColor->setText("#1");
	QMetaObject::invokeMethod(titleColor, "editingFinished", Qt::DirectConnection);
	check(controller.library.style.title == QColor("#12A0FF") && titleColor->text() == "#1" &&
		      titleColor->property("invalidColor").toBool(),
	      "invalid draft remains editable while last valid color stays active");
	bool pickerOpened = false;
	QTimer::singleShot(0, [&] {
		auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget());
		check(dialog, "swatch opens color selector");
		pickerOpened = true;
		dialog->setCurrentColor(QColor("#FA0408"));
		dialog->accept();
	});
	titlePicker->click();
	check(pickerOpened && controller.library.style.title == QColor("#FA0408") && titleColor->text() == "#FA0408",
	      "selecting a color updates input and configuration");
	QTimer::singleShot(0, [&] {
		auto *dialog = qobject_cast<QColorDialog *>(QApplication::activeModalWidget());
		check(dialog, "color selector can reopen");
		dialog->reject();
	});
	titlePicker->click();
	check(titleColor->text() == "#FA0408", "canceling color selection preserves current input");
	titleColor->setText("#FFFFFF");
	auto *iconSize = settings->findChild<QSpinBox *>("icon-size");
	const auto previewBeforeSize = controller.preview->grab().toImage();
	iconSize->setValue(128);
	app.processEvents();
	check(controller.library.style.iconSize == 128 && controller.preview->grab().toImage() != previewBeforeSize,
	      "emblem size control refreshes preview");
	for (const auto &key :
	     {QString("icon-size"), QString("radius"), QString("transition-ms"), QString("max-width")}) {
		auto *input = settings->findChild<QSpinBox *>(key);
		auto *slider = settings->findChild<QSlider *>(key + "-slider");
		check(input && slider, "adjustment has a slider and manual numeric input");
		const int previous = input->value();
		slider->setValue(slider->minimum() + 12);
		check(input->value() == slider->value(), "dragging slider updates manual value");
		input->setValue(previous);
		check(slider->value() == previous, "manual value updates slider");
	}
	auto *maximumWidth = settings->findChild<QSpinBox *>("max-width");
	controller.setCanvasWidth(1080);
	check(maximumWidth->maximum() == 1080 && maximumWidth->value() <= 1080 &&
		      controller.library.style.maxWidth <= 1080,
	      "portrait canvas clamps width setting and slider");
	controller.setCanvasWidth(1920);
	maximumWidth->setValue(1920);
	check(controller.library.style.maxWidth == 1920, "width can reach entire broadcast canvas");
	settings->resize(420, 360);
	app.processEvents();
	auto *settingsScroll = settings->findChild<QScrollArea *>("design-settings-scroll");
	check(settingsScroll && !settingsScroll->isAncestorOf(controller.preview) &&
		      settingsScroll->verticalScrollBar()->maximum() > 0,
	      "only settings controls scroll");
	const auto previewPosition = controller.preview->mapTo(settings, QPoint(0, 0));
	settingsScroll->verticalScrollBar()->setValue(settingsScroll->verticalScrollBar()->maximum());
	app.processEvents();
	check(controller.preview->mapTo(settings, QPoint(0, 0)) == previewPosition &&
		      controller.preview->visibleRegion().boundingRect().height() == controller.preview->height(),
	      "preview remains fully visible at the bottom of the settings scroll");
	auto *previewControls = settings->findChild<QWidget *>("preview-controls");
	check(previewControls && !settingsScroll->isAncestorOf(previewControls) &&
		      previewControls->visibleRegion().boundingRect().height() == previewControls->height(),
	      "preview transport stays fixed when settings scroll");
	if (argc > 1)
		settings->grab().save(QString(argv[1]) + "/panel-diseno-scroll-abajo.png");
	settingsScroll->verticalScrollBar()->setValue(0);
	settings->resize(520, 650);
	app.processEvents();
	{
		auto *line1 = settings->findChild<QToolButton *>("preview-line-1");
		auto *line2 = settings->findChild<QToolButton *>("preview-line-2");
		auto *line3 = settings->findChild<QToolButton *>("preview-line-3");
		auto *once = settings->findChild<QToolButton *>("preview-play");
		auto *stop = settings->findChild<QToolButton *>("preview-stop");
		auto *loop = settings->findChild<QToolButton *>("preview-loop");
		check(line1 && line2 && line3 && once && stop && loop, "preview line selectors and transport exist");
		const auto savedBefore = QJsonDocument(controller.library.json()).toJson();
		const auto sourceCalls = attachments;
		std::uint64_t revisionBefore;
		{
			std::lock_guard<std::mutex> lock(jcp::frames.mutex);
			revisionBefore = jcp::frames.revision;
		}
		check(!controller.previewCaption().lines[2].isEmpty() &&
			      controller.library.captions[0].lines[2].isEmpty(),
		      "empty source lines get example text only in preview");
		line2->click();
		line3->click();
		check(!controller.previewCaption().lines[0].isEmpty() &&
			      controller.previewCaption().lines[1].isEmpty() &&
			      controller.previewCaption().lines[2].isEmpty(),
		      "single-line preview preserves original line roles");
		if (argc > 1)
			settings->grab().save(QString(argv[1]) + "/panel-diseno-una-linea.png");
		line1->click();
		line2->click();
		check(controller.previewCaption().lines[0].isEmpty() && !controller.previewCaption().lines[1].isEmpty(),
		      "any individual line can be selected");
		line1->setChecked(true);
		line3->setChecked(true);
		const auto fullExample = controller.preview->grab().toImage();
		once->click();
		check(controller.previewPlayback.playing && !controller.previewPlayback.loop &&
			      controller.previewPlayback.phase == 0.0,
		      "play button starts one local preview");
		check(controller.preview->grab().toImage() != fullExample, "preview begins with transparent graphic");
		controller.previewPlayback.tick(10000);
		controller.refreshPreview();
		check(!controller.previewPlayback.playing && controller.previewPlayback.phase == 0.0,
		      "once does not restart itself");
		stop->click();
		check(controller.preview->grab().toImage() == fullExample, "stop shows the complete example again");
		loop->click();
		check(controller.previewPlayback.playing && controller.previewPlayback.loop && loop->isChecked(),
		      "loop starts and visibly selects repeat mode");
		const auto firstFrame = controller.preview->grab().toImage();
		QEventLoop animate;
		QTimer::singleShot(80, &animate, &QEventLoop::quit);
		animate.exec();
		check(controller.previewPlayback.phase > 0.0 && controller.preview->grab().toImage() != firstFrame,
		      "timer advances preview frames");
		controller.previewPlayback.tick(10000);
		controller.refreshPreview();
		check(controller.previewPlayback.playing && controller.previewPlayback.loop,
		      "loop continues across many cycles");
		once->click();
		check(!loop->isChecked() && !controller.previewPlayback.loop, "play once cancels loop mode");
		loop->click();
		settings->close();
		check(!controller.previewPlayback.playing && !loop->isChecked(),
		      "closing design stops preview playback");
		gear->click();
		stop->click();
		app.processEvents();
		check(QJsonDocument(controller.library.json()).toJson() == savedBefore && attachments == sourceCalls &&
			      !controller.playout.visible(),
		      "preview controls never edit the library or launch a scene source");
		{
			std::lock_guard<std::mutex> lock(jcp::frames.mutex);
			check(jcp::frames.revision == revisionBefore,
			      "preview never publishes frames to live source mailbox");
		}
	}
	auto click = [&](const QString &text) {
		for (auto *b : controls->findChildren<QAbstractButton *>())
			if (b->text() == text) {
				b->click();
				app.processEvents();
				return;
			}
		check(false, "button exists");
	};
	check(controller.library.captions.size() == 1, "starter caption");
	const auto firstId = controller.library.captions[0].id;
	auto card = [&](const QString &id) {
		for (auto *box : controller.rowsHost->findChildren<QFrame *>("caption-card"))
			if (box->property("captionId").toString() == id && !box->isHidden())
				return box;
		check(false, "caption row exists");
		return static_cast<QFrame *>(nullptr);
	};
	auto toggle = [&](const QString &id) {
		return card(id)->findChild<QToolButton *>("caption-toggle");
	};
	check(card(firstId)->findChild<QWidget *>("caption-editor")->isHidden(), "existing caption starts collapsed");
	auto *play = card(firstId)->findChild<QToolButton *>("launch-caption");
	check(play->isVisible() && !play->icon().isNull() && play->toolButtonStyle() == Qt::ToolButtonIconOnly,
	      "launch icon stays visible on collapsed row");
	play->click();
	check(attachments == 1 && controller.playout.active.id == firstId, "launch from collapsed header");
	check(!toggle(firstId)->isChecked(), "launch does not expand accordion");
	check(play->property("onAir").toBool() && play->text() == "Detener zócalo" &&
		      play->styleSheet().contains("#c62828"),
	      "live row switches to red stop");
	auto *generalStop = controls->findChild<QToolButton *>("stop-all");
	check(generalStop->property("onAir").toBool() && generalStop->styleSheet().contains("#c62828"),
	      "global stop is red with an on-air caption");
	controller.playout.tick(2.2);
	const auto stopIcon = play->icon().pixmap(18, 18).toImage();
	const auto revisionAtStop = jcp::frames.revision;
	play->click();
	check(controller.playout.visible() && controller.playout.exiting() && controller.playout.phase == 1.0 &&
		      attachments == 1 && clears == 1,
	      "row stop starts exit without cutting or relaunching");
	check(jcp::frames.revision == revisionAtStop, "stop does not replace live pixels with a blank frame");
	check(generalStop->styleSheet().contains("#c62828") && play->text() == "Detener zócalo",
	      "stop stays red throughout exit");
	controller.playout.tick(1.1);
	check(std::abs(controller.playout.phase - .5) < .00001, "stop respects configured exit duration");
	play->click();
	check(controller.playout.phase == .5 && controller.playout.exiting(),
	      "repeated stop does not restart or cut exit");
	controller.playout.tick(1.1);
	QEventLoop finishExit;
	QTimer::singleShot(45, &finishExit, &QEventLoop::quit);
	finishExit.exec();
	check(!controller.playout.visible() && play->text() == "Lanzar al vivo" &&
		      !generalStop->property("onAir").toBool(),
	      "buttons reset only when exit completes");
	check(!generalStop->styleSheet().contains("#c62828") && play->icon().pixmap(18, 18).toImage() != stopIcon,
	      "global stop returns neutral and row returns to play");
	controller.playout.hide();
	controller.playout.tick(10);
	toggle(firstId)->click();
	auto *nameField = card(firstId)->findChild<QLineEdit *>("caption-name");
	nameField->setText("Nombre actualizado");
	check(toggle(firstId)->accessibleName().contains("Nombre actualizado"), "header follows edited caption name");
	click("Agregar zócalo");
	check(controller.library.captions.size() == 2, "add from native panel");
	const auto newId = controller.library.captions[1].id;
	check(toggle(newId)->isChecked() && !toggle(firstId)->isChecked(), "new caption opens exclusively for editing");
	toggle(firstId)->click();
	check(!toggle(newId)->isChecked(), "opening one caption collapses the other");
	card(firstId)->findChild<QToolButton *>("scene-preview")->click();
	check(scenePreviews == 1 && shownCaption.id == firstId &&
		      shownCaption.lines == controller.library.captions[0].lines && attachments == 1 &&
		      !controller.playout.visible(),
	      "eye passes exact saved text to OBS preview without launching live");
	check(shownStyle.maxWidth == controller.library.style.maxWidth,
	      "OBS preview receives selected design and width");
	for (auto *b : card(firstId)->findChildren<QAbstractButton *>())
		if (b->text() == "Duplicar") {
			b->click();
			break;
		}
	app.processEvents();
	check(controller.library.captions.size() == 3, "duplicate from native panel");
	controller.sourceScale = [] {
		return 1.6;
	};
	card(firstId)->findChild<QToolButton *>("launch-caption")->click();
	check(attachments == 2 && controller.playout.visible(), "launch connects source");
	check(controller.playout.style.maxWidth == 1200,
	      "canvas width converts through existing OBS source scale without exceeding broadcast width");
	controller.liveSourceOnAir = [] {
		return false;
	};
	controller.rebuildRows();
	check(!controls->findChild<QToolButton *>("stop-all")->property("onAir").toBool(),
	      "general stop is neutral when live source is outside Program");
	controller.liveSourceOnAir = [] {
		return true;
	};
	controller.rebuildRows();
	check(controls->findChild<QToolButton *>("stop-all")->property("onAir").toBool(),
	      "general stop follows real Program visibility");
	const auto onAir = controller.playout.active.lines;
	const auto onAirId = controller.playout.active.id;
	settings->findChild<QToolButton *>("preview-line-1")->click();
	settings->findChild<QToolButton *>("preview-play")->click();
	check(controller.playout.active.id == onAirId && controller.playout.active.lines == onAir && attachments == 2,
	      "line selection and preview playback preserve the active on-air snapshot");
	settings->findChild<QToolButton *>("preview-stop")->click();
	settings->findChild<QToolButton *>("preview-line-1")->click();
	controller.library.captions[0].lines[0] = "Editado";
	controller.changed();
	check(controller.playout.active.lines == onAir, "editing preserves on-air snapshot");
	click("Ocultar zócalo");
	controller.playout.tick(10);
	check(!controller.playout.visible(), "hide from native panel");
	card(firstId)->findChild<QToolButton *>("launch-caption")->click();
	controller.search->setText("inexistente");
	app.processEvents();
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	check(controller.rowsHost->findChildren<QFrame *>("caption-card").isEmpty(), "search filters native rows");
	check(controls->findChild<QToolButton *>("stop-all")->isVisible(),
	      "global stop remains visible when active row is filtered out");
	controls->findChild<QToolButton *>("stop-all")->click();
	check(controller.playout.exiting() || !controller.playout.visible(),
	      "global stop requests exit with no matching rows");
	controller.playout.tick(10);
	check(!controller.playout.visible(), "filtered row finishes animated exit");
	auto *clearSearch = controls->findChild<QToolButton *>("clear-search");
	check(clearSearch && clearSearch->isEnabled(), "search exposes a clear icon button");
	const auto textsBeforeClear = QJsonDocument(controller.library.json()).toJson();
	clearSearch->click();
	check(controller.search->text().isEmpty() && !clearSearch->isEnabled() &&
		      QJsonDocument(controller.library.json()).toJson() == textsBeforeClear,
	      "clear search resets filter without editing library");
	app.processEvents();
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	QEventLoop settle;
	QTimer::singleShot(60, &settle, &QEventLoop::quit);
	settle.exec();
	{
		jcp::Controller ordered(nullptr, dir.path() + "/ordered.json");
		ordered.library.captions.clear();
		for (const auto &name :
		     {"Visible A", "Oculto B", "Visible C", "Oculto D", "Visible E", "Otro F", "Otro G"}) {
			auto entry = jcp::Caption::create();
			entry.name = QString::fromUtf8(name);
			entry.lines = {"Texto", "", ""};
			ordered.library.captions.append(entry);
		}
		const auto originals = ordered.library.captions;
		auto *panel = ordered.makeControlPanel();
		panel->setAttribute(Qt::WA_DontShowOnScreen);
		panel->resize(260, 205);
		panel->show();
		app.processEvents();
		auto *list = panel->findChild<QScrollArea *>("caption-list");
		auto findRow = [&](const QString &id) {
			for (auto *row : ordered.rowsHost->findChildren<QFrame *>("caption-card"))
				if (!row->isHidden() && row->property("captionId").toString() == id)
					return row;
			return static_cast<QFrame *>(nullptr);
		};
		auto drop = [&](const QString &id, const QString &target, bool after) {
			auto *row = findRow(target);
			if (!row) {
				std::cerr << "Missing drop target " << target.toStdString()
					  << " filter=" << ordered.search->text().toStdString()
					  << " rows=" << ordered.rowsHost->findChildren<QFrame *>("caption-card").size()
					  << '\n';
			}
			check(row, "drop target is visible");
			list->ensureWidgetVisible(row);
			app.processEvents();
			auto *source = findRow(id);
			check(source, "drag source is visible");
			QMimeData mime;
			mime.setData("application/x-jcp-sur-caption", source->findChild<QToolButton *>("caption-toggle")
									      ->property("dragPayload")
									      .toByteArray());
			const auto point = row->mapTo(list->viewport(), QPoint(10, after ? row->height() - 1 : 1));
			QDragEnterEvent enter(point, Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
			QApplication::sendEvent(list->viewport(), &enter);
			check(enter.isAccepted(), "native Qt drag target accepts this library's caption payload");
			QDragMoveEvent move(point, Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
			QApplication::sendEvent(list->viewport(), &move);
			check(move.isAccepted() && list->findChild<QFrame *>("caption-drop-indicator")->isVisible(),
			      "drag shows insertion line");
			QDropEvent event(QPointF(point), Qt::MoveAction, &mime, Qt::LeftButton, Qt::NoModifier);
			QApplication::sendEvent(list->viewport(), &event);
			check(event.isAccepted(), "native Qt drop reorders the library");
			QEventLoop dropSettles;
			QTimer::singleShot(40, &dropSettles, &QEventLoop::quit);
			dropSettles.exec();
			app.processEvents();
		};
		ordered.playout.launch(originals[0], ordered.library.style);
		ordered.playout.tick(2.2);
		const auto active = QJsonDocument(ordered.playout.active.json()).toJson();
		drop(originals[6].id, originals[0].id, false);
		check(ordered.library.captions[0].id == originals[6].id, "drag moves last caption to first");
		drop(originals[6].id, originals[5].id, true);
		check(ordered.library.captions.last().id == originals[6].id, "drag moves first caption to last");
		ordered.search->setText("Visible");
		app.processEvents();
		drop(originals[4].id, originals[0].id, false);
		check(ordered.library.captions[0].id == originals[4].id &&
			      ordered.library.captions[2].id == originals[0].id &&
			      ordered.library.captions[4].id == originals[2].id,
		      "filtered drag reorders visible matches");
		check(ordered.library.captions[1].id == originals[1].id &&
			      ordered.library.captions[3].id == originals[3].id,
		      "filtered drag preserves hidden slots");
		check(QJsonDocument(ordered.playout.active.json()).toJson() == active && ordered.playout.visible(),
		      "reordering preserves on-air text snapshot");
		check(ordered.save(), "reordered library saves");
		jcp::Library restoredOrder;
		check(jcp::loadLibrary(dir.path() + "/ordered.json", restoredOrder, error) &&
			      restoredOrder.captions[0].id == originals[4].id,
		      "custom order persists after reload");
		ordered.search->clear();
		app.processEvents();
		list->verticalScrollBar()->setValue(0);
		QMimeData invalidMime;
		invalidMime.setData("application/x-jcp-sur-caption", "different-library\ninvalid");
		QDragEnterEvent invalid(QPoint(10, 10), Qt::MoveAction, &invalidMime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(list->viewport(), &invalid);
		check(!invalid.isAccepted(), "reject external or foreign-library drags");
		auto *source = findRow(originals[4].id);
		QMimeData validMime;
		validMime.setData(
			"application/x-jcp-sur-caption",
			source->findChild<QToolButton *>("caption-toggle")->property("dragPayload").toByteArray());
		QPoint nearBottom(10, list->viewport()->height() - 2);
		QDragEnterEvent scrolling(nearBottom, Qt::MoveAction, &validMime, Qt::LeftButton, Qt::NoModifier);
		QApplication::sendEvent(list->viewport(), &scrolling);
		QEventLoop scrollWait;
		QTimer::singleShot(130, &scrollWait, &QEventLoop::quit);
		scrollWait.exec();
		check(list->verticalScrollBar()->value() > 0, "dragging near edge scrolls the caption list");
		QDragLeaveEvent leave;
		QApplication::sendEvent(list->viewport(), &leave);
		if (argc > 1)
			panel->grab().save(QString(argv[1]) + "/panel-ordenado.png");
		delete panel;
	}
	// Operation remains dockable; design is never registered as a dock.
	{
		QMainWindow main;
		main.setAttribute(Qt::WA_DontShowOnScreen);
		jcp::Controller docked(&main, dir.path() + "/docking/library.json");
		auto *controlDock = new QDockWidget("Zócalos", &main);
		controlDock->setWidget(docked.makeControlPanel());
		main.addDockWidget(Qt::RightDockWidgetArea, controlDock);
		controlDock->setAttribute(Qt::WA_DontShowOnScreen);
		controlDock->setFloating(true);
		docked.initializeDocking();
		check(!controlDock->isFloating(), "operation docks without requiring a design dock");
		check(controlDock->allowedAreas() == Qt::AllDockWidgetAreas, "panel can dock on any side");
		main.addDockWidget(Qt::LeftDockWidgetArea, controlDock);
		check(main.dockWidgetArea(controlDock) == Qt::LeftDockWidgetArea, "panel moves between docking areas");
		main.setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
		main.setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
		auto *bottomAction = controlDock->findChild<QAction *>("dock-bottom");
		check(bottomAction, "placement remains available through context menu");
		bottomAction->trigger();
		check(main.dockWidgetArea(controlDock) == Qt::BottomDockWidgetArea && !controlDock->isFloating(),
		      "dock below using context action with OBS side-corner layout");
		check(main.corner(Qt::BottomLeftCorner) == Qt::LeftDockWidgetArea,
		      "docking preserves OBS corner preferences");
		check(controlDock->minimumSizeHint().height() < 170, "compact panel fits a shallow bottom dock");
		controlDock->setFloating(true);
		docked.initializeDocking();
		check(controlDock->isFloating(), "subsequent startup preserves operator floating choice");
	}
	for (auto *b : controller.rowsHost->findChildren<QToolButton *>("caption-toggle"))
		b->setChecked(false);
	controls->resize(220, 240);
	app.processEvents();
	QEventLoop collapsed;
	QTimer::singleShot(40, &collapsed, &QEventLoop::quit);
	collapsed.exec();
	check(controls->minimumSizeHint().height() <= 140, "toolbar replaces separate rows and explanatory paragraph");
	check(controls->minimumSizeHint().width() <= 220, "panel can share a narrow dock");
	for (auto *box : controller.rowsHost->findChildren<QFrame *>("caption-card")) {
		check(box->height() <= 44, "collapsed caption has no unused title margin");
		auto *launch = box->findChild<QToolButton *>("launch-caption");
		auto *expand = box->findChild<QToolButton *>("caption-toggle");
		check(launch->mapTo(box, QPoint(0, 0)).x() < expand->mapTo(box, QPoint(0, 0)).x(),
		      "play remains on left of caption name");
		check(launch->visibleRegion().boundingRect().width() == launch->width(),
		      "play is not clipped at narrow panel width");
	}
	for (auto *action : controls->findChildren<QToolButton *>())
		if (action->parentWidget() == controls)
			check(action->toolButtonStyle() == Qt::ToolButtonIconOnly && !action->toolTip().isEmpty(),
			      "toolbar actions use labelled accessible icons");
	if (argc > 1) {
		controls->grab().save(QString(argv[1]) + "/panel-zocalos-estrecho.png");
		controls->resize(320, 280);
		app.processEvents();
		controls->grab().save(QString(argv[1]) + "/panel-zocalos.png");
		toggle(firstId)->click();
		app.processEvents();
		controls->grab().save(QString(argv[1]) + "/panel-zocalos-expandido.png");
		settings->grab().save(QString(argv[1]) + "/panel-diseno.png");
	}
	// Reproduce OBS-like QSS padding and its dark palette rather than relying
	// solely on the platform's unstyled widget size hints.
	for (auto *b : controller.rowsHost->findChildren<QToolButton *>("caption-toggle"))
		b->setChecked(false);
	QPalette dark = controls->palette();
	dark.setColor(QPalette::Window, QColor("#272a33"));
	dark.setColor(QPalette::WindowText, Qt::white);
	dark.setColor(QPalette::Base, QColor("#1d1f26"));
	dark.setColor(QPalette::Button, QColor("#3c404d"));
	dark.setColor(QPalette::ButtonText, Qt::white);
	dark.setColor(QPalette::Text, Qt::white);
	dark.setColor(QPalette::PlaceholderText, QColor("#9aa2b2"));
	app.setPalette(dark);
	controls->setPalette(dark);
	for (auto *widget : controls->findChildren<QWidget *>())
		widget->setPalette(dark);
	controls->setAutoFillBackground(true);
	controls->setStyleSheet(
		"QGroupBox { padding-top:32px;padding-bottom:5px;margin-bottom:4px; } QLineEdit { background:palette(button);color:palette(text);border:0;border-radius:3px;padding:4px 6px;height:22px; } QToolButton { background:palette(button);color:palette(button-text);border:0;border-radius:3px;padding:2px; } QToolButton:hover {background:palette(mid);}");
	controls->resize(320, 280);
	app.processEvents();
	QEventLoop themed;
	QTimer::singleShot(40, &themed, &QEventLoop::quit);
	themed.exec();
	check(controller.status->palette().color(QPalette::WindowText) == Qt::white,
	      "dark-theme status remains readable");
	for (auto *box : controller.rowsHost->findChildren<QFrame *>("caption-card"))
		check(box->height() <= 44, "OBS theme does not add group-title padding to caption rows");
	for (auto *box : controller.rowsHost->findChildren<QFrame *>("caption-card")) {
		const auto frame = box->grab().toImage();
		check(frame.pixelColor(frame.width() - 8, 2) == QColor("#1d1f26"),
		      "caption background follows OBS base role without requiring AlternateBase");
	}
	if (argc > 1)
		controls->grab().save(QString(argv[1]) + "/panel-zocalos-oscuro.png");
	card(firstId)->findChild<QToolButton *>("launch-caption")->click();
	app.processEvents();
	if (argc > 1)
		controls->grab().save(QString(argv[1]) + "/panel-zocalos-stop-rojo.png");
	controls->resize(220, 240);
	app.processEvents();
	for (auto *box : controller.rowsHost->findChildren<QFrame *>("caption-card")) {
		auto *launch = box->findChild<QToolButton *>("launch-caption");
		check(launch->visibleRegion().boundingRect().width() == launch->width(),
		      "play remains unclipped with OBS theme and narrow width");
	}
	controls->resize(480, 130);
	app.processEvents();
	check(controls->height() <= 140, "bottom panel can be reduced to a shallow strip");
	check(controls->findChild<QScrollArea *>("caption-list")->viewport()->height() >= 36,
	      "shallow bottom panel retains at least one usable caption row");
	if (argc > 1)
		controls->grab().save(QString(argv[1]) + "/panel-zocalos-franja-inferior.png");
	check(controller.save(), "native panel persistence");
	jcp::Library savedUi;
	check(jcp::loadLibrary(dir.path() + "/ui.json", savedUi, error) && savedUi.style.iconSize == 128,
	      "emblem size from settings persists after save");
	delete controls;
	delete settings;
	std::cout
		<< "PASS: persistence, playout, renderer, accordion, compact layouts, settings dialog, preview lines/once/loop isolation, typed/picked colors, bundled font, default values, sliders, fixed width, rounded-corner margins, contained separator, animated stops, conditional stop color, native designs, vertical and centre-outward sweeps, search clear, drag/drop reorder and OBS-preview routing\n";
}
