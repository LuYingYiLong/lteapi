#include "composition_server.h"

#include "component_registry.h"
#include "file_save_server.h"
#include "preferences_manager.h"
#include "project_manager.h"
#include "settings_config.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#ifdef LTE_ENABLE_COMPOSITION_PERF_TRACE
#define COMP_PERF_TRACE_BEGIN(name) uint64_t __perf_##name = godot::Time::get_singleton()->get_ticks_usec()
#define COMP_PERF_TRACE_END(name, label) \
	do { \
		uint64_t __elapsed = godot::Time::get_singleton()->get_ticks_usec() - __perf_##name; \
		UtilityFunctions::print(vformat("COMP_PERF %s: %d us", label, __elapsed)); \
	} while (0)
#else
#define COMP_PERF_TRACE_BEGIN(name) ((void)0)
#define COMP_PERF_TRACE_END(name, label) ((void)0)
#endif

namespace godot {
	LTECompositionServer* LTECompositionServer::singleton = nullptr;

	namespace {
		bool _is_scene_auto_save_enabled() {
			LTEPreferencesManager* preferences_manager = LTEPreferencesManager::get_singleton();
			return preferences_manager != nullptr && preferences_manager->get_bool_value("core.scene_auto_save", false);
		}
	}

	void LTECompositionServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_on_file_saved", "path", "tag", "revision"), &LTECompositionServer::_on_file_saved);
		ClassDB::bind_method(D_METHOD("_on_file_save_failed", "path", "tag", "revision", "error"), &LTECompositionServer::_on_file_save_failed);
		ClassDB::bind_method(D_METHOD("open_composition", "uuid", "scene_path"), &LTECompositionServer::open_composition);
		ClassDB::bind_method(D_METHOD("get_opened_composition_list"), &LTECompositionServer::get_opened_composition_list);
		ClassDB::bind_method(D_METHOD("find_opened_composition_info", "uuid", "scene_path"), &LTECompositionServer::find_opened_composition_info);
		ClassDB::bind_method(D_METHOD("get_composition", "scene_path"), &LTECompositionServer::get_composition);
		ClassDB::bind_method(D_METHOD("get_composition_layers", "scene_path"), &LTECompositionServer::get_composition_layers);
		ClassDB::bind_method(D_METHOD("set_composition_layers", "scene_path", "layers"), &LTECompositionServer::set_composition_layers);
		ClassDB::bind_method(D_METHOD("submit_composition_changes", "uuid", "scene_path", "new_composition"), &LTECompositionServer::submit_composition_changes);
		ClassDB::bind_method(D_METHOD("submit_normalized_composition_changes", "uuid", "scene_path", "new_composition"), &LTECompositionServer::submit_normalized_composition_changes);
		ClassDB::bind_method(D_METHOD("apply_composition_keyframe_deltas", "uuid", "scene_path", "deltas", "use_after"), &LTECompositionServer::apply_composition_keyframe_deltas, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("save_composition", "uuid", "scene_path"), &LTECompositionServer::save_composition);
		ClassDB::bind_method(D_METHOD("evaluate_composition", "scene_path", "time"), &LTECompositionServer::evaluate_composition);
		ClassDB::bind_method(D_METHOD("normalize_composition", "value"), &LTECompositionServer::normalize_composition);
		ClassDB::bind_method(D_METHOD("compute_interpolation_weight", "interpolation", "keyframe_data", "weight"), &LTECompositionServer::compute_interpolation_weight);
		ClassDB::bind_method(D_METHOD("get_last_opened_composition", "uuid"), &LTECompositionServer::get_last_opened_composition);
		ClassDB::bind_method(D_METHOD("set_last_opened_composition", "uuid", "scene_path"), &LTECompositionServer::set_last_opened_composition);
		ClassDB::bind_method(D_METHOD("fetch_timeline_config", "uuid", "scene_path"), &LTECompositionServer::fetch_timeline_config);
		ClassDB::bind_method(D_METHOD("get_scene_layers_collapsed_items", "uuid", "scene_path"), &LTECompositionServer::get_scene_layers_collapsed_items);
		ClassDB::bind_method(D_METHOD("set_scene_layers_collapsed_items", "uuid", "scene_path", "collapsed_items"), &LTECompositionServer::set_scene_layers_collapsed_items);
		ClassDB::bind_method(D_METHOD("set_timeline_snap_mode", "uuid", "scene_path", "enable"), &LTECompositionServer::set_timeline_snap_mode);
		ClassDB::bind_method(D_METHOD("set_timeline_step", "uuid", "scene_path", "step"), &LTECompositionServer::set_timeline_step);
		ClassDB::bind_method(D_METHOD("set_timeline_counting_unit", "uuid", "scene_path", "counting_unit"), &LTECompositionServer::set_timeline_counting_unit);
		ClassDB::bind_method(D_METHOD("set_timeline_per_bar", "uuid", "scene_path", "per_bar"), &LTECompositionServer::set_timeline_per_bar);
		ClassDB::bind_method(D_METHOD("set_timeline_playhead_auto_scroll", "uuid", "scene_path", "enable"), &LTECompositionServer::set_timeline_playhead_auto_scroll);
		ClassDB::bind_method(D_METHOD("set_timeline_auto_insert_keyframes_mode", "uuid", "scene_path", "enable"), &LTECompositionServer::set_timeline_auto_insert_keyframes_mode);
		ClassDB::bind_method(D_METHOD("set_timeline_view", "uuid", "scene_path", "scale", "h_scroll", "v_scroll"), &LTECompositionServer::set_timeline_view);

		ADD_SIGNAL(MethodInfo("composition_opened",
			PropertyInfo(Variant::STRING, "uuid"),
			PropertyInfo(Variant::DICTIONARY, "composition_info")));
		ADD_SIGNAL(MethodInfo("composition_changed",
			PropertyInfo(Variant::STRING, "uuid"),
			PropertyInfo(Variant::STRING, "scene_path"),
			PropertyInfo(Variant::DICTIONARY, "composition_info")));
		ADD_SIGNAL(MethodInfo("composition_saved", PropertyInfo(Variant::STRING, "scene_path")));
		ADD_SIGNAL(MethodInfo("composition_save_failed",
			PropertyInfo(Variant::STRING, "scene_path"),
			PropertyInfo(Variant::INT, "error")));
		ADD_SIGNAL(MethodInfo("composition_layers_changed",
			PropertyInfo(Variant::STRING, "scene_path"),
			PropertyInfo(Variant::ARRAY, "layers")));
	}

	LTECompositionServer::LTECompositionServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (file_save_server) {
			file_save_server->connect("file_saved", Callable(this, "_on_file_saved"));
			file_save_server->connect("file_save_failed", Callable(this, "_on_file_save_failed"));
		}
	}

	LTECompositionServer::~LTECompositionServer() {
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (file_save_server) {
			file_save_server->flush_pending_saves();
			Callable saved_callable = Callable(this, "_on_file_saved");
			Callable failed_callable = Callable(this, "_on_file_save_failed");
			if (file_save_server->is_connected("file_saved", saved_callable)) {
				file_save_server->disconnect("file_saved", saved_callable);
			}
			if (file_save_server->is_connected("file_save_failed", failed_callable)) {
				file_save_server->disconnect("file_save_failed", failed_callable);
			}
		}
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTECompositionServer* LTECompositionServer::get_singleton() {
		return singleton;
	}

	void LTECompositionServer::clear_project_state() {
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (file_save_server) {
			file_save_server->flush_pending_saves();
		}
		opened_compositions.clear();
		evaluation_composition_cache.clear();
	}

	String LTECompositionServer::_get_absolute_scene_path(const String& scene_path) const {
		String normalized_path = scene_path.replace("\\", "/");
		if (normalized_path.is_empty()) {
			return String();
		}
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_path = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (normalized_path.begins_with("/") && !root_path.is_empty()) {
			return (root_path + normalized_path).simplify_path();
		}
		if (Utils::is_absolute_path(normalized_path)) {
			return normalized_path.simplify_path();
		}
		if (root_path.is_empty()) {
			return normalized_path.simplify_path();
		}
		return root_path.path_join(normalized_path).simplify_path();
	}

	bool LTECompositionServer::_is_opened_composition_in_current_project(const Dictionary& opened_composition) const {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_path = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (root_path.is_empty()) {
			return false;
		}

		String opened_project_path = String(opened_composition.get("project_path", String())).replace("\\", "/").simplify_path();
		if (!opened_project_path.is_empty()) {
			return opened_project_path.to_lower() == root_path.to_lower();
		}

		String opened_scene_path = _get_absolute_scene_path(opened_composition.get("path", String())).replace("\\", "/").simplify_path();
		if (opened_scene_path.is_empty()) {
			return false;
		}
		if (!root_path.ends_with("/")) {
			root_path += "/";
		}
		return opened_scene_path.to_lower().begins_with(root_path.to_lower());
	}

	Variant LTECompositionServer::_load_composition_variant(const String& scene_path) const {
		if (scene_path.is_empty() || !FileAccess::file_exists(scene_path)) {
			return Variant();
		}
		Ref<FileAccess> file = FileAccess::open(scene_path, FileAccess::READ);
		if (file.is_null()) {
			ERR_PRINT(vformat("Failed to open composition file: %s", scene_path));
			return Variant();
		}
		String content = file->get_as_text();
		file->close();
		return JSON::parse_string(content);
	}

	int LTECompositionServer::_find_opened_composition_index(const String& uuid, const String& scene_path) const {
		String target_path = _get_absolute_scene_path(scene_path);
		for (int index = 0; index < opened_compositions.size(); ++index) {
			Dictionary opened_composition = opened_compositions[index];
			if (!_is_opened_composition_in_current_project(opened_composition)) {
				continue;
			}
			String opened_uuid = opened_composition.get("uuid", String());
			String opened_path = _get_absolute_scene_path(opened_composition.get("path", String()));
			if (opened_uuid == uuid && opened_path == target_path) {
				return index;
			}
		}
		return -1;
	}

	int LTECompositionServer::_find_first_opened_composition_index(const String& scene_path) const {
		String target_path = _get_absolute_scene_path(scene_path);
		for (int index = 0; index < opened_compositions.size(); ++index) {
			Dictionary opened_composition = opened_compositions[index];
			if (!_is_opened_composition_in_current_project(opened_composition)) {
				continue;
			}
			String opened_path = _get_absolute_scene_path(opened_composition.get("path", String()));
			if (opened_path == target_path) {
				return index;
			}
		}
		return -1;
	}

	Dictionary LTECompositionServer::_record_composition_data(const String& uuid, const String& scene_path) {
		String absolute_scene_path = _get_absolute_scene_path(scene_path);
		Variant parsed_composition = _load_composition_variant(absolute_scene_path);
		Dictionary composition = _normalize_composition_data(parsed_composition);
		bool repaired = FileAccess::file_exists(absolute_scene_path) && _value_needs_game_layer_repair(parsed_composition);
		Dictionary opened_composition;
		opened_composition["uuid"] = uuid;
		opened_composition["path"] = absolute_scene_path;
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		opened_composition["project_path"] = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		opened_composition["saved"] = !repaired;
		opened_composition["save_revision"] = 0;
		opened_composition["composition"] = composition;
		opened_compositions.append(opened_composition);
		evaluation_composition_cache[absolute_scene_path] = composition;
		return opened_composition;
	}

	Dictionary LTECompositionServer::_load_composition(const String& scene_path) const {
		if (scene_path.is_empty() || !FileAccess::file_exists(scene_path)) {
			return _make_default_composition(scene_path.get_extension().to_lower() == "seq" ? String("sequence") : String("scene"));
		}
		Variant parsed = _load_composition_variant(scene_path);
		return _normalize_composition_data(parsed);
	}

	Dictionary LTECompositionServer::_get_composition_for_evaluation(const String& scene_path) const {
		String absolute_scene_path = _get_absolute_scene_path(scene_path);
		if (absolute_scene_path.is_empty()) {
			return Dictionary();
		}
		int index = _find_first_opened_composition_index(absolute_scene_path);
		if (index >= 0) {
			Dictionary opened_composition = opened_compositions[index];
			Dictionary composition = opened_composition.get("composition", Dictionary());
			return composition;
		}
		if (evaluation_composition_cache.has(absolute_scene_path)) {
			Dictionary composition = evaluation_composition_cache[absolute_scene_path];
			return composition;
		}
		Dictionary composition = _load_composition(absolute_scene_path);
		evaluation_composition_cache[absolute_scene_path] = composition;
		return composition;
	}

	Error LTECompositionServer::_save_composition(const String& scene_path, const Dictionary& composition) const {
		if (scene_path.is_empty()) {
			return ERR_FILE_BAD_PATH;
		}
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (!file_save_server) {
			return ERR_UNCONFIGURED;
		}
		return file_save_server->save_json_now(scene_path, composition, "\t", true);
	}

	void LTECompositionServer::_queue_composition_save(const String& scene_path, const Dictionary& composition, const int64_t revision) {
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (!file_save_server || scene_path.is_empty()) {
			return;
		}
		file_save_server->queue_json_save_no_copy(scene_path, composition, "\t", _make_composition_save_tag(scene_path, revision), _get_auto_save_delay_msec(), true);
	}

	int32_t LTECompositionServer::_get_auto_save_delay_msec() const {
		LTEPreferencesManager* preferences_manager = LTEPreferencesManager::get_singleton();
		double interval = preferences_manager != nullptr
			? preferences_manager->get_float_value("core.auto_save_interval", DEFAULT_AUTO_SAVE_INTERVAL_SECONDS)
			: DEFAULT_AUTO_SAVE_INTERVAL_SECONDS;
		interval = UtilityFunctions::clampf(interval, 0.1, 60.0);
		return static_cast<int32_t>(interval * 1000.0);
	}

	String LTECompositionServer::_make_composition_save_tag(const String& scene_path, const int64_t revision) const {
		return "composition\n" + scene_path + "\n" + String::num_int64(revision);
	}

	bool LTECompositionServer::_parse_composition_save_tag(const String& tag, String& r_scene_path, int64_t& r_revision) const {
		PackedStringArray parts = tag.split("\n", false);
		if (parts.size() != 3 || parts[0] != "composition") {
			return false;
		}
		r_scene_path = parts[1];
		r_revision = parts[2].to_int();
		return true;
	}

	void LTECompositionServer::_on_file_saved(const String& path, const String& tag, const int64_t revision) {
		COMP_PERF_TRACE_BEGIN(on_file_saved);
		String scene_path;
		int64_t composition_revision = 0;
		if (!_parse_composition_save_tag(tag, scene_path, composition_revision)) {
			return;
		}
		String absolute_scene_path = _get_absolute_scene_path(scene_path);
		bool saved_any = false;
		for (int opened_index = 0; opened_index < opened_compositions.size(); ++opened_index) {
			Dictionary opened_composition = opened_compositions[opened_index];
			if (_get_absolute_scene_path(opened_composition.get("path", String())) != absolute_scene_path) {
				continue;
			}
			int64_t current_revision = static_cast<int64_t>(opened_composition.get("save_revision", 0));
			if (current_revision != composition_revision) {
				continue;
			}
			opened_composition["saved"] = true;
			opened_compositions[opened_index] = opened_composition;
			saved_any = true;
		}
		if (saved_any) {
			emit_signal("composition_saved", absolute_scene_path);
		}
		COMP_PERF_TRACE_END(on_file_saved, "_on_file_saved");
	}

	void LTECompositionServer::_on_file_save_failed(const String& path, const String& tag, const int64_t revision, const int64_t error) {
		String scene_path;
		int64_t composition_revision = 0;
		if (!_parse_composition_save_tag(tag, scene_path, composition_revision)) {
			return;
		}
		String absolute_scene_path = _get_absolute_scene_path(scene_path);
		for (int opened_index = 0; opened_index < opened_compositions.size(); ++opened_index) {
			Dictionary opened_composition = opened_compositions[opened_index];
			if (_get_absolute_scene_path(opened_composition.get("path", String())) != absolute_scene_path) {
				continue;
			}
			opened_composition["saved"] = false;
			opened_compositions[opened_index] = opened_composition;
		}
		ERR_PRINT(vformat("Failed to save composition: %s. Error Code: %d", path, error));
		emit_signal("composition_save_failed", absolute_scene_path, error);
	}

	Dictionary LTECompositionServer::_make_default_composition(const String& type) const {
		Dictionary composition;
		Array layers;
		String composition_type = type.is_empty() ? String("scene") : type;
		if (composition_type != "sequence") {
			layers.append(_make_default_game_layer());
		}
		composition["type"] = composition_type;
		composition["version"] = 1;
		composition["duration"] = 0.0;
		composition["layers"] = layers;
		composition["property_tracks"] = Array();
		return composition;
	}

	Dictionary LTECompositionServer::_make_default_game_layer() const {
		Dictionary layer;
		Array position;
		Array scale;
		Array size;
		position.append(0.0);
		position.append(0.0);
		scale.append(1.0);
		scale.append(1.0);
		size.append(1920.0);
		size.append(1080.0);

		layer["id"] = Utils::uuid(Utils::UUID_V7);
		layer["type"] = "game";
		layer["name"] = "Game";
		layer["visible"] = true;
		layer["locked"] = true;
		layer["position"] = position;
		layer["scale"] = scale;
		layer["size"] = size;
		layer["rotation"] = 0.0;
		layer["opacity"] = 1.0;
		layer["property_tracks"] = Array();
		return layer;
	}

	Dictionary LTECompositionServer::_normalize_composition_data(const Variant& value) const {
		Dictionary composition = _make_default_composition();
		if (value.get_type() == Variant::ARRAY) {
			composition["layers"] = _normalize_layers(value);
			return composition;
		}
		if (value.get_type() != Variant::DICTIONARY) {
			return composition;
		}

		Dictionary source = value;
		String composition_type = String(source.get("type", "scene"));
		if (composition_type.is_empty()) {
			composition_type = "scene";
		}
		bool require_game_layer = composition_type != "sequence";
		composition["type"] = composition_type;
		composition["version"] = int(source.get("version", 1));
		composition["duration"] = double(source.get("duration", 0.0));
		composition["layers"] = _normalize_layers(source.get("layers", Array()), require_game_layer);
		composition["property_tracks"] = _normalize_property_tracks(source.get("property_tracks", source.get("tracks", Array())));

		Array keys = source.keys();
		for (int index = 0; index < keys.size(); ++index) {
			Variant key = keys[index];
			if (String(key) == "layers" || String(key) == "clips" || String(key) == "property_tracks" || String(key) == "tracks") {
				continue;
			}
			composition[key] = source.get(key, Variant());
		}
		return composition;
	}

	Dictionary LTECompositionServer::_normalize_embedded_sequence_composition(const Variant& value) const {
		Dictionary composition = _make_default_composition("sequence");
		if (value.get_type() == Variant::ARRAY) {
			composition["layers"] = _normalize_layers(value, false);
			return composition;
		}
		if (value.get_type() != Variant::DICTIONARY) {
			return composition;
		}

		Dictionary source = value;
		composition["type"] = "sequence";
		composition["version"] = int(source.get("version", 1));
		composition["duration"] = double(source.get("duration", 0.0));
		composition["layers"] = _normalize_layers(source.get("layers", Array()), false);
		composition["property_tracks"] = _normalize_property_tracks(source.get("property_tracks", source.get("tracks", Array())));

		Array keys = source.keys();
		for (int index = 0; index < keys.size(); ++index) {
			Variant key = keys[index];
			if (String(key) == "type" || String(key) == "layers" || String(key) == "clips" || String(key) == "property_tracks" || String(key) == "tracks") {
				continue;
			}
			composition[key] = source.get(key, Variant());
		}
		return composition;
	}

	Dictionary LTECompositionServer::_normalize_layer(const Variant& value) const {
		if (value.get_type() != Variant::DICTIONARY) {
			return Dictionary();
		}
		Dictionary layer = value;
		Dictionary normalized = layer.duplicate(true);
		String layer_type = normalized.get("type", "image");
		if (layer_type == "texture") {
			layer_type = "image";
		}
		normalized["type"] = layer_type;
		if (String(normalized.get("id", String())).is_empty()) {
			normalized["id"] = Utils::uuid(Utils::UUID_V7);
		}
		if (String(normalized.get("name", String())).is_empty()) {
			normalized["name"] = layer_type == "game" ? "Game" : "Layer";
		}
		normalized["visible"] = bool(normalized.get("visible", true));
		normalized["locked"] = bool(normalized.get("locked", false));
		if (layer_type == "game") {
			normalized["name"] = "Game";
			normalized["locked"] = true;
		}
		normalized["position"] = _normalize_vector2_array(normalized.get("position", Array()), 0.0, 0.0);
		normalized["scale"] = _normalize_vector2_array(normalized.get("scale", Array()), 1.0, 1.0);
		normalized["size"] = _normalize_vector2_array(normalized.get("size", Array()), 1920.0, 1080.0);
		normalized["rotation"] = double(normalized.get("rotation", 0.0));
		double opacity = double(normalized.get("opacity", 1.0));
		if (opacity < 0.0) {
			opacity = 0.0;
		}
		if (opacity > 1.0) {
			opacity = 1.0;
		}
		normalized["opacity"] = opacity;
		if (layer_type == "text") {
			normalized["text"] = String(normalized.get("text", "Text"));
			normalized["font_path"] = String(normalized.get("font_path", normalized.get("font", String())));
			normalized.erase("font");
			normalized["font_size"] = int(normalized.get("font_size", 64));
			normalized["color"] = _normalize_color_string(normalized.get("color", "#ffffffff"), "#ffffffff");
			normalized["alignment"] = String(normalized.get("alignment", "left"));
			double visible_ratio = double(normalized.get("visible_ratio", 1.0));
			if (visible_ratio < 0.0) {
				visible_ratio = 0.0;
			}
			if (visible_ratio > 1.0) {
				visible_ratio = 1.0;
			}
			normalized["visible_ratio"] = visible_ratio;
		}
		else if (layer_type == "color_rect") {
			normalized["color"] = _normalize_color_string(normalized.get("color", "#202020ff"), "#202020ff");
		}
		else if (layer_type == "shader_layer") {
			normalized["shader_path"] = String(normalized.get("shader_path", String()));
			if (normalized.get("parameters", Dictionary()).get_type() != Variant::DICTIONARY) {
				normalized["parameters"] = Dictionary();
			}
		}
		else if (layer_type == "lt_track") {
			int internal_track = int(normalized.get("internal_track", -1));
			if (internal_track < 0) {
				internal_track = -1;
			}
			if (internal_track > 7) {
				internal_track = 7;
			}
			double width = double(normalized.get("width", 160.0));
			if (width < 1.0) {
				width = 1.0;
			}
			normalized["internal_track"] = internal_track;
			normalized["color_standardization"] = bool(normalized.get("color_standardization", false));
			int standard_color = int(normalized.get("standard_color", 3));
			if (standard_color < 0) {
				standard_color = 0;
			}
			if (standard_color > 3) {
				standard_color = 3;
			}
			normalized["standard_color"] = standard_color;
			normalized["color"] = _normalize_color_string(normalized.get("color", "#ffffffff"), "#ffffffff");
			normalized["offset"] = double(normalized.get("offset", 0.0));
			normalized["width"] = width;
		}
		else if (layer_type == "sequence") {
			normalized["sequence_path"] = String(normalized.get("sequence_path", normalized.get("path", String())));
			normalized.erase("path");
			if (normalized.get("composition", Variant()).get_type() == Variant::DICTIONARY || normalized.get("composition", Variant()).get_type() == Variant::ARRAY) {
				normalized["composition"] = _normalize_embedded_sequence_composition(normalized.get("composition", Dictionary()));
			}
			else if (String(normalized.get("sequence_path", String())).is_empty()) {
				normalized["composition"] = _make_default_composition("sequence");
			}
			normalized["start_time"] = double(normalized.get("start_time", 0.0));
			normalized["time_offset"] = double(normalized.get("time_offset", 0.0));
			double time_scale = double(normalized.get("time_scale", 1.0));
			if (std::abs(time_scale) < 0.000001) {
				time_scale = 1.0;
			}
			normalized["time_scale"] = time_scale;
			normalized["duration"] = double(normalized.get("duration", 0.0));
			Array clips;
			bool has_clips = normalized.has("clips");
			if (has_clips && normalized.get("clips", Array()).get_type() == Variant::ARRAY) {
				Array source_clips = normalized.get("clips", Array());
				for (int clip_index = 0; clip_index < source_clips.size(); ++clip_index) {
					if (source_clips[clip_index].get_type() != Variant::DICTIONARY) {
						continue;
					}
					Dictionary source_clip = source_clips[clip_index];
					Dictionary clip = source_clip.duplicate(true);
					if (String(clip.get("id", String())).is_empty()) {
						clip["id"] = Utils::uuid(Utils::UUID_V7);
					}
					double clip_start_time = double(clip.get("start_time", normalized.get("start_time", 0.0)));
					if (clip_start_time < 0.0) {
						clip_start_time = 0.0;
					}
					double clip_duration = double(clip.get("duration", normalized.get("duration", 0.0)));
					if (clip_duration < 0.001) {
						clip_duration = 0.001;
					}
					double clip_time_scale = double(clip.get("time_scale", normalized.get("time_scale", 1.0)));
					if (std::abs(clip_time_scale) < 0.000001) {
						clip_time_scale = 1.0;
					}
					clip["start_time"] = clip_start_time;
					clip["duration"] = clip_duration;
					clip["time_offset"] = double(clip.get("time_offset", normalized.get("time_offset", 0.0)));
					clip["time_scale"] = clip_time_scale;
					clips.append(clip);
				}
			}
			if (!has_clips) {
				Dictionary clip;
				clip["id"] = Utils::uuid(Utils::UUID_V7);
				clip["start_time"] = double(normalized.get("start_time", 0.0));
				clip["duration"] = std::max(double(normalized.get("duration", 0.0)), 0.001);
				clip["time_offset"] = double(normalized.get("time_offset", 0.0));
				clip["time_scale"] = double(normalized.get("time_scale", 1.0));
				clips.append(clip);
			}
			normalized["clips"] = clips;
		}
		normalized["property_tracks"] = _normalize_property_tracks(normalized.get("property_tracks", normalized.get("tracks", Array())));
		return normalized;
	}

	Array LTECompositionServer::_normalize_layers(const Variant& value, const bool require_game_layer) const {
		Array layers;
		if (value.get_type() == Variant::ARRAY) {
			Array source = value;
			for (int index = 0; index < source.size(); ++index) {
				Dictionary layer = _normalize_layer(source[index]);
				if (!layer.is_empty()) {
					layers.append(layer);
				}
			}
		}
		if (require_game_layer && !_has_game_layer(layers)) {
			layers.append(_make_default_game_layer());
		}
		return layers;
	}

	bool LTECompositionServer::_has_game_layer(const Array& layers) const {
		for (int index = 0; index < layers.size(); ++index) {
			if (layers[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary layer = layers[index];
			if (String(layer.get("type", String())) == "game") {
				return true;
			}
		}
		return false;
	}

	bool LTECompositionServer::_value_needs_game_layer_repair(const Variant& value) const {
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary composition = value;
			if (String(composition.get("type", "scene")) == "sequence") {
				return false;
			}
			return _value_needs_game_layer_repair(composition.get("layers", Array()));
		}
		if (value.get_type() != Variant::ARRAY) {
			return true;
		}
		Array layers = value;
		bool has_game_layer = false;
		for (int index = 0; index < layers.size(); ++index) {
			if (layers[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary layer = layers[index];
			if (String(layer.get("type", String())) == "game") {
				has_game_layer = true;
				if (String(layer.get("name", "Game")) != "Game" || !bool(layer.get("locked", true))) {
					return true;
				}
			}
		}
		return !has_game_layer;
	}

	Array LTECompositionServer::_normalize_property_tracks(const Variant& value) const {
		Array tracks;
		if (value.get_type() != Variant::ARRAY) {
			return tracks;
		}
		Array source = value;
		for (int index = 0; index < source.size(); ++index) {
			Dictionary track = _normalize_property_track(source[index]);
			if (!track.is_empty()) {
				tracks.append(track);
			}
		}
		return tracks;
	}

	Dictionary LTECompositionServer::_normalize_property_track(const Variant& value) const {
		if (value.get_type() != Variant::DICTIONARY) {
			return Dictionary();
		}
		Dictionary source = value;
		Dictionary track = source.duplicate(true);
		track["id"] = String(track.get("id", Utils::uuid(Utils::UUID_V7)));
		track["layer_id"] = String(track.get("layer_id", String()));
		track["property"] = String(track.get("property", String()));
		if (String(track.get("property", String())).is_empty()) {
			return Dictionary();
		}
		track["keyframes"] = _normalize_keyframes(track.get("keyframes", Array()));
		track["modifiers"] = _normalize_modifiers(track.get("modifiers", Array()));
		return track;
	}

	Array LTECompositionServer::_normalize_keyframes(const Variant& value) const {
		Array keyframes;
		if (value.get_type() != Variant::ARRAY) {
			return keyframes;
		}
		Array source = value;
		std::vector<Dictionary> normalized_keyframes;
		normalized_keyframes.reserve(source.size());
		for (int index = 0; index < source.size(); ++index) {
			Dictionary keyframe = _normalize_keyframe(source[index]);
			if (!keyframe.is_empty()) {
				normalized_keyframes.push_back(keyframe);
			}
		}
		std::stable_sort(normalized_keyframes.begin(), normalized_keyframes.end(), [](const Dictionary& first, const Dictionary& second) {
			return double(first.get("time", 0.0)) < double(second.get("time", 0.0));
			});
		for (const Dictionary& keyframe : normalized_keyframes) {
			keyframes.append(keyframe);
		}
		return keyframes;
	}

	Dictionary LTECompositionServer::_normalize_keyframe(const Variant& value) const {
		if (value.get_type() != Variant::DICTIONARY) {
			return Dictionary();
		}
		Dictionary source = value;
		if (!source.has("value")) {
			return Dictionary();
		}
		Dictionary keyframe = source.duplicate(true);
		keyframe["time"] = double(keyframe.get("time", 0.0));
		String interpolation = keyframe.get("interpolation", "linear");
		if (interpolation.is_empty() || _get_interpolation_curve_type(interpolation).is_empty()) {
			interpolation = "linear";
		}
		keyframe["interpolation"] = interpolation;
		if (keyframe.get("interpolation_options", Variant()).get_type() != Variant::DICTIONARY) {
			keyframe.erase("interpolation_options");
		}
		if (_get_interpolation_curve_type(interpolation) != "bezier") {
			keyframe.erase("in_handle");
			keyframe.erase("out_handle");
			keyframe.erase("component_handles");
		}
		return keyframe;
	}

	Array LTECompositionServer::_normalize_modifiers(const Variant& value) const {
		Array modifiers;
		if (value.get_type() != Variant::ARRAY) {
			return modifiers;
		}
		Array source = value;
		for (int index = 0; index < source.size(); ++index) {
			Dictionary modifier = _normalize_modifier(source[index]);
			if (!modifier.is_empty()) {
				modifiers.append(modifier);
			}
		}
		return modifiers;
	}

	Dictionary LTECompositionServer::_normalize_modifier(const Variant& value) const {
		if (value.get_type() != Variant::DICTIONARY) {
			return Dictionary();
		}
		Dictionary source = value;
		Dictionary modifier = source.duplicate(true);
		String type = modifier.get("type", String());
		if (type.is_empty()) {
			type = "shake";
		}
		double start_time = std::max(double(modifier.get("start_time", modifier.get("time", 0.0))), 0.0);
		double end_time = std::max(double(modifier.get("end_time", start_time + 1.0)), start_time + 0.001);
		modifier["id"] = String(modifier.get("id", Utils::uuid(Utils::UUID_V7)));
		modifier["type"] = type;
		modifier["start_time"] = start_time;
		modifier["end_time"] = end_time;
		modifier["enabled"] = bool(modifier.get("enabled", true));
		if (type == "shake") {
			modifier["amplitude"] = _normalize_vector2_array(modifier.get("amplitude", Array()), 20.0, 20.0);
			modifier["frequency"] = std::max(double(modifier.get("frequency", 18.0)), 0.0);
			modifier["seed"] = int(modifier.get("seed", 0));
			modifier["fade_in"] = std::max(double(modifier.get("fade_in", 0.0)), 0.0);
			modifier["fade_out"] = std::max(double(modifier.get("fade_out", 0.0)), 0.0);
		}
		else if (type == "beat_pulse") {
			modifier["amplitude"] = _normalize_vector2_array(modifier.get("amplitude", Array()), 0.0, -32.0);
			modifier["bpm"] = std::max(double(modifier.get("bpm", 120.0)), 1.0);
			modifier["beat_interval"] = std::max(double(modifier.get("beat_interval", 1.0)), 0.000001);
			modifier["decay"] = std::max(double(modifier.get("decay", 2.0)), 0.000001);
			modifier["phase"] = double(modifier.get("phase", 0.0));
			modifier["fade_in"] = std::max(double(modifier.get("fade_in", 0.0)), 0.0);
			modifier["fade_out"] = std::max(double(modifier.get("fade_out", 0.0)), 0.0);
		}
		return modifier;
	}

	Array LTECompositionServer::_normalize_vector2_array(const Variant& value, const double fallback_x, const double fallback_y) const {
		Array result;
		if (value.get_type() == Variant::VECTOR2) {
			Vector2 vector = value;
			result.append(vector.x);
			result.append(vector.y);
			return result;
		}
		if (value.get_type() == Variant::VECTOR2I) {
			Vector2i vector = value;
			result.append(double(vector.x));
			result.append(double(vector.y));
			return result;
		}
		if (value.get_type() == Variant::ARRAY) {
			Array array = value;
			if (array.size() >= 2) {
				result.append(double(array[0]));
				result.append(double(array[1]));
				return result;
			}
		}
		result.append(fallback_x);
		result.append(fallback_y);
		return result;
	}

	String LTECompositionServer::_normalize_color_string(const Variant& value, const String& fallback) const {
		String color = String(value);
		if (color.is_empty()) {
			return fallback;
		}
		if (!color.begins_with("#")) {
			return "#" + color;
		}
		return color;
	}

	bool LTECompositionServer::_is_number(const Variant& value) const {
		Variant::Type type = value.get_type();
		return type == Variant::INT || type == Variant::FLOAT;
	}

	bool LTECompositionServer::_try_get_color(const Variant& value, Color& color) const {
		if (value.get_type() == Variant::COLOR) {
			color = value;
			return true;
		}
		if (value.get_type() != Variant::STRING) {
			return false;
		}
		String color_text = value;
		if (color_text.is_empty()) {
			return false;
		}
		if (!color_text.begins_with("#")) {
			color_text = "#" + color_text;
		}
		if (!Color::html_is_valid(color_text)) {
			return false;
		}
		color = Color::from_string(color_text, Color());
		return true;
	}

	Variant LTECompositionServer::_make_color_variant_like(const Variant& source, const Color& color) const {
		if (source.get_type() == Variant::COLOR) {
			return color;
		}
		return "#" + color.to_html(true);
	}

	void LTECompositionServer::_apply_property_tracks(Dictionary& state, const Array& tracks, const String& layer_id, const double time) const {
		for (int index = 0; index < tracks.size(); ++index) {
			if (tracks[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary track = tracks[index];
			String track_layer_id = track.get("layer_id", String());
			if (!track_layer_id.is_empty() && track_layer_id != layer_id) {
				continue;
			}
			String property_name = track.get("property", String());
			if (property_name.is_empty()) {
				continue;
			}
			Variant fallback = state.get(property_name, track.get("default_value", Variant()));
			Variant sampled_value = _sample_keyframes(track.get("keyframes", Array()), time, fallback);
			state[property_name] = _apply_property_modifiers(property_name, sampled_value, track.get("modifiers", Array()), time);
		}
	}

	Variant LTECompositionServer::_apply_property_modifiers(const String& property_name, const Variant& value, const Array& modifiers, const double time) const {
		Variant result = value;
		for (int index = 0; index < modifiers.size(); ++index) {
			if (modifiers[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary modifier = modifiers[index];
			if (!bool(modifier.get("enabled", true))) {
				continue;
			}
			double start_time = double(modifier.get("start_time", 0.0));
			double end_time = double(modifier.get("end_time", start_time));
			if (time < start_time || time > end_time) {
				continue;
			}
			String type = modifier.get("type", String());
			if (type == "shake" && property_name == "position") {
				result = _apply_shake_modifier(result, modifier, time);
			}
			else if (type == "beat_pulse" && property_name == "position") {
				result = _apply_beat_pulse_modifier(result, modifier, time);
			}
		}
		return result;
	}

	Variant LTECompositionServer::_apply_shake_modifier(const Variant& value, const Dictionary& modifier, const double time) const {
		double weight = _sample_modifier_fade_weight(modifier, time);
		if (weight <= 0.0) {
			return value;
		}
		Array amplitude = _normalize_vector2_array(modifier.get("amplitude", Array()), 20.0, 20.0);
		double local_time = std::max(time - double(modifier.get("start_time", 0.0)), 0.0);
		double frequency = std::max(double(modifier.get("frequency", 18.0)), 0.0);
		int seed = int(modifier.get("seed", 0));
		double offset_x = _sample_shake_value(local_time, frequency, seed, 0) * double(amplitude[0]) * weight;
		double offset_y = _sample_shake_value(local_time, frequency, seed, 1) * double(amplitude[1]) * weight;
		return _apply_vector_offset(value, offset_x, offset_y);
	}

	Variant LTECompositionServer::_apply_beat_pulse_modifier(const Variant& value, const Dictionary& modifier, const double time) const {
		double weight = _sample_modifier_fade_weight(modifier, time);
		if (weight <= 0.0) {
			return value;
		}
		double bpm = std::max(double(modifier.get("bpm", 120.0)), 1.0);
		double beat_interval = std::max(double(modifier.get("beat_interval", 1.0)), 0.000001);
		double period = 60.0 / bpm * beat_interval;
		if (period <= 0.000001) {
			return value;
		}
		double local_time = std::max(time - double(modifier.get("start_time", 0.0)), 0.0);
		double phase = double(modifier.get("phase", 0.0));
		double pulse_time = std::fmod(local_time + phase * period, period);
		if (pulse_time < 0.0) {
			pulse_time += period;
		}
		double normalized_time = std::clamp(pulse_time / period, 0.0, 1.0);
		double decay = std::max(double(modifier.get("decay", 2.0)), 0.000001);
		double pulse = std::pow(1.0 - normalized_time, decay) * weight;
		Array amplitude = _normalize_vector2_array(modifier.get("amplitude", Array()), 0.0, -32.0);
		double offset_x = double(amplitude[0]) * pulse;
		double offset_y = double(amplitude[1]) * pulse;
		return _apply_vector_offset(value, offset_x, offset_y);
	}

	Variant LTECompositionServer::_apply_vector_offset(const Variant& value, const double offset_x, const double offset_y) const {
		if (value.get_type() == Variant::VECTOR2) {
			Vector2 vector = value;
			vector.x += float(offset_x);
			vector.y += float(offset_y);
			return vector;
		}
		if (value.get_type() == Variant::ARRAY) {
			Array result = value;
			if (result.size() >= 2 && _is_number(result[0]) && _is_number(result[1])) {
				result[0] = double(result[0]) + offset_x;
				result[1] = double(result[1]) + offset_y;
				return result;
			}
		}
		if (_is_number(value)) {
			return double(value) + offset_x;
		}
		return value;
	}

	double LTECompositionServer::_sample_shake_value(const double local_time, const double frequency, const int seed, const int axis) const {
		if (frequency <= 0.0) {
			return 0.0;
		}
		double sample_position = local_time * frequency;
		double sample_index = std::floor(sample_position);
		double weight = sample_position - sample_index;
		double smooth_weight = weight * weight * (3.0 - 2.0 * weight);
		double from_value = _sample_shake_noise(sample_index, seed, axis);
		double to_value = _sample_shake_noise(sample_index + 1.0, seed, axis);
		return from_value + (to_value - from_value) * smooth_weight;
	}

	double LTECompositionServer::_sample_shake_noise(const double sample_index, const int seed, const int axis) const {
		double hash_input = sample_index + double(seed) * 19.191 + double(axis) * 73.371;
		double value = std::sin(hash_input * 12.9898) * 43758.5453;
		double fraction = value - std::floor(value);
		return fraction * 2.0 - 1.0;
	}

	double LTECompositionServer::_sample_modifier_fade_weight(const Dictionary& modifier, const double time) const {
		double start_time = double(modifier.get("start_time", 0.0));
		double end_time = double(modifier.get("end_time", start_time));
		double weight = 1.0;
		double fade_in = std::max(double(modifier.get("fade_in", 0.0)), 0.0);
		double fade_out = std::max(double(modifier.get("fade_out", 0.0)), 0.0);
		if (fade_in > 0.000001) {
			weight = std::min(weight, (time - start_time) / fade_in);
		}
		if (fade_out > 0.000001) {
			weight = std::min(weight, (end_time - time) / fade_out);
		}
		return std::clamp(weight, 0.0, 1.0);
	}

	Variant LTECompositionServer::_sample_keyframes(const Array& keyframes, const double time, const Variant& fallback) const {
		if (keyframes.is_empty()) {
			return fallback;
		}

		Dictionary previous_keyframe;
		Dictionary next_keyframe;
		double previous_time = -1.0e30;
		double next_time = 1.0e30;
		int previous_index = -1;
		int next_index = -1;
		bool can_binary_search = true;
		int low = 0;
		int high = keyframes.size() - 1;
		while (low <= high) {
			int middle = low + (high - low) / 2;
			if (keyframes[middle].get_type() != Variant::DICTIONARY) {
				can_binary_search = false;
				break;
			}
			Dictionary keyframe = keyframes[middle];
			double keyframe_time = double(keyframe.get("time", 0.0));
			if (keyframe_time <= time) {
				previous_index = middle;
				low = middle + 1;
			}
			else {
				next_index = middle;
				high = middle - 1;
			}
		}
		if (can_binary_search) {
			if (previous_index >= 0) {
				previous_keyframe = keyframes[previous_index];
				previous_time = double(previous_keyframe.get("time", 0.0));
			}
			if (next_index >= 0) {
				next_keyframe = keyframes[next_index];
				next_time = double(next_keyframe.get("time", 0.0));
			}
		}
		else {
			for (int index = 0; index < keyframes.size(); ++index) {
				if (keyframes[index].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary keyframe = keyframes[index];
				double keyframe_time = double(keyframe.get("time", 0.0));
				if (keyframe_time <= time && keyframe_time >= previous_time) {
					previous_keyframe = keyframe;
					previous_time = keyframe_time;
				}
				if (keyframe_time > time && keyframe_time < next_time) {
					next_keyframe = keyframe;
					next_time = keyframe_time;
				}
			}
		}

		if (previous_keyframe.is_empty()) {
			return fallback.get_type() == Variant::NIL && !next_keyframe.is_empty() ? next_keyframe.get("value", Variant()) : fallback;
		}
		Variant previous_value = previous_keyframe.get("value", fallback);
		if (next_keyframe.is_empty()) {
			return previous_value;
		}

		String interpolation = previous_keyframe.get("interpolation", "linear");
		String curve_type = _get_interpolation_curve_type(interpolation);
		if (curve_type == "constant") {
			return previous_value;
		}
		double span = next_time - previous_time;
		if (span <= 0.000001) {
			return previous_value;
		}
		double weight = std::clamp((time - previous_time) / span, 0.0, 1.0);
		Variant next_value = next_keyframe.get("value", previous_value);
		if (curve_type == "bezier") {
			return _sample_bezier_variant(previous_value, next_value, previous_keyframe, next_keyframe, weight);
		}
		if (curve_type == "linear") {
			return _lerp_variant(previous_value, next_value, weight);
		}
		return _lerp_variant(previous_value, next_value, _sample_interpolation_weight(interpolation, previous_keyframe, weight));
	}

	Variant LTECompositionServer::_lerp_variant(const Variant& from, const Variant& to, const double weight) const {
		if (_is_number(from) && _is_number(to)) {
			double from_value = double(from);
			double to_value = double(to);
			return from_value + (to_value - from_value) * weight;
		}
		Color from_color;
		Color to_color;
		if (_try_get_color(from, from_color) && _try_get_color(to, to_color)) {
			return _make_color_variant_like(from, from_color.lerp(to_color, float(weight)));
		}
		if (from.get_type() == Variant::ARRAY && to.get_type() == Variant::ARRAY) {
			Array from_array = from;
			Array to_array = to;
			int count = std::min(from_array.size(), to_array.size());
			bool can_lerp = count > 0;
			for (int index = 0; index < count; ++index) {
				can_lerp = can_lerp && _is_number(from_array[index]) && _is_number(to_array[index]);
			}
			if (can_lerp) {
				Array result;
				for (int index = 0; index < count; ++index) {
					double from_value = double(from_array[index]);
					double to_value = double(to_array[index]);
					result.append(from_value + (to_value - from_value) * weight);
				}
				return result;
			}
		}
		return weight < 1.0 ? from : to;
	}

	Variant LTECompositionServer::_sample_bezier_variant(const Variant& from, const Variant& to, const Dictionary& previous_keyframe, const Dictionary& next_keyframe, const double weight) const {
		if (_is_number(from) && _is_number(to)) {
			return _sample_bezier_number(double(from), double(to), previous_keyframe, next_keyframe, weight);
		}
		Color from_color;
		Color to_color;
		if (_try_get_color(from, from_color) && _try_get_color(to, to_color)) {
			Color result;
			result.r = float(_sample_bezier_number(from_color.r, to_color.r, previous_keyframe, next_keyframe, weight, 0));
			result.g = float(_sample_bezier_number(from_color.g, to_color.g, previous_keyframe, next_keyframe, weight, 1));
			result.b = float(_sample_bezier_number(from_color.b, to_color.b, previous_keyframe, next_keyframe, weight, 2));
			result.a = float(_sample_bezier_number(from_color.a, to_color.a, previous_keyframe, next_keyframe, weight, 3));
			return _make_color_variant_like(from, result);
		}
		if (from.get_type() == Variant::ARRAY && to.get_type() == Variant::ARRAY) {
			Array from_array = from;
			Array to_array = to;
			int count = std::min(from_array.size(), to_array.size());
			bool can_sample = count > 0;
			for (int index = 0; index < count; ++index) {
				can_sample = can_sample && _is_number(from_array[index]) && _is_number(to_array[index]);
			}
			if (can_sample) {
				Array result;
				for (int index = 0; index < count; ++index) {
					result.append(_sample_bezier_number(double(from_array[index]), double(to_array[index]), previous_keyframe, next_keyframe, weight, index));
				}
				return result;
			}
		}
		return weight < 1.0 ? from : to;
	}

	double LTECompositionServer::_sample_bezier_number(const double from, const double to, const Dictionary& previous_keyframe, const Dictionary& next_keyframe, const double weight, const int component_index) const {
		Dictionary out_handle = _get_keyframe_handle(previous_keyframe, "out_handle", component_index);
		Dictionary in_handle = _get_keyframe_handle(next_keyframe, "in_handle", component_index);
		Vector2 point_a = Vector2(0.0, from);
		Vector2 point_b = Vector2(
			std::clamp(double(out_handle.get("x", 1.0 / 3.0)), 0.001, 0.999),
			from + double(out_handle.get("y", (to - from) / 3.0)));
		Vector2 point_c = Vector2(
			1.0 + std::clamp(double(in_handle.get("x", -1.0 / 3.0)), -0.999, -0.001),
			to + double(in_handle.get("y", (from - to) / 3.0)));
		Vector2 point_d = Vector2(1.0, to);
		double solved_weight = _solve_bezier_weight_for_x(point_a, point_b, point_c, point_d, weight);
		return _cubic_bezier_point(point_a, point_b, point_c, point_d, solved_weight).y;
	}

	double LTECompositionServer::_sample_interpolation_weight(const String& interpolation, const Dictionary& keyframe, const double weight) const {
		String curve_type = _get_interpolation_curve_type(interpolation);
		Dictionary options = _get_interpolation_options(keyframe);
		String easing = options.get("easing", "ease_in_out");
		if (easing == "auto") {
			easing = "ease_in_out";
		}
		return std::clamp(_apply_easing(curve_type, easing, weight), 0.0, 1.0);
	}

	double LTECompositionServer::_apply_easing(const String& curve_type, const String& easing, const double weight) const {
		double clamped_weight = std::clamp(weight, 0.0, 1.0);
		if (easing == "ease_in") {
			return _sample_ease_in(curve_type, clamped_weight);
		}
		if (easing == "ease_out") {
			return 1.0 - _sample_ease_in(curve_type, 1.0 - clamped_weight);
		}
		if (easing == "ease_in_out") {
			if (clamped_weight < 0.5) {
				return _sample_ease_in(curve_type, clamped_weight * 2.0) * 0.5;
			}
			return 1.0 - _sample_ease_in(curve_type, 2.0 - clamped_weight * 2.0) * 0.5;
		}
		return _sample_ease_in(curve_type, clamped_weight);
	}

	double LTECompositionServer::_sample_ease_in(const String& curve_type, const double weight) const {
		constexpr double pi = 3.14159265358979323846;
		constexpr double tau = 6.28318530717958647692;
		if (curve_type == "sinusoidal") {
			return 1.0 - std::cos(weight * pi * 0.5);
		}
		if (curve_type == "quadratic") {
			return weight * weight;
		}
		if (curve_type == "cubic") {
			return weight * weight * weight;
		}
		if (curve_type == "quartic") {
			return std::pow(weight, 4.0);
		}
		if (curve_type == "quintic") {
			return std::pow(weight, 5.0);
		}
		if (curve_type == "exponential") {
			if (std::abs(weight) <= 0.000001) {
				return 0.0;
			}
			return std::pow(2.0, 10.0 * weight - 10.0);
		}
		if (curve_type == "circular") {
			return 1.0 - std::sqrt(1.0 - weight * weight);
		}
		if (curve_type == "back") {
			double overshoot = 1.70158;
			return (overshoot + 1.0) * weight * weight * weight - overshoot * weight * weight;
		}
		if (curve_type == "bounce") {
			return 1.0 - _sample_bounce_out(1.0 - weight);
		}
		if (curve_type == "elastic") {
			if (std::abs(weight) <= 0.000001 || std::abs(weight - 1.0) <= 0.000001) {
				return weight;
			}
			return -std::pow(2.0, 10.0 * weight - 10.0) * std::sin((weight * 10.0 - 10.75) * tau / 3.0);
		}
		return weight;
	}

	double LTECompositionServer::_sample_bounce_out(const double weight) const {
		double bounce_scale = 7.5625;
		double bounce_step = 2.75;
		if (weight < 1.0 / bounce_step) {
			return bounce_scale * weight * weight;
		}
		if (weight < 2.0 / bounce_step) {
			double shifted_weight_a = weight - 1.5 / bounce_step;
			return bounce_scale * shifted_weight_a * shifted_weight_a + 0.75;
		}
		if (weight < 2.5 / bounce_step) {
			double shifted_weight_b = weight - 2.25 / bounce_step;
			return bounce_scale * shifted_weight_b * shifted_weight_b + 0.9375;
		}
		double shifted_weight_c = weight - 2.625 / bounce_step;
		return bounce_scale * shifted_weight_c * shifted_weight_c + 0.984375;
	}

	double LTECompositionServer::compute_interpolation_weight(const String& interpolation, const Dictionary& keyframe_data, const double weight) const {
		return _sample_interpolation_weight(interpolation, keyframe_data, weight);
	}

	double LTECompositionServer::_solve_bezier_weight_for_x(const Vector2& point_a, const Vector2& point_b, const Vector2& point_c, const Vector2& point_d, const double target_x) const {
		double low = 0.0;
		double high = 1.0;
		double middle = 0.5;
		for (int index = 0; index < 18; ++index) {
			middle = (low + high) * 0.5;
			Vector2 position = _cubic_bezier_point(point_a, point_b, point_c, point_d, middle);
			if (position.x < target_x) {
				low = middle;
			}
			else {
				high = middle;
			}
		}
		return middle;
	}

	Vector2 LTECompositionServer::_cubic_bezier_point(const Vector2& point_a, const Vector2& point_b, const Vector2& point_c, const Vector2& point_d, const double weight) const {
		double inverse_weight = 1.0 - weight;
		return point_a * inverse_weight * inverse_weight * inverse_weight +
			point_b * 3.0 * inverse_weight * inverse_weight * weight +
			point_c * 3.0 * inverse_weight * weight * weight +
			point_d * weight * weight * weight;
	}

	Dictionary LTECompositionServer::_get_keyframe_handle(const Dictionary& keyframe, const String& handle_name, const int component_index) const {
		if (component_index >= 0) {
			Variant component_handles_value = keyframe.get("component_handles", Dictionary());
			if (component_handles_value.get_type() == Variant::DICTIONARY) {
				Dictionary component_handles = component_handles_value;
				Array keys;
				keys.append(String::num_int64(component_index));
				keys.append(_get_component_handle_key(component_index));
				if (component_index == 3) {
					keys.append("h");
				}
				for (int index = 0; index < keys.size(); ++index) {
					Variant handles_value = component_handles.get(keys[index], Dictionary());
					if (handles_value.get_type() != Variant::DICTIONARY) {
						continue;
					}
					Dictionary handles = handles_value;
					Variant handle_value = handles.get(handle_name, Dictionary());
					if (handle_value.get_type() == Variant::DICTIONARY) {
						return handle_value;
					}
				}
			}
		}
		Variant handle_value = keyframe.get(handle_name, Dictionary());
		if (handle_value.get_type() == Variant::DICTIONARY) {
			return handle_value;
		}
		return Dictionary();
	}

	Dictionary LTECompositionServer::_get_interpolation_options(const Dictionary& keyframe) const {
		Variant options_value = keyframe.get("interpolation_options", Dictionary());
		if (options_value.get_type() == Variant::DICTIONARY) {
			return options_value;
		}
		return Dictionary();
	}

	String LTECompositionServer::_get_interpolation_curve_type(const String& interpolation) const {
		LTEComponentRegistry* registry = LTEComponentRegistry::get_singleton();
		if (registry == nullptr) {
			return interpolation;
		}
		Dictionary interpolation_info = registry->get_interpolation_info(interpolation);
		if (interpolation_info.is_empty()) {
			return String();
		}
		return interpolation_info.get("curve_type", interpolation);
	}

	String LTECompositionServer::_get_component_handle_key(const int component_index) const {
		switch (component_index) {
		case 0:
			return "x";
		case 1:
			return "y";
		case 2:
			return "z";
		case 3:
			return "w";
		}
		return String::num_int64(component_index);
	}

	void LTECompositionServer::open_composition(const String& uuid, const String& scene_path) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		int index = _find_opened_composition_index(uuid, scene_path);
		Dictionary opened_composition = index >= 0 ? Dictionary(opened_compositions[index]) : _record_composition_data(uuid, scene_path);
		emit_signal("composition_opened", uuid, opened_composition.duplicate(true));
	}

	Array LTECompositionServer::get_opened_composition_list() const {
		Array current_project_compositions;
		for (int index = 0; index < opened_compositions.size(); ++index) {
			Dictionary opened_composition = opened_compositions[index];
			if (_is_opened_composition_in_current_project(opened_composition)) {
				current_project_compositions.append(opened_composition.duplicate(true));
			}
		}
		return current_project_compositions;
	}

	Dictionary LTECompositionServer::find_opened_composition_info(const String& uuid, const String& scene_path) const {
		int index = _find_opened_composition_index(uuid, scene_path);
		if (index < 0) {
			return Dictionary();
		}
		Dictionary opened_composition = opened_compositions[index];
		return opened_composition.duplicate(true);
	}

	Dictionary LTECompositionServer::get_composition(const String& scene_path) const {
		int index = _find_first_opened_composition_index(scene_path);
		if (index >= 0) {
			Dictionary opened_composition = opened_compositions[index];
			Dictionary composition = opened_composition.get("composition", Dictionary());
			return composition.duplicate(true);
		}
		return _load_composition(_get_absolute_scene_path(scene_path));
	}

	Array LTECompositionServer::get_composition_layers(const String& scene_path) const {
		Dictionary composition = get_composition(scene_path);
		Array layers = composition.get("layers", Array());
		return layers.duplicate(true);
	}

	Dictionary LTECompositionServer::set_composition_layers(const String& scene_path, const Array& layers) {
		String absolute_scene_path = _get_absolute_scene_path(scene_path);
		if (absolute_scene_path.is_empty()) {
			return Dictionary();
		}

		Dictionary composition = get_composition(absolute_scene_path);
		bool require_game_layer = String(composition.get("type", "scene")) != "sequence";
		composition["layers"] = _normalize_layers(layers, require_game_layer);
		evaluation_composition_cache[absolute_scene_path] = composition;
		Error error = _save_composition(absolute_scene_path, composition);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to save composition: %s. Error Code: %d", absolute_scene_path, error));
			return Dictionary();
		}

		for (int index = 0; index < opened_compositions.size(); ++index) {
			Dictionary opened_composition = opened_compositions[index];
			if (_get_absolute_scene_path(opened_composition.get("path", String())) != absolute_scene_path) {
				continue;
			}
			opened_composition["composition"] = composition.duplicate(true);
			opened_composition["saved"] = true;
			opened_compositions[index] = opened_composition;
			emit_signal("composition_changed", opened_composition.get("uuid", String()), absolute_scene_path, opened_composition.duplicate(true));
		}
		emit_signal("composition_layers_changed", absolute_scene_path, composition.get("layers", Array()));
		emit_signal("composition_saved", absolute_scene_path);
		return composition.duplicate(true);
	}

	Dictionary LTECompositionServer::submit_composition_changes(const String& uuid, const String& scene_path, const Dictionary& new_composition) {
		return _submit_composition_changes_internal(uuid, scene_path, new_composition, true);
	}

	Dictionary LTECompositionServer::submit_normalized_composition_changes(const String& uuid, const String& scene_path, const Dictionary& new_composition) {
		return _submit_composition_changes_internal(uuid, scene_path, new_composition, false);
	}

	Dictionary LTECompositionServer::_submit_composition_changes_internal(const String& uuid, const String& scene_path, const Dictionary& new_composition, const bool normalize) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return Dictionary();
		}
		int index = _find_opened_composition_index(uuid, scene_path);
		Dictionary opened_composition = index >= 0 ? Dictionary(opened_compositions[index]) : _record_composition_data(uuid, scene_path);
		String absolute_scene_path = opened_composition.get("path", _get_absolute_scene_path(scene_path));
		Dictionary submitted_composition = normalize ? _normalize_composition_data(new_composition) : new_composition.duplicate(true);
		evaluation_composition_cache[absolute_scene_path] = submitted_composition;
		Dictionary result;
		int64_t save_revision = ++composition_save_revision_counter;
		for (int opened_index = 0; opened_index < opened_compositions.size(); ++opened_index) {
			Dictionary current_composition = opened_compositions[opened_index];
			if (_get_absolute_scene_path(current_composition.get("path", String())) != absolute_scene_path) {
				continue;
			}
			current_composition["composition"] = submitted_composition.duplicate(true);
			current_composition["saved"] = false;
			current_composition["save_revision"] = save_revision;
			opened_compositions[opened_index] = current_composition;
			if (String(current_composition.get("uuid", String())) == uuid) {
				result = current_composition.duplicate(true);
			}
			emit_signal("composition_changed", current_composition.get("uuid", String()), absolute_scene_path, current_composition.duplicate(false));
		}
		if (_is_scene_auto_save_enabled()) {
			_queue_composition_save(absolute_scene_path, submitted_composition, save_revision);
		}
		return result.is_empty() ? opened_composition.duplicate(true) : result;
	}

	Dictionary LTECompositionServer::apply_composition_keyframe_deltas(const String& uuid, const String& scene_path, const Array& deltas, const bool use_after) {
		COMP_PERF_TRACE_BEGIN(apply_deltas);
		if (uuid.is_empty() || scene_path.is_empty() || deltas.is_empty()) {
			return Dictionary();
		}

		int index = _find_opened_composition_index(uuid, scene_path);
		Dictionary opened_composition = index >= 0 ? Dictionary(opened_compositions[index]) : _record_composition_data(uuid, scene_path);
		String absolute_scene_path = opened_composition.get("path", _get_absolute_scene_path(scene_path));
		if (absolute_scene_path.is_empty()) {
			return Dictionary();
		}

		COMP_PERF_TRACE_BEGIN(duplicate_comp);
		Dictionary composition = Dictionary(opened_composition.get("composition", Dictionary())).duplicate(false);
		COMP_PERF_TRACE_END(duplicate_comp, "apply_composition_keyframe_deltas:duplicate(composition)");

		COMP_PERF_TRACE_BEGIN(apply_inner);
		if (!_apply_composition_keyframe_deltas(composition, deltas, use_after)) {
			COMP_PERF_TRACE_END(apply_inner, "apply_composition_keyframe_deltas:_apply_deltas(no_change)");
			COMP_PERF_TRACE_END(apply_deltas, "apply_composition_keyframe_deltas:total(no_change)");
			return opened_composition.duplicate(true);
		}
		COMP_PERF_TRACE_END(apply_inner, "apply_composition_keyframe_deltas:_apply_deltas");
		evaluation_composition_cache[absolute_scene_path] = composition;

		Dictionary result;
		int64_t save_revision = ++composition_save_revision_counter;
		for (int opened_index = 0; opened_index < opened_compositions.size(); ++opened_index) {
			Dictionary current_composition = opened_compositions[opened_index];
			if (_get_absolute_scene_path(current_composition.get("path", String())) != absolute_scene_path) {
				continue;
			}
			current_composition["composition"] = composition.duplicate(true);
			current_composition["saved"] = false;
			current_composition["save_revision"] = save_revision;
			opened_compositions[opened_index] = current_composition;
			if (String(current_composition.get("uuid", String())) == uuid) {
				result = current_composition.duplicate(true);
			}
			emit_signal("composition_changed", current_composition.get("uuid", String()), absolute_scene_path, current_composition.duplicate(false));
		}
		if (_is_scene_auto_save_enabled()) {
			COMP_PERF_TRACE_BEGIN(queue_save);
			_queue_composition_save(absolute_scene_path, composition, save_revision);
			COMP_PERF_TRACE_END(queue_save, "apply_composition_keyframe_deltas:_queue_save");
		}
		COMP_PERF_TRACE_END(apply_deltas, "apply_composition_keyframe_deltas:total");
		return result.is_empty() ? opened_composition.duplicate(true) : result;
	}

	bool LTECompositionServer::_apply_composition_keyframe_deltas(Dictionary& composition, const Array& deltas, const bool use_after) const {
		if (deltas.size() >= 2) {
			return _apply_composition_keyframe_deltas_batched(composition, deltas, use_after);
		}
		if (deltas.size() == 1 && deltas[0].get_type() == Variant::DICTIONARY) {
			return _apply_composition_keyframe_delta(composition, Dictionary(deltas[0]), use_after);
		}
		return false;
	}

	bool LTECompositionServer::_apply_composition_keyframe_deltas_batched(Dictionary& composition, const Array& deltas, const bool use_after) const {
		Dictionary grouped;
		for (int i = 0; i < deltas.size(); ++i) {
			if (deltas[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary delta = deltas[i];
			String layer_id = delta.get("layer_id", String());
			String property_name = delta.get("property", String());
			if (layer_id.is_empty() || property_name.is_empty()) {
				continue;
			}
			String group_key = layer_id + "\n" + property_name;
			Array group = grouped.get(group_key, Array());
			group.append(delta);
			grouped[group_key] = group;
		}
		if (grouped.is_empty()) {
			return false;
		}

		Array layers = Array(composition.get("layers", Array())).duplicate(false);
		Dictionary layer_index_map;
		for (int i = 0; i < layers.size(); ++i) {
			if (layers[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			String id = Dictionary(layers[i]).get("id", String());
			if (!id.is_empty()) {
				layer_index_map[id] = i;
			}
		}

		bool changed = false;
		Array group_keys = grouped.keys();
		for (int g = 0; g < group_keys.size(); ++g) {
			String group_key = group_keys[g];
			Array group_deltas = grouped[group_key];
			if (group_deltas.is_empty()) {
				continue;
			}
			PackedStringArray parts = group_key.split("\n");
			if (parts.size() != 2) {
				continue;
			}
			String layer_id = parts[0];
			String property_name = parts[1];

			if (!layer_index_map.has(layer_id)) {
				continue;
			}
			int32_t layer_index = int(layer_index_map[layer_id]);
			if (layer_index < 0 || layer_index >= layers.size() || layers[layer_index].get_type() != Variant::DICTIONARY) {
				continue;
			}

			Dictionary layer = Dictionary(layers[layer_index]).duplicate(false);
			if (bool(layer.get("locked", false))) {
				continue;
			}

			Array property_tracks = Array(layer.get("property_tracks", Array())).duplicate(false);
			int32_t property_track_index = _find_property_track_index(property_tracks, property_name);

			bool need_create_track = false;
			for (int d = 0; d < group_deltas.size(); ++d) {
				Dictionary delta = group_deltas[d];
				Variant target_value = delta.get(use_after ? "after" : "before", Variant());
				if (target_value.get_type() == Variant::DICTIONARY) {
					need_create_track = true;
					break;
				}
			}
			if (!need_create_track && property_track_index < 0) {
				continue;
			}

			Dictionary property_track;
			if (property_track_index >= 0 && property_tracks[property_track_index].get_type() == Variant::DICTIONARY) {
				property_track = Dictionary(property_tracks[property_track_index]).duplicate(false);
			}
			else if (need_create_track) {
				property_track["id"] = Utils::uuid(Utils::UUID_V7);
				property_track["layer_id"] = layer_id;
				property_track["property"] = property_name;
				property_track["keyframes"] = Array();
				property_track["modifiers"] = Array();
				if (layer.has(property_name)) {
					property_track["default_value"] = layer.get(property_name, Variant());
				}
				property_tracks.append(property_track);
				property_track_index = property_tracks.size() - 1;
			}

			Array keyframes = Array(property_track.get("keyframes", Array())).duplicate(false);

			for (int d = 0; d < group_deltas.size(); ++d) {
				Dictionary delta = group_deltas[d];
				Variant target_value = delta.get(use_after ? "after" : "before", Variant());
				Variant reference_value = delta.get(use_after ? "before" : "after", Variant());
				String keyframe_id = delta.get("keyframe_id", String());
				double reference_time = 0.0;
				bool use_reference_time = false;

				if (reference_value.get_type() == Variant::DICTIONARY) {
					Dictionary reference_keyframe = reference_value;
					if (keyframe_id.is_empty()) {
						keyframe_id = reference_keyframe.get("id", String());
					}
					reference_time = double(reference_keyframe.get("time", 0.0));
					use_reference_time = true;
				}
				if (target_value.get_type() == Variant::DICTIONARY) {
					Dictionary target_keyframe_dict = target_value;
					if (keyframe_id.is_empty()) {
						keyframe_id = target_keyframe_dict.get("id", String());
					}
				}

				int32_t keyframe_index = _find_keyframe_index(keyframes, keyframe_id, reference_time, use_reference_time);
				if (target_value.get_type() == Variant::DICTIONARY) {
					Dictionary target_keyframe = _normalize_keyframe(target_value);
					if (target_keyframe.is_empty()) {
						continue;
					}
					if (String(target_keyframe.get("id", String())).is_empty()) {
						target_keyframe["id"] = keyframe_id.is_empty() ? Utils::uuid(Utils::UUID_V7) : keyframe_id;
					}
					if (keyframe_index >= 0) {
						keyframes[keyframe_index] = target_keyframe;
					}
					else {
						keyframes.append(target_keyframe);
					}
					changed = true;
				}
				else {
					if (keyframe_index < 0) {
						continue;
					}
					keyframes.remove_at(keyframe_index);
					changed = true;
				}
			}

			Array normalized_keyframes = _normalize_keyframes(keyframes);
			property_track["keyframes"] = normalized_keyframes;
			Array modifiers = property_track.get("modifiers", Array());
			if (normalized_keyframes.is_empty() && modifiers.is_empty()) {
				property_tracks.remove_at(property_track_index);
			}
			else {
				property_track["layer_id"] = layer_id;
				property_track["property"] = property_name;
				property_tracks[property_track_index] = property_track;
			}
			layer["property_tracks"] = property_tracks;
			layers[layer_index] = layer;
		}

		if (changed) {
			composition["layers"] = layers;
		}
		return changed;
	}

	bool LTECompositionServer::_apply_composition_keyframe_delta(Dictionary& composition, const Dictionary& delta, const bool use_after) const {
		String layer_id = delta.get("layer_id", String());
		String property_name = delta.get("property", String());
		if (layer_id.is_empty() || property_name.is_empty()) {
			return false;
		}

		Array layers = Array(composition.get("layers", Array())).duplicate(false);
		int32_t layer_index = _find_layer_index_by_id(layers, layer_id);
		if (layer_index < 0 || layers[layer_index].get_type() != Variant::DICTIONARY) {
			return false;
		}

		Dictionary layer = Dictionary(layers[layer_index]).duplicate(false);
		if (bool(layer.get("locked", false))) {
			return false;
		}

		Variant target_value = delta.get(use_after ? "after" : "before", Variant());
		Variant reference_value = delta.get(use_after ? "before" : "after", Variant());
		String keyframe_id = delta.get("keyframe_id", String());
		double reference_time = 0.0;
		bool use_reference_time = false;
		if (reference_value.get_type() == Variant::DICTIONARY) {
			Dictionary reference_keyframe = reference_value;
			if (keyframe_id.is_empty()) {
				keyframe_id = reference_keyframe.get("id", String());
			}
			reference_time = double(reference_keyframe.get("time", 0.0));
			use_reference_time = true;
		}
		if (target_value.get_type() == Variant::DICTIONARY) {
			Dictionary target_keyframe = target_value;
			if (keyframe_id.is_empty()) {
				keyframe_id = target_keyframe.get("id", String());
			}
		}

		Array property_tracks = Array(layer.get("property_tracks", Array())).duplicate(false);
		int32_t property_track_index = _find_property_track_index(property_tracks, property_name);
		if (target_value.get_type() != Variant::DICTIONARY && property_track_index < 0) {
			return false;
		}

		Dictionary property_track;
		if (property_track_index >= 0 && property_tracks[property_track_index].get_type() == Variant::DICTIONARY) {
			property_track = Dictionary(property_tracks[property_track_index]).duplicate(false);
		}
		else {
			property_track["id"] = Utils::uuid(Utils::UUID_V7);
			property_track["layer_id"] = layer_id;
			property_track["property"] = property_name;
			property_track["keyframes"] = Array();
			property_track["modifiers"] = Array();
			if (layer.has(property_name)) {
				property_track["default_value"] = layer.get(property_name, Variant());
			}
			property_tracks.append(property_track);
			property_track_index = property_tracks.size() - 1;
		}

		Array keyframes = Array(property_track.get("keyframes", Array())).duplicate(false);
		int32_t keyframe_index = _find_keyframe_index(keyframes, keyframe_id, reference_time, use_reference_time);
		if (target_value.get_type() == Variant::DICTIONARY) {
			Dictionary target_keyframe = _normalize_keyframe(target_value);
			if (target_keyframe.is_empty()) {
				return false;
			}
			if (String(target_keyframe.get("id", String())).is_empty()) {
				target_keyframe["id"] = keyframe_id.is_empty() ? Utils::uuid(Utils::UUID_V7) : keyframe_id;
			}
			if (keyframe_index >= 0) {
				keyframes[keyframe_index] = target_keyframe;
			}
			else {
				keyframes.append(target_keyframe);
			}
		}
		else {
			if (keyframe_index < 0) {
				return false;
			}
			keyframes.remove_at(keyframe_index);
		}

		Array normalized_keyframes = _normalize_keyframes(keyframes);
		property_track["layer_id"] = layer_id;
		property_track["property"] = property_name;
		property_track["keyframes"] = normalized_keyframes;
		Array modifiers = property_track.get("modifiers", Array());
		if (normalized_keyframes.is_empty() && modifiers.is_empty()) {
			property_tracks.remove_at(property_track_index);
		}
		else {
			property_tracks[property_track_index] = property_track;
		}
		layer["property_tracks"] = property_tracks;
		layers[layer_index] = layer;
		composition["layers"] = layers;
		return true;
	}

	int32_t LTECompositionServer::_find_layer_index_by_id(const Array& layers, const String& layer_id) const {
		if (layer_id.is_empty()) {
			return -1;
		}
		for (int index = 0; index < layers.size(); ++index) {
			if (layers[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary layer = layers[index];
			if (String(layer.get("id", String())) == layer_id) {
				return index;
			}
		}
		return -1;
	}

	int32_t LTECompositionServer::_find_property_track_index(const Array& property_tracks, const String& property_name) const {
		if (property_name.is_empty()) {
			return -1;
		}
		for (int index = 0; index < property_tracks.size(); ++index) {
			if (property_tracks[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary property_track = property_tracks[index];
			if (String(property_track.get("property", String())) == property_name) {
				return index;
			}
		}
		return -1;
	}

	int32_t LTECompositionServer::_find_keyframe_index(const Array& keyframes, const String& keyframe_id, const double fallback_time, const bool use_fallback_time) const {
		if (!keyframe_id.is_empty()) {
			for (int index = 0; index < keyframes.size(); ++index) {
				if (keyframes[index].get_type() != Variant::DICTIONARY) {
					continue;
				}
				if (String(Dictionary(keyframes[index]).get("id", String())) == keyframe_id) {
					return index;
				}
			}
			return -1;
		}
		if (use_fallback_time) {
			int32_t lo = 0;
			int32_t hi = keyframes.size() - 1;
			while (lo <= hi) {
				int32_t mid = lo + (hi - lo) / 2;
				if (keyframes[mid].get_type() != Variant::DICTIONARY) {
					for (int index = 0; index < keyframes.size(); ++index) {
						if (keyframes[index].get_type() != Variant::DICTIONARY) {
							continue;
						}
						if (std::abs(double(Dictionary(keyframes[index]).get("time", 0.0)) - fallback_time) <= 0.0001) {
							return index;
						}
					}
					return -1;
				}
				double mid_time = double(Dictionary(keyframes[mid]).get("time", 0.0));
				if (std::abs(mid_time - fallback_time) <= 0.0001) {
					return mid;
				}
				if (mid_time < fallback_time) {
					lo = mid + 1;
				}
				else {
					hi = mid - 1;
				}
			}
			return -1;
		}
		return -1;
	}

	Dictionary LTECompositionServer::save_composition(const String& uuid, const String& scene_path) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return Dictionary();
		}
		int index = _find_opened_composition_index(uuid, scene_path);
		Dictionary opened_composition = index >= 0 ? Dictionary(opened_compositions[index]) : _record_composition_data(uuid, scene_path);
		Dictionary composition = _normalize_composition_data(opened_composition.get("composition", Dictionary()));
		String absolute_scene_path = opened_composition.get("path", _get_absolute_scene_path(scene_path));
		evaluation_composition_cache[absolute_scene_path] = composition;
		Error error = _save_composition(absolute_scene_path, composition);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to save composition: %s. Error Code: %d", absolute_scene_path, error));
			return Dictionary();
		}
		Dictionary result;
		for (int opened_index = 0; opened_index < opened_compositions.size(); ++opened_index) {
			Dictionary current_composition = opened_compositions[opened_index];
			if (_get_absolute_scene_path(current_composition.get("path", String())) != absolute_scene_path) {
				continue;
			}
			current_composition["composition"] = composition.duplicate(true);
			current_composition["saved"] = true;
			opened_compositions[opened_index] = current_composition;
			if (String(current_composition.get("uuid", String())) == uuid) {
				result = current_composition.duplicate(true);
			}
		}
		emit_signal("composition_saved", absolute_scene_path);
		for (int opened_index = 0; opened_index < opened_compositions.size(); ++opened_index) {
			Dictionary current_composition = opened_compositions[opened_index];
			if (_get_absolute_scene_path(current_composition.get("path", String())) == absolute_scene_path) {
				emit_signal("composition_changed", current_composition.get("uuid", String()), absolute_scene_path, current_composition.duplicate(true));
			}
		}
		return result.is_empty() ? opened_composition.duplicate(true) : result;
	}

	Array LTECompositionServer::_evaluate_composition_internal(const String& scene_path, const double time, Array visited_paths) const {
		String absolute_scene_path = _get_absolute_scene_path(scene_path);
		if (absolute_scene_path.is_empty() || visited_paths.has(absolute_scene_path)) {
			return Array();
		}
		visited_paths.append(absolute_scene_path);
		Dictionary composition = _get_composition_for_evaluation(absolute_scene_path);
		return _evaluate_composition_data_internal(composition, absolute_scene_path, time, visited_paths);
	}

	Array LTECompositionServer::_evaluate_composition_data_internal(const Dictionary& composition, const String& owner_path, const double time, Array visited_paths) const {
		if (!owner_path.is_empty() && !visited_paths.has(owner_path)) {
			visited_paths.append(owner_path);
		}
		Array layers = composition.get("layers", Array());
		Array global_tracks = composition.get("property_tracks", Array());
		Array states;
		for (int index = 0; index < layers.size(); ++index) {
			if (layers[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary layer = layers[index];
			Dictionary state = layer.duplicate(true);
			String layer_id = state.get("id", String());
			_apply_property_tracks(state, state.get("property_tracks", Array()), layer_id, time);
			_apply_property_tracks(state, global_tracks, layer_id, time);
			state["index"] = index;
			states.append(state);
			if (String(state.get("type", String())) == "sequence") {
				_append_sequence_child_states(states, state, time, visited_paths);
			}
		}
		return states;
	}

	void LTECompositionServer::_append_sequence_child_states(Array& states, const Dictionary& state, const double time, Array visited_paths) const {
		if (!bool(state.get("visible", true))) {
			return;
		}
		String sequence_path = String(state.get("sequence_path", String()));
		String sequence_id = String(state.get("id", String()));
		Array clips = state.get("clips", Array());
		if (clips.is_empty()) {
			return;
		}
		for (int clip_index = 0; clip_index < clips.size(); ++clip_index) {
			if (clips[clip_index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary clip = clips[clip_index];
			double start_time = double(clip.get("start_time", state.get("start_time", 0.0)));
			double duration = double(clip.get("duration", state.get("duration", 0.0)));
			if (duration > 0.0 && (time < start_time || time > start_time + duration)) {
				continue;
			}
			double time_scale = double(clip.get("time_scale", state.get("time_scale", 1.0)));
			if (std::abs(time_scale) < 0.000001) {
				time_scale = 1.0;
			}
			double local_time = (time - start_time) * time_scale + double(clip.get("time_offset", state.get("time_offset", 0.0)));
			String clip_id = String(clip.get("id", String::num_int64(clip_index)));
			Array child_visited_paths = visited_paths.duplicate(true);
			Array child_states;
			if (!sequence_path.is_empty()) {
				child_states = _evaluate_composition_internal(sequence_path, local_time, child_visited_paths);
			}
			else {
				Dictionary embedded_composition = _normalize_embedded_sequence_composition(state.get("composition", Dictionary()));
				child_states = _evaluate_composition_data_internal(embedded_composition, "embedded:" + sequence_id + ":" + clip_id, local_time, child_visited_paths);
			}
			for (int index = 0; index < child_states.size(); ++index) {
				if (child_states[index].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary child_state = child_states[index];
				_apply_sequence_parent_state(child_state, state);
				String child_id = String(child_state.get("id", String()));
				child_state["source_id"] = child_id;
				child_state["sequence_id"] = sequence_id;
				child_state["sequence_clip_id"] = clip_id;
				if (!sequence_path.is_empty()) {
					child_state["sequence_path"] = _get_absolute_scene_path(sequence_path);
				}
				if (!sequence_id.is_empty() && !child_id.is_empty()) {
					child_state["id"] = sequence_id + "/" + clip_id + "/" + child_id;
				}
				states.append(child_state);
			}
		}
	}

	void LTECompositionServer::_apply_sequence_parent_state(Dictionary& child_state, const Dictionary& parent_state) const {
		child_state["visible"] = bool(parent_state.get("visible", true)) && bool(child_state.get("visible", true));
		child_state["opacity"] = double(child_state.get("opacity", 1.0)) * double(parent_state.get("opacity", 1.0));
		Array child_position = _normalize_vector2_array(child_state.get("position", Array()), 0.0, 0.0);
		Array parent_position = _normalize_vector2_array(parent_state.get("position", Array()), 0.0, 0.0);
		Array child_scale = _normalize_vector2_array(child_state.get("scale", Array()), 1.0, 1.0);
		Array parent_scale = _normalize_vector2_array(parent_state.get("scale", Array()), 1.0, 1.0);
		double parent_rotation = double(parent_state.get("rotation", 0.0));
		constexpr double pi = 3.14159265358979323846;
		Vector2 transformed_position(
			double(child_position[0]) * double(parent_scale[0]),
			double(child_position[1]) * double(parent_scale[1]));
		transformed_position = transformed_position.rotated(float(parent_rotation * pi / 180.0));
		child_position[0] = double(parent_position[0]) + transformed_position.x;
		child_position[1] = double(parent_position[1]) + transformed_position.y;
		child_state["position"] = child_position;
		child_scale[0] = double(child_scale[0]) * double(parent_scale[0]);
		child_scale[1] = double(child_scale[1]) * double(parent_scale[1]);
		child_state["scale"] = child_scale;
		child_state["rotation"] = double(child_state.get("rotation", 0.0)) + parent_rotation;
	}

	Array LTECompositionServer::evaluate_composition(const String& scene_path, const double time) const {
		return _evaluate_composition_internal(scene_path, time, Array());
	}

	Dictionary LTECompositionServer::normalize_composition(const Variant& value) const {
		return _normalize_composition_data(value);
	}

	String LTECompositionServer::get_last_opened_composition(const String& uuid) const {
		if (uuid.is_empty()) {
			return String();
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return String();
		}
		return settings_config->composition_timeline_last_opened.get(uuid, String());
	}

	void LTECompositionServer::set_last_opened_composition(const String& uuid, const String& scene_path) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String absolute_scene_path = _get_absolute_scene_path(scene_path);
		if (String(settings_config->composition_timeline_last_opened.get(uuid, String())) == absolute_scene_path) {
			return;
		}
		settings_config->composition_timeline_last_opened[uuid] = absolute_scene_path;
		settings_config->save_settings_config(false);
	}

	Dictionary LTECompositionServer::fetch_timeline_config(const String& uuid, const String& scene_path) const {
		Dictionary config;
		config["snap_mode"] = true;
		config["counting_unit"] = "beat";
		config["per_bar"] = 4;
		config["step"] = 0.01;
		config["playhead_auto_scroll"] = true;
		config["auto_insert_keyframes_mode"] = false;
		config["scale"] = 96.0;
		config["h_scroll"] = 0;
		config["v_scroll"] = 0;
		if (uuid.is_empty() || scene_path.is_empty()) {
			return config;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return config;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary saved_config = scene_settings.get(scene_key, Dictionary());
		config["snap_mode"] = saved_config.get("snap_mode", config["snap_mode"]);
		config["counting_unit"] = saved_config.get("counting_unit", config["counting_unit"]);
		config["per_bar"] = saved_config.get("per_bar", config["per_bar"]);
		config["step"] = saved_config.get("step", config["step"]);
		config["playhead_auto_scroll"] = saved_config.get("playhead_auto_scroll", config["playhead_auto_scroll"]);
		config["auto_insert_keyframes_mode"] = saved_config.get("auto_insert_keyframes_mode", config["auto_insert_keyframes_mode"]);
		config["scale"] = saved_config.get("scale", config["scale"]);
		config["h_scroll"] = saved_config.get("h_scroll", config["h_scroll"]);
		config["v_scroll"] = saved_config.get("v_scroll", config["v_scroll"]);
		return config;
	}

	PackedStringArray LTECompositionServer::get_scene_layers_collapsed_items(const String& uuid, const String& scene_path) const {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return PackedStringArray();
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return PackedStringArray();
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		Variant collapsed_items = config.get("scene_layers_collapsed_items", PackedStringArray());
		if (collapsed_items.get_type() == Variant::PACKED_STRING_ARRAY) {
			return collapsed_items;
		}
		PackedStringArray result;
		if (collapsed_items.get_type() == Variant::ARRAY) {
			Array source_items = collapsed_items;
			for (int index = 0; index < source_items.size(); ++index) {
				result.append(String(source_items[index]));
			}
		}
		return result;
	}

	void LTECompositionServer::set_scene_layers_collapsed_items(const String& uuid, const String& scene_path, const PackedStringArray& collapsed_items) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		config["scene_layers_collapsed_items"] = collapsed_items;
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config(false);
	}

	void LTECompositionServer::set_timeline_snap_mode(const String& uuid, const String& scene_path, const bool enable) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		config["snap_mode"] = enable;
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config();
	}

	void LTECompositionServer::set_timeline_step(const String& uuid, const String& scene_path, const double step) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		config["step"] = std::max(step, 0.000001);
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config();
	}

	void LTECompositionServer::set_timeline_counting_unit(const String& uuid, const String& scene_path, const String& counting_unit) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String normalized_counting_unit = counting_unit == "time" ? String("time") : String("beat");
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		config["counting_unit"] = normalized_counting_unit;
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config();
	}

	void LTECompositionServer::set_timeline_per_bar(const String& uuid, const String& scene_path, const int per_bar) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		config["per_bar"] = std::max(per_bar, 1);
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config();
	}

	void LTECompositionServer::set_timeline_playhead_auto_scroll(const String& uuid, const String& scene_path, const bool enable) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		config["playhead_auto_scroll"] = enable;
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config();
	}

	void LTECompositionServer::set_timeline_auto_insert_keyframes_mode(const String& uuid, const String& scene_path, const bool enable) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		config["auto_insert_keyframes_mode"] = enable;
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config();
	}

	void LTECompositionServer::set_timeline_view(const String& uuid, const String& scene_path, const double scale, const int h_scroll, const int v_scroll) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary scene_settings = settings_config->composition_timeline_scene_settings.get(uuid, Dictionary());
		Dictionary config = scene_settings.get(scene_key, Dictionary());
		double next_scale = std::max(scale, 0.000001);
		int next_h_scroll = std::max(h_scroll, 0);
		int next_v_scroll = std::max(v_scroll, 0);
		if (std::abs(double(config.get("scale", 0.0)) - next_scale) <= 0.000001 &&
			int(config.get("h_scroll", 0)) == next_h_scroll &&
			int(config.get("v_scroll", 0)) == next_v_scroll) {
			return;
		}
		config["scale"] = next_scale;
		config["h_scroll"] = next_h_scroll;
		config["v_scroll"] = next_v_scroll;
		scene_settings[scene_key] = config;
		settings_config->composition_timeline_scene_settings[uuid] = scene_settings;
		settings_config->save_settings_config(false);
	}
}
