#include "renderer.hpp"
#include <QFile>
#include <QFontMetricsF>
#include <QFontDatabase>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QEasingCurve>
#include <QLinearGradient>
#include <QRadialGradient>
#include <algorithm>
#include <cmath>

namespace jcp {
bool loadBundledFonts()
{
	static const int id = QFontDatabase::addApplicationFont(":/jcp/AdventSans-Logo.otf");
	return id >= 0;
}
static QFont lineFont(const Style &s, int role, int count)
{
	QFont font(s.font);
	font.setBold(true);
	font.setItalic(role != 0);
	font.setPixelSize(role == 0 ? (count == 3 ? 52 : 64) : (count == 3 ? 32 : 40));
	return font;
}
int Renderer::width(const Style &s)
{
	return std::clamp(s.maxWidth, 64, 16384);
}
static double unit(double x)
{
	return std::clamp(x, 0.0, 1.0);
}
static double ease(double x)
{
	return QEasingCurve(QEasingCurve::InOutCubic).valueForProgress(unit(x));
}
static void drawLines(QPainter &p, const Caption &c, const Style &s, const QRectF &area, bool trail = false)
{
	QStringList lines;
	QVector<int> indices;
	for (int i = 0; i < c.lines.size() && i < 3; ++i)
		if (!c.lines[i].trimmed().isEmpty()) {
			lines.append(c.lines[i]);
			indices.append(i);
		}
	QVector<double> heights;
	double total = 0.0;
	for (int i = 0; i < lines.size(); ++i) {
		double h = indices[i] == 0 ? (lines.size() == 3 ? 59.0 : 74.0) : (lines.size() == 3 ? 39.0 : 48.0);
		if (trail)
			h = indices[i] == 0 ? (lines.size() == 3 ? 52.0 : lines.size() == 2 ? 62.0 : 94.0) : 30.0;
		heights.append(h);
		total += h + (i ? 4 : 0);
	}
	p.save();
	p.setClipRect(area, Qt::IntersectClip);
	double y = area.y() + (area.height() - total) / 2.0;
	for (int i = 0; i < lines.size(); ++i) {
		const bool title = indices[i] == 0;
		QFont font = lineFont(s, indices[i], int(lines.size()));
		if (trail) {
			font.setBold(false);
			font.setItalic(false);
			font.setPixelSize(title ? (lines.size() == 3   ? 46
						   : lines.size() == 2 ? 54
								       : 78)
						: (lines.size() == 3 ? 26 : 30));
		}
		const double width = QFontMetricsF(font).horizontalAdvance(lines[i]);
		if (width > area.width())
			font.setPixelSize(std::max(1, int(font.pixelSize() * area.width() / width)));
		p.setFont(font);
		p.setPen(title ? s.title : s.detail);
		p.drawText(QRectF(area.x(), y, area.width(), heights[i]),
			   Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, lines[i]);
		y += heights[i] + 4;
	}
	p.restore();
}
void Renderer::loadIcon(const Style &s)
{
	QString path;
	switch (s.logo) {
	case ChurchLogo: path = ":/jcp/web-art/isotipo-iglesia.svg"; break;
	case CircularLogo: path = ":/jcp/web-art/simbolo-circular-oficial.svg"; break;
	case MusicLogo: path = ":/jcp/web-art/music.svg"; break;
	case CrownLogo: path = ":/jcp/web-art/crown.svg"; break;
	case BibleLogo: path = ":/jcp/web-art/bible.svg"; break;
	case ImportedLogo: path = s.icon; break;
	default:
		switch (s.design) {
		case GoldenTrail: path = ":/jcp/trail-note.svg"; break;
		case Broadcast:
		case Modern: path = ":/jcp/web-art/isotipo-iglesia.svg"; break;
		case Communion: path = ":/jcp/web-art/communion.svg"; break;
		case Ribbon3D: path = ":/jcp/web-art/crown.svg"; break;
		default: path = ":/jcp/emblem.svg"; break;
		}
		break;
	}
	const QColor ink = !s.paletteColors && s.design == SideCard && s.logo == DefaultLogo ? s.plate : s.accent;
	const QColor symbolColor = !s.paletteColors && s.design == GoldenTrail ? ink.lighter(150) : ink;
	const auto colorKey = symbolColor.name() + (s.paletteColors ? "/palette" : "");
	if (path == cachedPath && colorKey == cachedColor)
		return;
	cachedPath = path;
	cachedColor = colorKey;
	icon = QImage(196, 196, QImage::Format_ARGB32_Premultiplied);
	icon.fill(Qt::transparent);
	QPainter p(&icon);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	if (path.endsWith(".svg", Qt::CaseInsensitive)) {
		QFile f(path);
		if (!f.open(QIODevice::ReadOnly))
			return;
		auto bytes = f.readAll();
		if (path == ":/jcp/web-art/isotipo-iglesia.svg")
			bytes.replace("#f5f6fa", symbolColor.name().toUtf8());
		else if (s.logo != ImportedLogo && path != ":/jcp/web-art/simbolo-circular-oficial.svg")
			bytes.replace("#f7a813", symbolColor.name().toUtf8());
		QSvgRenderer svg(bytes);
		if (svg.isValid()) {
			const QSizeF size = svg.viewBoxF().size().scaled(QSizeF(196, 196), Qt::KeepAspectRatio);
			svg.render(&p, QRectF((196 - size.width()) / 2, (196 - size.height()) / 2,
					      size.width(), size.height()));
			if (s.paletteColors) {
				p.setCompositionMode(QPainter::CompositionMode_SourceIn);
				p.fillRect(icon.rect(), symbolColor);
			}
		}
	} else {
		const QImage image(path);
		if (image.isNull())
			return;
		const auto size = image.size().scaled(180, 180, Qt::KeepAspectRatio);
		p.drawImage(QRectF((196 - size.width()) / 2.0, (196 - size.height()) / 2.0, size.width(),
				   size.height()),
			    image);
	}
}
QImage Renderer::renderTrail(const Caption &c, const Style &s, double phase)
{
	const int w = width(s);
	QImage image(w, 196, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	if (phase <= 0)
		return image.convertToFormat(QImage::Format_RGBA8888);
	loadIcon(s);
	const double progress = 1 - std::pow(1 - unit(phase), 3);
	const double travel = std::max(0, w - 184) * (1 - progress), edge = (s.showIcon ? 96.0 : 0.0) + travel;
	const double alpha = unit(phase * 12), radius = std::clamp(double(s.radius), 0.0, 71.0);
	const double left = s.showIcon ? 205.0 : std::max(36.0, radius + 20);
	const double right = 28 + std::max(0.0, radius - 40);
	QColor transparent = s.plate;
	transparent.setAlpha(0);
	QLinearGradient bg(0, 0, w, 0);
	auto start = s.plate;
	start.setAlphaF(.94f);
	auto mid = s.plate;
	mid.setAlphaF(.82f);
	bg.setColorAt(0, start);
	bg.setColorAt(.8, mid);
	bg.setColorAt(1, transparent);
	QColor goldEnd = s.accent;
	goldEnd.setAlpha(0);
	QLinearGradient gold(s.showIcon ? 95 : left, 0, w, 0);
	gold.setColorAt(0, s.accent);
	gold.setColorAt(.32, s.accent.lighter(160));
	gold.setColorAt(.75, s.accent);
	gold.setColorAt(1, goldEnd);
	QPainter p(&image);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::TextAntialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	p.setPen(Qt::NoPen);
	p.setBrush(bg);
	p.drawRoundedRect(QRectF(s.showIcon ? 95 : 0, 28, w - (s.showIcon ? 95 : 0), 142), radius, radius);
	p.setPen(QPen(QBrush(gold), 1.8));
	const double ruleLeft = s.showIcon ? 95 : std::max(16.0, radius);
	p.drawLine(QPointF(ruleLeft, 28), QPointF(w - 9, 28));
	p.drawLine(QPointF(ruleLeft, 170), QPointF(w - 9, 170));
	const double center = left + std::max(1.0, w - right - left) * .5;
	p.setPen(QPen(QBrush(gold), 1.2));
	p.drawLine(QPointF(left, 162), QPointF(center - 12, 162));
	p.drawLine(QPointF(center + 12, 162), QPointF(w - right, 162));
	QPainterPath diamond;
	diamond.moveTo(center, 156);
	diamond.lineTo(center + 5, 162);
	diamond.lineTo(center, 168);
	diamond.lineTo(center - 5, 162);
	diamond.closeSubpath();
	p.fillPath(diamond, s.accent.lighter(160));
	if (s.showIcon) {
		for (int i = 0; i < 3; ++i) {
			QPainterPath ribbon;
			ribbon.moveTo(140, 121 + i * 12);
			ribbon.cubicTo(175, 119 + i * 10, 180, 84 + i * 12, left - 12, 84 + i * 12);
			p.drawPath(ribbon);
		}
	}
	drawLines(p, c, s, QRectF(left, 28, std::max(1.0, w - right - left), 124), true);
	if (phase < 1) {
		QLinearGradient mask(edge - 35, 0, edge + 55, 0);
		mask.setColorAt(0, Qt::transparent);
		mask.setColorAt(1, Qt::white);
		p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		p.fillRect(image.rect(), mask);
		p.setCompositionMode(QPainter::CompositionMode_SourceOver);
	}
	p.end();
	// The circle travels in front of the reveal edge, independently of panel text.
	QPainter badge(&image);
	badge.setRenderHint(QPainter::Antialiasing);
	badge.setRenderHint(QPainter::SmoothPixmapTransform);
	badge.setOpacity(1.0);
	if (s.showIcon) {
		const double size = std::clamp(s.iconSize, 32, 196) * .93, r = size / 2, cx = 96 + travel;
		QRadialGradient disc(QPointF(cx, 98), r);
		disc.setColorAt(0, s.plate.lighter(160));
		disc.setColorAt(1, s.plate);
		QLinearGradient ring(cx - r, 98 - r, cx + r, 98 + r);
		ring.setColorAt(0, s.accent);
		ring.setColorAt(.45, s.accent.lighter(160));
		ring.setColorAt(1, s.accent);
		badge.setBrush(disc);
		badge.setPen(QPen(QBrush(ring), 2.4));
		badge.drawEllipse(QPointF(cx, 98), r, r);
		badge.setBrush(Qt::NoBrush);
		badge.setPen(QPen(QBrush(ring), .9));
		badge.drawEllipse(QPointF(cx, 98), std::max(1.0, r - 6), std::max(1.0, r - 6));
		const double iconSize = size * (s.logo == DefaultLogo ? 1.24 : .71);
		badge.drawImage(QRectF(cx - iconSize / 2, 98 - iconSize / 2, iconSize, iconSize), icon);
	}
	badge.end();
	if (alpha < 1) {
		QPainter fade(&image);
		fade.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		fade.fillRect(image.rect(), QColor(255, 255, 255, int(alpha * 255)));
	}
	return image.convertToFormat(QImage::Format_RGBA8888);
}
QImage Renderer::renderRibbon(const Caption &c, const Style &s, double phase)
{
	const int w = width(s);
	QImage image(w, 196, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);
	if (phase <= 0)
		return image;

	// The web app's Blender atlas stores 45 ribbon frames followed by two plate layers.
	static const QImage atlas(":/jcp/web-art/hymn-3d.png");
	const double frame = 1 + 59 * unit(phase);
	const auto progress = [frame](double start, double end) {
		const double t = unit((frame - start) / (end - start));
		return t * t * (3 - 2 * t);
	};
	const double scale = w / 900.0;
	QPainter p(&image);
	p.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
	if (s.paletteColors && ribbonColor != s.plate) {
		ribbonColor = s.plate;
		ribbonFrames.clear();
	}
	const auto tile = [&](int index, const QRectF &slice, const QRectF &target) {
		const int x = (index % 8) * 900, y = (index / 8) * 196;
		const QRectF destination(target.x() * scale, target.y(), target.width() * scale, target.height());
		if (!s.paletteColors || index >= 45) {
			p.drawImage(destination, atlas, slice.translated(x, y));
			return;
		}
		auto image = ribbonFrames.constFind(index);
		if (image == ribbonFrames.cend()) {
			auto frame = atlas.copy(x, y, 900, 196).convertToFormat(QImage::Format_RGBA8888);
			const int tones[] = {ribbonColor.red(), ribbonColor.green(), ribbonColor.blue()};
			constexpr double originalMidtone = .2126 * 116 + .7152 * 68 + .0722 * 172;
			for (int row = 0; row < frame.height(); ++row) {
				auto *pixel = frame.scanLine(row);
				for (int column = 0; column < frame.width(); ++column, pixel += 4) {
					const int purple = int(pixel[2]) - int(pixel[1]);
					if (!pixel[3] || purple <= 5)
						continue;
					const double t = unit((purple - 5) / 27.0);
					const double blend = t * t * (3 - 2 * t);
					const double light = .2126 * pixel[0] + .7152 * pixel[1] + .0722 * pixel[2];
					for (int channel = 0; channel < 3; ++channel) {
						const double tone = tones[channel];
						const double shade = light < originalMidtone
							? tone * light / originalMidtone
							: tone + (255 - tone) * (light - originalMidtone) / (255 - originalMidtone);
						pixel[channel] = uchar(std::lround(pixel[channel] + (shade - pixel[channel]) * blend));
					}
				}
			}
			ribbonFrames.insert(index, std::move(frame));
			image = ribbonFrames.constFind(index);
		}
		p.drawImage(destination, image.value(), slice);
	};
	const QRectF full(0, 0, 900, 196);
	tile(std::clamp(int(frame) - 1, 0, 44), full, full);
	const QRectF upper(117, 40, 759, 96), lower(168, 117, 538, 67);
	const auto plate = [&](int index, const QRectF &bounds, double amount, double tip) {
		const double visible = bounds.width() * amount;
		if (visible <= tip) {
			tile(index, bounds, QRectF(bounds.x(), bounds.y(), visible, bounds.height()));
			return;
		}
		auto body = bounds;
		body.setWidth(bounds.width() - tip);
		tile(index, body, QRectF(bounds.x(), bounds.y(), visible - tip, bounds.height()));
		auto end = bounds;
		end.setLeft(bounds.right() - tip);
		tile(index, end, QRectF(bounds.x() + visible - tip, bounds.y(), tip, bounds.height()));
	};
	plate(45, upper, progress(34, 54), 76);
	plate(46, lower, progress(37, 59), 68);
	if (frame > 34)
		tile(15, full, full);

	if (s.showIcon) {
		loadIcon(s);
		p.setOpacity(progress(10, 19));
		const double size = s.iconSize * .78;
		const double iconWidth = size * std::min(1.0, scale);
		p.drawImage(QRectF(137 * scale - iconWidth / 2, 94 - size / 2, iconWidth, size), icon);
	}
	QStringList details;
	for (int index = 1; index < c.lines.size() && index < 3; ++index) {
		if (!c.lines[index].trimmed().isEmpty())
			details.append(c.lines[index]);
	}
	const double textLeft = (s.showIcon ? 249 : 197) * scale;
	const auto draw = [&](const QString &line, QRectF area, QFont font, const QColor &color, double opacity) {
		if (line.trimmed().isEmpty())
			return;
		const double advance = QFontMetricsF(font).horizontalAdvance(line);
		if (advance > area.width())
			font.setPixelSize(std::max(1, int(font.pixelSize() * area.width() / advance)));
		p.setOpacity(opacity);
		p.setFont(font);
		p.setPen(color);
		p.save();
		p.setClipRect(area);
		p.drawText(area, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, line);
		p.restore();
	};
	const Style defaults;
	// Cinta 3D keeps its white and gold Blender plates. Use dark ink when a
	// palette's regular text color would disappear on those light surfaces.
	const auto ribbonText = [&s](const QColor &color, double limit, const QColor &fallback) {
		const auto luminance = [](const QColor &c) {
			return .2126 * c.redF() + .7152 * c.greenF() + .0722 * c.blueF();
		};
		if (s.paletteColors && luminance(color) > limit)
			return luminance(s.plate) <= limit ? s.plate : fallback;
		return color;
	};
	const QColor title = !s.paletteColors && s.title == defaults.title
				 ? QColor("#251145") : ribbonText(s.title, .40, QColor("#17120b"));
	const QColor detail = !s.paletteColors && s.detail == defaults.detail
				  ? QColor("#34210e") : ribbonText(s.detail, .32, QColor("#17120b"));
	if (!c.lines.isEmpty())
		draw(c.lines[0], QRectF(textLeft, 45, 802 * scale - textLeft, 75),
		     lineFont(s, 0, 3), title, progress(46, 56));
	for (int index = 0; index < details.size(); ++index) {
		const int height = details.size() == 2 ? 27 : 54;
		draw(details[index], QRectF(textLeft, 119 + height * index, 634 * scale - textLeft, height),
		     lineFont(s, 1, 3), detail, progress(48, 59));
	}
	p.end();
	return image;
}
static QColor alpha(QColor color, double opacity)
{
	color.setAlphaF(float(opacity));
	return color;
}

static QFont fitFont(QFont font, const QString &text, const QSizeF &space)
{
	// Measure in actual OBS pixels, before any plate transform is applied.
	while (font.pixelSize() > 1) {
		const QFontMetricsF metrics(font);
		const auto ink = metrics.boundingRect(text);
		const double width = std::max(metrics.horizontalAdvance(text), ink.right()) - std::min(0.0, ink.left());
		const double ratio =
			std::min(space.width() / std::max(1.0, width), space.height() / std::max(1.0, ink.height()));
		if (ratio >= 1)
			break;
		font.setPixelSize(std::max(1, std::min(font.pixelSize() - 1, int(font.pixelSize() * ratio))));
	}
	return font;
}

QImage Renderer::renderWebStyle(const Caption &c, const Style &s, double phase)
{
	const int w = width(s);
	QImage image(w, 196, QImage::Format_RGBA8888);
	image.fill(Qt::transparent);
	if (phase <= 0)
		return image;

	const bool broadcast = s.design == Broadcast;
	const bool communion = s.design == Communion;
	const bool modern = s.design == Modern;
	const double scale = w / 900.0;
	const double time = phase * 2200;
	const auto progress = [time](double begin, double end) {
		const double t = unit((time - begin) / (end - begin));
		return t * t * (3 - 2 * t);
	};

	Style colors = s;
	const Style defaults;
	if (!s.paletteColors && s.plate == defaults.plate)
		colors.plate = broadcast ? "#102545" : communion ? "#281e18" : modern ? "#071d30" : "#17120b";
	if (!s.paletteColors && s.title == defaults.title)
		colors.title = communion ? "#fff9ef" : modern ? "#f5faff" : "#ffffff";
	if (!s.paletteColors && s.detail == defaults.detail)
		colors.detail = broadcast ? "#ffffff" : communion ? "#f6dfbb" : modern ? "#e8f7ff" : "#17120b";
	if (!s.paletteColors && s.accent == defaults.accent && (communion || modern))
		colors.accent = communion ? "#d39b4e" : "#16bffa";
	const auto &ink = colors.plate;
	const auto &accent = colors.accent;

	int count = 0;
	int detailCount = 0;
	for (int i = 0; i < std::min(3, int(c.lines.size())); ++i)
		if (!c.lines[i].trimmed().isEmpty()) {
			++count;
			detailCount += i > 0;
		}
	const bool three = count == 3;
	const double left = broadcast ? 248 : communion ? 226 : modern ? 223 : 116;
	const double right = broadcast ? 788 : communion ? 809 : modern ? 689 : 826;
	const double titleHeight = broadcast   ? (three ? 51 : 64)
				   : communion ? (three ? 45 : 57)
				   : modern    ? 80
					       : (three ? 58 : 70);
	const double detailHeight = broadcast   ? (three ? 27 : 36)
				    : communion ? (three ? 25 : 34)
				    : modern    ? (detailCount == 2 ? 21 : 42)
						: (three ? 28 : 43);
	const double baseY = broadcast ? 40 : communion ? 41 : modern ? 42 : 29;
	const auto textFont = [&](int role, const QString &text) {
		const bool title = role == 0;
		const QString family = s.font != defaults.font                           ? s.font
				       : title && (communion || (!broadcast && !modern)) ? "Georgia"
											 : "Arial";
		QFont font(family);
		font.setWeight(title ? (modern || broadcast ? QFont::ExtraBold : QFont::Bold) : QFont::Normal);
		font.setItalic(broadcast || (!modern && !title));
		font.setPixelSize(title ? (broadcast   ? (three ? 36 : 43)
					   : communion ? (three ? 40 : 53)
					   : modern    ? 64
						       : (three ? 40 : 48))
					: (broadcast   ? (three ? 22 : 28)
					   : communion ? (three ? 21 : 30)
					   : modern    ? (detailCount == 2 ? 20 : 31)
						       : (three ? 23 : 30)));
		if (modern && title)
			font.setLetterSpacing(QFont::PercentageSpacing, 102.5);
		return fitFont(font, text, QSizeF((right - left) * scale - 2, title ? titleHeight : detailHeight));
	};

	QPainter p(&image);
	p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
	p.save();
	p.scale(scale, 1);
	const auto polygon = [&](std::initializer_list<QPointF> points, const QColor &color) {
		p.setPen(Qt::NoPen);
		p.setBrush(color);
		p.drawPolygon(QPolygonF(points));
	};
	const auto stroke = [&](const QPainterPath &path, const QBrush &brush, double size) {
		p.setPen(QPen(brush, size, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		p.setBrush(Qt::NoBrush);
		p.drawPath(path);
	};
	const QColor clearWhite(255, 255, 255, 0);
	if (broadcast) {
		QPainterPath badge;
		badge.moveTo(99, 12);
		badge.lineTo(223, 12);
		badge.quadTo(244, 12, 236, 33);
		badge.lineTo(185, 159);
		badge.quadTo(177, 180, 157, 180);
		badge.lineTo(45, 180);
		badge.quadTo(20, 180, 30, 157);
		badge.lineTo(76, 35);
		badge.quadTo(84, 12, 99, 12);
		badge.closeSubpath();

		QPainterPath bottom;
		bottom.moveTo(157, 180);
		bottom.lineTo(650, 180);
		bottom.cubicTo(733, 180, 745, 149, 830, 149);
		bottom.lineTo(848, 149);
		auto base = bottom;
		base.quadTo(857, 149, 862, 141);
		base.lineTo(888, 95);
		base.lineTo(125, 95);
		base.lineTo(95, 180);
		base.closeSubpath();
		p.save();
		p.setClipRect(QRectF(0, 0, 900 * progress(740, 1500), 196));
		p.fillPath(base, ink);
		polygon({{10, 52}, {870, 52}, {841, 104}, {0, 104}}, ink);
		p.restore();

		QPainterPath plate;
		plate.moveTo(240, 38);
		plate.lineTo(846, 38);
		plate.quadTo(854, 38, 849, 48);
		plate.lineTo(803, 116);
		plate.lineTo(196, 116);
		plate.closeSubpath();
		p.save();
		p.setClipRect(QRectF(0, 0, 900 * progress(980, 1700), 196));
		p.fillPath(plate, colors.title);
		p.restore();

		QPainterPath slash;
		slash.moveTo(222, 22);
		slash.lineTo(257, 22);
		slash.quadTo(265, 22, 260, 31);
		slash.lineTo(196, 139);
		slash.lineTo(180, 139);
		slash.closeSubpath();
		p.fillPath(slash, alpha(accent, progress(700, 1040)));
		p.save();
		p.setClipRect(QRectF(0, 0, 900 * progress(740, 1500), 196));
		stroke(bottom, alpha(accent, .48), 1.4);
		p.restore();
		p.fillPath(badge, alpha(ink, progress(220, 660)));
		const double trace = progress(0, 650);
		if (trace == 1)
			stroke(badge, accent, 2.5);
		else if (trace > 0) {
			QPen dash(alpha(accent, progress(0, 120)), 2.5, Qt::CustomDashLine, Qt::RoundCap);
			const double length = badge.length() / dash.widthF();
			dash.setDashPattern({trace * length, length});
			p.setPen(dash);
			p.setBrush(Qt::NoBrush);
			p.drawPath(badge);
		}
		for (const auto *path : {&badge, &bottom}) {
			const QPointF center = path->pointAtPercent(path == &badge ? .4 : .65);
			QRadialGradient shine(center, 70);
			shine.setColorAt(0, alpha(Qt::white, .85 * progress(1720, 2150)));
			shine.setColorAt(1, clearWhite);
			stroke(*path, shine, 1.6);
		}
	} else if (communion) {
		QPainterPath edge;
		edge.moveTo(0, 159);
		edge.lineTo(785, 159);
		edge.quadTo(831, 159, 856, 115);
		edge.lineTo(900, 36);
		auto body = edge;
		body.lineTo(0, 36);
		body.closeSubpath();
		const double reveal = progress(490, 1510);
		const double top = 170 - 144 * reveal;
		const double fold = 31 * std::sin(3.141592653589793 * reveal);
		QPainterPath wave;
		wave.moveTo(0, top);
		wave.cubicTo(290, top - fold, 610, top + fold, 900, top);
		wave.lineTo(900, 196);
		wave.lineTo(0, 196);
		wave.closeSubpath();
		p.setClipPath(wave);
		QLinearGradient plate(0, 36, 0, 159);
		plate.setColorAt(0, alpha(ink, .92));
		plate.setColorAt(1, alpha(ink, .78));
		p.fillPath(body, plate);
		QLinearGradient metal(0, 36, 720, 159);
		metal.setColorAt(0, QColor("#fff1cf"));
		metal.setColorAt(.34, accent);
		metal.setColorAt(.56, QColor("#ffe4ad"));
		metal.setColorAt(1, accent);
		p.save();
		p.setClipPath(body, Qt::IntersectClip);
		for (int i = 0; i < 2; ++i) {
			QPainterPath reflection;
			reflection.moveTo(0, 159);
			if (i == 0) {
				reflection.cubicTo(235, 149, 425, 191, 644, 141);
				reflection.cubicTo(738, 120, 817, 87, 900, 52);
			} else {
				reflection.cubicTo(230, 154, 440, 181, 623, 156);
				reflection.cubicTo(751, 138, 806, 132, 900, 36);
			}
			reflection.lineTo(900, 159);
			reflection.closeSubpath();
			p.setOpacity(i == 0 ? .19 : .13);
			p.fillPath(reflection, metal);
		}
		p.restore();
		stroke(edge, accent, 2);
		p.setPen(QPen(alpha(accent, .74), 1.2));
		p.drawLine(QPointF(0, 36), QPointF(900, 36));
		p.setPen(QPen(accent, 1.5));
		p.drawLine(QPointF(198, 58), QPointF(198, 137));
		p.save();
		p.translate(335, 159);
		p.scale(1, .10);
		QRadialGradient glint(0, 0, 100);
		glint.setColorAt(0, QColor("#fff5db"));
		glint.setColorAt(.28, alpha(accent, .8));
		glint.setColorAt(1, alpha(accent, 0));
		p.setOpacity(.72 * progress(1510, 1980));
		p.setPen(Qt::NoPen);
		p.setBrush(glint);
		p.drawEllipse(QPointF(0, 0), 100, 100);
		p.restore();
	} else if (modern) {
		const QString heading = c.lines.value(0);
		const double titleWidth = QFontMetricsF(textFont(0, heading)).horizontalAdvance(heading) / scale;
		const double edge = std::max(507.0, left + titleWidth + 18);
		QPainterPath body;
		body.moveTo(87, 35);
		body.lineTo(edge, 35);
		body.quadTo(edge + 17, 35, edge + 27, 46);
		body.lineTo(edge + 50, 67);
		body.quadTo(edge + 59, 75, edge + 74, 75);
		body.lineTo(873, 75);
		body.quadTo(896, 75, 887, 93);
		body.lineTo(861, 133);
		body.quadTo(855, 143, 842, 143);
		body.lineTo(740, 143);
		body.quadTo(729, 143, 721, 156);
		body.lineTo(713, 166);
		body.quadTo(706, 174, 691, 174);
		body.lineTo(33, 174);
		body.quadTo(9, 174, 18, 151);
		body.lineTo(65, 53);
		body.quadTo(73, 35, 87, 35);
		body.closeSubpath();
		QLinearGradient border(16, 0, 890, 0);
		border.setColorAt(0, accent);
		border.setColorAt(.42, alpha(accent, .34));
		border.setColorAt(1, alpha(accent, .84));
		p.save();
		const double opening = 900 * progress(330, 1160);
		const double openingHeight = 192 * progress(205, 850);
		p.setClipRect(
			QRectF(std::max(0.0, 103 - opening / 2), 104 - openingHeight / 2, opening, openingHeight));
		p.setOpacity(.16);
		stroke(body, border, 7);
		p.setOpacity(1);
		p.fillPath(body, ink);
		QLinearGradient blue(16, 35, 165, 165);
		blue.setColorAt(0, ink);
		blue.setColorAt(1, alpha(accent, .43));
		QPainterPath inset;
		inset.moveTo(87, 35);
		inset.lineTo(191, 35);
		inset.lineTo(131, 165);
		inset.lineTo(33, 165);
		inset.quadTo(15, 165, 21, 150);
		inset.lineTo(66, 54);
		inset.quadTo(74, 35, 87, 35);
		p.fillPath(inset, blue);
		QPainterPath strip;
		strip.moveTo(150, 122);
		strip.lineTo(734, 122);
		strip.lineTo(713, 156);
		strip.quadTo(708, 165, 691, 165);
		strip.lineTo(34, 165);
		auto trim = strip;
		strip.lineTo(130, 165);
		strip.closeSubpath();
		blue.setStart(130, 122);
		blue.setFinalStop(650, 165);
		p.fillPath(strip, blue);
		trim.moveTo(201, 58);
		trim.lineTo(201, 106);
		trim.moveTo(edge + 45, 67);
		trim.quadTo(edge + 56, 80, edge + 75, 80);
		trim.lineTo(789, 80);
		stroke(trim, border, 1.8);
		stroke(body, border, 2.4);
		for (int i = 0; i < 3; ++i) {
			const double x = 18 * i;
			const double begin = 1120 + i * 180;
			polygon({{808 + x, 86}, {818 + x, 86}, {809 + x, 102}, {798 + x, 102}},
				alpha(accent, .9 * progress(begin, begin + 320)));
		}
		p.restore();
		const double signal = progress(45, 630);
		const double radius = 22 + 44 * signal;
		p.setBrush(Qt::NoBrush);
		p.setOpacity(.84 * std::sin(3.141592653589793 * signal));
		p.setPen(QPen(border, 2));
		p.drawEllipse(QPointF(104, 103), radius, radius);
	} else {
		const double edge = 972 * (1 - std::pow(1 - unit(time / 1500), 3));
		QPainterPath reveal;
		reveal.addPolygon(QPolygonF({{0, 0}, {edge, 0}, {edge - 72, 196}, {0, 196}}));
		p.setClipPath(reveal);
		polygon({{0, 174}, {55, 22}, {91, 22}, {36, 174}}, accent);
		polygon({{91, 22}, {900, 22}, {865, 108}, {60, 108}}, ink);
		polygon({{60, 108}, {865, 108}, {861, 114}, {58, 114}}, accent);
		polygon({{58, 114}, {861, 114}, {839, 174}, {36, 174}}, s.paletteColors ? ink : colors.title);
	}
	p.restore();

	if (s.showIcon) {
		if (!s.paletteColors && (broadcast || modern)) {
			// Keep the web palette's light default emblem until the accent is customized.
			if ((s.logo == DefaultLogo && s.accent == defaults.accent) ||
			    (s.logo != DefaultLogo && s.logo != ChurchLogo))
				colors.accent = colors.title;
		}
		loadIcon(colors);
		const double amount = broadcast   ? progress(430, 870)
				      : modern    ? progress(95, 580)
				      : communion ? progress(160, 800)
						  : progress(100, 900);
		const double factor = s.iconSize / 170.0;
		const double edge = (broadcast ? 110 : modern ? 84 : communion ? 115 : 45) * factor;
		const double height = (communion ? 84 * factor : edge) * (modern ? .88 + .12 * amount : 1);
		const double iconWidth = std::min(edge * scale, edge);
		const double cx = (broadcast ? 111 : modern ? 104 : communion ? 121.5 : 92.5) * scale;
		const double cy = broadcast ? 100 + (1 - amount) * 10 : modern ? 103 : communion ? 96 : 65.5;
		const double size = communion && s.logo == DefaultLogo ? iconWidth : std::min(iconWidth, height);
		p.setOpacity(amount);
		p.drawImage(QRectF(cx - size / 2, cy - size / 2, size, size), icon);
	}

	const double titleOpacity = broadcast   ? progress(1380, 1900)
				    : modern    ? progress(675, 1300)
				    : communion ? progress(1020, 1870)
						: unit((unit(time / 1500) - .25) / .5);
	const double detailOpacity = broadcast ? titleOpacity * progress(1550, 2070)
				     : modern  ? titleOpacity * progress(1030, 1570)
					       : titleOpacity;
	const double drop = broadcast || communion ? (1 - titleOpacity) * 12 : 0;
	double y = baseY + drop;
	if (broadcast || communion) {
		const double spacing = broadcast ? (three ? 1 : 3) : (three ? 3 : 0);
		const double contentHeight = (count > detailCount ? titleHeight : 0) + detailCount * detailHeight +
					     std::max(0, count - 1) * spacing;
		y += std::max(0.0, ((broadcast ? 120 : 108) - contentHeight) / 2.0);
	}
	for (int role = 0; role < std::min(3, int(c.lines.size())); ++role) {
		const bool title = role == 0;
		if (c.lines[role].trimmed().isEmpty()) {
			if (title && modern)
				y += titleHeight;
			continue;
		}
		const QString text = title && (broadcast || s.design == Diagonal) ? c.lines[role].toUpper()
										  : c.lines[role];
		// White subtitles must start beneath the white title band, including with two detail rows.
		if (broadcast && !title)
			y = std::max(y, 116 + drop);
		const QRectF area(left * scale, y, (right - left) * scale, title ? titleHeight : detailHeight);
		const auto font = textFont(role, text);
		p.save();
		p.setClipRect(modern ? QRectF(area.x(), area.y(), area.width() * titleOpacity, area.height()) : area);
		p.setFont(font);
		p.setOpacity(title ? titleOpacity : detailOpacity);
		p.setPen(title ? (broadcast ? ink : colors.title) : colors.detail);
		const QFontMetricsF metrics(font);
		const auto bounds = metrics.boundingRect(text);
		p.drawText(QPointF(area.left() - std::min(0.0, bounds.left()), area.center().y() - bounds.center().y()),
			   text);
		p.restore();
		y += area.height() + (broadcast ? (three ? 1 : 3) : communion && three ? 3 : 0);
	}
	p.end();
	return image;
}
QImage Renderer::render(const Caption &c, const Style &s, double phase)
{
	if (s.design == GoldenTrail)
		return renderTrail(c, s, phase);
	if (s.design == Ribbon3D)
		return renderRibbon(c, s, phase);
	if (s.design >= Diagonal)
		return renderWebStyle(c, s, phase);
	const int canvasWidth = width(s);
	QImage image(canvasWidth, 196, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::transparent);
	if (phase <= 0.0)
		return image.convertToFormat(QImage::Format_RGBA8888);
	loadIcon(s);
	QPainter p(&image);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::TextAntialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	p.setPen(Qt::NoPen);
	if (s.design >= TopBar) {
		const double progress = ease(phase);
		p.setOpacity(progress);
		if (s.design == SideCard)
			p.translate(-28 * (1 - progress), 0);
	} else
		p.setOpacity(unit(phase / .08));
	p.setBrush(s.plate);
	const double radius = s.design == 2 ? 0.0
					    : std::clamp(double(s.radius), 0.0, std::min(98.0, canvasWidth / 2.0));
	QPainterPath plate;
	plate.addRoundedRect(QRectF(0, 0, canvasWidth, 196), radius, radius);
	p.drawPath(plate);
	p.setOpacity(s.design >= TopBar ? ease(phase) : 1.0);
	const double barRest = s.showIcon ? 195.0 : std::max(16.0, radius);
	const double left = s.design == SideCard ? (s.showIcon ? 225.0 : std::max(60.0, radius + 20.0) + 24.0)
			    : s.showIcon         ? 225.0
						 : barRest + 20.0;
	const double rightPadding = 28.0 + std::max(0.0, radius - 40.0);
	const double textWidth = std::max(1.0, canvasWidth - rightPadding - left);
	const double sweep = ease(phase);
	// Each line travels along the edge of the vertical text reveal. Double lines
	// begin together at the centre and separate towards their resting positions.
	const double upperY = s.design == DoubleBar ? 96.0 + (16.0 - 96.0) * sweep : 177.0 + (16.0 - 177.0) * sweep;
	const double lowerY = s.design == DoubleBar ? 96.0 + (177.0 - 96.0) * sweep : 16.0 + (177.0 - 16.0) * sweep;
	// Keep the bar's full height inside the curve at both ends of its travel.
	const double cornerInset =
		radius > 27.0 ? radius - std::sqrt(radius * radius - (radius - 27.0) * (radius - 27.0)) : 0.0;
	const double barStart = canvasWidth - 7.0 - std::max(28.0, std::ceil(cornerInset) + 2.0);
	const double distance = std::max(1.0, barStart - barRest);
	if (s.design >= TopBar) {
		p.save();
		p.setClipPath(plate, Qt::IntersectClip);
		p.setBrush(s.accent);
		if (s.design == TopBar || s.design == DoubleBar)
			p.drawRoundedRect(QRectF(left, upperY, textWidth, s.design == DoubleBar ? 4 : 7), 2, 2);
		if (s.design == BottomBar || s.design == DoubleBar)
			p.drawRoundedRect(QRectF(left, lowerY, textWidth, s.design == DoubleBar ? 4 : 7), 2, 2);
		if (s.design == SideCard)
			p.fillRect(QRectF(0, 0, s.showIcon ? 196 : std::max(60.0, radius + 20), 196),
				   s.paletteColors ? s.plate.lighter(150) : s.accent);
		if (s.design == Outline) {
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(s.accent, 2));
			p.drawRoundedRect(QRectF(4, 4, canvasWidth - 8, 188), std::max(0.0, radius - 4),
					  std::max(0.0, radius - 4));
		}
		p.restore();
	}
	const double travel = s.design == 0 ? distance * (1.0 - ease((phase - .10) / .38)) : 0.0;
	const double reveal = s.design == Liquid ? 1.0 - travel / distance : s.design >= TopBar ? 1.0 : ease(phase);
	p.save();
	p.setClipPath(plate, Qt::IntersectClip);
	p.setClipRect(QRectF(left + textWidth * (1.0 - reveal), 10, textWidth * reveal, 176), Qt::IntersectClip);
	if (s.design == TopBar)
		p.setClipRect(QRectF(left, upperY + 7, textWidth, std::max(0.0, 186.0 - upperY - 7)),
			      Qt::IntersectClip);
	if (s.design == BottomBar)
		p.setClipRect(QRectF(left, 10, textWidth, std::max(0.0, lowerY - 10)), Qt::IntersectClip);
	if (s.design == DoubleBar)
		p.setClipRect(QRectF(left, upperY + 4, textWidth, std::max(0.0, lowerY - upperY - 4)),
			      Qt::IntersectClip);
	drawLines(p, c, s, QRectF(left, 0, textWidth, 196));
	p.restore();
	if (s.design < TopBar) {
		p.save();
		p.setClipPath(plate, Qt::IntersectClip);
		p.setPen(Qt::NoPen);
		p.setOpacity(unit((phase - .05) / .05));
		p.setBrush(s.accent);
		const double barX = barRest + travel;
		p.drawRoundedRect(QRectF(barX, 27, 7, 142), 3.5, 3.5);
		p.restore();
	}
	p.setOpacity(s.design >= TopBar ? ease(phase) : 1.0);
	if (s.showIcon) {
		const double iconSize = std::clamp(s.iconSize, 32, 196);
		const double iconScale = iconSize / 196.0, iconInset = (196.0 - iconSize) / 2.0;
		const auto iconPoint = [&](double x, double y) {
			return QPointF(iconInset + x * iconScale, iconInset + y * iconScale);
		};
		const auto barPoint = [&](double y) {
			return QPointF(198, 98 + (y - 98) * iconScale);
		};
		double release = s.design == 0 ? ease((phase - .48) / .35) : ease(phase);
		p.save();
		p.setClipRect(QRectF(travel, 0, 198, 196), s.design >= TopBar ? Qt::IntersectClip : Qt::ReplaceClip);
		p.setOpacity(s.design == Liquid   ? unit((phase - .48) * 200)
			     : s.design >= TopBar ? ease(phase)
						  : reveal);
		// A stretching neck joins the intact official icon and travelling bar.
		if (s.design == 0 && phase > .73 && phase < .856) {
			double stretch = unit((phase - .73) / .125);
			QPainterPath neck;
			neck.moveTo(iconPoint(184 - 19 * stretch, 65));
			neck.cubicTo(iconPoint(194 - 15 * stretch, 78 + 19 * stretch),
				     QPointF(198 - (5 + 6 * stretch) * iconScale,
					     98 + (-12 + 12 * stretch) * iconScale),
				     barPoint(72 + 25 * stretch));
			neck.lineTo(barPoint(124 - 25 * stretch));
			neck.cubicTo(QPointF(198 - (5 + 6 * stretch) * iconScale, 98 + (12 - 12 * stretch) * iconScale),
				     iconPoint(194 - 15 * stretch, 118 - 19 * stretch),
				     iconPoint(184 - 19 * stretch, 131));
			neck.closeSubpath();
			p.fillPath(neck, s.accent);
		}
		p.drawImage(QRectF((s.design >= TopBar ? 0.0 : travel - 4 + 181 * (1.0 - release)) + iconInset,
				   iconInset, iconSize, iconSize),
			    icon);
		p.restore();
	}
	p.end();
	return image.convertToFormat(QImage::Format_RGBA8888);
}
} // namespace jcp
