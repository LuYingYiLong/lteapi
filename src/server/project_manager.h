#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include "file_system_server.h"
#include "lte_api.h"
#include "settings_config.h"
#include "source_monitor_server.h"
#include "utils.h"
#include "workspace_manager.h"
#include "zip_helper.h"
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace godot {
	class LTEProjectManager : public Object {
		GDCLASS(LTEProjectManager, Object)

	private:
		static LTEProjectManager* singleton;

		String config_path = "user://projects_config.cfg";
		String project_path;
		PackedStringArray project_paths;
		PackedStringArray recently_opened_projects;
		uint64_t project_manager_instance_id = 0;
		LTEAPI* api = nullptr;

		void _on_project_manager_instance_tree_exited();
		void _save_config(bool notify = true);
		void _add_project_path(const String& path);
		String _normalize_project_path(const String& path) const;
		bool _is_same_project_path(const String& a, const String& b) const;
		bool _has_project_path(const PackedStringArray& paths, const String& path) const;
		void _remove_project_path(PackedStringArray& paths, const String& path);
		bool _sanitize_project_lists();
		void _clear_runtime_project_state();
		bool _has_project_info_file(const String& path) const;
		bool _is_filesystem_root_path(const String& path) const;
		bool _is_directory_empty(const String& path) const;
		String _get_project_display_name(const String& path) const;
		String _normalize_project_file_path(const String& root_path, const String& file_path) const;
		bool _is_same_project_file_path(const String& root_path, const String& a, const String& b) const;
		LTESourceMonitorServer::GameDifficulty _get_difficulty_from_name(const String& difficulty_name) const;
		String _get_chart_path_for_difficulty(const Dictionary& difficulty_dict, const LTESourceMonitorServer::GameDifficulty difficulty) const;
		String _get_chart_difficulty_name(const Dictionary& track_info, const String& root_path, const String& chart_path) const;
		bool _sort_a_z_compare(const Dictionary& a, const Dictionary& b) const;
		bool _sort_z_a_compare(const Dictionary& a, const Dictionary& b) const;
		bool _is_openable_project_path(const String& path) const;

	protected:
		static void _bind_methods();

	public:
		LTEProjectManager();
		~LTEProjectManager();

		static LTEProjectManager* get_singleton();

		enum SortType {
			SORT_A_TO_Z,
			SORT_Z_TO_A,
			SORT_DATE_MODIFIED_NEWSET_FIRST,
			SORT_DATE_MODIFIED_OLDEST_FIRST
		};

		enum ViewsType {
			LIST_VIEWS,
			CONTENT_VIEWS
		};

		SortType sort_type = SORT_A_TO_Z;
		ViewsType views_type = LIST_VIEWS;

		Dictionary fetch_project_manager_config() const;
		String get_project_path() const;
		String get_last_opened_project() const;
		void open_project_manager();
		void open_project(const String& path);
		void close_project();
		void set_project_manager_sort_type(const SortType type);
		void set_project_manager_views_type(const ViewsType type);
		PackedStringArray get_sorted_project_paths() const;
		Error create_project(const String& p_path, const String& p_name, const String& p_description = "", const String& p_desinger = "", const String& p_illustrator = "", const String& p_version = "");
		Error import_project(const String& path, const String& save_path);
		Error scan_project_dir(const String& path);
		Dictionary get_project_info(const String& path);
		Error run_game_instance_headless(const String& path, const LTESourceMonitorServer::GameDifficulty difficulty, const bool auto_play = false);
		Error run_game_instance_headless_by_chart_path(const String& path, const String& chart_path, const bool auto_play = false);
		Error rename_project(const String& p_path, const String& new_name);
		Error remove_project(const String& p_path, const bool move_to_trash = false);
	};
}

VARIANT_ENUM_CAST(LTEProjectManager::SortType);
VARIANT_ENUM_CAST(LTEProjectManager::ViewsType);

#endif // !PROJECT_MANAGER_H
