#include "source_monitor_server.h"

#include "project_manager.h"

#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTESourceMonitorServer* LTESourceMonitorServer::singleton = nullptr;

	void LTESourceMonitorServer::_bind_methods() {
		BIND_ENUM_CONSTANT(EASY);
		BIND_ENUM_CONSTANT(HARD);
		BIND_ENUM_CONSTANT(EXPERT);
		BIND_ENUM_CONSTANT(MASTER);
		BIND_ENUM_CONSTANT(ERROR_DIFFICULTY);
		ClassDB::bind_method(D_METHOD("fetch_monitor_config", "runtime_uuid"), &LTESourceMonitorServer::fetch_monitor_config);
		ClassDB::bind_method(D_METHOD("get_monitor_chart_time", "runtime_uuid", "chart_path"), &LTESourceMonitorServer::get_monitor_chart_time);
		ClassDB::bind_method(D_METHOD("set_monitor_chart_time", "runtime_uuid", "chart_path", "time"), &LTESourceMonitorServer::set_monitor_chart_time);
		ClassDB::bind_method(D_METHOD("get_game_blackground_light_color", "difficulty"), &LTESourceMonitorServer::get_game_blackground_light_color);
		ClassDB::bind_method(D_METHOD("open_game", "uuid", "game_scene", "difficulty"), &LTESourceMonitorServer::open_game);
		ClassDB::bind_method(D_METHOD("open_game_by_chart_path", "uuid", "game_scene", "chart_path"), &LTESourceMonitorServer::open_game_by_chart_path);
		ClassDB::bind_method(D_METHOD("scan_difficulty_from_chart_path", "chart_path"), &LTESourceMonitorServer::scan_difficulty_from_chart_path);
		ClassDB::bind_method(D_METHOD("get_chart_difficulty_name", "chart_path"), &LTESourceMonitorServer::get_chart_difficulty_name);
		ClassDB::bind_method(D_METHOD("get_game"), &LTESourceMonitorServer::get_game);
		ClassDB::bind_method(D_METHOD("humanize_difficulty", "difficulty"), &LTESourceMonitorServer::humanize_difficulty);
		ClassDB::bind_method(D_METHOD("get_difficulty", "diff_str"), &LTESourceMonitorServer::get_difficulty);
		ClassDB::bind_method(D_METHOD("get_game_status", "difficulty"), &LTESourceMonitorServer::get_game_status);
		ClassDB::bind_method(D_METHOD("get_game_status_by_chart_path", "chart_path"), &LTESourceMonitorServer::get_game_status_by_chart_path);
		ClassDB::bind_method(D_METHOD("get_game_time", "difficulty"), &LTESourceMonitorServer::get_game_time);
		ClassDB::bind_method(D_METHOD("get_game_time_by_chart_path", "chart_path"), &LTESourceMonitorServer::get_game_time_by_chart_path);
		ClassDB::bind_method(D_METHOD("game_hot_reload", "difficulty"), &LTESourceMonitorServer::game_hot_reload);
		ClassDB::bind_method(D_METHOD("game_hot_reload_by_chart_path", "chart_path"), &LTESourceMonitorServer::game_hot_reload_by_chart_path);
		ClassDB::bind_method(D_METHOD("game_hot_reload_skin", "difficulty", "skin_path"), &LTESourceMonitorServer::game_hot_reload_skin, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("game_hot_reload_skin_by_chart_path", "chart_path", "skin_path"), &LTESourceMonitorServer::game_hot_reload_skin_by_chart_path, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("game_play", "difficulty", "from_sec", "display_time_scale", "display_time_offset", "display_duration", "display_scene_path"), &LTESourceMonitorServer::game_play, DEFVAL(0.0), DEFVAL(1.0), DEFVAL(0.0), DEFVAL(-1.0), DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("game_play_by_chart_path", "chart_path", "from_sec", "display_time_scale", "display_time_offset", "display_duration", "display_scene_path"), &LTESourceMonitorServer::game_play_by_chart_path, DEFVAL(0.0), DEFVAL(1.0), DEFVAL(0.0), DEFVAL(-1.0), DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("game_pause", "difficulty"), &LTESourceMonitorServer::game_pause);
		ClassDB::bind_method(D_METHOD("game_pause_by_chart_path", "chart_path"), &LTESourceMonitorServer::game_pause_by_chart_path);
		ClassDB::bind_method(D_METHOD("game_scrub_to_frame", "difficulty", "time", "scrub_audio", "continue_play", "origin", "display_time_scale", "display_time_offset", "display_duration", "display_scene_path"), &LTESourceMonitorServer::game_scrub_to_frame, DEFVAL(true), DEFVAL(true), DEFVAL(String()), DEFVAL(1.0), DEFVAL(0.0), DEFVAL(-1.0), DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("game_scrub_to_frame_by_chart_path", "chart_path", "time", "scrub_audio", "continue_play", "origin", "display_time_scale", "display_time_offset", "display_duration", "display_scene_path"), &LTESourceMonitorServer::game_scrub_to_frame_by_chart_path, DEFVAL(true), DEFVAL(true), DEFVAL(String()), DEFVAL(1.0), DEFVAL(0.0), DEFVAL(-1.0), DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("get_game_instance_window_id", "runtime_uuid"), &LTESourceMonitorServer::get_game_instance_window_id);
		ClassDB::bind_method(D_METHOD("run_game_instance_on_window", "difficulty", "runtime_uuid", "from_sec"), &LTESourceMonitorServer::run_game_instance_on_window, DEFVAL(0.0));
		ClassDB::bind_method(D_METHOD("run_game_instance_on_window_by_chart_path", "chart_path", "runtime_uuid", "from_sec"), &LTESourceMonitorServer::run_game_instance_on_window_by_chart_path, DEFVAL(0.0));
		ClassDB::bind_method(D_METHOD("stop_game_instance_on_window", "window_id"), &LTESourceMonitorServer::stop_game_instance_on_window);
		ClassDB::bind_method(D_METHOD("_on_runtime_window_tree_exiting", "window_id"), &LTESourceMonitorServer::_on_runtime_window_tree_exiting);
		ClassDB::bind_method(D_METHOD("enable_monitor_judgement_line", "uuid", "enable"), &LTESourceMonitorServer::enable_monitor_judgement_line);
		ClassDB::bind_method(D_METHOD("set_monitor_ratio", "uuid", "ratio"), &LTESourceMonitorServer::set_monitor_ratio);
		ClassDB::bind_method(D_METHOD("enable_monitor_clear_color", "uuid", "enable"), &LTESourceMonitorServer::enable_monitor_clear_color);
		ClassDB::bind_method(D_METHOD("enable_monitor_auto_play", "uuid", "enable"), &LTESourceMonitorServer::enable_monitor_auto_play);
		ClassDB::bind_method(D_METHOD("enable_monitor_debug_info", "uuid", "enable"), &LTESourceMonitorServer::enable_monitor_debug_info);
		ClassDB::bind_method(D_METHOD("enable_monitor_smart_guides", "uuid", "enable"), &LTESourceMonitorServer::enable_monitor_smart_guides);
		ClassDB::bind_method(D_METHOD("set_monitor_snap_settings", "uuid", "snap_settings"), &LTESourceMonitorServer::set_monitor_snap_settings);
		ClassDB::bind_method(D_METHOD("set_monitor_screen", "uuid", "screen"), &LTESourceMonitorServer::set_monitor_screen);
		ClassDB::bind_method(D_METHOD("set_monitor_window_mode", "uuid", "mode"), &LTESourceMonitorServer::set_monitor_window_mode);
		ADD_SIGNAL(MethodInfo("game_opened", PropertyInfo(Variant::INT, "difficulty", PROPERTY_HINT_ENUM, "Easy,Hard,Expert,Master"), PropertyInfo(Variant::DICTIONARY, "status")));
		ADD_SIGNAL(MethodInfo("game_playing", PropertyInfo(Variant::INT, "difficulty", PROPERTY_HINT_ENUM, "Easy,Hard,Expert,Master"), PropertyInfo(Variant::FLOAT, "form_sec"), PropertyInfo(Variant::DICTIONARY, "current_status")));
		ADD_SIGNAL(MethodInfo("game_paused", PropertyInfo(Variant::INT, "difficulty", PROPERTY_HINT_ENUM, "Easy,Hard,Expert,Master"), PropertyInfo(Variant::DICTIONARY, "current_status")));
		ADD_SIGNAL(MethodInfo("game_scrubbing", PropertyInfo(Variant::INT, "difficulty", PROPERTY_HINT_ENUM, "Easy,Hard,Expert,Master"), PropertyInfo(Variant::FLOAT, "curr_sec"), PropertyInfo(Variant::DICTIONARY, "current_status")));
		ADD_SIGNAL(MethodInfo("game_instance_running", PropertyInfo(Variant::INT, "window_id")));
		ADD_SIGNAL(MethodInfo("game_instance_stopped", PropertyInfo(Variant::INT, "window_id")));
	}

	LTESourceMonitorServer::LTESourceMonitorServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
		settings_config = LTESettingsConfig::get_singleton();
		workspace_manager = LTEWorkspaceManager::get_singleton();
		file_system = LTEFileSystemServer::get_singleton();
	}

	LTESourceMonitorServer::~LTESourceMonitorServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
		settings_config = nullptr;
		workspace_manager = nullptr;
		file_system = nullptr;
		game_scene_instance_id = 0;
		status_list.clear();
		game_scene_instance_ids.clear();
		chart_last_play_msec.clear();
		chart_last_pause_msec.clear();
		runtime_instances.clear();
		runtime_instance_uuids.clear();
	}

	LTESourceMonitorServer* LTESourceMonitorServer::get_singleton() {
		return singleton;
	}

	void LTESourceMonitorServer::clear_project_state() {
		game_scene_instance_id = 0;
		status_list.clear();
		game_scene_instance_ids.clear();
		chart_last_play_msec.clear();
		chart_last_pause_msec.clear();
		runtime_instances.clear();
		runtime_instance_uuids.clear();
	}

	int32_t LTESourceMonitorServer::_find_status_index(const int32_t difficulty) const {
		int32_t target_diff = difficulty;
		for (int32_t index = 0; index < status_list.size(); ++index) {
			Dictionary status_dict = status_list[index];
			int32_t stored_diff = status_dict.get("difficulty", -1);
			if (stored_diff == target_diff) {
				return index;
			}
		}
		return -1;
	}

	int32_t LTESourceMonitorServer::_find_status_index_by_chart_path(const String& chart_path) const {
		String target_chart_path = _get_absolute_chart_path(chart_path);
		if (target_chart_path.is_empty()) {
			return -1;
		}
		for (int32_t index = 0; index < status_list.size(); ++index) {
			Dictionary status_dict = status_list[index];
			String stored_chart_path = status_dict.get("chart_abs_path", String());
			if (stored_chart_path.is_empty()) {
				stored_chart_path = status_dict.get("chart_path", String());
			}
			if (_get_absolute_chart_path(stored_chart_path) == target_chart_path) {
				return index;
			}
		}
		return -1;
	}

	String LTESourceMonitorServer::_get_absolute_chart_path(const String& chart_path) const {
		String normalized_path = chart_path.replace("\\", "/");
		if (normalized_path.is_empty()) {
			return String();
		}
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_dir = project_manager ? project_manager->get_project_path() : String();
		if (normalized_path.begins_with("/") && !root_dir.is_empty()) {
			return (root_dir + normalized_path).simplify_path();
		}
		if (Utils::is_absolute_path(normalized_path)) {
			return normalized_path.simplify_path();
		}
		if (root_dir.is_empty()) {
			return normalized_path.simplify_path();
		}
		return root_dir.path_join(normalized_path).simplify_path();
	}

	String LTESourceMonitorServer::_get_relative_chart_path(const String& chart_path) const {
		String abs_chart_path = _get_absolute_chart_path(chart_path);
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_dir = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (!root_dir.is_empty() && abs_chart_path.begins_with(root_dir + "/")) {
			return abs_chart_path.substr(root_dir.length());
		}
		return chart_path.replace("\\", "/").simplify_path();
	}

	String LTESourceMonitorServer::_scan_difficulty_name_from_chart_path(const String& chart_path) const {
		String chart_rela_path = _get_relative_chart_path(chart_path);
		Dictionary track_info = file_system->get_track_info();
		Dictionary difficulty_dict = track_info.get("difficulty", Dictionary());
		Array difficulty_keys = difficulty_dict.keys();
		for (int index = 0; index < difficulty_keys.size(); ++index) {
			String key = difficulty_keys[index];
			String path = difficulty_dict.get(key, String());
			if (_get_relative_chart_path(path) == chart_rela_path) {
				return key.to_lower();
			}
		}
		return String();
	}

	String LTESourceMonitorServer::_get_chart_path_for_difficulty(const Dictionary& difficulty_dict, const GameDifficulty difficulty) const {
		String base_name = humanize_difficulty(difficulty);
		if (base_name.is_empty()) {
			return String();
		}
		String chart_path = difficulty_dict.get(base_name, String());
		if (!chart_path.is_empty()) {
			return chart_path;
		}

		Array preferred_keys;
		preferred_keys.append(String("4k") + base_name);
		preferred_keys.append(String("6k") + base_name);
		preferred_keys.append(String("8k") + base_name);
		preferred_keys.append(String("lt") + base_name);
		for (int index = 0; index < preferred_keys.size(); ++index) {
			String key = preferred_keys[index];
			chart_path = difficulty_dict.get(key, String());
			if (!chart_path.is_empty()) {
				return chart_path;
			}
		}

		Array difficulty_keys = difficulty_dict.keys();
		for (int index = 0; index < difficulty_keys.size(); ++index) {
			String key = difficulty_keys[index];
			key = key.to_lower();
			if (!key.ends_with(base_name)) {
				continue;
			}
			chart_path = difficulty_dict.get(difficulty_keys[index], String());
			if (!chart_path.is_empty()) {
				return chart_path;
			}
		}
		return String();
	}

	LTESourceMonitorServer::GameDifficulty LTESourceMonitorServer::_get_difficulty_from_name(const String& difficulty_name) const {
		String lower_name = difficulty_name.to_lower();
		if (lower_name.ends_with("easy")) {
			return EASY;
		}
		else if (lower_name.ends_with("hard")) {
			return HARD;
		}
		else if (lower_name.ends_with("expert")) {
			return EXPERT;
		}
		else if (lower_name.ends_with("master")) {
			return MASTER;
		}
		return ERROR_DIFFICULTY;
	}

	Control* LTESourceMonitorServer::_get_game_from_instance_id(const uint64_t instance_id, const bool quiet) const {
		Object* object = ObjectDB::get_instance(instance_id);
		Control* game = Object::cast_to<Control>(object);
		if (!game || game->is_queued_for_deletion() || !game->is_inside_tree()) {
			if (!quiet) {
				ERR_PRINT("Failed to get game scene");
			}
			return nullptr;
		}
		return game;
	}

	Array LTESourceMonitorServer::_get_valid_game_scene_ids(const String& chart_abs_path) {
		Array ids = game_scene_instance_ids.get(chart_abs_path, Array());
		Array valid_ids;
		for (int32_t index = 0; index < ids.size(); ++index) {
			uint64_t instance_id = static_cast<uint64_t>(int64_t(ids[index]));
			if (_get_game_from_instance_id(instance_id, true)) {
				valid_ids.append(static_cast<int64_t>(instance_id));
			}
		}
		if (valid_ids.is_empty()) {
			game_scene_instance_ids.erase(chart_abs_path);
		}
		else {
			game_scene_instance_ids[chart_abs_path] = valid_ids;
		}
		return valid_ids;
	}

	void LTESourceMonitorServer::_register_game_scene(const String& chart_abs_path, Control* p_game_scene) {
		if (!p_game_scene || chart_abs_path.is_empty()) {
			return;
		}
		uint64_t instance_id = p_game_scene->get_instance_id();
		Array ids = _get_valid_game_scene_ids(chart_abs_path);
		for (int32_t index = 0; index < ids.size(); ++index) {
			if (static_cast<uint64_t>(int64_t(ids[index])) == instance_id) {
				return;
			}
		}
		ids.append(static_cast<int64_t>(instance_id));
		game_scene_instance_ids[chart_abs_path] = ids;
	}

	bool LTESourceMonitorServer::_call_game_method(const Dictionary& status, const StringName& method) {
		String chart_abs_path = status.get("chart_abs_path", String());
		Array ids = _get_valid_game_scene_ids(chart_abs_path);
		bool called = false;
		for (int32_t index = 0; index < ids.size(); ++index) {
			uint64_t instance_id = static_cast<uint64_t>(int64_t(ids[index]));
			Control* game = _get_game_from_instance_id(instance_id, true);
			if (!game) {
				continue;
			}
			if (!game->has_method(method)) {
				ERR_PRINT(vformat("Failed to call '%s' method", String(method)));
				continue;
			}
			game->call(method);
			called = true;
		}
		return called;
	}

	bool LTESourceMonitorServer::_call_game_method(const Dictionary& status, const StringName& method, const Variant& arg0) {
		String chart_abs_path = status.get("chart_abs_path", String());
		Array ids = _get_valid_game_scene_ids(chart_abs_path);
		bool called = false;
		for (int32_t index = 0; index < ids.size(); ++index) {
			uint64_t instance_id = static_cast<uint64_t>(int64_t(ids[index]));
			Control* game = _get_game_from_instance_id(instance_id, true);
			if (!game) {
				continue;
			}
			if (!game->has_method(method)) {
				ERR_PRINT(vformat("Failed to call '%s' method", String(method)));
				continue;
			}
			game->call(method, arg0);
			called = true;
		}
		return called;
	}

	bool LTESourceMonitorServer::_call_game_method(const Dictionary& status, const StringName& method, const Variant& arg0, const Variant& arg1) {
		String chart_abs_path = status.get("chart_abs_path", String());
		Array ids = _get_valid_game_scene_ids(chart_abs_path);
		bool called = false;
		for (int32_t index = 0; index < ids.size(); ++index) {
			uint64_t instance_id = static_cast<uint64_t>(int64_t(ids[index]));
			Control* game = _get_game_from_instance_id(instance_id, true);
			if (!game) {
				continue;
			}
			if (!game->has_method(method)) {
				ERR_PRINT(vformat("Failed to call '%s' method", String(method)));
				continue;
			}
			game->call(method, arg0, arg1);
			called = true;
		}
		return called;
	}

	bool LTESourceMonitorServer::_try_get_game_time(const uint64_t instance_id, double& r_time) const {
		Control* game = _get_game_from_instance_id(instance_id, true);
		if (!game) {
			return false;
		}
		if (!game->has_method("get_time")) {
			ERR_PRINT("Failed to call \"get_time\" method");
			return false;
		}
		r_time = game->call("get_time");
		return true;
	}

	double LTESourceMonitorServer::_get_status_game_time(const Dictionary& status) const {
		String chart_abs_path = status.get("chart_abs_path", String());
		uint64_t primary_instance_id = static_cast<uint64_t>(int64_t(status.get("game_scene_id", 0)));
		double game_time = 0.0;
		if (primary_instance_id != 0 && _try_get_game_time(primary_instance_id, game_time)) {
			return game_time;
		}

		Array ids = game_scene_instance_ids.get(chart_abs_path, Array());
		for (int32_t index = 0; index < ids.size(); ++index) {
			uint64_t instance_id = static_cast<uint64_t>(int64_t(ids[index]));
			if (instance_id == primary_instance_id) {
				continue;
			}
			if (_try_get_game_time(instance_id, game_time)) {
				return game_time;
			}
		}
		return status.get("time", 0.0);
	}

	double LTESourceMonitorServer::_get_saved_monitor_chart_time(const String& uuid, const String& chart_path) const {
		if (!settings_config || uuid.is_empty() || chart_path.is_empty()) {
			return 0.0;
		}
		Dictionary chart_times = settings_config->source_monitor_chart_time.get(uuid, Dictionary());
		String relative_chart_path = _get_relative_chart_path(chart_path);
		if (chart_times.has(relative_chart_path)) {
			return chart_times.get(relative_chart_path, 0.0);
		}
		String absolute_chart_path = _get_absolute_chart_path(chart_path);
		if (chart_times.has(absolute_chart_path)) {
			return chart_times.get(absolute_chart_path, 0.0);
		}
		Array keys = chart_times.keys();
		for (int32_t index = 0; index < keys.size(); ++index) {
			String key = keys[index];
			if (_get_absolute_chart_path(key) == absolute_chart_path) {
				return chart_times.get(key, 0.0);
			}
		}
		return 0.0;
	}

	void LTESourceMonitorServer::_set_saved_monitor_chart_time(const String& uuid, const String& chart_path, const double time, const bool signal_emit) {
		if (!settings_config || uuid.is_empty() || chart_path.is_empty()) {
			return;
		}
		Dictionary chart_times = settings_config->source_monitor_chart_time.get(uuid, Dictionary());
		String key = _get_relative_chart_path(chart_path);
		if (key.is_empty()) {
			key = _get_absolute_chart_path(chart_path);
		}
		chart_times[key] = time;
		settings_config->source_monitor_chart_time[uuid] = chart_times;
		settings_config->save_settings_config(signal_emit);
	}

	void LTESourceMonitorServer::_apply_display_clock_status(Dictionary& r_status, const double time_scale, const double time_offset, const double duration, const String& scene_path) const {
		if (duration < 0.0) {
			r_status.erase("display_time_scale");
			r_status.erase("display_time_offset");
			r_status.erase("display_duration");
			r_status.erase("display_scene_path");
			return;
		}
		r_status["display_time_scale"] = time_scale;
		r_status["display_time_offset"] = time_offset;
		r_status["display_duration"] = duration;
		r_status["display_scene_path"] = scene_path;
	}

	void LTESourceMonitorServer::_store_game_status(const Dictionary& status) {
		String chart_abs_path = status.get("chart_abs_path", String());
		int32_t index = chart_abs_path.is_empty() ? -1 : _find_status_index_by_chart_path(chart_abs_path);
		if (index < 0 && chart_abs_path.is_empty()) {
			int32_t difficulty = status.get("difficulty", -1);
			index = _find_status_index(difficulty);
		}
		if (index >= 0) {
			status_list[index] = status;
		}
		else {
			status_list.append(status);
		}
	}

	Dictionary LTESourceMonitorServer::fetch_monitor_config(const String& uuid) {
		Dictionary config;
		config["last_opened"] = settings_config->source_monitor_last_opened.get(uuid, "");
		config["ratio"] = settings_config->source_monitor_ratio.get(uuid, Vector2i(16, 9));
		config["view_judgement_line"] = settings_config->source_monitor_view_judgement_line.get(uuid, false);
		config["show_clear_color"] = settings_config->source_monitor_show_clear_color.get(uuid, false);
		config["auto_play"] = settings_config->source_monitor_auto_play.get(uuid, false);
		config["show_debug_info"] = settings_config->source_monitor_show_debug_info.get(uuid, false);
		config["smart_guides"] = settings_config->source_monitor_smart_guides.get(uuid, false);
		Dictionary snap_settings = settings_config->source_monitor_snap_settings.get(uuid, Dictionary());
		Dictionary normalized_snap_settings;
		normalized_snap_settings["grid_snap"] = snap_settings.get("grid_snap", false);
		normalized_snap_settings["grid_size"] = snap_settings.get("grid_size", 32.0);
		normalized_snap_settings["lt_track_edge_snap"] = snap_settings.get("lt_track_edge_snap", true);
		normalized_snap_settings["lt_track_grid_size"] = snap_settings.get("lt_track_grid_size", 160.0);
		config["snap_settings"] = normalized_snap_settings;
		config["screen"] = settings_config->source_monitor_screen.get(uuid, DisplayServer::get_singleton()->get_primary_screen());
		config["window_mode"] = settings_config->source_monitor_window_mode.get(uuid, 0);
		return config;
	}

	double LTESourceMonitorServer::get_monitor_chart_time(const String& uuid, const String& chart_path) const {
		return _get_saved_monitor_chart_time(uuid, chart_path);
	}

	void LTESourceMonitorServer::set_monitor_chart_time(const String& uuid, const String& chart_path, const double time) {
		_set_saved_monitor_chart_time(uuid, chart_path, time, false);
	}

	Color LTESourceMonitorServer::get_game_blackground_light_color(const GameDifficulty difficulty) const {
		switch (difficulty) {
		case EASY: return Color("#00aaa6");
		case HARD: return Color("#ffc700");
		case EXPERT: return Color("#ff0069");
		case MASTER: return Color("#00baff");
		}
		return Color("#ffffff");
	}

	void LTESourceMonitorServer::open_game(const String& uuid, Control* p_game_scene, const GameDifficulty difficulty) {
		if (difficulty < EASY || difficulty > MASTER) {
			ERR_PRINT(vformat("Invalid difficulty index passed: %s", String::num(static_cast<int64_t>(difficulty))));
			return;
		}
		Dictionary track_info = file_system->get_track_info();
		Dictionary difficulty_dict = track_info.get("difficulty", Dictionary());
		String hd = humanize_difficulty(difficulty);
		String chart_path = _get_chart_path_for_difficulty(difficulty_dict, difficulty);
		if (chart_path.is_empty()) {
			ERR_PRINT(vformat("Chart path not found for difficulty: %s", hd));
			return;
		}
		open_game_by_chart_path(uuid, p_game_scene, chart_path);
	}

	void LTESourceMonitorServer::open_game_by_chart_path(const String& uuid, Control* p_game_scene, const String& chart_path) {
		Dictionary track_info = file_system->get_track_info();
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_dir = project_manager ? project_manager->get_project_path() : String();
		String abs_chart_path = _get_absolute_chart_path(chart_path);
		if (abs_chart_path.is_empty() || !FileAccess::file_exists(abs_chart_path)) {
			ERR_PRINT(vformat("Chart path not found: %s", chart_path));
			return;
		}
		game_scene_instance_id = p_game_scene ? p_game_scene->get_instance_id() : 0;
		Dictionary previous_status = get_game_status_by_chart_path(abs_chart_path);
		double previous_time = previous_status.is_empty() ? _get_saved_monitor_chart_time(uuid, abs_chart_path) : _get_status_game_time(previous_status);
		bool previous_is_playing = previous_status.get("is_playing", false);
		String relative_chart_path = _get_relative_chart_path(abs_chart_path);
		GameDifficulty difficulty = scan_difficulty_from_chart_path(abs_chart_path);
		String difficulty_name = get_chart_difficulty_name(abs_chart_path);
		if (difficulty_name.is_empty()) {
			difficulty_name = abs_chart_path.get_file().get_basename();
		}
		if (p_game_scene && p_game_scene->has_method("set_parameters")) {
			Dictionary parameters;
			parameters["info"] = track_info;
			parameters["difficulty"] = int64_t(difficulty);
			parameters["difficulty_name"] = difficulty_name;
			parameters["root_dir"] = root_dir;
			parameters["chart_path"] = abs_chart_path;
			parameters["debug"] = true;
			parameters["auto_play"] = true;
			parameters["show_audio_error_dialog"] = false;
			p_game_scene->call("set_parameters", parameters);
		}
		_register_game_scene(abs_chart_path, p_game_scene);
		Array ids = game_scene_instance_ids.get(abs_chart_path, Array());
		Dictionary chart = Utils::load_json_file(abs_chart_path);
		Dictionary chart_metadata = chart.get("metadata", Dictionary());
		double duration = chart_metadata.get("duration", 0.0);
		Dictionary status_dict;
		status_dict["runtime_uuid"] = uuid;
		status_dict["difficulty"] = difficulty;
		status_dict["difficulty_name"] = difficulty_name;
		status_dict["game_scene_id"] = game_scene_instance_id;
		status_dict["game_scene_ids"] = ids;
		status_dict["chart_path"] = relative_chart_path;
		status_dict["chart_abs_path"] = abs_chart_path;
		status_dict["duration"] = duration;
		status_dict["time"] = previous_time;
		status_dict["is_playing"] = previous_is_playing;
		status_dict["origin"] = String();
		if (p_game_scene && previous_time > 0.0 && p_game_scene->has_method("scrub_to_frame")) {
			p_game_scene->call("scrub_to_frame", previous_time, false);
		}
		if (p_game_scene && previous_is_playing && p_game_scene->has_method("play")) {
			p_game_scene->call("play", previous_time);
		}
		_store_game_status(status_dict);
		settings_config->source_monitor_last_opened[uuid] = relative_chart_path;
		_set_saved_monitor_chart_time(uuid, abs_chart_path, previous_time, false);
		settings_config->save_settings_config(false);
		emit_signal("game_opened", static_cast<int32_t>(difficulty), status_dict);
	}

	LTESourceMonitorServer::GameDifficulty LTESourceMonitorServer::scan_difficulty_from_chart_path(const String& chart_path) const {
		return _get_difficulty_from_name(_scan_difficulty_name_from_chart_path(chart_path));
	}

	String LTESourceMonitorServer::get_chart_difficulty_name(const String& chart_path) const {
		return _scan_difficulty_name_from_chart_path(chart_path);
	}

	String LTESourceMonitorServer::humanize_difficulty(const GameDifficulty difficulty) const {
		switch (difficulty) {
		case EASY: return "easy";
		case HARD: return "hard";
		case EXPERT: return "expert";
		case MASTER: return "master";
		default: return String();
		}
	}

	LTESourceMonitorServer::GameDifficulty LTESourceMonitorServer::get_difficulty(const String& diff_str) const {
		return _get_difficulty_from_name(diff_str);
	}

	Control* LTESourceMonitorServer::get_game() const {
		return _get_game_from_instance_id(game_scene_instance_id);
	}

	Dictionary LTESourceMonitorServer::get_game_status(const GameDifficulty difficulty) const {
		int32_t index = _find_status_index(static_cast<int32_t>(difficulty));
		if (index >= 0) {
			return status_list[index];
		}
		return Dictionary();
	}

	Dictionary LTESourceMonitorServer::get_game_status_by_chart_path(const String& chart_path) const {
		int32_t index = _find_status_index_by_chart_path(chart_path);
		if (index >= 0) {
			return status_list[index];
		}
		return Dictionary();
	}

	double LTESourceMonitorServer::get_game_time(const GameDifficulty difficulty) const {
		Dictionary status_dict = get_game_status(difficulty);
		if (status_dict.is_empty()) return 0.0;
		return _get_status_game_time(status_dict);
	}

	double LTESourceMonitorServer::get_game_time_by_chart_path(const String& chart_path) const {
		Dictionary status_dict = get_game_status_by_chart_path(chart_path);
		if (status_dict.is_empty()) return 0.0;
		return _get_status_game_time(status_dict);
	}

	void LTESourceMonitorServer::game_hot_reload(const GameDifficulty difficulty) {
		Dictionary status_dict = get_game_status(difficulty);
		if (status_dict.is_empty()) return;
		game_hot_reload_by_chart_path(status_dict.get("chart_abs_path", String()));
	}

	void LTESourceMonitorServer::game_hot_reload_by_chart_path(const String& chart_path) {
		Dictionary status_dict = get_game_status_by_chart_path(chart_path);
		if (status_dict.is_empty()) return;
		if (!_call_game_method(status_dict, StringName("hot_reload"))) return;
		String chart_abs_path = status_dict.get("chart_abs_path", String());
		if (!chart_abs_path.is_empty()) {
			Dictionary chart = Utils::load_json_file(chart_abs_path);
			Dictionary chart_metadata = chart.get("metadata", Dictionary());
			status_dict["duration"] = chart_metadata.get("duration", status_dict.get("duration", 0.0));
		}
		status_dict["time"] = _get_status_game_time(status_dict);
		status_dict["origin"] = String();
		_store_game_status(status_dict);
		int32_t difficulty = status_dict.get("difficulty", ERROR_DIFFICULTY);
		emit_signal("game_opened", difficulty, status_dict);
	}

	void LTESourceMonitorServer::game_hot_reload_skin(const GameDifficulty difficulty, const String& skin_path) {
		Dictionary status_dict = get_game_status(difficulty);
		if (status_dict.is_empty()) return;
		game_hot_reload_skin_by_chart_path(status_dict.get("chart_abs_path", String()), skin_path);
	}

	void LTESourceMonitorServer::game_hot_reload_skin_by_chart_path(const String& chart_path, const String& skin_path) {
		Dictionary status_dict = get_game_status_by_chart_path(chart_path);
		if (status_dict.is_empty()) return;
		if (!_call_game_method(status_dict, StringName("hot_reload_note_skin"), skin_path)) return;
		status_dict["time"] = _get_status_game_time(status_dict);
		status_dict["origin"] = String();
		_store_game_status(status_dict);
	}

	void LTESourceMonitorServer::game_play(const GameDifficulty difficulty, const double from_sec, const double display_time_scale, const double display_time_offset, const double display_duration, const String& display_scene_path) {
		Dictionary status_dict = get_game_status(difficulty);
		if (status_dict.is_empty()) return;
		game_play_by_chart_path(status_dict.get("chart_abs_path", String()), from_sec, display_time_scale, display_time_offset, display_duration, display_scene_path);
	}

	void LTESourceMonitorServer::game_play_by_chart_path(const String& chart_path, const double from_sec, const double display_time_scale, const double display_time_offset, const double display_duration, const String& display_scene_path) {
		Dictionary status_dict = get_game_status_by_chart_path(chart_path);
		if (status_dict.is_empty()) return;
		String chart_abs_path = status_dict.get("chart_abs_path", _get_absolute_chart_path(chart_path));
		uint64_t now_msec = Time::get_singleton()->get_ticks_msec();
		uint64_t last_pause_msec = static_cast<uint64_t>(int64_t(chart_last_pause_msec.get(chart_abs_path, 0)));
		if (last_pause_msec > 0 && now_msec - last_pause_msec <= PLAY_PAUSE_DOUBLE_TOGGLE_GUARD_MSEC) return;
		if (!_call_game_method(status_dict, StringName("play"), from_sec)) return;
		status_dict["time"] = from_sec;
		status_dict["is_playing"] = true;
		status_dict["origin"] = String();
		_apply_display_clock_status(status_dict, display_time_scale, display_time_offset, display_duration, display_scene_path);
		chart_last_play_msec[chart_abs_path] = static_cast<int64_t>(now_msec);
		_store_game_status(status_dict);
		int32_t difficulty = status_dict.get("difficulty", ERROR_DIFFICULTY);
		emit_signal("game_playing", difficulty, from_sec, status_dict);
	}

	void LTESourceMonitorServer::game_pause(const GameDifficulty difficulty) {
		Dictionary status_dict = get_game_status(difficulty);
		if (status_dict.is_empty()) return;
		game_pause_by_chart_path(status_dict.get("chart_abs_path", String()));
	}

	void LTESourceMonitorServer::game_pause_by_chart_path(const String& chart_path) {
		Dictionary status_dict = get_game_status_by_chart_path(chart_path);
		if (status_dict.is_empty()) return;
		String chart_abs_path = status_dict.get("chart_abs_path", _get_absolute_chart_path(chart_path));
		uint64_t now_msec = Time::get_singleton()->get_ticks_msec();
		uint64_t last_play_msec = static_cast<uint64_t>(int64_t(chart_last_play_msec.get(chart_abs_path, 0)));
		if (last_play_msec > 0 && now_msec - last_play_msec <= PLAY_PAUSE_DOUBLE_TOGGLE_GUARD_MSEC) return;
		if (!_call_game_method(status_dict, StringName("pause"))) return;
		status_dict["time"] = _get_status_game_time(status_dict);
		status_dict["is_playing"] = false;
		status_dict["origin"] = String();
		chart_last_pause_msec[chart_abs_path] = static_cast<int64_t>(now_msec);
		_store_game_status(status_dict);
		int32_t difficulty = status_dict.get("difficulty", ERROR_DIFFICULTY);
		emit_signal("game_paused", difficulty, status_dict);
	}

	void LTESourceMonitorServer::game_scrub_to_frame(const GameDifficulty difficulty, const double time, const bool scrub_audio, const bool continue_play, const String& origin, const double display_time_scale, const double display_time_offset, const double display_duration, const String& display_scene_path) {
		Dictionary status_dict = get_game_status(difficulty);
		if (status_dict.is_empty()) return;
		game_scrub_to_frame_by_chart_path(status_dict.get("chart_abs_path", String()), time, scrub_audio, continue_play, origin, display_time_scale, display_time_offset, display_duration, display_scene_path);
	}

	void LTESourceMonitorServer::game_scrub_to_frame_by_chart_path(const String& chart_path, const double time, const bool scrub_audio, const bool continue_play, const String& origin, const double display_time_scale, const double display_time_offset, const double display_duration, const String& display_scene_path) {
		Dictionary status_dict = get_game_status_by_chart_path(chart_path);
		if (status_dict.is_empty()) return;
		// 如果继续播放，则无需音频擦洗
		if (!_call_game_method(status_dict, StringName("scrub_to_frame"), time, continue_play ? false : scrub_audio)) return;
		status_dict["time"] = time;
		status_dict["is_playing"] = continue_play;
		status_dict["origin"] = origin;
		_apply_display_clock_status(status_dict, display_time_scale, display_time_offset, display_duration, display_scene_path);
		_store_game_status(status_dict);
		int32_t difficulty = status_dict.get("difficulty", ERROR_DIFFICULTY);
		emit_signal("game_scrubbing", difficulty, time, status_dict);
	}

	int32_t LTESourceMonitorServer::get_game_instance_window_id(const String& runtime_uuid) const {
		for (const KeyValue<int32_t, String>& pair : runtime_instance_uuids) {
			if (pair.value != runtime_uuid) {
				continue;
			}
			const uint64_t* window_instance_id = runtime_instances.getptr(pair.key);
			if (!window_instance_id) {
				continue;
			}
			Object* object = ObjectDB::get_instance(*window_instance_id);
			Window* window = Object::cast_to<Window>(object);
			if (window && !window->is_queued_for_deletion()) {
				return pair.key;
			}
		}
		return -1;
	}

	int32_t LTESourceMonitorServer::run_game_instance_on_window(const GameDifficulty difficulty, const String& runtime_uuid, const double from_sec) {
		if (difficulty < EASY || difficulty > MASTER) {
			ERR_PRINT(vformat("Invalid difficulty index passed: %s", String::num(static_cast<int64_t>(difficulty))));
			return -1;
		}
		Dictionary track_info = file_system->get_track_info();
		Dictionary difficulty_dict = track_info.get("difficulty", Dictionary());
		String hd = humanize_difficulty(difficulty);
		String chart_path = _get_chart_path_for_difficulty(difficulty_dict, difficulty);
		if (chart_path.is_empty()) {
			ERR_PRINT(vformat("Chart path not found for difficulty: %s", hd));
			return -1;
		}
		return run_game_instance_on_window_by_chart_path(chart_path, runtime_uuid, from_sec);
	}

	int32_t LTESourceMonitorServer::run_game_instance_on_window_by_chart_path(const String& chart_path, const String& runtime_uuid, const double from_sec) {
		// 获取项目信息
		Dictionary track_info = file_system->get_track_info();
		String track_name = track_info.get("name", "");
		String track_illustrator = track_info.get("illustrator", "");
		String abs_chart_path = _get_absolute_chart_path(chart_path);
		if (abs_chart_path.is_empty() || !FileAccess::file_exists(abs_chart_path)) {
			ERR_PRINT(vformat("Chart path not found: %s", chart_path));
			return -1;
		}
		GameDifficulty difficulty = scan_difficulty_from_chart_path(abs_chart_path);
		String difficulty_name = get_chart_difficulty_name(abs_chart_path);
		if (difficulty_name.is_empty()) {
			difficulty_name = abs_chart_path.get_file().get_basename();
		}
		// 创建窗口
		Window* window = workspace_manager->create_empty_window(Vector2i(1152, 648), vformat("%s-%s", track_name, track_illustrator), false, false);
		if (!window) {
			ERR_PRINT("run_game_instance_on_window: editor_instance is null");
			return -1;
		}
		int32_t window_mode = settings_config->source_monitor_window_mode.get(runtime_uuid, 0);
		switch (window_mode) {
		case 0: window->set_mode(Window::MODE_WINDOWED); break;
		case 1: window->set_mode(Window::MODE_MAXIMIZED); break;
		case 2: window->set_mode(Window::MODE_FULLSCREEN); break;
		default: WARN_PRINT("run_game_instance_on_window: Unknow window mode!"); break;
		}
		int32_t screen = settings_config->source_monitor_screen.get(runtime_uuid, DisplayServer::get_singleton()->get_primary_screen());
		window->set_current_screen(screen);
		window->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_OTHER_SCREEN);
		window->set_content_scale_size(Vector2i(1920, 1080));
		window->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
		window->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_EXPAND);
		// 创建背景
		bool show_clear_color = settings_config->source_monitor_show_clear_color.get(runtime_uuid, false);
		if (!show_clear_color) {
			Ref<PackedScene> packed_bg = Utils::load<PackedScene>("uid://bictapl04k1aa");
			if (packed_bg.is_valid() && packed_bg->can_instantiate()) {
				Node* bg = packed_bg->instantiate();
				Color bg_color = get_game_blackground_light_color(difficulty);
				window->add_child(bg);
				if (bg->has_method("set_glow_color")) bg->call("set_glow_color", bg_color);
				if (bg->has_method("set_debris_color")) bg->call("set_debris_color", bg_color);
			}
		}
		else {
			WARN_PRINT("run_game_instance_on_window: Failed to load BG");
		}
		// 创建游戏
		Ref<PackedScene> packed_game = Utils::load<PackedScene>("uid://b7ntx22skglln");
		if (packed_game.is_valid() && packed_game->can_instantiate()) {
			Node* game = packed_game->instantiate();
			if (game->has_method("set_parameters")) {
				bool auto_play = settings_config->source_monitor_auto_play.get(runtime_uuid, false);
				bool show_debug_info = settings_config->source_monitor_show_debug_info.get(runtime_uuid, false);
				window->add_child(game);
				Dictionary parameters;
				parameters["info"] = track_info;
				parameters["difficulty"] = int64_t(difficulty);
				parameters["difficulty_name"] = difficulty_name;
				LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
				parameters["root_dir"] = project_manager ? project_manager->get_project_path() : String();
				parameters["chart_path"] = abs_chart_path;
				parameters["auto_play"] = auto_play;
				parameters["start_time"] = from_sec > 0.0 ? from_sec : 0.0;
				parameters["show_debug_info"] = show_debug_info;
				parameters["show_audio_error_dialog"] = false;
				game->call_deferred("set_parameters", parameters);	// 延迟调用
			}
		}
		else {
			ERR_PRINT("run_game_instance_on_window: Failed to load game");
		}
		Ref<PackedScene> packed_overlay = Utils::load<PackedScene>("uid://c76rqxy2gq4bh");
		if (packed_overlay.is_valid() && packed_overlay->can_instantiate()) {
			CanvasLayer* overlay_layer = memnew(CanvasLayer);
			overlay_layer->set_name("OverlayCanvasLayer");
			overlay_layer->set_layer(200);
			Node* overlay = packed_overlay->instantiate();
			window->add_child(overlay_layer);
			overlay_layer->add_child(overlay);
		}
		else {
			WARN_PRINT("run_game_instance_on_window: Failed to load overlay");
		}
		window->show();
		int32_t window_id = window->get_window_id();
		window->connect("close_requested", Callable(this, "stop_game_instance_on_window").bind(window_id));
		window->connect("tree_exiting", Callable(this, "_on_runtime_window_tree_exiting").bind(window_id), CONNECT_ONE_SHOT);
		runtime_instances[window_id] = window->get_instance_id();
		runtime_instance_uuids[window_id] = runtime_uuid;
		emit_signal("game_instance_running", window_id);
		return window_id;
	}

	void LTESourceMonitorServer::stop_game_instance_on_window(const int32_t window_id) {
		if (runtime_instances.has(window_id)) {
			uint64_t window_instance_id = runtime_instances.get(window_id);
			runtime_instances.erase(window_id);
			runtime_instance_uuids.erase(window_id);
			Object* object = ObjectDB::get_instance(window_instance_id);
			Window* window = Object::cast_to<Window>(object);
			if (window && !window->is_queued_for_deletion()) {
				window->call_deferred("queue_free");
			}
			emit_signal("game_instance_stopped", window_id);
		}
		else {
			ERR_PRINT("stop_game_instance_on_window: Runtime game instance is not found");
		}
	}

	void LTESourceMonitorServer::_on_runtime_window_tree_exiting(const int32_t window_id) {
		if (!runtime_instances.has(window_id)) {
			return;
		}
		runtime_instances.erase(window_id);
		runtime_instance_uuids.erase(window_id);
		emit_signal("game_instance_stopped", window_id);
	}

	void LTESourceMonitorServer::_set_runtime_windows_debug_info(const String& uuid, const bool enabled) {
		StringName method = "set_debug_info_visible";
		for (const KeyValue<int32_t, uint64_t>& pair : runtime_instances) {
			int32_t window_id = pair.key;
			const String* instance_uuid = runtime_instance_uuids.getptr(window_id);
			if (!instance_uuid || *instance_uuid != uuid) {
				continue;
			}
			Object* object = ObjectDB::get_instance(pair.value);
			Window* window = Object::cast_to<Window>(object);
			if (!window || window->is_queued_for_deletion()) {
				continue;
			}
			for (int32_t index = 0; index < window->get_child_count(); ++index) {
				Node* child = window->get_child(index);
				if (child && child->has_method(method)) {
					child->call(method, enabled);
				}
			}
		}
	}

	void LTESourceMonitorServer::enable_monitor_judgement_line(const String& uuid, const bool enable) {
		settings_config->source_monitor_view_judgement_line[uuid] = enable;
		settings_config->save_settings_config();
	}

	void LTESourceMonitorServer::set_monitor_ratio(const String& uuid, const Vector2i& ratio) {
		settings_config->source_monitor_ratio[uuid] = ratio;
		settings_config->save_settings_config();
	}

	void LTESourceMonitorServer::enable_monitor_clear_color(const String& uuid, const bool enable) {
		settings_config->source_monitor_show_clear_color[uuid] = enable;
		settings_config->save_settings_config();
	}

	void LTESourceMonitorServer::enable_monitor_auto_play(const String& uuid, const bool enable) {
		settings_config->source_monitor_auto_play[uuid] = enable;
		settings_config->save_settings_config();
	}

	void LTESourceMonitorServer::enable_monitor_debug_info(const String& uuid, const bool enable) {
		settings_config->source_monitor_show_debug_info[uuid] = enable;
		settings_config->save_settings_config();
		_set_runtime_windows_debug_info(uuid, enable);
	}

	void LTESourceMonitorServer::enable_monitor_smart_guides(const String& uuid, const bool enable) {
		settings_config->source_monitor_smart_guides[uuid] = enable;
		settings_config->save_settings_config();
	}

	void LTESourceMonitorServer::set_monitor_snap_settings(const String& uuid, const Dictionary& snap_settings) {
		Dictionary normalized_snap_settings;
		normalized_snap_settings["grid_snap"] = snap_settings.get("grid_snap", false);
		normalized_snap_settings["grid_size"] = snap_settings.get("grid_size", 32.0);
		normalized_snap_settings["lt_track_edge_snap"] = snap_settings.get("lt_track_edge_snap", true);
		normalized_snap_settings["lt_track_grid_size"] = snap_settings.get("lt_track_grid_size", 160.0);
		settings_config->source_monitor_snap_settings[uuid] = normalized_snap_settings;
		settings_config->save_settings_config();
	}

	void LTESourceMonitorServer::set_monitor_screen(const String& uuid, const int screen) {
		settings_config->source_monitor_screen[uuid] = screen;
		settings_config->save_settings_config();
	}

	void LTESourceMonitorServer::set_monitor_window_mode(const String& uuid, const int mode) {
		settings_config->source_monitor_window_mode[uuid] = mode;
		settings_config->save_settings_config();
	}
} // namespace godot
