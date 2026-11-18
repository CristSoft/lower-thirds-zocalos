#pragma once
#include "renderer.hpp"
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPixmap>
#include <QTimer>
#include <QWidget>
#include <QDialog>
#include <QToolButton>
#include <QHash>
#include <QSpinBox>
#include <QSlider>
#include <array>
#include <functional>
#include <cstdint>
#include <mutex>

namespace jcp {
// Only this mailbox crosses between OBS's video thread and Qt's UI thread.
struct FrameMailbox {
	std::mutex mutex;
	QImage image;
	uint64_t revision = 0;
};
extern FrameMailbox frames;

class Preview : public QLabel {
public:
	Preview();
	void setFrame(const QImage &image);

protected:
	void resizeEvent(QResizeEvent *event) override;

private:
	QPixmap frame;
	void fit();
};

class Controller : public QObject {
public:
	explicit Controller(QObject *parent, QString configPath);
	~Controller() override;
	Library library;
	Playout playout;
	PreviewPlayback previewPlayback;
	Renderer renderer;
	QPointer<Preview> preview;
	QPointer<QLabel> status;
	QPointer<QLineEdit> search;
	QPointer<QWidget> rowsHost;
	std::function<bool(QString &)> ensureSource;
	std::function<bool(const Caption &, const Style &, QString &)> showScenePreview;
	std::function<void()> clearScenePreviews;
	std::function<void()> hideScenePreviews;
	std::function<void(double)> tickScenePreviews;
	std::function<bool()> scenePlatesOnAir, liveSourceOnAir;
	std::function<double()> sourceScale;
	std::function<void(int)> fitSource;
	void stopLive();
	void resetLive();
	void setCanvasWidth(int width);
	QWidget *makeControlPanel();
	QWidget *makeDesignPanel();
	QDialog *makeDesignWindow(QWidget *parent = nullptr);
	void showDesign();
	void rebuildRows();
	bool moveCaption(const QString &id, const QString &target, bool after);
	void changed();
	void refreshPreview();
	Caption previewCaption() const;
	void initializeDocking();
	bool save();
	QString startupError;

private:
	QString configPath;
	QString saveError;
	QTimer ticker, saver;
	QElapsedTimer elapsed;
	QString selectedId;
	QString expandedId;
	QString dragToken;
	int canvasWidth = 1920;
	int fittedWidth = 0;
	QPointer<QToolButton> globalStop;
	QHash<QString, QPointer<QToolButton>> launchButtons;
	QPointer<QSpinBox> widthInput;
	QPointer<QSlider> widthSlider;
	QPointer<QDialog> designWindow;
	QPointer<QToolButton> previewLoopButton;
	std::array<bool, 3> previewLines{true, true, true};
	Renderer previewRenderer;
	void startPreview(bool repeat);
	void stopPreview();
	void launch(const QString &id);
	void updateStatus();
	void dockControlPanel(Qt::DockWidgetArea area);
};
} // namespace jcp
