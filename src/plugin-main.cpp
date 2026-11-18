#include "panels.hpp"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <graphics/graphics.h>
#include <util/bmem.h>
#include <QApplication>
#include <QMessageBox>
#include <QThread>
#include <algorithm>
#include <cstring>
#include <QBuffer>
#include <QJsonDocument>
#include <atomic>
#include <cmath>
#include <QUuid>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("jcp-sur-lower-thirds", "es-ES")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "Zócalos para OBS: fuente nativa, panel acoplable de operación y ventana de diseño.";
}

namespace {
constexpr const char *sourceId = "jcp_sur_lower_third";
constexpr const char *sourceName = "Zócalos · En vivo";
constexpr const char *legacySourceName = "JCP Sur · Zócalos";
jcp::Controller *controller = nullptr;
bool controlsAdded = false;
bool isPreviewSource(obs_source_t *source);
struct Source {
	gs_texture_t *texture = nullptr;
	uint64_t revision = uint64_t(-1);
	jcp::FrameMailbox snapshot;
	std::atomic_bool preview{false};
	uint32_t textureWidth = 0, textureHeight = 0;
	// Preview animation is driven on Qt's UI thread, under snapshot.mutex.
	jcp::Caption caption;
	jcp::Style style;
	jcp::Renderer renderer;
	QImage still;
	double phase = 0.0;
	bool exiting = false, hasModel = false;
};
const char *getName(void *)
{
	return "Zócalos";
}
void update(void *data, obs_data_t *settings)
{
	auto *s = static_cast<Source *>(data);
	s->preview = obs_data_get_bool(settings, "jcp-preview");
	if (!s->preview)
		return;
	const auto png = QByteArray::fromBase64(obs_data_get_string(settings, "jcp-preview-png"));
	QString error;
	const auto model = jcp::Library::parse(QByteArray(obs_data_get_string(settings, "jcp-preview-model")), error);
	QImage image = QImage::fromData(png, "PNG").convertToFormat(QImage::Format_RGBA8888);
	const bool visible = !image.isNull();
	if (image.isNull()) {
		image = QImage(900, 196, QImage::Format_RGBA8888);
		image.fill(Qt::transparent);
	}
	std::lock_guard<std::mutex> lock(s->snapshot.mutex);
	s->hasModel = model && !model->captions.isEmpty();
	if (s->hasModel) {
		s->caption = model->captions[0];
		s->style = model->style;
	}
	s->phase = visible ? 1.0 : 0.0;
	s->exiting = false;
	s->still = image;
	s->snapshot.image = std::move(image);
	++s->snapshot.revision;
}
void *create(obs_data_t *settings, obs_source_t *)
{
	auto *s = new Source;
	update(s, settings);
	return s;
}
jcp::FrameMailbox &mailbox(Source *s)
{
	return s->preview ? s->snapshot : jcp::frames;
}
uint32_t sourceWidth(void *data)
{
	auto &box = mailbox(static_cast<Source *>(data));
	std::lock_guard<std::mutex> lock(box.mutex);
	return box.image.isNull() ? 900u : uint32_t(box.image.width());
}
void destroy(void *data)
{
	auto *s = static_cast<Source *>(data);
	if (s->texture) {
		obs_enter_graphics();
		gs_texture_destroy(s->texture);
		obs_leave_graphics();
	}
	delete s;
}
void render(void *data, gs_effect_t *)
{
	auto *s = static_cast<Source *>(data);
	QImage image;
	uint64_t revision;
	{
		auto &box = mailbox(s);
		std::lock_guard<std::mutex> lock(box.mutex);
		revision = box.revision;
		if (revision != s->revision)
			image = box.image;
	}
	if (!image.isNull()) {
		if (s->texture &&
		    (s->textureWidth != uint32_t(image.width()) || s->textureHeight != uint32_t(image.height()))) {
			gs_texture_destroy(s->texture);
			s->texture = nullptr;
		}
		if (!s->texture) {
			s->textureWidth = uint32_t(image.width());
			s->textureHeight = uint32_t(image.height());
			s->texture =
				gs_texture_create(s->textureWidth, s->textureHeight, GS_RGBA, 1, nullptr, GS_DYNAMIC);
		}
		if (s->texture) {
			gs_texture_set_image(s->texture, image.constBits(), uint32_t(image.bytesPerLine()), false);
			s->revision = revision;
		}
	}
	if (!s->texture)
		return;
	auto *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA);
	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"), s->texture);
	while (gs_effect_loop(effect, "Draw"))
		gs_draw_sprite(s->texture, 0, 0, 0);
	gs_blend_state_pop();
}
bool ensureSource(QString &error)
{
	// In Studio Mode this is explicitly the program scene, not the preview scene.
	obs_source_t *current = obs_frontend_get_current_scene();
	if (!current) {
		error = "No hay una escena de programa disponible.";
		return false;
	}
	obs_scene_t *scene = obs_scene_from_source(current);
	if (!scene) {
		obs_source_release(current);
		error = "La escena de programa no admite fuentes.";
		return false;
	}
	obs_source_t *source = obs_get_source_by_name(sourceName);
	if (!source) {
		source = obs_get_source_by_name(legacySourceName);
		if (source && std::strcmp(obs_source_get_id(source), sourceId) == 0)
			obs_source_set_name(source, sourceName);
		else if (source) {
			obs_source_release(source);
			source = nullptr;
		}
	}
	if (source && std::strcmp(obs_source_get_id(source), sourceId) != 0) {
		obs_source_release(source);
		obs_source_release(current);
		error = "Ya existe otra fuente llamada Zócalos · En vivo. Cambiá su nombre para poder agregar el zócalo.";
		return false;
	}
	if (!source)
		source = obs_source_create(sourceId, sourceName, nullptr, nullptr);
	if (!source) {
		obs_source_release(current);
		error = "OBS no pudo crear la fuente del zócalo.";
		return false;
	}
	// Reuse one source across scenes. Existing item transforms remain operator controlled.
	auto *item = obs_scene_find_source(scene, sourceName);
	if (!item) {
		item = obs_scene_add(scene, source);
		if (item) {
			obs_video_info video = {};
			obs_get_video_info(&video);
			const float width = video.base_width ? float(video.base_width) : 1920.f;
			const float height = video.base_height ? float(video.base_height) : 1080.f;
			const float scale = std::min(width * .90f / 900.f, 1.6f);
			vec2 position, size;
			vec2_set(&position, width * .05f, height - 196.f * scale - height * .08f);
			vec2_set(&size, scale, scale);
			obs_sceneitem_set_pos(item, &position);
			obs_sceneitem_set_scale(item, &size);
		}
	}
	if (item) {
		obs_scene_enum_items(
			scene,
			[](obs_scene_t *, obs_sceneitem_t *other, void *) {
				if (isPreviewSource(obs_sceneitem_get_source(other)))
					obs_sceneitem_set_visible(other, false);
				return true;
			},
			nullptr);
		obs_sceneitem_set_visible(item, true);
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_TOP);
	}
	obs_source_release(source);
	obs_source_release(current);
	if (!item)
		error = "No se pudo agregar el zócalo a la escena de programa.";
	return item != nullptr;
}
double currentSourceScale()
{
	obs_video_info video = {};
	obs_get_video_info(&video);
	const float width = video.base_width ? float(video.base_width) : 1920.f;
	const float initialScale = std::min(width * .90f / 900.f, 1.6f);
	auto *current = obs_frontend_get_current_scene();
	if (!current)
		return initialScale;
	auto *scene = obs_scene_from_source(current);
	auto *item = scene ? obs_scene_find_source(scene, sourceName) : nullptr;
	if (!item && scene)
		item = obs_scene_find_source(scene, legacySourceName);
	vec2 scale;
	vec2_set(&scale, initialScale, initialScale);
	if (item)
		obs_sceneitem_get_scale(item, &scale);
	obs_source_release(current);
	return std::max(.01, double(std::abs(scale.x)));
}
void fitSource(int nativeWidth)
{
	auto *current = obs_frontend_get_current_scene();
	if (!current)
		return;
	auto *scene = obs_scene_from_source(current);
	auto *item = scene ? obs_scene_find_source(scene, sourceName) : nullptr;
	if (!item && scene)
		item = obs_scene_find_source(scene, legacySourceName);
	obs_video_info video = {};
	if (item && obs_get_video_info(&video) && video.base_width) {
		vec2 scale, position;
		obs_sceneitem_get_scale(item, &scale);
		obs_sceneitem_get_pos(item, &position);
		position.x = std::clamp(position.x, 0.f,
					std::max(0.f, float(video.base_width) - nativeWidth * std::abs(scale.x)));
		obs_sceneitem_set_pos(item, &position);
	}
	obs_source_release(current);
}
bool isPreviewSource(obs_source_t *source)
{
	if (std::strcmp(obs_source_get_id(source), sourceId) != 0)
		return false;
	auto *settings = obs_source_get_settings(source);
	const bool preview = obs_data_get_bool(settings, "jcp-preview");
	obs_data_release(settings);
	return preview;
}
void clearScenePreviews()
{
	obs_enum_sources(
		[](void *, obs_source_t *source) {
			if (isPreviewSource(source)) {
				auto *settings = obs_source_get_settings(source);
				obs_data_set_string(settings, "jcp-preview-png", "");
				obs_source_update(source, settings);
				obs_data_release(settings);
			}
			return true;
		},
		nullptr);
}
bool liveSourceOnAir()
{
	auto *source = obs_get_source_by_name(sourceName);
	if (!source)
		source = obs_get_source_by_name(legacySourceName);
	const bool active = source && obs_source_active(source);
	if (source)
		obs_source_release(source);
	return active;
}
bool scenePlatesOnAir()
{
	bool active = false;
	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			if (!isPreviewSource(source) || !obs_source_active(source))
				return true;
			auto *s = static_cast<Source *>(obs_obj_get_data(source));
			if (s) {
				std::lock_guard<std::mutex> lock(s->snapshot.mutex);
				if (s->phase > 0.0)
					*static_cast<bool *>(data) = true;
			}
			return true;
		},
		&active);
	return active;
}
void hideScenePreviews()
{
	obs_enum_sources(
		[](void *, obs_source_t *source) {
			if (isPreviewSource(source)) {
				auto *s = static_cast<Source *>(obs_obj_get_data(source));
				if (s) {
					std::lock_guard<std::mutex> lock(s->snapshot.mutex);
					s->exiting = s->phase > 0.0;
				}
			}
			return true;
		},
		nullptr);
}
void tickScenePreviews(double seconds)
{
	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			if (!isPreviewSource(source))
				return true;
			auto *s = static_cast<Source *>(obs_obj_get_data(source));
			if (!s)
				return true;
			bool finished = false;
			{
				std::lock_guard<std::mutex> lock(s->snapshot.mutex);
				if (!s->exiting)
					return true;
				s->phase =
					std::max(0.0, s->phase - std::max(0.0, *static_cast<double *>(data)) /
									 (std::clamp(s->style.transitionMs, 150, 5000) /
									  1000.0));
				if (s->hasModel)
					s->snapshot.image = s->renderer.render(s->caption, s->style, s->phase);
				else {
					// Older saved snapshots contain pixels only; animate their opacity.
					s->snapshot.image = QImage(s->still.size(), QImage::Format_RGBA8888);
					s->snapshot.image.fill(Qt::transparent);
					QPainter painter(&s->snapshot.image);
					painter.setOpacity(s->phase);
					painter.drawImage(0, 0, s->still);
				}
				++s->snapshot.revision;
				finished = s->phase == 0.0;
				s->exiting = !finished;
			}
			if (finished) {
				auto *settings = obs_source_get_settings(source);
				obs_data_set_string(settings, "jcp-preview-png", "");
				obs_data_release(settings);
			}
			return true;
		},
		&seconds);
}
bool showScenePreview(const jcp::Caption &caption, const jcp::Style &requested, QString &error)
{
	if (!obs_frontend_preview_program_mode_active())
		obs_frontend_set_preview_program_mode(true);
	auto *current = obs_frontend_get_current_preview_scene();
	if (!current) {
		error = "No hay una escena de Vista previa disponible.";
		return false;
	}
	auto *scene = obs_scene_from_source(current);
	if (!scene) {
		obs_source_release(current);
		error = "La Vista previa no admite fuentes.";
		return false;
	}
	// A scene shared with Program cannot be edited without changing the output.
	// Copy only scene items; cameras/media remain shared and keep playing normally.
	if (obs_source_active(current)) {
		const auto name = QString::fromUtf8(obs_source_get_name(current)) + " · Vista previa zócalos " +
				  QUuid::createUuid().toString(QUuid::WithoutBraces).left(6);
		auto *copy = obs_scene_duplicate(scene, name.toUtf8().constData(), OBS_SCENE_DUP_REFS);
		obs_source_release(current);
		if (!copy) {
			error = "No se pudo preparar una copia de la escena para Vista previa.";
			return false;
		}
		scene = copy;
		current = obs_source_get_ref(obs_scene_get_source(scene));
		obs_frontend_set_current_preview_scene(current);
		obs_scene_release(copy);
	}
	// Replace the previous preview item in this scene, without updating any snapshot
	// that the operator may already have transitioned to Program in another scene.
	obs_scene_enum_items(
		scene,
		[](obs_scene_t *, obs_sceneitem_t *item, void *) {
			auto *source = obs_sceneitem_get_source(item);
			if (isPreviewSource(source))
				obs_sceneitem_remove(item);
			else if (std::strcmp(obs_source_get_id(source), sourceId) == 0)
				obs_sceneitem_set_visible(item, false);
			return true;
		},
		nullptr);
	obs_video_info video = {};
	obs_get_video_info(&video);
	const float width = video.base_width ? float(video.base_width) : 1920.f;
	const float height = video.base_height ? float(video.base_height) : 1080.f;
	const float scale = std::min(width * .90f / 900.f, 1.6f);
	auto style = requested;
	style.maxWidth = std::max(64, int(std::floor(std::min(double(style.maxWidth), double(width)) / scale + .0001)));
	jcp::Renderer renderer;
	auto image = renderer.render(caption, style, 1.0);
	QByteArray png;
	QBuffer buffer(&png);
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "PNG");
	auto *settings = obs_data_create();
	obs_data_set_bool(settings, "jcp-preview", true);
	obs_data_set_string(settings, "jcp-preview-png", png.toBase64().constData());
	jcp::Library model;
	model.captions.append(caption);
	model.style = style;
	obs_data_set_string(settings, "jcp-preview-model",
			    QJsonDocument(model.json()).toJson(QJsonDocument::Compact).constData());
	const auto name =
		QString("Zócalos · Vista previa · ") + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
	auto *source = obs_source_create(sourceId, name.toUtf8().constData(), settings, nullptr);
	obs_data_release(settings);
	auto *item = source ? obs_scene_add(scene, source) : nullptr;
	if (item) {
		vec2 position, size;
		vec2_set(&position, std::min(width * .05f, std::max(0.f, width - float(image.width()) * scale)),
			 height - 196.f * scale - height * .08f);
		vec2_set(&size, scale, scale);
		obs_sceneitem_set_pos(item, &position);
		obs_sceneitem_set_scale(item, &size);
		obs_sceneitem_set_order(item, OBS_ORDER_MOVE_TOP);
	}
	if (source)
		obs_source_release(source);
	obs_source_release(current);
	if (!item)
		error = "No se pudo mostrar el zócalo en Vista previa.";
	return item != nullptr;
}
void refreshCanvasWidth()
{
	obs_video_info video = {};
	if (controller && obs_get_video_info(&video) && video.base_width)
		controller->setCanvasWidth(int(video.base_width));
}
void cleanup()
{
	if (!controller)
		return;
	controller->save();
	if (controlsAdded)
		obs_frontend_remove_dock("jcp-sur-playout");
	controlsAdded = false;
	delete controller;
	controller = nullptr;
	std::lock_guard<std::mutex> lock(jcp::frames.mutex);
	jcp::frames.image = QImage(900, 196, QImage::Format_RGBA8888);
	jcp::frames.image.fill(Qt::transparent);
	++jcp::frames.revision;
}
void frontendEvent(obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING && controller) {
		controller->initializeDocking();
		refreshCanvasWidth();
	}
	if (event == OBS_FRONTEND_EVENT_PROFILE_CHANGED || event == OBS_FRONTEND_EVENT_SCENE_CHANGED)
		refreshCanvasWidth();
	if (event == OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING && controller)
		controller->resetLive();
	if (event == OBS_FRONTEND_EVENT_EXIT) {
		cleanup();
		obs_frontend_remove_event_callback(frontendEvent, nullptr);
	}
}
} // namespace

bool obs_module_load(void)
{
	obs_source_info info = {};
	info.id = sourceId;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
	info.get_name = getName;
	info.create = create;
	info.update = update;
	info.destroy = destroy;
	info.video_render = render;
	info.get_width = sourceWidth;
	info.get_height = [](void *) -> uint32_t {
		return 196;
	};
	info.icon_type = OBS_ICON_TYPE_TEXT;
	obs_register_source(&info);
	blog(LOG_INFO, "[Zócalos] Plugin nativo 1.7.0 cargado");
	return true;
}
void obs_module_post_load(void)
{
	if (controller)
		return;
	auto *mainWindow = static_cast<QWidget *>(obs_frontend_get_main_window());
	if (!mainWindow)
		return;
	char *config = obs_module_config_path("library.json");
	const auto configPath = QString::fromUtf8(config);
	bfree(config);
	controller = new jcp::Controller(mainWindow, configPath);
	controller->ensureSource = ensureSource;
	controller->showScenePreview = showScenePreview;
	controller->clearScenePreviews = clearScenePreviews;
	controller->hideScenePreviews = hideScenePreviews;
	controller->tickScenePreviews = tickScenePreviews;
	controller->scenePlatesOnAir = scenePlatesOnAir;
	controller->liveSourceOnAir = liveSourceOnAir;
	controller->sourceScale = currentSourceScale;
	controller->fitSource = fitSource;
	refreshCanvasWidth();
	auto *control = controller->makeControlPanel();
	controlsAdded = obs_frontend_add_dock_by_id("jcp-sur-playout", "Zócalos", control);
	if (!controlsAdded)
		delete control;
	obs_frontend_add_event_callback(frontendEvent, nullptr);
	if (!controller->startupError.isEmpty())
		QMessageBox::warning(mainWindow, "Biblioteca de zócalos", controller->startupError);
}
void obs_module_unload(void)
{ /* Qt docks are released by the frontend EXIT event. */
}
