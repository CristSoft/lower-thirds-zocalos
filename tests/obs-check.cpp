// Exercise the actual plugin adapter against libobs; only the frontend selection
// functions are replaced. No OBS UI, profile, collection or installation is opened.
#include "../src/plugin-main.cpp"
#include <QDir>
#include <iostream>
#include <cstdlib>

namespace {
obs_source_t *programScene = nullptr, *previewScene = nullptr;
bool studio = false;
void check(bool value, const char *message)
{
	if (!value) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}
obs_source_t *previewSource(obs_source_t *scene)
{
	obs_source_t *found = nullptr;
	obs_scene_enum_items(
		obs_scene_from_source(scene),
		[](obs_scene_t *, obs_sceneitem_t *item, void *data) {
			auto *source = obs_sceneitem_get_source(item);
			if (isPreviewSource(source))
				*static_cast<obs_source_t **>(data) = source;
			return true;
		},
		&found);
	return found;
}
int countItems(obs_source_t *scene)
{
	int count = 0;
	obs_scene_enum_items(
		obs_scene_from_source(scene),
		[](obs_scene_t *, obs_sceneitem_t *, void *data) {
			++*static_cast<int *>(data);
			return true;
		},
		&count);
	return count;
}
} // namespace
extern "C" {
obs_source_t *obs_frontend_get_current_scene()
{
	return obs_source_get_ref(programScene);
}
obs_source_t *obs_frontend_get_current_preview_scene()
{
	return studio ? obs_source_get_ref(previewScene) : nullptr;
}
bool obs_frontend_preview_program_mode_active()
{
	return studio;
}
void obs_frontend_set_preview_program_mode(bool value)
{
	studio = value;
}
void obs_frontend_set_current_preview_scene(obs_source_t *scene)
{
	auto *previous = previewScene;
	previewScene = obs_source_get_ref(scene);
	if (previous)
		obs_source_release(previous);
}
void *obs_frontend_get_main_window()
{
	return nullptr;
}
bool obs_frontend_add_dock_by_id(const char *, const char *, void *)
{
	return false;
}
void obs_frontend_remove_dock(const char *) {}
void obs_frontend_add_event_callback(obs_frontend_event_cb, void *) {}
void obs_frontend_remove_event_callback(obs_frontend_event_cb, void *) {}
}
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	check(jcp::loadBundledFonts(), "bundled font loads in libobs harness");
	check(obs_startup("es-ES", nullptr, nullptr), "libobs starts independently");
	const bool graphics = argc > 1;
	QByteArray graphicsModule;
	if (graphics) {
		obs_audio_info audio = {48000, SPEAKERS_STEREO};
		check(obs_reset_audio(&audio), "isolated mixer starts for orderly OBS teardown");
		const auto root = QDir(QString::fromLocal8Bit(argv[1])).absolutePath();
		// OBS 31 exposes only this legacy entry point for standalone core data paths.
		PRAGMA_WARN_PUSH
		PRAGMA_WARN_DEPRECATION
		obs_add_data_path((root + "/data/libobs/").toUtf8().constData());
		PRAGMA_WARN_POP
		graphicsModule = (root + "/bin/64bit/libobs-d3d11.dll").toUtf8();
		obs_video_info video = {};
		video.graphics_module = graphicsModule.constData();
		video.fps_num = 30;
		video.fps_den = 1;
		video.base_width = video.output_width = 1920;
		video.base_height = video.output_height = 1080;
		video.output_format = VIDEO_FORMAT_RGBA;
		video.colorspace = VIDEO_CS_709;
		video.range = VIDEO_RANGE_FULL;
		video.scale_type = OBS_SCALE_BICUBIC;
		check(obs_reset_video(&video) == OBS_VIDEO_SUCCESS, "isolated D3D11 video renderer starts");
	}
	check(obs_module_load(), "native source registers in real libobs");
	auto *program = obs_scene_create("Program");
	auto *preview = obs_scene_create("Preview");
	programScene = obs_scene_get_source(program);
	previewScene = obs_source_get_ref(obs_scene_get_source(preview));
	obs_source_inc_active(programScene);
	auto *legacy = obs_source_create(sourceId, legacySourceName, nullptr, nullptr);
	check(legacy != nullptr, "legacy source exists before upgrade");
	obs_scene_add(program, legacy);
	QString error;
	check(ensureSource(error), "live adapter adds native source to Program");
	check(QString::fromUtf8(obs_source_get_name(legacy)) == QString::fromUtf8(sourceName),
	      "legacy source is renamed and reused after upgrade");
	obs_source_release(legacy);
	auto *live = obs_scene_find_source(program, sourceName);
	check(live && obs_sceneitem_visible(live), "Program contains visible live item");
	jcp::Style style;
	auto caption = jcp::Caption::create();
	caption.lines = {"Prueba de Vista previa", "Línea dos", ""};
	jcp::Renderer renderer;
	auto liveStyle = style;
	liveStyle.maxWidth = 900;
	{
		std::lock_guard<std::mutex> lock(jcp::frames.mutex);
		jcp::frames.image = renderer.render(caption, liveStyle, 1);
		++jcp::frames.revision;
	}
	if (graphics) {
		auto *liveData = static_cast<Source *>(obs_obj_get_data(obs_sceneitem_get_source(live)));
		const auto draw = [&] {
			obs_enter_graphics();
			auto *target = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
			check(gs_texrender_begin(target, 1920, 196), "offscreen GPU target begins");
			gs_ortho(0, 1920, 0, 196, -100, 100);
			render(liveData, nullptr);
			gs_texrender_end(target);
			gs_texrender_destroy(target);
			obs_leave_graphics();
		};
		draw();
		check(liveData->texture && liveData->textureWidth == 900, "GPU uploads original native frame");
		auto longCaption = caption;
		longCaption.lines = {"Congreso de Comunicación y Esperanza", "", ""};
		auto wideStyle = style;
		wideStyle.maxWidth = 1920;
		const auto wide = renderer.render(longCaption, wideStyle, 1);
		{
			std::lock_guard<std::mutex> lock(jcp::frames.mutex);
			jcp::frames.image = wide;
			++jcp::frames.revision;
		}
		draw();
		check(liveData->textureWidth == uint32_t(wide.width()) && liveData->textureWidth > 900 &&
			      sourceWidth(liveData) == liveData->textureWidth,
		      "GPU texture and OBS source match configured fixed width");
		{
			std::lock_guard<std::mutex> lock(jcp::frames.mutex);
			jcp::frames.image = renderer.render(caption, liveStyle, 1);
			++jcp::frames.revision;
		}
		draw();
		auto shortCaption = caption;
		shortCaption.lines = {"Ana", "", ""};
		{
			std::lock_guard<std::mutex> lock(jcp::frames.mutex);
			jcp::frames.image = renderer.render(shortCaption, wideStyle, 1);
			++jcp::frames.revision;
		}
		draw();
		check(liveData->textureWidth == 1920 && sourceWidth(liveData) == 1920,
		      "GPU retains full fixed width when text becomes shorter");
		{
			std::lock_guard<std::mutex> lock(jcp::frames.mutex);
			jcp::frames.image = renderer.render(caption, liveStyle, 1);
			++jcp::frames.revision;
		}
		draw();
		check(liveData->textureWidth == 900, "GPU resizes when operator changes fixed width");
	}
	const auto previewBaselineRevision = jcp::frames.revision;
	check(showScenePreview(caption, style, error), "eye enables Studio Mode and populates selected Preview scene");
	check(studio && previewScene == obs_scene_get_source(preview), "different Preview scene remains selected");
	check(countItems(programScene) == 1 && obs_sceneitem_visible(live),
	      "preview does not alter Program composition");
	auto *snapshot = previewSource(previewScene);
	check(snapshot && snapshot != obs_sceneitem_get_source(live), "preview uses an independent source instance");
	auto *snapshotData = static_cast<Source *>(obs_obj_get_data(snapshot));
	check(!snapshotData->snapshot.image.isNull() && sourceWidth(snapshotData) <= 900,
	      "native source receives preview pixels and bounded width");
	check(jcp::frames.revision == previewBaselineRevision, "preview never writes live mailbox");
	obs_source_t *previousSnapshot = obs_source_get_ref(snapshot);
	const QImage previousImage = snapshotData->snapshot.image;
	caption.lines[0] = "Otro ejemplo";
	check(showScenePreview(caption, style, error) && countItems(previewScene) == 1,
	      "repeated preview replaces the item without stacking graphics");
	check(static_cast<Source *>(obs_obj_get_data(previousSnapshot))->snapshot.image == previousImage,
	      "replacing preview leaves other source snapshots unchanged");
	obs_source_release(previousSnapshot);
	// The current Preview can be the same scene as Program, or be nested in it.
	// Both cases are active scenes and must use a scene-item copy before editing.
	obs_frontend_set_current_preview_scene(programScene);
	const int originalCount = countItems(programScene);
	check(showScenePreview(caption, style, error), "preview of Program scene succeeds using a copy");
	check(previewScene != programScene && countItems(programScene) == originalCount && obs_sceneitem_visible(live),
	      "copy prevents accidental changes to on-air scene");
	check(previewSource(previewScene), "copied scene displays preview snapshot");
	auto *copyLive = obs_scene_find_source(obs_scene_from_source(previewScene), sourceName);
	check(copyLive && !obs_sceneitem_visible(copyLive), "live overlay is hidden only in preview copy");
	// Simulate an operator transitioning that preview scene to Program.
	obs_source_dec_active(programScene);
	obs_source_t *oldProgram = programScene;
	programScene = obs_source_get_ref(previewScene);
	obs_source_inc_active(programScene);
	auto *promotedSnapshot = obs_source_get_ref(previewSource(programScene));
	const QImage promotedImage = static_cast<Source *>(obs_obj_get_data(promotedSnapshot))->snapshot.image;
	check(showScenePreview(caption, style, error),
	      "next eye click copies a promoted preview instead of editing Program");
	check(static_cast<Source *>(obs_obj_get_data(promotedSnapshot))->snapshot.image == promotedImage,
	      "promoted snapshot is stable on Program");
	check(ensureSource(error), "launch reuses live source in formerly previewed scene");
	auto *promotedItem =
		obs_scene_find_source(obs_scene_from_source(programScene), obs_source_get_name(promotedSnapshot));
	check(promotedItem && !obs_sceneitem_visible(promotedItem),
	      "live launch hides previous preview item in Program");
	// The promoted item was hidden by ensureSource; make it visible again to test
	// the general Stop's real Program-state query and native reverse animation.
	obs_sceneitem_set_visible(promotedItem, true);
	auto *promotedData = static_cast<Source *>(obs_obj_get_data(promotedSnapshot));
	check(scenePlatesOnAir(), "promoted preview is detected as an on-air plate");
	const auto revisionBeforeStop = promotedData->snapshot.revision;
	hideScenePreviews();
	check(promotedData->phase == 1.0 && promotedData->exiting &&
		      promotedData->snapshot.revision == revisionBeforeStop,
	      "preview stop preserves current frame and starts exit");
	tickScenePreviews(1.1);
	check(std::abs(promotedData->phase - .5) < .00001 && scenePlatesOnAir(),
	      "promoted preview remains on air during exit");
	check(promotedData->snapshot.image == renderer.render(promotedData->caption, promotedData->style, .5),
	      "preview uses same reverse animation as live plate");
	hideScenePreviews();
	check(promotedData->phase == .5, "repeated stop preserves preview exit progress");
	tickScenePreviews(1.1);
	check(promotedData->phase == 0.0 && !scenePlatesOnAir(), "on-air state resets when preview exit completes");
	auto *settings = obs_source_get_settings(promotedSnapshot);
	check(std::strlen(obs_data_get_string(settings, "jcp-preview-png")) == 0,
	      "completed exit clears saved preview snapshots including promoted ones");
	obs_data_release(settings);
	for (int design = 0; design < jcp::DesignCount; ++design) {
		auto variant = style;
		variant.design = design;
		check(showScenePreview(caption, variant, error), "all designs can create native OBS preview sources");
		auto *variantData = static_cast<Source *>(obs_obj_get_data(previewSource(previewScene)));
		check(variantData->style.design == design, "native preview retains new style IDs in saved model");
		hideScenePreviews();
		tickScenePreviews(1.1);
		check(variantData->snapshot.image == renderer.render(variantData->caption, variantData->style, .5),
		      "each design animates out using its own native renderer");
		tickScenePreviews(1.1);
		check(variantData->phase == 0.0, "each design completes native exit");
	}
	obs_source_release(promotedSnapshot);
	obs_source_dec_active(programScene);
	obs_source_release(programScene);
	programScene = oldProgram;
	obs_source_release(previewScene);
	previewScene = nullptr;
	obs_scene_release(preview);
	obs_scene_release(program);
	// Public scenes are also held by the main canvas. Remove them as the actual
	// frontend does before shutting down, then drain deferred destruction.
	QVector<obs_source_t *> scenes;
	obs_enum_scenes(
		[](void *data, obs_source_t *source) {
			static_cast<QVector<obs_source_t *> *>(data)->append(obs_source_get_ref(source));
			return true;
		},
		&scenes);
	for (auto *scene : scenes) {
		obs_source_remove(scene);
		obs_source_release(scene);
	}
	if (graphics) {
		while (obs_wait_for_destroy_queue()) {
		}
	} else
		obs_queue_task(OBS_TASK_DESTROY, [](void *) {}, nullptr, true);
	obs_shutdown();
	std::cout
		<< "PASS: real libobs source creation, preview scene routing, same-scene isolation, snapshot isolation, promoted previews, animated exit, all styles and real on-air state\n";
}
