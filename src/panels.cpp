#include "panels.hpp"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QImageReader>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QPainterPath>
#include <QAction>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSpinBox>
#include <QSlider>
#include <QSignalBlocker>
#include <QSettings>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QToolButton>
#include <QUuid>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QScrollBar>
#include <QApplication>
#include <algorithm>
#include <cmath>

namespace jcp {
FrameMailbox frames;
enum class ActionIcon {
	Add,
	Play,
	Loop,
	Stop,
	Import,
	Export,
	Settings,
	Preview,
	Duplicate,
	Delete,
	Image,
	Reset,
	Clear
};
class ActionButton : public QToolButton {
public:
	void setAction(ActionIcon icon, bool critical = false, bool launch = false)
	{
		kind = icon;
		urgent = critical;
		amber = launch;
		setStyleSheet(
			critical
				? "QToolButton {background:#c62828;color:white;border:1px solid #ef5350;border-radius:4px;} QToolButton:hover {background:#e53935;} QToolButton:focus {border:2px solid white;}"
			: launch
				? "QToolButton {background:#f7a813;color:#17120b;border:1px solid #f7a813;border-radius:4px;} QToolButton:hover {background:#ffc04c;} QToolButton:focus {border:2px solid palette(text);}"
				: QString());
		refreshIcon();
	}
	explicit ActionButton(ActionIcon kind) : kind(kind)
	{
		setAutoRaise(true);
		setToolButtonStyle(Qt::ToolButtonIconOnly);
		setFixedSize(28, 28);
		setIconSize({18, 18});
		refreshIcon();
		connect(this, &QToolButton::toggled, this, [this] { refreshIcon(); });
	}

protected:
	void changeEvent(QEvent *event) override
	{
		QToolButton::changeEvent(event);
		if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange)
			refreshIcon();
	}

private:
	ActionIcon kind;
	bool urgent = false, amber = false;
	void refreshIcon()
	{
		QPixmap pixmap(36, 36);
		pixmap.fill(Qt::transparent);
		QPainter p(&pixmap);
		p.setRenderHint(QPainter::Antialiasing);
		p.scale(1.5, 1.5);
		p.setPen(QPen(urgent  ? QColor(Qt::white)
			      : amber ? QColor("#17120b")
				      : palette().color(isChecked() ? QPalette::HighlightedText : QPalette::ButtonText),
			      1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		const auto line = [&](double x, double y, double a, double b) {
			p.drawLine(QPointF(x, y), QPointF(a, b));
		};
		switch (kind) {
		case ActionIcon::Clear:
			line(6, 6, 18, 18);
			line(6, 18, 18, 6);
			break;
		case ActionIcon::Add:
			line(5, 12, 19, 12);
			line(12, 5, 12, 19);
			break;
		case ActionIcon::Stop:
			p.fillRect(QRectF(6, 6, 12, 12), p.pen().color());
			break;
		case ActionIcon::Play: {
			QPainterPath triangle;
			triangle.moveTo(7, 4);
			triangle.lineTo(20, 12);
			triangle.lineTo(7, 20);
			triangle.closeSubpath();
			p.fillPath(triangle, p.pen().color());
			break;
		}
		case ActionIcon::Loop: {
			QPainterPath arrows;
			arrows.moveTo(5, 10);
			arrows.lineTo(5, 8);
			arrows.quadTo(5, 5, 8, 5);
			arrows.lineTo(19, 5);
			arrows.moveTo(16, 2);
			arrows.lineTo(19, 5);
			arrows.lineTo(16, 8);
			arrows.moveTo(19, 14);
			arrows.lineTo(19, 16);
			arrows.quadTo(19, 19, 16, 19);
			arrows.lineTo(5, 19);
			arrows.moveTo(8, 16);
			arrows.lineTo(5, 19);
			arrows.lineTo(8, 22);
			p.drawPath(arrows);
			break;
		}
		case ActionIcon::Import:
		case ActionIcon::Export:
			line(5, 15, 5, 20);
			line(5, 20, 19, 20);
			line(19, 20, 19, 15);
			line(12, 4, 12, 16);
			if (kind == ActionIcon::Import) {
				line(8, 12, 12, 16);
				line(12, 16, 16, 12);
			} else {
				line(8, 8, 12, 4);
				line(12, 4, 16, 8);
			}
			break;
		case ActionIcon::Settings: {
			QPainterPath gear;
			for (int tooth = 0; tooth < 8; ++tooth) {
				for (int corner = 0; corner < 4; ++corner) {
					const double angle =
						(tooth * 45.0 + corner * 11.25) * 3.141592653589793 / 180.0;
					const double radius = (corner == 1 || corner == 2) ? 9.0 : 6.9;
					const QPointF point(12 + std::cos(angle) * radius,
							    12 + std::sin(angle) * radius);
					if (tooth == 0 && corner == 0)
						gear.moveTo(point);
					else
						gear.lineTo(point);
				}
			}
			gear.closeSubpath();
			p.drawPath(gear);
			p.drawEllipse(QPointF(12, 12), 3, 3);
			break;
		}
		case ActionIcon::Preview: {
			QPainterPath eye;
			eye.moveTo(2, 12);
			eye.quadTo(12, 1, 22, 12);
			eye.quadTo(12, 23, 2, 12);
			p.drawPath(eye);
			p.drawEllipse(QPointF(12, 12), 3, 3);
			break;
		}
		case ActionIcon::Duplicate:
			p.drawRoundedRect(QRectF(8, 8, 12, 12), 1, 1);
			line(4, 16, 4, 4);
			line(4, 4, 16, 4);
			break;
		case ActionIcon::Delete:
			line(4, 6, 20, 6);
			line(9, 3, 15, 3);
			line(6, 6, 7, 21);
			line(7, 21, 17, 21);
			line(17, 21, 18, 6);
			line(10, 10, 10, 17);
			line(14, 10, 14, 17);
			break;
		case ActionIcon::Image:
			p.drawRoundedRect(QRectF(3, 4, 18, 16), 1, 1);
			p.drawEllipse(QPointF(8, 9), 1.5, 1.5);
			line(4, 18, 10, 12);
			line(10, 12, 14, 16);
			line(14, 16, 17, 13);
			line(17, 13, 21, 17);
			break;
		case ActionIcon::Reset:
			p.drawArc(QRectF(5, 5, 14, 14), 30 * 16, 290 * 16);
			line(20, 4, 20, 10);
			line(20, 10, 14, 10);
			break;
		}
		p.end();
		setIcon(QIcon(pixmap));
	}
};
class StatusLabel : public QLabel {
public:
	StatusLabel()
	{
		setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
		setMinimumWidth(0);
	}

protected:
	void paintEvent(QPaintEvent *) override
	{
		QPainter painter(this);
		painter.setPen(palette().color(QPalette::WindowText));
		painter.setFont(font());
		painter.drawText(contentsRect(), Qt::AlignLeft | Qt::AlignVCenter,
				 fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width()));
	}
};
// Keep long names from imposing a minimum width on OBS's dock layout.
class CaptionHeader : public QToolButton {
public:
	CaptionHeader()
	{
		setCheckable(true);
		setAutoRaise(true);
		setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		setArrowType(Qt::RightArrow);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setMinimumWidth(0);
		setMinimumHeight(28);
		setCursor(Qt::OpenHandCursor);
	}
	void setCaptionName(const QString &name)
	{
		fullName = name.trimmed().isEmpty() ? QString("Sin nombre") : name;
		setAccessibleName("Editar zócalo: " + fullName);
		setToolTip(fullName + "\nArrastrá el nombre para reordenar; clic para editar.");
		fit();
	}
	QSize sizeHint() const override { return {140, 28}; }
	QSize minimumSizeHint() const override { return {40, 28}; }

protected:
	void mousePressEvent(QMouseEvent *event) override
	{
		origin = event->position().toPoint();
		QToolButton::mousePressEvent(event);
	}
	void mouseMoveEvent(QMouseEvent *event) override
	{
		if (!(event->buttons() & Qt::LeftButton) || !isDown() ||
		    (event->position().toPoint() - origin).manhattanLength() < QApplication::startDragDistance()) {
			QToolButton::mouseMoveEvent(event);
			return;
		}
		setDown(false);
		QPointer<CaptionHeader> guard(this);
		QPointer<QDrag> drag = new QDrag(this);
		auto *mime = new QMimeData;
		mime->setData("application/x-jcp-sur-caption", property("dragPayload").toByteArray());
		drag->setMimeData(mime);
		drag->setPixmap(grab());
		drag->setHotSpot(QPoint(20, height() / 2));
		drag->exec(Qt::MoveAction);
		if (drag)
			drag->deleteLater();
		if (guard)
			setDown(false);
	}
	void resizeEvent(QResizeEvent *event) override
	{
		QToolButton::resizeEvent(event);
		fit();
	}

private:
	QString fullName;
	QPoint origin;
	void fit() { setText(fontMetrics().elidedText(fullName, Qt::ElideRight, std::max(20, width() - 40))); }
};
class CaptionList : public QScrollArea {
public:
	std::function<bool(const QString &, const QString &, bool)> move;
	CaptionList(QWidget *host, QString token) : host(host), token(std::move(token))
	{
		setWidgetResizable(true);
		setFrameShape(QFrame::NoFrame);
		setWidget(host);
		setAcceptDrops(true);
		viewport()->setAcceptDrops(true);
		indicator = new QFrame(viewport());
		indicator->setObjectName("caption-drop-indicator");
		indicator->setAttribute(Qt::WA_TransparentForMouseEvents);
		indicator->setStyleSheet("background:palette(highlight);");
		indicator->hide();
		autoScroll.setInterval(35);
		connect(&autoScroll, &QTimer::timeout, this, [this] {
			const int step = point.y() < 24 ? -12 : point.y() > viewport()->height() - 24 ? 12 : 0;
			verticalScrollBar()->setValue(verticalScrollBar()->value() + step);
			mark();
		});
	}

protected:
	void dragEnterEvent(QDragEnterEvent *event) override
	{
		if (!valid(event->mimeData())) {
			event->ignore();
			return;
		}
		point = event->position().toPoint();
		autoScroll.start();
		mark();
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	void dragMoveEvent(QDragMoveEvent *event) override
	{
		if (!valid(event->mimeData())) {
			event->ignore();
			return;
		}
		point = event->position().toPoint();
		mark();
		event->setDropAction(Qt::MoveAction);
		event->accept();
	}
	void dragLeaveEvent(QDragLeaveEvent *event) override
	{
		reset();
		event->accept();
	}
	void dropEvent(QDropEvent *event) override
	{
		point = event->position().toPoint();
		const auto target = destination();
		const auto payload = event->mimeData()->data("application/x-jcp-sur-caption");
		const auto id = QString::fromUtf8(payload.mid(token.toUtf8().size() + 1));
		const bool accepted = valid(event->mimeData()) && !target.id.isEmpty() && move &&
				      move(id, target.id, target.after);
		reset();
		if (accepted) {
			event->setDropAction(Qt::MoveAction);
			event->accept();
		} else
			event->ignore();
	}

private:
	QWidget *host;
	QString token;
	QFrame *indicator;
	QTimer autoScroll;
	QPoint point;
	struct Target {
		QString id;
		bool after = false;
		int y = 0;
	};
	bool valid(const QMimeData *mime) const
	{
		return mime->data("application/x-jcp-sur-caption").startsWith(token.toUtf8() + '\n');
	}
	Target destination() const
	{
		Target target;
		for (int i = 0; i < host->layout()->count(); ++i) {
			auto *card = host->layout()->itemAt(i)->widget();
			if (!card || card->isHidden() || !card->property("captionId").isValid())
				continue;
			const int top = card->mapTo(viewport(), QPoint(0, 0)).y();
			const auto id = card->property("captionId").toString();
			if (point.y() < top + card->height() / 2)
				return {id, false, top};
			target = {id, true, top + card->height()};
		}
		return target;
	}
	void mark()
	{
		const auto target = destination();
		indicator->setVisible(!target.id.isEmpty());
		indicator->setGeometry(2, std::clamp(target.y, 0, std::max(0, viewport()->height() - 2)),
				       std::max(0, viewport()->width() - 4), 2);
		indicator->raise();
	}
	void reset()
	{
		autoScroll.stop();
		indicator->hide();
	}
};
static bool matchesCaption(const Caption &caption, const QString &filter)
{
	return filter.isEmpty() || (caption.name + ' ' + caption.lines.join(' ')).contains(filter, Qt::CaseInsensitive);
}
static QDockWidget *parentDock(QWidget *content)
{
	for (auto *parent = content; parent; parent = parent->parentWidget())
		if (auto *dock = qobject_cast<QDockWidget *>(parent))
			return dock;
	return nullptr;
}
void Controller::initializeDocking()
{
	auto *controls = parentDock(rowsHost);
	if (!controls || configPath.isEmpty())
		return;
	controls->setAllowedAreas(Qt::AllDockWidgetAreas);
	QSettings preferences(QFileInfo(configPath).absolutePath() + "/panel-layout.ini", QSettings::IniFormat);
	if (preferences.value("Docking/initialized", false).toBool())
		return;
	// OBS restores the old floating state during startup. Migrate once after
	// FINISHED_LOADING, then leave subsequent operator layout choices intact.
	controls->setFloating(false);
	preferences.setValue("Docking/initialized", true);
}
void Controller::dockControlPanel(Qt::DockWidgetArea area)
{
	auto *dock = parentDock(rowsHost);
	if (!dock)
		return;
	for (auto *parent = dock->parentWidget(); parent; parent = parent->parentWidget())
		if (auto *main = qobject_cast<QMainWindow *>(parent)) {
			dock->setAllowedAreas(Qt::AllDockWidgetAreas);
			main->addDockWidget(area, dock);
			dock->setFloating(false);
			dock->show();
			dock->raise();
			return;
		}
}
Preview::Preview()
{
	setAlignment(Qt::AlignCenter);
	setMinimumSize(0, 96);
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}
void Preview::setFrame(const QImage &image)
{
	frame = QPixmap::fromImage(image);
	fit();
}
void Preview::fit()
{
	if (!frame.isNull())
		setPixmap(frame.scaled(contentsRect().size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
void Preview::resizeEvent(QResizeEvent *event)
{
	QLabel::resizeEvent(event);
	fit();
}
static QLabel *note(const QString &text)
{
	auto *l = new QLabel(text);
	l->setWordWrap(true);
	return l;
}
static QToolButton *iconButton(ActionIcon icon, const QString &text, QBoxLayout *layout,
			       const std::function<void()> &action)
{
	auto *b = new ActionButton(icon);
	b->setText(text);
	b->setToolTip(text);
	b->setAccessibleName(text);
	layout->addWidget(b);
	QObject::connect(b, &QToolButton::clicked, b, action);
	return b;
}
static QScrollArea *scroll(QWidget *content)
{
	auto *s = new QScrollArea;
	s->setWidgetResizable(true);
	s->setFrameShape(QFrame::NoFrame);
	s->setWidget(content);
	return s;
}
Controller::Controller(QObject *parent, QString path) : QObject(parent), configPath(std::move(path))
{
	dragToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
	loadBundledFonts();
	if (QFile::exists(configPath) && !loadLibrary(configPath, library, startupError)) {
		// Preserve a damaged library before writing a replacement.
		const auto backup =
			configPath + ".recovery-" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmsszzz");
		if (!QFile::copy(configPath, backup)) {
			startupError += " No se pudo respaldar el archivo: el guardado automático está deshabilitado.";
			configPath.clear();
		} else
			startupError += " Se conservó una copia en " + backup;
	}
	if (library.captions.isEmpty() && !QFile::exists(configPath)) {
		auto c = Caption::create();
		c.name = "Presentación";
		c.lines = {"Nombre y apellido", "Cargo o descripción", ""};
		library.captions.append(c);
	}
	saver.setSingleShot(true);
	saver.setInterval(450);
	connect(&saver, &QTimer::timeout, this, [this] { save(); });
	ticker.setInterval(33);
	elapsed.start();
	connect(&ticker, &QTimer::timeout, this, [this] {
		const double seconds = elapsed.restart() / 1000.0;
		const bool wasVisible = playout.visible();
		const auto oldGeneration = playout.generation;
		const double oldPhase = playout.phase;
		playout.tick(seconds);
		if (wasVisible || playout.visible()) {
			if (oldPhase != playout.phase || oldGeneration != playout.generation ||
			    wasVisible != playout.visible()) {
				auto img = renderer.render(playout.active, playout.style, playout.phase);
				if (fitSource && fittedWidth != img.width()) {
					fitSource(img.width());
					fittedWidth = img.width();
				}
				std::lock_guard<std::mutex> lock(frames.mutex);
				frames.image = std::move(img);
				++frames.revision;
			}
		}
		if (previewPlayback.playing && preview && preview->isVisible()) {
			previewPlayback.tick(seconds);
			refreshPreview();
		}
		if (tickScenePreviews)
			tickScenePreviews(seconds);
		updateStatus();
	});
	ticker.start();
}
Controller::~Controller()
{
	delete designWindow.data();
}
QDialog *Controller::makeDesignWindow(QWidget *parent)
{
	if (designWindow)
		return designWindow;
	auto *dialog = new QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
	designWindow = dialog;
	dialog->setObjectName("design-window");
	dialog->setWindowTitle("Diseño de zócalos");
	dialog->setModal(false);
	dialog->resize(520, 650);
	dialog->setMinimumSize(360, 340);
	auto *layout = new QVBoxLayout(dialog);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(makeDesignPanel());
	connect(dialog, &QDialog::finished, this, [this] { stopPreview(); });
	return dialog;
}
void Controller::showDesign()
{
	auto *dialog = makeDesignWindow(qobject_cast<QWidget *>(parent()));
	refreshPreview();
	dialog->show();
	dialog->raise();
	dialog->activateWindow();
}
bool Controller::save()
{
	if (configPath.isEmpty())
		return false;
	if (saveLibrary(configPath, library, saveError)) {
		saveError.clear();
		return true;
	}
	updateStatus();
	return false;
}
void Controller::changed()
{
	saver.start();
	if (!previewPlayback.playing)
		previewPlayback.stop();
	refreshPreview();
}
Caption Controller::previewCaption() const
{
	auto it = std::find_if(library.captions.cbegin(), library.captions.cend(),
			       [this](const Caption &c) { return c.id == selectedId; });
	if (it == library.captions.cend() && !library.captions.isEmpty())
		it = library.captions.cbegin();
	const QStringList example = {"Nombre y apellido", "Cargo o descripción", "Información adicional"};
	Caption caption = it == library.captions.cend() ? Caption{"design-preview", "Ejemplo", example} : *it;
	while (caption.lines.size() < 3)
		caption.lines.append("");
	for (int i = 0; i < 3; ++i) {
		if (caption.lines[i].trimmed().isEmpty())
			caption.lines[i] = example[i];
		if (!previewLines[static_cast<std::size_t>(i)])
			caption.lines[i].clear();
	}
	return caption;
}
void Controller::startPreview(bool repeat)
{
	previewPlayback.start(library.style, repeat);
	if (previewLoopButton)
		previewLoopButton->setChecked(repeat);
	refreshPreview();
}
void Controller::stopPreview()
{
	previewPlayback.stop();
	if (previewLoopButton)
		previewLoopButton->setChecked(false);
	refreshPreview();
}
void Controller::refreshPreview()
{
	if (!preview)
		return;
	auto style = library.style;
	const double scale = sourceScale ? std::max(.01, sourceScale()) : std::min(canvasWidth * .90 / 900.0, 1.6);
	style.maxWidth = std::max(64, int(std::floor(std::min(style.maxWidth, canvasWidth) / scale + .0001)));
	auto image = previewRenderer.render(previewCaption(), style, previewPlayback.phase);
	QImage bg(image.size(), QImage::Format_RGB32);
	bg.fill(QColor("#3c454c"));
	QPainter p(&bg);
	p.drawImage(0, 0, image);
	p.end();
	preview->setFrame(bg);
}
void Controller::updateStatus()
{
	if (globalStop) {
		const bool onAir = (playout.visible() && (!liveSourceOnAir || liveSourceOnAir())) ||
				   (scenePlatesOnAir && scenePlatesOnAir());
		if (!globalStop->property("onAir").isValid() || globalStop->property("onAir").toBool() != onAir) {
			globalStop->setProperty("onAir", onAir);
			static_cast<ActionButton *>(globalStop.data())->setAction(ActionIcon::Stop, onAir);
		}
	}
	for (auto it = launchButtons.begin(); it != launchButtons.end(); ++it) {
		if (!it.value())
			continue;
		auto *button = static_cast<ActionButton *>(it.value().data());
		const bool active = playout.controls(it.key());
		if (button->property("onAir").isValid() && button->property("onAir").toBool() == active)
			continue;
		button->setProperty("onAir", active);
		button->setAction(active ? ActionIcon::Stop : ActionIcon::Play, active, !active);
		button->setText(active ? "Detener zócalo" : "Lanzar al vivo");
		const auto label = button->text() + ": " + button->property("captionName").toString();
		button->setAccessibleName(label);
		button->setToolTip(label);
	}
	if (!status)
		return;
	const auto text = playout.visible() ? (playout.exiting() ? QString("Saliendo: ") : QString("En escena: ")) +
						      playout.active.name
					    : QString("Zócalo oculto");
	status->setText(saveError.isEmpty() ? text : text + "\n" + saveError);
	status->setToolTip(status->text());
}
void Controller::stopLive()
{
	playout.hide();
	if (hideScenePreviews)
		hideScenePreviews();
	updateStatus();
}
void Controller::resetLive()
{
	playout.stop();
	{
		std::lock_guard<std::mutex> lock(frames.mutex);
		if (frames.image.isNull())
			frames.image = QImage(900, 196, QImage::Format_RGBA8888);
		frames.image.fill(Qt::transparent);
		++frames.revision;
	}
	if (clearScenePreviews)
		clearScenePreviews();
	updateStatus();
}
void Controller::setCanvasWidth(int width)
{
	canvasWidth = std::clamp(width, 64, 16384);
	library.style.maxWidth = std::clamp(library.style.maxWidth, std::min(640, canvasWidth), canvasWidth);
	if (widthSlider)
		widthSlider->setRange(std::min(640, canvasWidth), canvasWidth);
	if (widthInput) {
		widthInput->setRange(std::min(640, canvasWidth), canvasWidth);
		widthInput->setValue(library.style.maxWidth);
	}
	refreshPreview();
}
void Controller::launch(const QString &id)
{
	if (playout.controls(id)) {
		stopLive();
		return;
	}
	const auto it = std::find_if(library.captions.cbegin(), library.captions.cend(),
				     [&](const Caption &c) { return c.id == id; });
	if (it == library.captions.cend())
		return;
	if (std::none_of(it->lines.cbegin(), it->lines.cend(),
			 [](const QString &s) { return !s.trimmed().isEmpty(); })) {
		QMessageBox::information(nullptr, "Zócalo vacío", "Completá al menos una línea antes de lanzarlo.");
		return;
	}
	QString error;
	if (!ensureSource || !ensureSource(error)) {
		QMessageBox::warning(nullptr, "No se pudo lanzar", error);
		return;
	}
	selectedId = id;
	auto style = library.style;
	const double scale = sourceScale ? std::max(.01, sourceScale()) : 1.0;
	style.maxWidth = std::max(64, int(std::floor(std::min(style.maxWidth, canvasWidth) / scale + .0001)));
	fittedWidth = 0;
	playout.launch(*it, style);
	refreshPreview();
	updateStatus();
}
QWidget *Controller::makeControlPanel()
{
	auto *root = new QWidget;
	root->setMinimumWidth(180);
	auto *v = new QVBoxLayout(root);
	v->setContentsMargins(4, 4, 4, 4);
	v->setSpacing(4);
	auto *tools = new QHBoxLayout;
	tools->setSpacing(2);
	v->addLayout(tools);
	status = new StatusLabel;
	status->setText("Zócalo oculto");
	auto statusFont = status->font();
	statusFont.setBold(true);
	status->setFont(statusFont);
	tools->addWidget(status, 1);
	globalStop = iconButton(ActionIcon::Stop, "Ocultar zócalo", tools, [this] { stopLive(); });
	globalStop->setObjectName("stop-all");
	globalStop->setToolTip("Retirar los zócalos con la animación de salida.");
	globalStop->setAccessibleName("Ocultar los zócalos con animación de salida");
	iconButton(ActionIcon::Add, "Agregar zócalo", tools, [this] {
		auto c = Caption::create();
		selectedId = c.id;
		expandedId = c.id;
		library.captions.append(c);
		changed();
		rebuildRows();
	});
	search = new QLineEdit;
	search->setPlaceholderText("Buscar zócalos…");
	search->setAccessibleName("Buscar zócalos");
	auto *searchRow = new QHBoxLayout;
	searchRow->setSpacing(2);
	searchRow->addWidget(search, 1);
	auto *clearSearch = iconButton(ActionIcon::Clear, "Borrar búsqueda", searchRow, [this] {
		search->clear();
		search->setFocus();
	});
	clearSearch->setObjectName("clear-search");
	clearSearch->setEnabled(!search->text().isEmpty());
	connect(search, &QLineEdit::textChanged, clearSearch,
		[clearSearch](const QString &value) { clearSearch->setEnabled(!value.isEmpty()); });
	v->addLayout(searchRow);
	connect(search, &QLineEdit::textChanged, this, [this] { rebuildRows(); });
	rowsHost = new QWidget;
	auto *rows = new QVBoxLayout(rowsHost);
	rows->setContentsMargins(0, 0, 0, 0);
	rows->setSpacing(3);
	auto *list = new CaptionList(rowsHost, dragToken);
	list->move = [this](const QString &id, const QString &target, bool after) {
		return moveCaption(id, target, after);
	};
	list->setObjectName("caption-list");
	list->setMinimumSize(0, 40);
	list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	v->addWidget(list, 1);
	iconButton(ActionIcon::Export, "Exportar biblioteca", tools, [this] {
		const auto path = QFileDialog::getSaveFileName(nullptr, "Exportar biblioteca", "zocalos.json",
							       "Biblioteca (*.json)");
		if (path.isEmpty())
			return;
		QString error;
		if (!saveLibrary(path, library, error))
			QMessageBox::warning(nullptr, "Exportación", error);
	});
	iconButton(ActionIcon::Import, "Importar biblioteca", tools, [this] {
		const auto path =
			QFileDialog::getOpenFileName(nullptr, "Importar biblioteca", {}, "Biblioteca (*.json)");
		if (path.isEmpty())
			return;
		Library imported;
		QString error;
		if (!loadLibrary(path, imported, error)) {
			QMessageBox::warning(nullptr, "Importación", error);
			return;
		}
		// Append texts; never silently replace existing entries or live styling.
		for (auto c : imported.captions) {
			c.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
			library.captions.append(c);
		}
		changed();
		rebuildRows();
	});
	auto *settingsButton =
		iconButton(ActionIcon::Settings, "Diseño y configuración", tools, [this] { showDesign(); });
	settingsButton->setObjectName("open-design");
	// Keep placement actions available without occupying the settings button.
	const auto dockAction = [&](const QString &label, const char *id, Qt::DockWidgetArea area) {
		auto *action = new QAction(label, root);
		root->addAction(action);
		action->setObjectName(id);
		connect(action, &QAction::triggered, this, [this, area] { dockControlPanel(area); });
	};
	dockAction("Acoplar abajo", "dock-bottom", Qt::BottomDockWidgetArea);
	dockAction("Acoplar arriba", "dock-top", Qt::TopDockWidgetArea);
	dockAction("Acoplar a la izquierda", "dock-left", Qt::LeftDockWidgetArea);
	dockAction("Acoplar a la derecha", "dock-right", Qt::RightDockWidgetArea);
	root->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(root, &QWidget::customContextMenuRequested, root, [root](const QPoint &position) {
		QMenu menu;
		menu.addActions(root->actions());
		menu.exec(root->mapToGlobal(position));
	});
	rebuildRows();
	return root;
}
bool Controller::moveCaption(const QString &id, const QString &target, bool after)
{
	const auto filter = search ? search->text() : QString();
	QVector<Caption> visible;
	QVector<int> positions;
	int from = -1, to = -1;
	for (int i = 0; i < library.captions.size(); ++i) {
		const auto &caption = library.captions[i];
		if (!matchesCaption(caption, filter))
			continue;
		if (caption.id == id)
			from = int(visible.size());
		if (caption.id == target)
			to = int(visible.size());
		positions.append(i);
		visible.append(caption);
	}
	if (from < 0 || to < 0 || from == to)
		return false;
	int destination = to + (after ? 1 : 0);
	if (from < destination)
		--destination;
	if (from == destination)
		return false;
	visible.move(from, destination);
	for (int i = 0; i < positions.size(); ++i)
		library.captions[positions[i]] = visible[i];
	changed();
	// Let the native drag finish before disposing of its source header.
	QTimer::singleShot(0, this, [this] { rebuildRows(); });
	return true;
}
void Controller::rebuildRows()
{
	if (!rowsHost)
		return;
	launchButtons.clear();
	auto *layout = qobject_cast<QVBoxLayout *>(rowsHost->layout());
	while (auto *item = layout->takeAt(0)) {
		if (item->widget()) {
			item->widget()->hide();
			item->widget()->deleteLater();
		}
		delete item;
	}
	const auto filter = search ? search->text() : QString();
	int count = 0;
	for (const auto &c : library.captions) {
		if (!matchesCaption(c, filter))
			continue;
		++count;
		auto *box = new QFrame;
		box->setObjectName("caption-card");
		box->setProperty("captionId", c.id);
		box->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
		box->setStyleSheet("QFrame#caption-card { background:palette(base);border:0;border-radius:3px; }");
		auto *v = new QVBoxLayout(box);
		v->setContentsMargins(4, 4, 4, 4);
		v->setSpacing(2);
		v->setSizeConstraint(QLayout::SetMinAndMaxSize);
		v->setAlignment(Qt::AlignTop);
		const auto id = c.id;
		auto *header = new QHBoxLayout;
		header->setSpacing(4);
		v->addLayout(header);
		auto *toggle = new CaptionHeader;
		toggle->setObjectName("caption-toggle");
		toggle->setProperty("dragPayload", dragToken.toUtf8() + '\n' + id.toUtf8());
		toggle->setCaptionName(c.name);
		auto *launchButton = new ActionButton(ActionIcon::Play);
		launchButton->setObjectName("launch-caption");
		launchButton->setProperty("captionName", c.name);
		launchButtons.insert(id, launchButton);
		launchButton->setIconSize({20, 20});
		launchButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
		launchButton->setText("Lanzar al vivo");
		launchButton->setAccessibleName("Lanzar al vivo: " + c.name);
		launchButton->setToolTip("Lanzar al vivo: " + c.name);
		launchButton->setFixedSize(28, 28);
		header->addWidget(launchButton);
		header->addWidget(toggle, 1);
		connect(launchButton, &QToolButton::clicked, this, [this, id] { launch(id); });
		auto *editor = new QWidget;
		editor->setObjectName("caption-editor");
		auto *editorLayout = new QVBoxLayout(editor);
		editorLayout->setContentsMargins(0, 3, 0, 0);
		editorLayout->setSpacing(3);
		auto *form = new QFormLayout;
		form->setSpacing(3);
		form->setRowWrapPolicy(QFormLayout::WrapLongRows);
		editorLayout->addLayout(form);
		v->addWidget(editor);
		const bool expanded = expandedId == id;
		toggle->setChecked(expanded);
		toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
		editor->setVisible(expanded);
		connect(toggle, &QToolButton::toggled, this, [this, toggle, editor, id](bool open) {
			editor->setVisible(open);
			toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
			if (open) {
				expandedId = id;
				selectedId = id;
				for (auto *other : rowsHost->findChildren<QToolButton *>("caption-toggle"))
					if (other != toggle && other->isVisibleTo(rowsHost))
						other->setChecked(false);
				refreshPreview();
			} else if (expandedId == id)
				expandedId.clear();
			auto *card = editor->parentWidget();
			card->layout()->invalidate();
			card->layout()->activate();
			card->updateGeometry();
			rowsHost->layout()->activate();
		});
		auto edit = [this, id](const std::function<void(Caption &)> &fn) {
			for (auto &entry : library.captions)
				if (entry.id == id) {
					fn(entry);
					selectedId = id;
					changed();
					break;
				}
		};
		auto *name = new QLineEdit(c.name);
		name->setObjectName("caption-name");
		name->setMaxLength(100);
		form->addRow("Nombre", name);
		connect(name, &QLineEdit::textChanged, this, [edit, toggle, launchButton](const QString &value) {
			edit([&](Caption &e) { e.name = value; });
			toggle->setCaptionName(value);
			launchButton->setProperty("captionName", value);
			launchButton->setAccessibleName(launchButton->text() + ": " + value);
			launchButton->setToolTip(launchButton->accessibleName());
		});
		for (int i = 0; i < 3; ++i) {
			auto *input = new QLineEdit(c.lines.value(i));
			input->setMaxLength(500);
			input->setPlaceholderText(i ? "Opcional" : "Nombre o título");
			form->addRow(QString("Línea %1").arg(i + 1), input);
			connect(input, &QLineEdit::textChanged, this,
				[edit, i](const QString &value) { edit([&](Caption &e) { e.lines[i] = value; }); });
		}
		auto *actions = new QHBoxLayout;
		actions->setSpacing(2);
		editorLayout->addLayout(actions);
		iconButton(ActionIcon::Preview, "Vista previa", actions, [this, id] {
			selectedId = id;
			refreshPreview();
			for (const auto &caption : library.captions)
				if (caption.id == id) {
					QString error;
					if (!showScenePreview || !showScenePreview(caption, library.style, error))
						QMessageBox::warning(nullptr, "Vista previa de OBS",
								     error.isEmpty() ? "No hay una escena disponible."
										     : error);
					break;
				}
		})->setObjectName("scene-preview");
		iconButton(ActionIcon::Duplicate, "Duplicar", actions, [this, id] {
			for (const auto &entry : library.captions)
				if (entry.id == id) {
					auto copy = entry;
					copy.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
					copy.name += " (copia)";
					expandedId = copy.id;
					selectedId = copy.id;
					library.captions.append(copy);
					break;
				}
			changed();
			rebuildRows();
		});
		iconButton(ActionIcon::Delete, "Eliminar", actions, [this, id] {
			if (QMessageBox::question(
				    nullptr, "Eliminar zócalo",
				    "¿Eliminar este zócalo de la biblioteca? Si está en escena, seguirá visible hasta que lo ocultes.",
				    QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				return;
			library.captions.erase(std::remove_if(library.captions.begin(), library.captions.end(),
							      [&](const Caption &e) { return e.id == id; }),
					       library.captions.end());
			if (expandedId == id)
				expandedId.clear();
			changed();
			rebuildRows();
		});
		actions->addStretch();
		layout->addWidget(box);
	}
	if (!count)
		layout->addWidget(note(library.captions.isEmpty()
					       ? "Todavía no hay zócalos. Usá Agregar zócalo para crear el primero."
					       : "No hay zócalos que coincidan con la búsqueda."));
	layout->addStretch();
	updateStatus();
}
QWidget *Controller::makeDesignPanel()
{
	auto *root = new QWidget;
	root->setObjectName("design-panel");
	auto *outer = new QVBoxLayout(root);
	outer->setContentsMargins(6, 6, 6, 6);
	outer->setSpacing(4);
	outer->addWidget(note("Vista previa"));
	preview = new Preview;
	preview->setObjectName("design-preview");
	preview->setFixedHeight(112);
	outer->addWidget(preview);
	auto *previewControls = new QWidget;
	previewControls->setObjectName("preview-controls");
	auto *transport = new QHBoxLayout(previewControls);
	transport->setContentsMargins(0, 0, 0, 0);
	transport->setSpacing(3);
	transport->addWidget(new QLabel("Líneas"));
	for (int i = 0; i < 3; ++i) {
		auto *line = new QToolButton;
		line->setObjectName(QString("preview-line-%1").arg(i + 1));
		line->setText(QString::number(i + 1));
		line->setToolButtonStyle(Qt::ToolButtonTextOnly);
		line->setCheckable(true);
		line->setChecked(previewLines[static_cast<std::size_t>(i)]);
		line->setFixedSize(28, 28);
		line->setAccessibleName(QString("Mostrar línea %1 en la vista previa").arg(i + 1));
		line->setToolTip(
			QString("Mostrar u ocultar la línea %1 del ejemplo; no modifica el zócalo al aire.").arg(i + 1));
		line->setStyleSheet(
			"QToolButton:checked { background:palette(highlight);color:palette(highlighted-text);border:1px solid palette(highlight);border-radius:3px; }");
		transport->addWidget(line);
		connect(line, &QToolButton::toggled, this, [this, i](bool show) {
			previewLines[static_cast<std::size_t>(i)] = show;
			if (!previewPlayback.playing)
				previewPlayback.stop();
			refreshPreview();
		});
	}
	transport->addStretch();
	iconButton(ActionIcon::Play, "Reproducir vista previa una vez", transport, [this] {
		startPreview(false);
	})->setObjectName("preview-play");
	previewControls->findChild<QToolButton *>("preview-play")
		->setToolTip("Reproducir entrada, permanencia y salida. Con permanencia 0, el ejemplo usa 2 segundos.");
	auto *stop = iconButton(ActionIcon::Stop, "Detener vista previa", transport, [this] { stopPreview(); });
	stop->setObjectName("preview-stop");
	stop->setToolTip("Detener y mostrar el zócalo completo.");
	previewLoopButton = iconButton(ActionIcon::Loop, "Reproducir vista previa en loop", transport, [] {});
	previewLoopButton->setObjectName("preview-loop");
	previewLoopButton->setCheckable(true);
	previewLoopButton->setStyleSheet(
		"QToolButton:checked {background:palette(highlight);border:1px solid palette(highlight);border-radius:3px;}");
	connect(previewLoopButton, &QToolButton::clicked, this, [this](bool repeat) {
		if (repeat)
			startPreview(true);
		else
			stopPreview();
	});
	outer->addWidget(previewControls);
	auto *content = new QWidget;
	auto *v = new QVBoxLayout(content);
	v->setContentsMargins(0, 4, 0, 0);
	v->setSpacing(4);
	auto *form = new QFormLayout;
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);
	v->addLayout(form);
	auto *design = new QComboBox;
	design->setObjectName("design-choice");
	design->addItems(designNames());
	design->setCurrentIndex(library.style.design);
	form->addRow("Diseño", design);
	connect(design, &QComboBox::currentIndexChanged, this, [this](int i) {
		library.style.design = i;
		changed();
	});
	auto color = [this, form](const QString &label, const QString &key, QColor Style::*member) {
		auto *row = new QWidget;
		auto *layout = new QHBoxLayout(row);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(4);
		auto *input = new QLineEdit((library.style.*member).name().toUpper());
		input->setObjectName("color-" + key);
		input->setAccessibleName(label + ", color hexadecimal");
		input->setPlaceholderText("#RRGGBB");
		input->setMaxLength(7);
		input->setValidator(new QRegularExpressionValidator(QRegularExpression("^#?[0-9a-fA-F]{0,6}$"), input));
		layout->addWidget(input, 1);
		auto *picker = new QToolButton;
		picker->setObjectName("color-" + key + "-picker");
		picker->setAccessibleName("Elegir color: " + label);
		picker->setFixedSize(28, 28);
		layout->addWidget(picker);
		form->addRow(label, row);
		const auto parse = [](QString text) {
			text = text.trimmed();
			if (!text.startsWith('#'))
				text.prepend('#');
			return (text.size() == 4 || text.size() == 7) ? QColor(text) : QColor();
		};
		const auto swatch = [this, picker, member] {
			const auto value = (library.style.*member).name();
			picker->setProperty("color", value.toUpper());
			picker->setToolTip("Seleccionar color: " + value.toUpper());
			picker->setStyleSheet(
				"QToolButton { background-color:" + value +
				";border:1px solid palette(mid);border-radius:3px; } QToolButton:focus { border:2px solid palette(text); }");
		};
		swatch();
		connect(input, &QLineEdit::textChanged, this,
			[this, input, member, parse, swatch](const QString &text) {
				const auto value = parse(text);
				if (!value.isValid())
					return;
				library.style.*member = value;
				input->setProperty("invalidColor", false);
				input->setStyleSheet("");
				input->setToolTip("Color hexadecimal, por ejemplo #FFFFFF");
				swatch();
				changed();
			});
		connect(input, &QLineEdit::editingFinished, this, [input, parse] {
			const auto value = parse(input->text());
			if (value.isValid())
				input->setText(value.name().toUpper());
			else {
				input->setProperty("invalidColor", true);
				input->setStyleSheet("QLineEdit { border:1px solid #d96666; }");
				input->setToolTip(
					"Completá un color hexadecimal como #FFFFFF. Se conserva el último color válido.");
			}
		});
		connect(picker, &QToolButton::clicked, this, [this, input, picker, member, label] {
			const auto value = QColorDialog::getColor(library.style.*member, picker->window(),
								  "Elegir color: " + label);
			if (value.isValid())
				input->setText(value.name().toUpper());
		});
	};
	color("Fondo", "plate", &Style::plate);
	color("Acento e ícono", "accent", &Style::accent);
	color("Primera línea", "title", &Style::title);
	color("Líneas 2 y 3", "detail", &Style::detail);
	auto *font = new QFontComboBox;
	font->setCurrentFont(QFont(library.style.font));
	form->addRow("Tipografía", font);
	connect(font, &QFontComboBox::currentFontChanged, this, [this](const QFont &f) {
		library.style.font = f.family();
		changed();
	});
	auto *logoChoice = new QComboBox;
	logoChoice->setObjectName("logo-choice");
	logoChoice->addItem("Predeterminado del estilo", DefaultLogo);
	logoChoice->addItem("Ninguno", -1);
	logoChoice->addItem("Isotipo de la iglesia", ChurchLogo);
	logoChoice->addItem("Símbolo circular", CircularLogo);
	logoChoice->addItem("Música · Corchea doble", MusicLogo);
	logoChoice->addItem("Corona y Biblia", CrownLogo);
	logoChoice->addItem("Biblia", BibleLogo);
	if (!library.style.icon.isEmpty())
		logoChoice->addItem("Logo importado", ImportedLogo);
	logoChoice->setCurrentIndex(logoChoice->findData(library.style.showIcon ? library.style.logo : -1));
	form->addRow("Logo", logoChoice);
	connect(logoChoice, &QComboBox::currentIndexChanged, this, [this, logoChoice] {
		const int selection = logoChoice->currentData().toInt();
		library.style.showIcon = selection != -1;
		if (library.style.showIcon)
			library.style.logo = selection;
		changed();
	});
	auto *iconActions = new QHBoxLayout;
	iconActions->setSpacing(2);
	v->addLayout(iconActions);
	iconButton(ActionIcon::Image, "Importar logo…", iconActions, [this, logoChoice] {
		auto path = QFileDialog::getOpenFileName(nullptr, "Seleccionar ícono", {},
							 "Imagen (*.png *.jpg *.jpeg *.webp *.svg)");
		if (path.isEmpty())
			return;
		bool valid = false;
		if (path.endsWith(".svg", Qt::CaseInsensitive)) {
			QSvgRenderer svg(path);
			valid = svg.isValid();
		} else {
			QImageReader reader(path);
			const auto size = reader.size();
			valid = size.isValid() && size.width() <= 8192 && size.height() <= 8192 &&
				!reader.read().isNull();
		}
		if (!valid) {
			QMessageBox::warning(nullptr, "Ícono inválido",
					     "Elegí una imagen válida de hasta 8192 × 8192 píxeles o un SVG.");
			return;
		}
		QFile source(path);
		if (!source.open(QIODevice::ReadOnly))
			return;
		const auto bytes = source.readAll();
		const auto dir = QFileInfo(configPath).absolutePath() + "/icons";
		QDir().mkpath(dir);
		const auto dest = dir + "/" +
				  QString(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()) + "." +
				  QFileInfo(path).suffix().toLower();
		if (!QFile::exists(dest) && !QFile::copy(path, dest)) {
			QMessageBox::warning(nullptr, "Ícono", "No se pudo copiar el ícono a la biblioteca.");
			return;
		}
		library.style.icon = dest;
		library.style.logo = ImportedLogo;
		library.style.showIcon = true;
		{
			const QSignalBlocker blocker(logoChoice);
			int index = logoChoice->findData(ImportedLogo);
			if (index < 0) {
				index = logoChoice->count();
				logoChoice->addItem("Logo importado", ImportedLogo);
			}
			logoChoice->setItemText(index, QFileInfo(path).fileName());
			logoChoice->setCurrentIndex(index);
		}
		changed();
	});
	iconActions->addStretch();
	auto spin = [this, form](const QString &label, const QString &key, int Style::*member, int lo, int hi,
				 const QString &suffix, bool sliding) {
		auto *row = new QWidget;
		auto *layout = new QHBoxLayout(row);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(6);
		auto *s = new QSpinBox;
		s->setObjectName(key);
		s->setAccessibleName(label + ", valor");
		s->setRange(lo, hi);
		s->setSuffix(suffix);
		s->setValue(library.style.*member);
		if (sliding) {
			auto *slider = new QSlider(Qt::Horizontal);
			slider->setObjectName(key + "-slider");
			slider->setAccessibleName(label);
			slider->setRange(lo, hi);
			slider->setValue(s->value());
			slider->setMinimumWidth(45);
			layout->addWidget(slider, 1);
			s->setFixedWidth(98);
			connect(slider, &QSlider::valueChanged, s, &QSpinBox::setValue);
			connect(s, &QSpinBox::valueChanged, slider, &QSlider::setValue);
			if (member == &Style::maxWidth)
				widthSlider = slider;
		}
		layout->addWidget(s);
		form->addRow(label, row);
		connect(s, &QSpinBox::valueChanged, this, [this, member](int n) {
			library.style.*member = n;
			changed();
		});
		return s;
	};
	spin("Tamaño del emblema", "icon-size", &Style::iconSize, 32, 196, " px", true);
	spin("Redondeo", "radius", &Style::radius, 0, 98, " px", true);
	spin("Entrada / salida", "transition-ms", &Style::transitionMs, 150, 5000, " ms", true);
	widthInput =
		spin("Ancho fijo", "max-width", &Style::maxWidth, std::min(640, canvasWidth), canvasWidth, " px", true);
	spin("Ocultar después de", "hold-seconds", &Style::holdSeconds, 0, 3600, " s", false);
	v->addWidget(note(
		"El zócalo conserva este ancho aunque el texto sea corto. Si el texto no entra, se reduce su tamaño. El límite es el ancho del lienzo de OBS."));
	v->addWidget(note(
		"0 segundos: permanece visible hasta pulsar Ocultar. Las líneas vacías no ocupan espacio y el texto se ajusta al ancho."));
	v->addWidget(note(
		"La fuente se agrega a la escena de programa al lanzar. Podés moverla y cambiar su tamaño desde el lienzo de OBS."));
	v->addStretch();
	auto *settingsScroll = scroll(content);
	settingsScroll->setObjectName("design-settings-scroll");
	settingsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	outer->addWidget(settingsScroll, 1);
	refreshPreview();
	return root;
}
} // namespace jcp
