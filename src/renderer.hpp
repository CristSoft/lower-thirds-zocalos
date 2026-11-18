#pragma once
#include "model.hpp"
#include <QPainter>

namespace jcp {
// Registers the bundled font privately in OBS; independent of Windows font setup.
bool loadBundledFonts();
class Renderer {
public:
	QImage render(const Caption &caption, const Style &style, double phase);
	static int width(const Style &style);

private:
	QString cachedPath, cachedColor;
	QImage icon;
	void loadIcon(const Style &style);
	QImage renderRibbon(const Caption &caption, const Style &style, double phase);
	QImage renderWebStyle(const Caption &caption, const Style &style, double phase);
	QImage renderTrail(const Caption &caption, const Style &style, double phase);
};
} // namespace jcp
