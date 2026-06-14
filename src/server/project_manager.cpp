#include "project_manager.h"

#include "composition_server.h"
#include "graph_editor_server.h"
#include "note_skin_server.h"
#include "properties_server.h"
#include "timeline_server.h"
#include "undo_redo_server.h"

#include <godot_cpp/classes/canvas_layer.hpp>

namespace godot {
	LTEProjectManager* LTEProjectManager::singleton = nullptr;

	void LTEProjectManager::_bind_methods() {
		BIND_ENUM_CONSTANT(SORT_A_TO_Z);
		BIND_ENUM_CONSTANT(SORT_Z_TO_A);
		BIND_ENUM_CONSTANT(SORT_DATE_MODIFIED_NEWSET_FIRST);
		BIND_ENUM_CONSTANT(SORT_DATE_MODIFIED_OLDEST_FIRST);
		BIND_ENUM_CONSTANT(LIST_VIEWS);
		BIND_ENUM_CONSTANT(CONTENT_VIEWS);
		ClassDB::bind_method(D_METHOD("fetch_project_manager_config"), &LTEProjectManager::fetch_project_manager_config);
		ClassDB::bind_method(D_METHOD("get_project_path"), &LTEProjectManager::get_project_path);
		ClassDB::bind_method(D_METHOD("get_last_opened_project"), &LTEProjectManager::get_last_opened_project);
		ClassDB::bind_method(D_METHOD("open_project_manager"), &LTEProjectManager::open_project_manager);
		ClassDB::bind_method(D_METHOD("open_project", "path"), &LTEProjectManager::open_project);
		ClassDB::bind_method(D_METHOD("close_project"), &LTEProjectManager::close_project);
		ClassDB::bind_method(D_METHOD("set_project_manager_sort_type", "type"), &LTEProjectManager::set_project_manager_sort_type);
		ClassDB::bind_method(D_METHOD("set_project_manager_views_type", "type"), &LTEProjectManager::set_project_manager_views_type);
		ClassDB::bind_method(D_METHOD("get_sorted_project_paths"), &LTEProjectManager::get_sorted_project_paths);
		ClassDB::bind_method(D_METHOD("create_project", "path", "name", "description", "desinger", "illustrator", "version"), &LTEProjectManager::create_project, DEFVAL(String()), DEFVAL(String()), DEFVAL(String()), DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("import_project", "path", "save_path"), &LTEProjectManager::import_project);
		ClassDB::bind_method(D_METHOD("scan_project_dir", "path"), &LTEProjectManager::scan_project_dir);
		ClassDB::bind_method(D_METHOD("get_project_info", "path"), &LTEProjectManager::get_project_info);
		ClassDB::bind_method(D_METHOD("run_game_instance_headless", "path", "difficulty", "auto_play"), &LTEProjectManager::run_game_instance_headless, DEFVAL(false));
		ClassDB::bind_method(D_METHOD("run_game_instance_headless_by_chart_path", "path", "chart_path", "auto_play"), &LTEProjectManager::run_game_instance_headless_by_chart_path, DEFVAL(false));
		ClassDB::bind_method(D_METHOD("rename_project", "path", "name"), &LTEProjectManager::rename_project);
		ClassDB::bind_method(D_METHOD("remove_project", "path", "move_to_trash"), &LTEProjectManager::remove_project, DEFVAL(false));
		ADD_SIGNAL(MethodInfo("projects_config_changed"));
		ADD_SIGNAL(MethodInfo("project_opened", PropertyInfo(Variant::STRING, "project_path")));
		ADD_SIGNAL(MethodInfo("project_closed", PropertyInfo(Variant::STRING, "project_path")));
		ADD_SIGNAL(MethodInfo("project_created", PropertyInfo(Variant::STRING, "project_path")));
		ADD_SIGNAL(MethodInfo("project_imported", PropertyInfo(Variant::STRING, "project_path")));
		ADD_SIGNAL(MethodInfo("project_scanned", PropertyInfo(Variant::STRING, "project_path")));
		ADD_SIGNAL(MethodInfo("project_renamed", PropertyInfo(Variant::STRING, "from"), PropertyInfo(Variant::STRING, "to")));
		ADD_SIGNAL(MethodInfo("project_removed", PropertyInfo(Variant::STRING, "project_path"), PropertyInfo(Variant::BOOL, "moved_to_trash")));
	}

	LTEProjectManager::LTEProjectManager() {
		if (singleton == nullptr) {
			singleton = this;
		}
		if (FileAccess::file_exists(config_path)) {
			Ref<ConfigFile> file;
			file.instantiate();
			Error err = file->load(config_path);
			if (err == OK) {
				project_paths = file->get_value("project", "project_paths", PackedStringArray());
				recently_opened_projects = file->get_value("project", "recently_opened_projects", PackedStringArray());
				sort_type = static_cast<SortType>(int(file->get_value("manager", "sort_type", SORT_A_TO_Z)));
				views_type = static_cast<ViewsType>(int(file->get_value("manager", "views_type", LIST_VIEWS)));
			}
		}
		if (_sanitize_project_lists()) {
			_save_config(false);
		}
		if (LTEAPI::get_singleton()) {
			api = LTEAPI::get_singleton();
		}
	}

	LTEProjectManager::~LTEProjectManager() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEProjectManager* LTEProjectManager::get_singleton() {
		return singleton;
	}

	void LTEProjectManager::_on_project_manager_instance_tree_exited() {
		project_manager_instance_id = 0;
	}

	void LTEProjectManager::_save_config(bool notify) {
		_sanitize_project_lists();
		Ref<ConfigFile> file;
		file.instantiate();
		file->set_value("project", "project_paths", project_paths);
		file->set_value("project", "recently_opened_projects", recently_opened_projects);
		file->set_value("manager", "sort_type", sort_type);
		file->set_value("manager", "views_type", views_type);
		Error err = file->save(config_path);
		if (err != OK) {
			ERR_PRINT(vformat("Failed to save project manager config, error code: %d", err));
			return;
		}
		if (notify) {
			emit_signal("projects_config_changed");
		}
	}

	void LTEProjectManager::_add_project_path(const String& path) {
		String normalized_path = _normalize_project_path(path);
		if (normalized_path.is_empty() || _is_filesystem_root_path(normalized_path) || !_has_project_info_file(normalized_path)) {
			return;
		}
		_remove_project_path(project_paths, normalized_path);
		_remove_project_path(recently_opened_projects, normalized_path);
		project_paths.append(normalized_path);
		recently_opened_projects.insert(0, normalized_path);
		_save_config();
	}

	String LTEProjectManager::_normalize_project_path(const String& path) const {
		String normalized_path = path.replace("\\", "/").strip_edges().simplify_path();
		while (normalized_path.length() > 3 && normalized_path.ends_with("/")) {
			normalized_path = normalized_path.substr(0, normalized_path.length() - 1);
		}
		return normalized_path;
	}

	bool LTEProjectManager::_is_same_project_path(const String& a, const String& b) const {
		String normalized_a = _normalize_project_path(a);
		String normalized_b = _normalize_project_path(b);
		if (normalized_a.is_empty() || normalized_b.is_empty()) {
			return false;
		}
		return normalized_a.to_lower() == normalized_b.to_lower();
	}

	bool LTEProjectManager::_has_project_path(const PackedStringArray& paths, const String& path) const {
		for (int64_t index = 0; index < paths.size(); index++) {
			if (_is_same_project_path(paths[index], path)) {
				return true;
			}
		}
		return false;
	}

	void LTEProjectManager::_remove_project_path(PackedStringArray& paths, const String& path) {
		for (int64_t index = paths.size() - 1; index >= 0; index--) {
			if (_is_same_project_path(paths[index], path)) {
				paths.remove_at(index);
			}
		}
	}

	bool LTEProjectManager::_sanitize_project_lists() {
		bool changed = false;
		PackedStringArray clean_project_paths;
		for (int64_t index = 0; index < project_paths.size(); index++) {
			String normalized_path = _normalize_project_path(project_paths[index]);
			if (normalized_path.is_empty()) {
				changed = true;
				continue;
			}
			if (_is_filesystem_root_path(normalized_path)) {
				changed = true;
				continue;
			}
			if (_has_project_path(clean_project_paths, normalized_path)) {
				changed = true;
				continue;
			}
			if (normalized_path != project_paths[index]) {
				changed = true;
			}
			clean_project_paths.append(normalized_path);
		}

		PackedStringArray clean_recently_opened_projects;
		for (int64_t index = 0; index < recently_opened_projects.size(); index++) {
			String normalized_path = _normalize_project_path(recently_opened_projects[index]);
			if (normalized_path.is_empty()) {
				changed = true;
				continue;
			}
			if (!_has_project_path(clean_project_paths, normalized_path)) {
				changed = true;
				continue;
			}
			if (_has_project_path(clean_recently_opened_projects, normalized_path)) {
				changed = true;
				continue;
			}
			if (normalized_path != recently_opened_projects[index]) {
				changed = true;
			}
			clean_recently_opened_projects.append(normalized_path);
		}

		project_paths = clean_project_paths;
		recently_opened_projects = clean_recently_opened_projects;
		return changed;
	}

	bool LTEProjectManager::_has_project_info_file(const String& path) const {
		String normalized_path = _normalize_project_path(path);
		if (normalized_path.is_empty()) {
			return false;
		}
		Ref<DirAccess> dir = DirAccess::open(normalized_path);
		return dir.is_valid() && dir->file_exists("info.json");
	}

	bool LTEProjectManager::_is_filesystem_root_path(const String& path) const {
		String normalized_path = _normalize_project_path(path);
		while (normalized_path.length() > 1 && normalized_path.ends_with("/")) {
			normalized_path = normalized_path.substr(0, normalized_path.length() - 1);
		}
		return normalized_path.is_empty() ||
				normalized_path == "." ||
				normalized_path == "/" ||
				(normalized_path.length() == 2 && normalized_path.ends_with(":"));
	}

	bool LTEProjectManager::_is_directory_empty(const String& path) const {
		Ref<DirAccess> dir = DirAccess::open(_normalize_project_path(path));
		if (dir.is_null()) {
			return false;
		}
		dir->list_dir_begin();
		String file_name = dir->get_next();
		while (!file_name.is_empty()) {
			if (file_name != "." && file_name != "..") {
				dir->list_dir_end();
				return false;
			}
			file_name = dir->get_next();
		}
		dir->list_dir_end();
		return true;
	}

	String LTEProjectManager::_get_project_display_name(const String& path) const {
		String normalized_path = _normalize_project_path(path);
		String display_name = normalized_path.get_file();
		if (display_name.is_empty()) {
			display_name = normalized_path;
		}
		if (!_has_project_info_file(normalized_path)) {
			return display_name;
		}
		Dictionary track_info = Utils::load_json_file(normalized_path + "/info.json");
		return String(track_info.get("name", display_name));
	}

	String LTEProjectManager::_normalize_project_file_path(const String& root_path, const String& file_path) const {
		String normalized_file_path = file_path.replace("\\", "/").strip_edges();
		if (normalized_file_path.is_empty()) {
			return String();
		}
		String normalized_root_path = _normalize_project_path(root_path);
		if (normalized_file_path.begins_with("/") && !normalized_root_path.is_empty()) {
			return (normalized_root_path + normalized_file_path).simplify_path();
		}
		if (Utils::is_absolute_path(normalized_file_path)) {
			return normalized_file_path.simplify_path();
		}
		if (normalized_root_path.is_empty()) {
			return normalized_file_path.simplify_path();
		}
		return normalized_root_path.path_join(normalized_file_path).simplify_path();
	}

	bool LTEProjectManager::_is_same_project_file_path(const String& root_path, const String& a, const String& b) const {
		String normalized_a = _normalize_project_file_path(root_path, a);
		String normalized_b = _normalize_project_file_path(root_path, b);
		if (normalized_a.is_empty() || normalized_b.is_empty()) {
			return false;
		}
		return normalized_a.to_lower() == normalized_b.to_lower();
	}

	LTESourceMonitorServer::GameDifficulty LTEProjectManager::_get_difficulty_from_name(const String& difficulty_name) const {
		String lower_name = difficulty_name.to_lower();
		if (lower_name.ends_with("easy")) {
			return LTESourceMonitorServer::EASY;
		}
		else if (lower_name.ends_with("hard")) {
			return LTESourceMonitorServer::HARD;
		}
		else if (lower_name.ends_with("expert")) {
			return LTESourceMonitorServer::EXPERT;
		}
		else if (lower_name.ends_with("master")) {
			return LTESourceMonitorServer::MASTER;
		}
		return LTESourceMonitorServer::ERROR_DIFFICULTY;
	}

	String LTEProjectManager::_get_chart_path_for_difficulty(const Dictionary& difficulty_dict, const LTESourceMonitorServer::GameDifficulty difficulty) const {
		String base_name;
		switch (difficulty) {
		case LTESourceMonitorServer::EASY: base_name = "easy"; break;
		case LTESourceMonitorServer::HARD: base_name = "hard"; break;
		case LTESourceMonitorServer::EXPERT: base_name = "expert"; break;
		case LTESourceMonitorServer::MASTER: base_name = "master"; break;
		default: break;
		}
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

	String LTEProjectManager::_get_chart_difficulty_name(const Dictionary& track_info, const String& root_path, const String& chart_path) const {
		Dictionary difficulty_dict = track_info.get("difficulty", Dictionary());
		Array difficulty_keys = difficulty_dict.keys();
		for (int index = 0; index < difficulty_keys.size(); ++index) {
			String key = difficulty_keys[index];
			String difficulty_chart_path = difficulty_dict.get(key, String());
			if (_is_same_project_file_path(root_path, difficulty_chart_path, chart_path)) {
				return key.to_lower();
			}
		}
		return String();
	}

	void LTEProjectManager::_clear_runtime_project_state() {
		if (!api) {
			return;
		}
		if (api->get_file_system_server()) {
			api->get_file_system_server()->clear_project_state();
		}
		if (api->get_workspace_manager()) {
			api->get_workspace_manager()->clear_project_state();
		}
		if (api->get_timeline_server()) {
			api->get_timeline_server()->clear_project_state();
		}
		if (api->get_graph_editor_server()) {
			api->get_graph_editor_server()->clear_project_state();
		}
		if (api->get_source_monitor_server()) {
			api->get_source_monitor_server()->clear_project_state();
		}
		if (api->get_properties_server()) {
			api->get_properties_server()->clear_project_state();
		}
		if (api->get_composition_server()) {
			api->get_composition_server()->clear_project_state();
		}
		if (api->get_note_skin_server()) {
			api->get_note_skin_server()->clear_project_state();
		}
		if (api->get_undo_redo()) {
			api->get_undo_redo()->clear_history();
		}
	}

	Dictionary LTEProjectManager::fetch_project_manager_config() const {
		Dictionary config;
		config["project_paths"] = project_paths;
		config["recently_opened_projects"] = recently_opened_projects;
		config["sort_type"] = int(sort_type);
		config["views_type"] = int(views_type);
		return config;
	}

	String LTEProjectManager::get_project_path() const {
		return project_path;
	}

	bool LTEProjectManager::_is_openable_project_path(const String& path) const {
		String normalized_path = _normalize_project_path(path);
		if (normalized_path.is_empty() || !_has_project_path(project_paths, normalized_path)) {
			return false;
		}
		return !_is_filesystem_root_path(normalized_path) && DirAccess::dir_exists_absolute(normalized_path) && _has_project_info_file(normalized_path);
	}

	String LTEProjectManager::get_last_opened_project() const {
		for (int64_t index = 0; index < recently_opened_projects.size(); index++) {
			String path = recently_opened_projects[index];
			if (_is_openable_project_path(path)) {
				return path;
			}
		}
		for (int64_t index = 0; index < project_paths.size(); index++) {
			String path = project_paths[index];
			if (_is_openable_project_path(path)) {
				return path;
			}
		}
		return String();
	}

	void LTEProjectManager::open_project_manager() {
		if (project_manager_instance_id != 0) {
			Object* object = ObjectDB::get_instance(project_manager_instance_id);
			Window* project_manager = Object::cast_to<Window>(object);
			if (project_manager && !project_manager->is_queued_for_deletion()) {
				project_manager->set_position(Vector2i(0, 0));
				project_manager->show();
				return;
			}
			project_manager_instance_id = 0;
		}
		if (!api) {
			ERR_PRINT("open_project_manager: api is null");
			return;
		}
		Node* editor_instance = api->get_editor_instance();
		if (!editor_instance) {
			ERR_PRINT("open_project_manager: editor_instance is null");
			return;
		}
		Ref<PackedScene> packed_scene = Utils::load<PackedScene>("uid://dt5ptp0pn7gtd");
		if (packed_scene.is_null()) {
			ERR_PRINT("open_project_manager: Failed to load packed scene");
			return;
		}
		if (!packed_scene->can_instantiate()) {
			ERR_PRINT("open_project_manager: packed scene can not instantiate");
			return;
		}
		Node* instance = packed_scene->instantiate();
		if (!instance) {
			ERR_PRINT("open_project_manager: Failed to instantiate scene");
			return;
		}
		Window* window = Object::cast_to<Window>(instance);
		if (!window) {
			memdelete(instance);
			ERR_PRINT("open_project_manager: scene is not window type");
			return;
		}
		project_manager_instance_id = window->get_instance_id();
		editor_instance->add_child(window);
		window->connect("tree_exited", callable_mp(this, &LTEProjectManager::_on_project_manager_instance_tree_exited), CONNECT_ONE_SHOT);
		if (window->has_signal("open_project") && editor_instance->has_method("_on_project_manager_open_project")) {
			window->connect("open_project", Callable(editor_instance, "_on_project_manager_open_project"));
		}
		if (window->has_signal("home_page_requested") && editor_instance->has_method("_on_project_manager_home_page_requested")) {
			window->connect("home_page_requested", Callable(editor_instance, "_on_project_manager_home_page_requested"));
		}
	}

	void LTEProjectManager::open_project(const String& path) {
		String normalized_path = _normalize_project_path(path);
		if (_is_same_project_path(normalized_path, project_path)) return;
		if (!_has_project_path(project_paths, normalized_path)) {
			ERR_PRINT("open_project: Project path not found");
			return;
		}
		if (!_is_openable_project_path(normalized_path)) {
			ERR_PRINT("open_project: Project path is missing or invalid");
			return;
		}

		// 创建各模块所需的文件夹
		Ref<DirAccess> dir = DirAccess::open(normalized_path);
		if (dir.is_null()) {
			ERR_PRINT("open_project: Failed to open dir!");
			return;
		}
		if (!project_path.is_empty()) {
			close_project();
		}
		project_path = normalized_path;
		_clear_runtime_project_state();
		dir->make_dir_recursive(".editor/record");
		dir->make_dir_recursive(".editor/screenshot");
		dir->make_dir_recursive(".editor/temp");
		dir->make_dir_recursive(".editor/workspaces");

		// 如果图标不存在就创建一个
		if (!dir->file_exists("icon.png") &&
			!dir->file_exists("icon.jpg") &&
			!dir->file_exists("icon.jpeg") &&
			!dir->file_exists("icon.svg")) {
			Ref<Image> image = Utils::get_icon_image();
			Error err = image->save_png(project_path + "/icon.png");
			if (err != OK) {
				WARN_PRINT(vformat("open_project: Failed to save icon, error code: %s", err));
			}
		}

		// 加载设置配置文件
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) return;
		settings_config->load_settings_config();

		// 扫描项目文件
		LTEFileSystemServer* file_system_server = LTEFileSystemServer::get_singleton();
		if (!file_system_server) return;
		file_system_server->scan_directory_completely();

		// 添加到最近打开项目
		_remove_project_path(recently_opened_projects, normalized_path);
		recently_opened_projects.insert(0, normalized_path);
		_save_config(false);
		emit_signal("project_opened", normalized_path);
	}

	void LTEProjectManager::close_project() {
		if (project_path.is_empty()) {
			return;
		}
		String closed_project_path = project_path;
		project_path = String();
		_clear_runtime_project_state();
		emit_signal("project_closed", closed_project_path);
	}

	void LTEProjectManager::set_project_manager_sort_type(const SortType type) {
		sort_type = type;
		_save_config();
	}

	void LTEProjectManager::set_project_manager_views_type(const ViewsType type) {
		views_type = type;
		_save_config();
	}

	PackedStringArray LTEProjectManager::get_sorted_project_paths() const {
		Array projects_info;
		PackedStringArray sorted_projects;
		for (int32_t index = 0; index < project_paths.size(); index++) {
			String project_path = project_paths[index];
			Dictionary info;
			info["path"] = project_path;
			info["name"] = _get_project_display_name(project_path);
			projects_info.append(info);
		}
		switch (sort_type) {
		case SORT_A_TO_Z: {
			projects_info.sort_custom(callable_mp(this, &LTEProjectManager::_sort_a_z_compare));
			for (int32_t index = 0; index < projects_info.size(); index++) {
				Dictionary info = projects_info[index];
				String path = info.get("path", "");
				sorted_projects.append(path);
			}
		} break;
		case SORT_Z_TO_A: {
			projects_info.sort_custom(callable_mp(this, &LTEProjectManager::_sort_z_a_compare));
			for (int32_t index = 0; index < projects_info.size(); index++) {
				Dictionary info = projects_info[index];
				String path = info.get("path", "");
				sorted_projects.append(path);
			}
		} break;
		case SORT_DATE_MODIFIED_NEWSET_FIRST: {
			sorted_projects = recently_opened_projects;
		} break;
		case SORT_DATE_MODIFIED_OLDEST_FIRST: {
			PackedStringArray copy = recently_opened_projects;
			copy.reverse();
			sorted_projects = copy;
		} break;
		}
		return sorted_projects;
	}

	bool LTEProjectManager::_sort_a_z_compare(const Dictionary& a, const Dictionary& b) const {
		String a_name = a.get("name", "");
		String b_name = b.get("name", "");
		return a_name.naturalnocasecmp_to(b_name) < 0;
	}

	bool LTEProjectManager::_sort_z_a_compare(const Dictionary& a, const Dictionary& b) const {
		String a_name = a.get("name", "");
		String b_name = b.get("name", "");
		return a_name.naturalnocasecmp_to(b_name) > 0;
	}

	Error LTEProjectManager::create_project(const String& p_path, const String& p_name, const String& p_description, const String& p_desinger, const String& p_illustrator, const String& p_version) {
		String path = _normalize_project_path(p_path);
		if (!Utils::is_absolute_path(path) || _is_filesystem_root_path(path)) return ERR_CANT_CREATE;
		if (p_name.strip_edges().is_empty()) return ERR_INVALID_PARAMETER;
		Error dir_err = DirAccess::make_dir_recursive_absolute(path);
		if (dir_err != OK) return dir_err;
		Dictionary info;
		info["version"] = p_version;
		info["editor_version"] = ProjectSettings::get_singleton()->get_setting("application/config/version", "1.0");
		info["icon"] = String("/icon.png");
		info["desinger"] = p_desinger;
		info["illustrator"] = p_illustrator;
		info["name"] = p_name;
		info["description"] = p_description;
		info["uuid"] = Utils::uuid(Utils::UUID_V7);
		Error info_err = Utils::save_json_file(path + "/info.json", info);
		if (info_err != OK) return info_err;
		Ref<Image> image = Utils::get_icon_image();
		Error err = image->save_png(path + "/icon.png");
		if (err != OK) WARN_PRINT(vformat("create_project: Failed to save icon, error code: %s", err));
		LTEWorkspaceManager* workspace_manager = LTEWorkspaceManager::get_singleton();
		if (workspace_manager) {
			Error workspace_err = workspace_manager->ensure_default_workspaces(path, true);
			if (workspace_err != OK) {
				WARN_PRINT(vformat("create_project: Failed to initialize default workspaces, error code: %s", workspace_err));
			}
		}
		_add_project_path(path);
		emit_signal("project_created", path);
		return OK;
	}

	Error LTEProjectManager::import_project(const String& path, const String& save_path) {
		String project_path = _normalize_project_path(save_path);
		if (!FileAccess::file_exists(path) ||
				project_path.is_empty() ||
				!Utils::is_absolute_path(project_path) ||
				_is_filesystem_root_path(project_path) ||
				!DirAccess::dir_exists_absolute(project_path) ||
				!_is_directory_empty(project_path)) {
			return ERR_INVALID_PARAMETER;
		}
		PackedByteArray buffer = ZipHelper::read_zip_single_file(path, "info.json");
		if (buffer.is_empty()) return ERR_DOES_NOT_EXIST;
		Variant var = JSON::parse_string(buffer.get_string_from_utf8());
		if (var.get_type() != Variant::DICTIONARY) return ERR_INVALID_DATA;
		Dictionary info = var;
		if (!info.has("name")) return ERR_INVALID_DATA;
		Error err = ZipHelper::extract_all_from_zip(path, project_path);
		if (err != OK) return err;
		_add_project_path(project_path);
		emit_signal("project_imported", project_path);
		return OK;
	}

	Error LTEProjectManager::scan_project_dir(const String& path) {
		String project_path = _normalize_project_path(path);
		if (!Utils::is_absolute_path(project_path) || _is_filesystem_root_path(project_path)) return ERR_INVALID_PARAMETER;
		Ref<DirAccess> dir = DirAccess::open(project_path);
		if (dir.is_null()) return ERR_CANT_OPEN;
		if (!dir->file_exists("info.json")) return ERR_FILE_NOT_FOUND;
		Dictionary info = Utils::load_json_file(dir->get_current_dir() + "/info.json");
		if (info.is_empty()) return ERR_INVALID_DATA;
		if (!info.has("name")) return ERR_INVALID_DATA;
		_add_project_path(project_path);
		emit_signal("project_scanned", project_path);
		return OK;
	}

	Dictionary LTEProjectManager::get_project_info(const String& path) {
		String project_path = _normalize_project_path(path);
		Dictionary info;
		info["path"] = project_path;
		info["name"] = _get_project_display_name(project_path);
		info["missing"] = !_has_project_info_file(project_path);
		if (bool(info.get("missing", false))) {
			return info;
		}
		Dictionary loaded_info = Utils::load_json_file(project_path + "/info.json");
		Array keys = loaded_info.keys();
		for (int64_t index = 0; index < keys.size(); index++) {
			Variant key = keys[index];
			info[key] = loaded_info[key];
		}
		info["missing"] = false;
		return info;
	}

	Error LTEProjectManager::run_game_instance_headless(const String& path, const LTESourceMonitorServer::GameDifficulty difficulty, const bool auto_play) {
		Dictionary track_info = get_project_info(path);
		if (track_info.is_empty()) return ERR_INVALID_DATA;
		Dictionary difficulty_dict = track_info.get("difficulty", Dictionary());
		String chart_path = _get_chart_path_for_difficulty(difficulty_dict, difficulty);
		if (chart_path.is_empty()) return ERR_FILE_NOT_FOUND;
		return run_game_instance_headless_by_chart_path(path, chart_path, auto_play);
	}

	Error LTEProjectManager::run_game_instance_headless_by_chart_path(const String& path, const String& chart_path, const bool auto_play) {
		// 获取项目信息
		String project_root_path = _normalize_project_path(path);
		Dictionary track_info = get_project_info(project_root_path);
		if (track_info.is_empty()) return ERR_INVALID_DATA;
		if (!track_info.has("difficulty")) return ERR_INVALID_DATA;
		String abs_chart_path = _normalize_project_file_path(project_root_path, chart_path);
		if (abs_chart_path.is_empty() || !FileAccess::file_exists(abs_chart_path)) return ERR_FILE_NOT_FOUND;
		String difficulty_name = _get_chart_difficulty_name(track_info, project_root_path, abs_chart_path);
		LTESourceMonitorServer::GameDifficulty difficulty = _get_difficulty_from_name(difficulty_name);
		if (difficulty_name.is_empty()) {
			difficulty_name = abs_chart_path.get_file().get_basename();
		}
		// 创建窗口
		String track_name = track_info.get("name", "");
		String track_illustrator = track_info.get("illustrator", "");
		LTEWorkspaceManager* workspace_manager = LTEWorkspaceManager::get_singleton();
		if (!workspace_manager) return ERR_UNCONFIGURED;
		Window* window = workspace_manager->create_empty_window(Vector2i(1152, 648), vformat("%s-%s", track_name, track_illustrator), false);
		if (!window) return ERR_CANT_CREATE;
		// 设置窗口属性
		window->set_content_scale_size(Vector2i(1920, 1080));
		window->set_content_scale_mode(Window::CONTENT_SCALE_MODE_CANVAS_ITEMS);
		window->set_content_scale_aspect(Window::CONTENT_SCALE_ASPECT_EXPAND);
		// 创建背景
		Ref<PackedScene> packed_bg = Utils::load<PackedScene>("uid://bictapl04k1aa");
		if (packed_bg.is_valid() && packed_bg->can_instantiate()) {
			Node* bg = packed_bg->instantiate();
			LTESourceMonitorServer* source_monitor_server = LTESourceMonitorServer::get_singleton();
			Color bg_color = source_monitor_server ? source_monitor_server->get_game_blackground_light_color(difficulty) : Color("#ffffff");
			window->add_child(bg);
			if (bg->has_method("set_glow_color")) bg->call("set_glow_color", bg_color);
			if (bg->has_method("set_debris_color")) bg->call("set_debris_color", bg_color);
		}
		// 创建游戏
		Ref<PackedScene> packed_game = Utils::load<PackedScene>("uid://b7ntx22skglln");
		if (packed_game.is_valid() && packed_game->can_instantiate()) {
			Node* game = packed_game->instantiate();
			if (game->has_method("set_parameters")) {
				window->add_child(game);
				Dictionary parameters;
				parameters["info"] = track_info;
				parameters["difficulty"] = int64_t(difficulty);
				parameters["difficulty_name"] = difficulty_name;
				parameters["root_dir"] = project_root_path;
				parameters["chart_path"] = abs_chart_path;
				parameters["auto_play"] = auto_play;
				game->call_deferred("set_parameters", parameters);	// 延迟调用
			}
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
		window->show();
		return OK;
	}

	Error LTEProjectManager::rename_project(const String& p_path, const String& new_name) {
		String project_path = _normalize_project_path(p_path);
		// 查询该项目是否存在已记录的项目列表中
		if (!_has_project_path(project_paths, project_path)) return ERR_QUERY_FAILED;
		// 获取项目信息
		Dictionary track_info = get_project_info(project_path);
		if (track_info.is_empty()) return ERR_INVALID_DATA;
		// 修改项目名并保存
		track_info["name"] = new_name;
		String info_path = project_path + "/info.json";
		Utils::save_json_file(info_path, track_info);
		// 修改目录名
		String new_project_path = _normalize_project_path(project_path.get_base_dir() + "/" + new_name);
		Error err = DirAccess::rename_absolute(project_path, new_project_path);
		if (err != OK) return err;
		// 同步项目列表和最近打开的项目列表
		for (int64_t index = 0; index < project_paths.size(); index++) {
			String path = project_paths[index];
			if (_is_same_project_path(path, project_path)) {
				project_paths.set(index, new_project_path);
			}
		}
		for (int64_t index = 0; index < recently_opened_projects.size(); index++) {
			String path = recently_opened_projects[index];
			if (_is_same_project_path(path, project_path)) {
				recently_opened_projects.set(index, new_project_path);
			}
		}
		if (_is_same_project_path(this->project_path, project_path)) {
			this->project_path = new_project_path;
		}
		_save_config();
		emit_signal("project_renamed", project_path, new_project_path);
		return OK;
	}

	Error LTEProjectManager::remove_project(const String& p_path, const bool move_to_trash) {
		String project_path = _normalize_project_path(p_path);
		// 查询该项目是否存在已记录的项目列表中
		if (!_has_project_path(project_paths, project_path)) return ERR_QUERY_FAILED;
		// 如果要则移动至系统回收站
		bool moved_to_trash = false;
		if (move_to_trash && DirAccess::dir_exists_absolute(project_path)) {
			Error err = OS::get_singleton()->move_to_trash(project_path);
			if (err != OK) return err;
			moved_to_trash = true;
		}
		// 同步项目列表和最近打开的项目列表
		_remove_project_path(project_paths, project_path);
		_remove_project_path(recently_opened_projects, project_path);
		if (_is_same_project_path(this->project_path, project_path)) {
			this->project_path = String();
		}
		_save_config();
		emit_signal("project_removed", project_path, moved_to_trash);
		return OK;
	}
}
