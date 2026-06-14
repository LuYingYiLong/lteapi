#ifndef SOURCE_MONITOR_SERVER_H
#define SOURCE_MONITOR_SERVER_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include "chart_helper.h"
#include "file_system_server.h"
#include "settings_config.h"
#include "utils.h"
#include "workspace_manager.h"

namespace godot {
	class LTESourceMonitorServer : public Object {
		GDCLASS(LTESourceMonitorServer, Object)

	public:
		enum GameDifficulty {
			EASY,
			HARD,
			EXPERT,
			MASTER,
			ERROR_DIFFICULTY
		};

	private:
		static LTESourceMonitorServer* singleton;
		static constexpr uint64_t PLAY_PAUSE_DOUBLE_TOGGLE_GUARD_MSEC = 100;

		Array status_list;
		Dictionary game_scene_instance_ids;
		Dictionary chart_last_play_msec;
		Dictionary chart_last_pause_msec;
		LTESettingsConfig* settings_config = nullptr;
		LTEWorkspaceManager* workspace_manager = nullptr;
		LTEFileSystemServer* file_system = nullptr;
		uint64_t game_scene_instance_id = 0;
		HashMap<int32_t, uint64_t> runtime_instances;
		HashMap<int32_t, String> runtime_instance_uuids;
		int32_t _find_status_index(const int32_t difficulty) const;
		int32_t _find_status_index_by_chart_path(const String& chart_path) const;
		String _get_absolute_chart_path(const String& chart_path) const;
		String _get_relative_chart_path(const String& chart_path) const;
		Control* _get_game_from_instance_id(const uint64_t instance_id, const bool quiet = false) const;
		Array _get_valid_game_scene_ids(const String& chart_abs_path);
		void _register_game_scene(const String& chart_abs_path, Control* p_game_scene);
		bool _call_game_method(const Dictionary& status, const StringName& method);
		bool _call_game_method(const Dictionary& status, const StringName& method, const Variant& arg0);
		bool _call_game_method(const Dictionary& status, const StringName& method, const Variant& arg0, const Variant& arg1);
		bool _try_get_game_time(const uint64_t instance_id, double& r_time) const;
		double _get_status_game_time(const Dictionary& status) const;
		double _get_saved_monitor_chart_time(const String& uuid, const String& chart_path) const;
		void _set_saved_monitor_chart_time(const String& uuid, const String& chart_path, const double time, const bool signal_emit = false);
		void _apply_display_clock_status(Dictionary& r_status, const double time_scale, const double time_offset, const double duration, const String& scene_path) const;
		void _store_game_status(const Dictionary& status);
		String _scan_difficulty_name_from_chart_path(const String& chart_path) const;
		String _get_chart_path_for_difficulty(const Dictionary& difficulty_dict, const GameDifficulty difficulty) const;
		GameDifficulty _get_difficulty_from_name(const String& difficulty_name) const;
		void _on_runtime_window_tree_exiting(const int32_t window_id);
		void _set_runtime_windows_debug_info(const String& uuid, const bool enabled);

	protected:
		static void _bind_methods();

	public:
		LTESourceMonitorServer();
		~LTESourceMonitorServer();

		static LTESourceMonitorServer* get_singleton();

		void clear_project_state();
		Dictionary fetch_monitor_config(const String& uuid);
		double get_monitor_chart_time(const String& uuid, const String& chart_path) const;
		void set_monitor_chart_time(const String& uuid, const String& chart_path, const double time);
		Color get_game_blackground_light_color(const GameDifficulty difficulty) const;
		void open_game(const String& uuid, Control* p_game_scene, const GameDifficulty difficulty);
		void open_game_by_chart_path(const String& uuid, Control* p_game_scene, const String& chart_path);
		GameDifficulty scan_difficulty_from_chart_path(const String& chart_path) const;
		String get_chart_difficulty_name(const String& chart_path) const;
		String humanize_difficulty(const GameDifficulty difficulty) const;
		GameDifficulty get_difficulty(const String& diff_str) const;
		Control* get_game() const;
		Dictionary get_game_status(const GameDifficulty difficulty) const;
		Dictionary get_game_status_by_chart_path(const String& chart_path) const;
		double get_game_time(const GameDifficulty difficulty) const;
		double get_game_time_by_chart_path(const String& chart_path) const;
		void game_hot_reload(const GameDifficulty difficulty);
		void game_hot_reload_by_chart_path(const String& chart_path);
		void game_hot_reload_skin(const GameDifficulty difficulty, const String& skin_path = String());
		void game_hot_reload_skin_by_chart_path(const String& chart_path, const String& skin_path = String());
		void game_play(const GameDifficulty difficulty, const double from_sec = 0.0, const double display_time_scale = 1.0, const double display_time_offset = 0.0, const double display_duration = -1.0, const String& display_scene_path = String());
		void game_play_by_chart_path(const String& chart_path, const double from_sec = 0.0, const double display_time_scale = 1.0, const double display_time_offset = 0.0, const double display_duration = -1.0, const String& display_scene_path = String());
		void game_pause(const GameDifficulty difficulty);
		void game_pause_by_chart_path(const String& chart_path);
		void game_scrub_to_frame(const GameDifficulty difficulty, const double time, const bool scrub_audio = true, const bool continue_play = true, const String& origin = String(), const double display_time_scale = 1.0, const double display_time_offset = 0.0, const double display_duration = -1.0, const String& display_scene_path = String());
		void game_scrub_to_frame_by_chart_path(const String& chart_path, const double time, const bool scrub_audio = true, const bool continue_play = true, const String& origin = String(), const double display_time_scale = 1.0, const double display_time_offset = 0.0, const double display_duration = -1.0, const String& display_scene_path = String());
		int32_t get_game_instance_window_id(const String& runtime_uuid) const;
		int32_t run_game_instance_on_window(const GameDifficulty difficulty, const String& runtime_uuid, const double from_sec = 0.0);
		int32_t run_game_instance_on_window_by_chart_path(const String& chart_path, const String& runtime_uuid, const double from_sec = 0.0);
		void stop_game_instance_on_window(const int32_t window_id);
		void enable_monitor_judgement_line(const String& uuid, const bool enable);
		void set_monitor_ratio(const String& uuid, const Vector2i& ratio);
		void enable_monitor_clear_color(const String& uuid, const bool enable);
		void enable_monitor_auto_play(const String& uuid, const bool enable);
		void enable_monitor_debug_info(const String& uuid, const bool enable);
		void enable_monitor_smart_guides(const String& uuid, const bool enable);
		void set_monitor_snap_settings(const String& uuid, const Dictionary& snap_settings);
		void set_monitor_screen(const String& uuid, const int screen);
		void set_monitor_window_mode(const String& uuid, const int mode);
	};
}

VARIANT_ENUM_CAST(LTESourceMonitorServer::GameDifficulty);

#endif // !SOURCE_MONITOR_SERVER_H
