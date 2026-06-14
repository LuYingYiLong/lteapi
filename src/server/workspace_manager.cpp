#include "workspace_manager.h"

#include "project_manager.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>

namespace godot {
	LTEWorkspaceManager* LTEWorkspaceManager::singleton = nullptr;
	static const char* DEFAULT_WORKSPACES_DIR = "res://datas/default_workspaces";
	static const char* DEFAULT_WORKSPACE_NAMES[] = {
		"charting",
		"composition",
		"shadering",
		"skin"
	};
	static const int DEFAULT_WORKSPACE_COUNT = 4;
	static const char* TIMELINE_PANEL_UUID = "0198e50b-c5be-74e9-a19e-3fb78c3b035a";
	static const char* COMPOSITION_TIMELINE_PANEL_UUID = "019e0da6-3257-7a9f-9d3a-776dc3f575fe";
	static const char* SHADER_EDITOR_PANEL_UUID = "019a76fd-5439-7c3d-84eb-4fa99d9caa23";
	static const char* NOTE_SKIN_DESIGNER_PANEL_UUID = "019e24dd-d8d3-7945-bd27-e711d5c54c23";

	void LTEWorkspaceManager::_bind_methods() {
		BIND_ENUM_CONSTANT(SPLIT_HORZ_LEFT);
		BIND_ENUM_CONSTANT(SPLIT_HORZ_RIGHT);
		BIND_ENUM_CONSTANT(SPLIT_VERT_TOP);
		BIND_ENUM_CONSTANT(SPLIT_VERT_BOTTOM);
		ClassDB::bind_method(D_METHOD("_resume_coroutine", "handle_address", "wait_token"), &LTEWorkspaceManager::_resume_coroutine);
		ClassDB::bind_method(D_METHOD("scan_workspace_directory"), &LTEWorkspaceManager::scan_workspace_directory);
		ClassDB::bind_method(D_METHOD("get_layout_files"), &LTEWorkspaceManager::get_layout_files);
		ClassDB::bind_method(D_METHOD("set_layout_files_order", "ordered_layout_files"), &LTEWorkspaceManager::set_layout_files_order);
		ClassDB::bind_method(D_METHOD("get_dockable_layout_count"), &LTEWorkspaceManager::get_dockable_layout_count);
		ClassDB::bind_method(D_METHOD("get_current_layout_panels"), &LTEWorkspaceManager::get_current_layout_panels);
		ClassDB::bind_method(D_METHOD("serialize_layout"), &LTEWorkspaceManager::serialize_layout);
		ClassDB::bind_method(D_METHOD("open_layout", "file_path", "force"), &LTEWorkspaceManager::open_layout, DEFVAL(false));
		ClassDB::bind_method(D_METHOD("float_layout", "file_path", "window_size"), &LTEWorkspaceManager::float_layout);
		ClassDB::bind_method(D_METHOD("is_layout_floating", "file_path"), &LTEWorkspaceManager::is_layout_floating);
		ClassDB::bind_method(D_METHOD("get_last_opened_layout"), &LTEWorkspaceManager::get_last_opened_layout);
		ClassDB::bind_method(D_METHOD("save_layout", "file_path"), &LTEWorkspaceManager::save_layout);
		ClassDB::bind_method(D_METHOD("layout_fit_to_window", "file_path"), &LTEWorkspaceManager::layout_fit_to_window);
		ClassDB::bind_method(D_METHOD("rename_layout", "file_path", "new_name"), &LTEWorkspaceManager::rename_layout);
		ClassDB::bind_method(D_METHOD("remove_layout", "file_path"), &LTEWorkspaceManager::remove_layout);
		ClassDB::bind_method(D_METHOD("export_layout", "file_path", "export_path"), &LTEWorkspaceManager::export_layout);
		ClassDB::bind_method(D_METHOD("copy_layout", "file_path"), &LTEWorkspaceManager::copy_layout);
		ClassDB::bind_method(D_METHOD("reset_panel", "file_path", "new_panel_uuid", "old_runtime_uuid"), &LTEWorkspaceManager::reset_panel);
		ClassDB::bind_method(D_METHOD("change_panel", "file_path", "panel", "panel_uuid", "runtime_uuid"), &LTEWorkspaceManager::change_panel);
		ClassDB::bind_method(D_METHOD("add_panel", "file_path", "panel", "panel_uuid", "split_direction"), &LTEWorkspaceManager::add_panel);
		ClassDB::bind_method(D_METHOD("close_panel", "file_path", "panel"), &LTEWorkspaceManager::close_panel);
		ClassDB::bind_method(D_METHOD("enter_panel_focus_mode", "panel"), &LTEWorkspaceManager::enter_panel_focus_mode);
		ClassDB::bind_method(D_METHOD("exit_panel_focus_mode"), &LTEWorkspaceManager::exit_panel_focus_mode);
		ClassDB::bind_method(D_METHOD("is_panel_focus_mode_active"), &LTEWorkspaceManager::is_panel_focus_mode_active);
		ClassDB::bind_method(D_METHOD("create_empty_window", "window_size", "title", "show_now", "auto_release"), &LTEWorkspaceManager::create_empty_window, DEFVAL(Vector2i(1152, 648)), DEFVAL("New Window"), DEFVAL(true), DEFVAL(true));
		ClassDB::bind_method(D_METHOD("open_window_instance", "window_size", "panel_uuid", "runtime_uuid", "parameters"), &LTEWorkspaceManager::open_window_instance, DEFVAL(""), DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("duplicate_panel_into_new_window", "panel", "panel_uuid", "runtime_uuid", "title"), &LTEWorkspaceManager::duplicate_panel_into_new_window, DEFVAL(""), DEFVAL(""));
		ClassDB::bind_method(D_METHOD("open_window_instance_from_scene", "window_size", "title", "packed_scene"), &LTEWorkspaceManager::open_window_instance_from_scene);
		ClassDB::bind_method(D_METHOD("add_scene_to_editor", "packed_scene", "parameters"), &LTEWorkspaceManager::add_scene_to_editor, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("get_all_layout_runtime_uuids"), &LTEWorkspaceManager::get_all_layout_runtime_uuids);
		ADD_SIGNAL(MethodInfo("scan_layout_files_completed"));
		ADD_SIGNAL(MethodInfo("layout_opened", PropertyInfo(Variant::STRING, "file_path")));
		ADD_SIGNAL(MethodInfo("layout_renamed", PropertyInfo(Variant::STRING, "from"), PropertyInfo(Variant::STRING, "to")));
		ADD_SIGNAL(MethodInfo("layout_removed", PropertyInfo(Variant::STRING, "removed_file"), PropertyInfo(Variant::STRING, "last_opened_file")));
		ADD_SIGNAL(MethodInfo("layout_copyed", PropertyInfo(Variant::STRING, "from"), PropertyInfo(Variant::STRING, "to")));
		ADD_SIGNAL(MethodInfo("layout_order_changed"));
		ADD_SIGNAL(MethodInfo("layout_floating_changed", PropertyInfo(Variant::STRING, "file_path"), PropertyInfo(Variant::BOOL, "floating")));
		ADD_SIGNAL(MethodInfo("panel_focused"));
		ADD_SIGNAL(MethodInfo("panel_unfocused"));
	}

	LTEWorkspaceManager::LTEWorkspaceManager() {
		if (singleton == nullptr) {
			singleton = this;
		}
		if (LTEAPI::get_singleton()) {
			api = LTEAPI::get_singleton();
		}
		settings_config = LTESettingsConfig::get_singleton();
		component_registry = LTEComponentRegistry::get_singleton();
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		if (project_manager) {
			project_manager->connect("project_opened", callable_mp(this, &LTEWorkspaceManager::_on_project_opened));
		}
		_connect_plugin_signals();
	}

	LTEWorkspaceManager::~LTEWorkspaceManager() {
		if (singleton == this) {
			singleton = nullptr;
		}
		api = nullptr;
		settings_config = nullptr;
		component_registry = nullptr;
	}

	LTEWorkspaceManager* LTEWorkspaceManager::get_singleton() {
		return singleton;
	}

	void LTEWorkspaceManager::_on_project_opened(const String& path) {
		scan_workspace_directory();
		if (get_layout_files().is_empty()) {
			PackedStringArray copied_layout_files;
			_copy_default_workspace_templates(path, false, copied_layout_files);
			scan_workspace_directory();
			if (settings_config) {
				settings_config->settings_config_garbage_collection();
			}
			String first_layout_path = copied_layout_files.is_empty() ? _get_first_available_layout() : copied_layout_files[0];
			if (!first_layout_path.is_empty()) {
				open_layout(first_layout_path);
			}
		}
		else {
			if (settings_config) {
				settings_config->settings_config_garbage_collection();
			}
			_restore_floating_workspaces();
			String last_opened_layout = get_last_opened_layout();
			if (last_opened_layout.is_empty() || !layout_files.has(last_opened_layout) || is_layout_floating(last_opened_layout)) {
				last_opened_layout = _get_first_dockable_layout();
			}
			if (!last_opened_layout.is_empty()) {
				open_layout(last_opened_layout);
			}
		}
	}

	void LTEWorkspaceManager::_find_panel_option_button(Node* node, String current_path, Dictionary& layout_dict) {
		String node_path = current_path + "/" + String(node->get_name());
		PanelOptionButton* panel_option_button = Object::cast_to<PanelOptionButton>(node);
		if (panel_option_button) {
			Dictionary data;
			data["panel"] = panel_option_button->get_current_panel_uuid();
			String current_uuid = panel_option_button->get_current_runtime_uuid();
			if (current_uuid.is_empty()) current_uuid = Utils::uuid(Utils::UUID_V7);
			data["uuid"] = current_uuid;
			Array children = layout_dict["children"];
			children.append(data);
		}
		for (int i = 0; i < node->get_child_count(); i++) {
			Node* child_node = node->get_child(i);
			if (!child_node) continue;
			_find_panel_option_button(child_node, node_path, layout_dict);
		}
	}

	void LTEWorkspaceManager::_resume_coroutine(uint64_t handle_address, uint64_t wait_token) {
		(void)wait_token;
		auto handle = std::coroutine_handle<>::from_address(reinterpret_cast<void*>(handle_address));
		if (handle && !handle.done()) {
			handle.resume();
		}
	}

	void LTEWorkspaceManager::_on_split_drag_ended(const String& file_path, Node* root_node) {
		_save_layout_from_root(file_path, root_node);
	}

	void LTEWorkspaceManager::_save_layout_from_root(const String& file_path, Node* root_node) {
		if (!root_node) {
			ERR_PRINT("save_layout: root_node is null");
			return;
		}
		Dictionary layout_dict = _serialize_node(root_node);
		if (layout_dict.is_empty()) {
			ERR_PRINT("save_layout: layout_dict is empty");
			return;
		}
		_normalize_layout_root(layout_dict, file_path);
		Utils::save_json_file(file_path, layout_dict);
	}

	void LTEWorkspaceManager::_on_window_instance_close_requested(Window* window) {
		window->queue_free();
	}

	void LTEWorkspaceManager::_on_window_instance_visibility_changed(Window* window) {
		if (window->is_visible()) {
			Ref<RefCounted> dwm_ref;
			if (Utils::check_dwm_at_runtime()) {
				Color title_bar_color("#282828");
				Color border_color("#5a5a5a");
				if (window->has_meta("dwm_title_bar_color")) {
					title_bar_color = window->get_meta("dwm_title_bar_color");
				}
				if (window->has_meta("dwm_border_color")) {
					border_color = window->get_meta("dwm_border_color");
				}
				ClassDBSingleton* class_db = ClassDBSingleton::get_singleton();
				if (class_db) {
					class_db->class_call_static("DWM", "set_title_bar_color", window, title_bar_color, true);
					class_db->class_call_static("DWM", "set_border_color", window, border_color);
				}
			}
			else {
				WARN_PRINT("DWM class not found");
			}
		}
	}

	void LTEWorkspaceManager::scan_workspace_directory() {
		layout_files.clear();
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String project_path = project_manager ? project_manager->get_project_path() : String();
		String root_dir = project_path + "/.editor/workspaces";
		Ref<DirAccess> dir = DirAccess::open(root_dir);
		if (dir.is_null()) {
			WARN_PRINT(vformat("Failed to open workspace directory: %s", root_dir));
			return;
		}
		dir->list_dir_begin();
		String file_name = dir->get_next();
		while (!file_name.is_empty()) {
			if (!dir->current_is_dir() && (file_name.ends_with(".layout") || file_name.ends_with(".json"))) {
				String abs_path = root_dir + "/" + file_name;
				layout_files.append(abs_path);
			}
			file_name = dir->get_next();
		}
		dir->list_dir_end();
		_apply_workspace_tabs_order();
		emit_signal("scan_layout_files_completed");
	}

	void LTEWorkspaceManager::clear_project_state() {
		layout_load_generation++;
		_close_floating_workspace_windows();
		is_opened_layout = false;
		layout_files.clear();
		current_layout_panels.clear();
		floating_workspace_window_ids.clear();
		focused_panel_parent_path = NodePath();
		focused_panel_index = -1;
		current_opened_layout = String();
		_panel_owner_cache.clear();
		_hot_replace_map.clear();
	}

	Error LTEWorkspaceManager::ensure_default_workspaces(const String& project_path, bool include_all) {
		PackedStringArray copied_layout_files;
		return _copy_default_workspace_templates(project_path, include_all, copied_layout_files);
	}

	PackedStringArray LTEWorkspaceManager::get_layout_files() const {
		return layout_files;
	}

	void LTEWorkspaceManager::set_layout_files_order(const PackedStringArray& ordered_layout_files) {
		PackedStringArray new_layout_files;
		for (int64_t index = 0; index < ordered_layout_files.size(); index++) {
			String file_path = ordered_layout_files[index];
			if (!layout_files.has(file_path) || new_layout_files.has(file_path)) {
				continue;
			}
			new_layout_files.append(file_path);
		}
		for (int64_t index = 0; index < layout_files.size(); index++) {
			String file_path = layout_files[index];
			if (!new_layout_files.has(file_path)) {
				new_layout_files.append(file_path);
			}
		}
		layout_files = new_layout_files;
		_save_workspace_tabs_order();
		emit_signal("layout_order_changed");
	}

	int32_t LTEWorkspaceManager::get_dockable_layout_count() const {
		int32_t count = 0;
		for (int64_t index = 0; index < layout_files.size(); index++) {
			if (!is_layout_floating(layout_files[index])) {
				count++;
			}
		}
		return count;
	}

	void LTEWorkspaceManager::_apply_workspace_tabs_order() {
		layout_files.sort();
		if (!settings_config) {
			return;
		}
		if (settings_config->workspace_tabs.is_empty()) {
			_save_workspace_tabs_order();
			return;
		}
		PackedStringArray ordered_layout_files;
		for (int64_t order_index = 0; order_index < settings_config->workspace_tabs.size(); order_index++) {
			String order_entry = settings_config->workspace_tabs[order_index];
			for (int64_t file_index = 0; file_index < layout_files.size(); file_index++) {
				String file_path = layout_files[file_index];
				if (ordered_layout_files.has(file_path)) {
					continue;
				}
				if (_is_same_workspace_tab(order_entry, file_path)) {
					ordered_layout_files.append(file_path);
					break;
				}
			}
		}
		for (int64_t index = 0; index < layout_files.size(); index++) {
			String file_path = layout_files[index];
			if (!ordered_layout_files.has(file_path)) {
				ordered_layout_files.append(file_path);
			}
		}
		layout_files = ordered_layout_files;
		_save_workspace_tabs_order();
	}

	void LTEWorkspaceManager::_save_workspace_tabs_order() {
		if (!settings_config) {
			return;
		}
		Array workspace_tabs;
		for (int64_t index = 0; index < layout_files.size(); index++) {
			workspace_tabs.append(layout_files[index].get_file());
		}
		settings_config->workspace_tabs = workspace_tabs;
		settings_config->save_settings_config(false);
	}

	bool LTEWorkspaceManager::_is_same_workspace_tab(const String& order_entry, const String& file_path) const {
		String file_name = file_path.get_file();
		String base_name = file_name.get_slice(".", 0);
		return order_entry == file_path || order_entry == file_name || order_entry == base_name;
	}

	PackedStringArray LTEWorkspaceManager::_get_default_workspace_templates(bool include_all) const {
		PackedStringArray template_files;
		int count = include_all ? DEFAULT_WORKSPACE_COUNT : 1;
		for (int index = 0; index < count; index++) {
			String template_path = String(DEFAULT_WORKSPACES_DIR).path_join(String(DEFAULT_WORKSPACE_NAMES[index]) + ".json");
			if (!FileAccess::file_exists(template_path)) {
				WARN_PRINT(vformat("Default workspace template not found: %s", template_path));
				continue;
			}
			template_files.append(template_path);
		}
		return template_files;
	}

	Error LTEWorkspaceManager::_copy_default_workspace_templates(const String& project_path, bool include_all, PackedStringArray& copied_layout_files) const {
		copied_layout_files.clear();
		String normalized_project_path = project_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_project_path.is_empty()) {
			return ERR_INVALID_PARAMETER;
		}
		String workspace_dir = normalized_project_path.path_join(".editor/workspaces");
		Error dir_error = DirAccess::make_dir_recursive_absolute(workspace_dir);
		if (dir_error != OK) {
			ERR_PRINT(vformat("Failed to create workspace directory: %s. Error Code: %d", workspace_dir, dir_error));
			return dir_error;
		}
		PackedStringArray template_files = _get_default_workspace_templates(include_all);
		Error first_error = OK;
		for (int64_t index = 0; index < template_files.size(); index++) {
			String template_path = template_files[index];
			Dictionary layout = Utils::load_json_file(template_path);
			if (layout.is_empty()) {
				if (first_error == OK) {
					first_error = ERR_INVALID_DATA;
				}
				WARN_PRINT(vformat("Default workspace template is empty: %s", template_path));
				continue;
			}
			String save_path = workspace_dir.path_join(template_path.get_file());
			Error save_error = Utils::save_json_file(save_path, layout);
			if (save_error != OK) {
				if (first_error == OK) {
					first_error = save_error;
				}
				WARN_PRINT(vformat("Failed to save default workspace: %s. Error Code: %d", save_path, save_error));
				continue;
			}
			copied_layout_files.append(save_path);
		}
		return first_error;
	}

	String LTEWorkspaceManager::_get_first_available_layout() const {
		if (layout_files.is_empty()) {
			return String();
		}
		return layout_files[0];
	}

	String LTEWorkspaceManager::_resolve_workspace_file_path(const String& file_path) const {
		for (int64_t index = 0; index < layout_files.size(); index++) {
			String layout_file = layout_files[index];
			if (_is_same_workspace_tab(file_path, layout_file)) {
				return layout_file;
			}
		}
		return file_path;
	}

	String LTEWorkspaceManager::_get_first_dockable_layout(const String& excluded_file_path) const {
		for (int64_t index = 0; index < layout_files.size(); index++) {
			String layout_file = layout_files[index];
			if (!excluded_file_path.is_empty() && _is_same_workspace_tab(excluded_file_path, layout_file)) {
				continue;
			}
			if (!is_layout_floating(layout_file)) {
				return layout_file;
			}
		}
		return String();
	}

	String LTEWorkspaceManager::_get_workspace_state_key(const String& file_path) const {
		String resolved_path = _resolve_workspace_file_path(file_path);
		return resolved_path.get_file();
	}

	bool LTEWorkspaceManager::_has_floating_workspace_marker(const String& file_path) const {
		if (!settings_config) {
			return false;
		}
		String resolved_path = _resolve_workspace_file_path(file_path);
		for (int64_t index = 0; index < settings_config->floating_workspaces.size(); index++) {
			String entry = settings_config->floating_workspaces[index];
			if (_is_same_workspace_tab(entry, resolved_path)) {
				return true;
			}
		}
		return false;
	}

	void LTEWorkspaceManager::_set_floating_workspace_marker(const String& file_path, bool floating) {
		if (!settings_config) {
			return;
		}
		String state_key = _get_workspace_state_key(file_path);
		Array floating_workspaces = settings_config->floating_workspaces.duplicate();
		for (int64_t index = floating_workspaces.size() - 1; index >= 0; index--) {
			String entry = floating_workspaces[index];
			if (_is_same_workspace_tab(entry, file_path)) {
				floating_workspaces.remove_at(index);
			}
		}
		if (floating) {
			floating_workspaces.append(state_key);
		}
		settings_config->floating_workspaces = floating_workspaces;
		settings_config->save_settings_config(false);
	}

	bool LTEWorkspaceManager::is_layout_floating(const String& file_path) const {
		return _has_floating_workspace_marker(file_path);
	}

	Array LTEWorkspaceManager::get_current_layout_panels() const {
		return current_layout_panels;
	}

	Dictionary LTEWorkspaceManager::serialize_layout() {
		Node* root_node = api->get_editor_workspace_root();
		if (!root_node) {
			ERR_PRINT("serialize_layout: root_node is null");
			return Dictionary();
		}
		return _serialize_node(root_node);
	}

	Dictionary LTEWorkspaceManager::_serialize_node(Node* root_node) {
		Dictionary layout_dict;
		layout_dict["type"] = root_node->get_class();
		layout_dict["name"] = String(root_node->get_name());
		Array children;
		layout_dict["children"] = children;
		SplitContainer* split = Object::cast_to<SplitContainer>(root_node);
		if (split) {
			double split_offset = (double)split->get_split_offset();
			Node* parent = root_node->get_parent();
			Control* parent_control = parent ? Object::cast_to<Control>(parent) : nullptr;
			if (parent_control) {
				Vector2 parent_size = parent_control->get_size();
				if (root_node->is_class("HSplitContainer")) {
					double size_x = parent_size.x > 0 ? parent_size.x : 1.0;
					layout_dict["ratio"] = UtilityFunctions::clampf(split_offset / size_x, 0.001, 1.000);
				}
				else {
					double size_y = parent_size.y > 0 ? parent_size.y : 1.0;
					layout_dict["ratio"] = UtilityFunctions::clampf(split_offset / size_y, 0.001, 1.000);
				}
			}
			else {
				layout_dict["ratio"] = 0.5;
			}
		}
		for (int i = 0; i < root_node->get_child_count(); i++) {
			Node* child_node = root_node->get_child(i);
			if (!child_node) continue;
			if (child_node->is_class("SplitContainer")) {
				children.append(_serialize_node(child_node));
			}
			else _find_panel_option_button(child_node, "", layout_dict);
		}
		return layout_dict;
	}

	void LTEWorkspaceManager::open_layout(const String& file_path, bool force) {
		if (!force && file_path == current_opened_layout) {
			WARN_PRINT("open_layout: can not open same layout!");
			return;
		}
		Node* root_node = api->get_editor_workspace_root();
		if (!root_node) {
			ERR_PRINT("serialize_layout: root_node is null");
			return;
		}
		Control* root_control = Object::cast_to<Control>(root_node);
		if (!root_control) {
			ERR_PRINT("Root node must be a Control.");
			return;
		}
		Dictionary layout_dict = Utils::load_json_file(file_path);
		if (layout_dict.is_empty()) {
			ERR_PRINT("load_layout: layout_dict is empty");
			return;
		}
		if (_normalize_layout_root(layout_dict, file_path)) {
			Utils::save_json_file(file_path, layout_dict);
		}
		current_opened_layout = file_path;
		layout_load_generation++;
		int32_t generation = layout_load_generation;
		current_layout_panels.clear();
		_clear_workspace_root_children(root_node);
		Array children = layout_dict.get("children", Array());
		_load_layout_children(children, root_control, file_path, generation, true, root_node->get_instance_id());
		String file_name = file_path.get_file();
		file_name = file_name.get_slice(".", 0);
		is_opened_layout = true;
		settings_config->last_opened_workspace = file_name;
		settings_config->save_settings_config(false);
	}

	void LTEWorkspaceManager::_clear_workspace_root_children(Node* root_node) {
		if (!root_node) {
			return;
		}
		while (root_node->get_child_count() > 0) {
			Node* child = root_node->get_child(0);
			if (!child) {
				break;
			}
			root_node->remove_child(child);
			child->queue_free();
		}
	}

	bool LTEWorkspaceManager::_normalize_layout_root(Dictionary& layout_dict, const String& file_path) const {
		Array children = layout_dict.get("children", Array());
		if (children.size() <= 1) {
			return false;
		}
		int64_t selected_index = -1;
		String preferred_panel_uuid = _get_preferred_workspace_panel_uuid(file_path);
		if (!preferred_panel_uuid.is_empty()) {
			for (int64_t index = 0; index < children.size(); index++) {
				if (_layout_branch_has_panel(children[index], preferred_panel_uuid)) {
					selected_index = index;
					break;
				}
			}
		}
		if (selected_index < 0) {
			selected_index = children.size() - 1;
		}
		Array normalized_children;
		normalized_children.append(children[selected_index]);
		layout_dict["children"] = normalized_children;
		WARN_PRINT(vformat(
			"Workspace layout had multiple root children and was normalized: %s",
			file_path
		));
		return true;
	}

	bool LTEWorkspaceManager::_layout_branch_has_panel(const Variant& branch, const String& panel_uuid) const {
		if (panel_uuid.is_empty() || branch.get_type() != Variant::DICTIONARY) {
			return false;
		}
		Dictionary branch_dict = branch;
		if (String(branch_dict.get("panel", "")) == panel_uuid) {
			return true;
		}
		Array children = branch_dict.get("children", Array());
		for (int64_t index = 0; index < children.size(); index++) {
			if (_layout_branch_has_panel(children[index], panel_uuid)) {
				return true;
			}
		}
		return false;
	}

	String LTEWorkspaceManager::_get_preferred_workspace_panel_uuid(const String& file_path) const {
		String workspace_name = file_path.get_file().get_slice(".", 0);
		if (workspace_name == "charting") {
			return TIMELINE_PANEL_UUID;
		}
		if (workspace_name == "composition") {
			return COMPOSITION_TIMELINE_PANEL_UUID;
		}
		if (workspace_name == "shadering") {
			return SHADER_EDITOR_PANEL_UUID;
		}
		if (workspace_name == "skin") {
			return NOTE_SKIN_DESIGNER_PANEL_UUID;
		}
		return String();
	}

	LTEWorkspaceManager::CoVoid LTEWorkspaceManager::_load_layout_children(const Array& children, Control* parent_node, const String file_path, int32_t generation, bool main_layout, uint64_t save_root_id) {
		// 初始化栈
		std::vector<StackItem> stack;
		stack.push_back({ parent_node->get_instance_id(), children });
		bool reserialize = false;
		while (!stack.empty()) {
			if (main_layout && generation != layout_load_generation) {
				co_return;
			}
			StackItem item = stack.back();
			stack.pop_back();
			Control* parent = Object::cast_to<Control>(ObjectDB::get_instance(item.parent_id));
			if (!parent) continue;
			Array kids = item.kids;
			// 遍历子节点
			for (int i = 0; i < kids.size(); i++) {
				Dictionary child = kids[i];
				// 普通节点分支
				if (child.has("type")) {
					String type_name = child.get("type", "");
					// ClassDB 检查
					if (!ClassDB::class_exists(type_name)) {
						continue;
					}
					// 实例化
					Object* obj = ClassDB::instantiate(type_name);
					Control* node = Object::cast_to<Control>(obj);
					if (!node) {
						if (obj) memdelete(obj); // 如果不是Control，清理内存
						continue;
					}
					parent->add_child(node);
					node->set_name(child.get("name", type_name));
					if (Object::cast_to<SplitContainer>(node)) {
						uint64_t parent_id = parent->get_instance_id();
						uint64_t node_id = node->get_instance_id();
						SceneTree* tree = parent->get_tree();
						if (tree) {
							co_await wait_for_signal(tree, "process_frame");
						}
						if (main_layout && generation != layout_load_generation) {
							co_return;
						}
						parent = Object::cast_to<Control>(ObjectDB::get_instance(parent_id));
						node = Object::cast_to<Control>(ObjectDB::get_instance(node_id));
						if (!node || !parent) continue;
						SplitContainer* split = Object::cast_to<SplitContainer>(node);
						Node* save_root_node = Object::cast_to<Node>(ObjectDB::get_instance(save_root_id));
						if (split && save_root_node) {
							double ratio = child.get("ratio", 0.5);
							if (ratio < 0.0 || ratio > 1.0) {
								ratio = 0.5;
							}
							Vector2 parent_size = parent->get_size();
							if (split->is_class("HSplitContainer")) {
								split->set_split_offset(ratio * parent_size.x);
							}
							else {
								split->set_split_offset(ratio * parent_size.y);
							}
							node->connect("drag_ended", callable_mp(this, &LTEWorkspaceManager::_on_split_drag_ended).bind(file_path, save_root_node));
						}
					}
					// 递归压栈
					if (child.has("children")) {
						if (main_layout && generation != layout_load_generation) {
							co_return;
						}
						stack.push_back({ node->get_instance_id(), child["children"] });
					}
				}
				// 预制窗口分支
				else {
					if (main_layout && generation != layout_load_generation) {
						co_return;
					}
					if (main_layout) {
						current_layout_panels.append(child);
					}
					String window_id = child.get("panel", "");
					Ref<PackedScene> packed_scene = component_registry->get_component_panel_scene(window_id);
					if (packed_scene.is_null()) {
						continue;
					}
					Node* scene = packed_scene->instantiate();
					parent->add_child(scene);
					String uuid = child.get("uuid", "");
					if (uuid.is_empty()) {
						uuid = Utils::uuid(Utils::UUID_V7);
						reserialize = true;
					}
					if (scene->has_method("set_uuid")) {
						scene->call("set_uuid", uuid);
					}
					String owner = component_registry->get_panel_owner(window_id);
					if (!owner.is_empty()) {
						_panel_owner_cache[uuid] = owner;
						_known_panel_owners[window_id] = owner;
					}
					Array children = scene->find_children("PanelOptionButton");
					for (int index = 0; index < children.size(); index++) {
						Node* child = Object::cast_to<Node>(children[index]);
						if (child && child->has_method("set_current_runtime_uuid")) child->call("set_current_runtime_uuid", uuid);
					}
				}
			}
		}
		if (main_layout && generation != layout_load_generation) {
			co_return;
		}
		if (reserialize) {
			Node* serialize_root = Object::cast_to<Node>(ObjectDB::get_instance(save_root_id));
			if (!serialize_root) {
				co_return;
			}
			Dictionary new_layout = main_layout ? serialize_layout() : _serialize_node(serialize_root);
			Utils::save_json_file(file_path, new_layout);
		}
		if (main_layout) {
			emit_signal("layout_opened", file_path);
		}
	}

	bool LTEWorkspaceManager::_load_layout_children_immediate(const Array& children, Control* parent_node, const String& file_path, Node* save_root_node) {
		if (!parent_node || !save_root_node) {
			return false;
		}
		std::vector<StackItem> stack;
		stack.push_back({ parent_node->get_instance_id(), children });
		bool reserialize = false;
		while (!stack.empty()) {
			StackItem item = stack.back();
			stack.pop_back();
			Control* parent = Object::cast_to<Control>(ObjectDB::get_instance(item.parent_id));
			if (!parent) {
				continue;
			}
			Array kids = item.kids;
			for (int32_t index = 0; index < kids.size(); index++) {
				Dictionary child = kids[index];
				if (child.has("type")) {
					String type_name = child.get("type", "");
					if (!ClassDB::class_exists(type_name)) {
						continue;
					}
					Object* object = ClassDB::instantiate(type_name);
					Control* node = Object::cast_to<Control>(object);
					if (!node) {
						if (object) {
							memdelete(object);
						}
						continue;
					}
					parent->add_child(node);
					node->set_name(child.get("name", type_name));
					if (Object::cast_to<SplitContainer>(node)) {
						node->connect("drag_ended", callable_mp(this, &LTEWorkspaceManager::_on_split_drag_ended).bind(file_path, save_root_node));
					}
					if (child.has("children")) {
						stack.push_back({ node->get_instance_id(), child["children"] });
					}
					continue;
				}

				String window_id = child.get("panel", "");
				Ref<PackedScene> packed_scene = component_registry->get_component_panel_scene(window_id);
				if (packed_scene.is_null()) {
					continue;
				}
				Node* scene = packed_scene->instantiate();
				if (!scene) {
					continue;
				}
				parent->add_child(scene);
				String uuid = child.get("uuid", "");
				if (uuid.is_empty()) {
					uuid = Utils::uuid(Utils::UUID_V7);
					reserialize = true;
				}
				if (scene->has_method("set_uuid")) {
					scene->call("set_uuid", uuid);
				}
				String owner = component_registry->get_panel_owner(window_id);
				if (!owner.is_empty()) {
					_panel_owner_cache[uuid] = owner;
					_known_panel_owners[window_id] = owner;
				}
				Array panel_option_buttons = scene->find_children("PanelOptionButton");
				for (int32_t button_index = 0; button_index < panel_option_buttons.size(); button_index++) {
					Node* panel_option_button = Object::cast_to<Node>(panel_option_buttons[button_index]);
					if (panel_option_button && panel_option_button->has_method("set_current_runtime_uuid")) {
						panel_option_button->call("set_current_runtime_uuid", uuid);
					}
				}
			}
		}
		if (reserialize) {
			Dictionary new_layout = _serialize_node(save_root_node);
			Utils::save_json_file(file_path, new_layout);
		}
		return true;
	}

	void LTEWorkspaceManager::float_layout(const String& file_path, const Vector2i& window_size) {
		String resolved_path = _resolve_workspace_file_path(file_path);
		if (resolved_path.is_empty() || !layout_files.has(resolved_path)) {
			ERR_PRINT("float_layout: layout file is not found");
			return;
		}
		if (layout_files.size() <= 1 || get_dockable_layout_count() <= 1) {
			WARN_PRINT("float_layout: at least one docked workspace must remain");
			return;
		}
		if (is_layout_floating(resolved_path)) {
			Window* existing_window = _get_floating_workspace_window(resolved_path);
			if (existing_window) {
				existing_window->show();
				existing_window->grab_focus();
			}
			return;
		}

		bool floating_current_layout = _is_same_workspace_tab(resolved_path, current_opened_layout);
		if (floating_current_layout) {
			save_layout(resolved_path);
		}

		Window* window = _open_floating_workspace_window(resolved_path, window_size, true);
		if (!window) {
			return;
		}

		_set_floating_workspace_marker(resolved_path, true);
		if (floating_current_layout) {
			String next_layout = _get_first_dockable_layout(resolved_path);
			if (!next_layout.is_empty()) {
				open_layout(next_layout, true);
			}
		}
		emit_signal("layout_floating_changed", resolved_path, true);
	}

	Window* LTEWorkspaceManager::_get_floating_workspace_window(const String& file_path) const {
		String resolved_path = _resolve_workspace_file_path(file_path);
		if (!floating_workspace_window_ids.has(resolved_path)) {
			return nullptr;
		}
		uint64_t instance_id = static_cast<uint64_t>(static_cast<int64_t>(floating_workspace_window_ids[resolved_path]));
		Object* object = ObjectDB::get_instance(instance_id);
		Window* window = Object::cast_to<Window>(object);
		if (!window || window->is_queued_for_deletion()) {
			return nullptr;
		}
		return window;
	}

	Window* LTEWorkspaceManager::_open_floating_workspace_window(const String& file_path, const Vector2i& fallback_size, bool restore_state) {
		Window* existing_window = _get_floating_workspace_window(file_path);
		if (existing_window) {
			existing_window->show();
			existing_window->grab_focus();
			return existing_window;
		}

		Dictionary layout_dict = Utils::load_json_file(file_path);
		if (layout_dict.is_empty()) {
			ERR_PRINT("float_layout: layout_dict is empty");
			return nullptr;
		}
		if (_normalize_layout_root(layout_dict, file_path)) {
			Utils::save_json_file(file_path, layout_dict);
		}

		String title = file_path.get_file().get_basename();
		Window* window = create_empty_window(fallback_size, title, false, false);
		if (!window) {
			return nullptr;
		}
		window->set_meta("dwm_title_bar_color", Color("#1b1b1b"));
		window->set_wrap_controls(false);
		PanelContainer* root = memnew(PanelContainer);
		root->set_name("Layout");
		root->set_theme_type_variation("WorkspacePanel");
		root->set_anchors_preset(Control::PRESET_FULL_RECT);
		root->set_offsets_preset(Control::PRESET_FULL_RECT);
		root->set_position(Vector2());
		window->add_child(root);
		MarginContainer* margin_container = memnew(MarginContainer);
		margin_container->set_name("Margin");
		margin_container->set_anchors_preset(Control::PRESET_FULL_RECT);
		margin_container->set_offsets_preset(Control::PRESET_FULL_RECT);
		margin_container->add_theme_constant_override("margin_left", 8);
		margin_container->add_theme_constant_override("margin_top", 8);
		margin_container->add_theme_constant_override("margin_right", 8);
		margin_container->add_theme_constant_override("margin_bottom", 8);
		root->add_child(margin_container);

		window->connect("close_requested", callable_mp(this, &LTEWorkspaceManager::_on_floating_workspace_close_requested).bind(file_path, window));
		window->connect("size_changed", callable_mp(this, &LTEWorkspaceManager::_on_floating_workspace_size_changed).bind(file_path, window->get_instance_id(), margin_container->get_instance_id()));
		window->connect("focus_exited", callable_mp(this, &LTEWorkspaceManager::_on_floating_workspace_state_changed).bind(file_path, window->get_instance_id()));
		window->connect("visibility_changed", callable_mp(this, &LTEWorkspaceManager::_on_floating_workspace_state_changed).bind(file_path, window->get_instance_id()));
		floating_workspace_window_ids[file_path] = static_cast<int64_t>(window->get_instance_id());

		if (restore_state) {
			_apply_floating_workspace_window_state(file_path, window, fallback_size);
		}
		_resize_floating_workspace_root(window);
		window->show();
		_resize_floating_workspace_root(window);
		Array children = layout_dict.get("children", Array());
		if (!_load_layout_children_immediate(children, margin_container, file_path, margin_container)) {
			window->queue_free();
			floating_workspace_window_ids.erase(file_path);
			return nullptr;
		}
		_fit_floating_workspace_on_frame(file_path, margin_container->get_instance_id(), 2);
		return window;
	}

	void LTEWorkspaceManager::_resize_floating_workspace_root(Window* window) {
		if (!window || window->is_queued_for_deletion()) {
			return;
		}
		Vector2 window_size = window->get_visible_rect().size;
		if (window_size.x <= 0.0 || window_size.y <= 0.0) {
			window_size = Vector2(window->get_size());
		}
		for (int32_t index = 0; index < window->get_child_count(); index++) {
			PanelContainer* panel_container = Object::cast_to<PanelContainer>(window->get_child(index));
			if (!panel_container || String(panel_container->get_name()) != "Layout") {
				continue;
			}
			panel_container->set_position(Vector2());
			panel_container->set_size(window_size);
		}
	}

	void LTEWorkspaceManager::_restore_floating_workspaces() {
		if (!settings_config || layout_files.size() <= 1) {
			if (settings_config && !settings_config->floating_workspaces.is_empty()) {
				settings_config->floating_workspaces.clear();
				settings_config->save_settings_config(false);
			}
			return;
		}
		Array floating_workspaces = settings_config->floating_workspaces.duplicate();
		Array valid_workspaces;
		for (int64_t index = 0; index < floating_workspaces.size(); index++) {
			String entry = floating_workspaces[index];
			String layout_file = _resolve_workspace_file_path(entry);
			if (layout_file.is_empty() || !layout_files.has(layout_file)) {
				continue;
			}
			valid_workspaces.append(_get_workspace_state_key(layout_file));
		}
		if (valid_workspaces.size() >= layout_files.size()) {
			valid_workspaces.remove_at(0);
		}
		settings_config->floating_workspaces = valid_workspaces;
		for (int64_t index = 0; index < valid_workspaces.size(); index++) {
			String layout_file = _resolve_workspace_file_path(String(valid_workspaces[index]));
			if (!layout_file.is_empty() && layout_files.has(layout_file)) {
				_open_floating_workspace_window(layout_file, Vector2i(1152, 648), true);
			}
		}
		settings_config->save_settings_config(false);
	}

	void LTEWorkspaceManager::_close_floating_workspace_windows() {
		Array keys = floating_workspace_window_ids.keys();
		for (int64_t index = 0; index < keys.size(); index++) {
			String file_path = keys[index];
			Window* window = _get_floating_workspace_window(file_path);
			floating_workspace_window_ids.erase(file_path);
			if (window && !window->is_queued_for_deletion()) {
				_save_floating_workspace_window_state(file_path, window);
				window->queue_free();
			}
		}
		floating_workspace_window_ids.clear();
	}

	void LTEWorkspaceManager::_save_floating_workspace_window_state(const String& file_path, Window* window) {
		if (!settings_config || !window || window->is_queued_for_deletion()) {
			return;
		}
		Vector2i position = window->get_position();
		Vector2i size = window->get_size();
		Dictionary state;
		state["screen"] = _get_screen_for_window_rect(position, size, window->get_current_screen());
		state["position"] = position;
		state["size"] = size;
		state["mode"] = static_cast<int64_t>(window->get_mode());
		settings_config->workspace_window_states[_get_workspace_state_key(file_path)] = state;
		settings_config->save_settings_config(false);
	}

	void LTEWorkspaceManager::_apply_floating_workspace_window_state(const String& file_path, Window* window, const Vector2i& fallback_size) {
		if (!settings_config || !window) {
			return;
		}
		Dictionary state = settings_config->workspace_window_states.get(_get_workspace_state_key(file_path), Dictionary());
		if (state.is_empty()) {
			return;
		}
		Dictionary sanitized_state = _sanitize_window_state(state, fallback_size);
		window->set_initial_position(Window::WINDOW_INITIAL_POSITION_ABSOLUTE);
		window->set_current_screen(static_cast<int32_t>(static_cast<int64_t>(sanitized_state.get("screen", 0))));
		window->set_size(sanitized_state.get("size", fallback_size));
		window->set_position(sanitized_state.get("position", Vector2i(0, 0)));
		int32_t mode = static_cast<int32_t>(static_cast<int64_t>(sanitized_state.get("mode", Window::MODE_WINDOWED)));
		window->set_mode(static_cast<Window::Mode>(mode));
	}

	Dictionary LTEWorkspaceManager::_sanitize_window_state(const Dictionary& state, const Vector2i& fallback_size) const {
		Dictionary result;
		DisplayServer* display_server = DisplayServer::get_singleton();
		int32_t screen_count = display_server ? display_server->get_screen_count() : 1;
		int32_t primary_screen = display_server ? display_server->get_primary_screen() : 0;
		Rect2i fallback_rect = display_server ? display_server->screen_get_usable_rect(primary_screen) : Rect2i(Vector2i(0, 0), fallback_size);
		Vector2i size = state.get("size", fallback_size);
		size.x = MAX(size.x, 320);
		size.y = MAX(size.y, 240);
		Vector2i position = state.get("position", fallback_rect.position + (fallback_rect.size - size) / 2);
		int32_t stored_screen = static_cast<int32_t>(static_cast<int64_t>(state.get("screen", primary_screen)));
		int32_t screen = _get_screen_for_window_rect(position, size, stored_screen);
		if (screen < 0 || screen >= screen_count) {
			screen = primary_screen;
		}
		Rect2i usable_rect = display_server ? display_server->screen_get_usable_rect(screen) : Rect2i(Vector2i(0, 0), fallback_size);
		size.x = CLAMP(size.x, 320, MAX(320, usable_rect.size.x));
		size.y = CLAMP(size.y, 240, MAX(240, usable_rect.size.y));
		position.x = CLAMP(position.x, usable_rect.position.x, usable_rect.position.x + MAX(0, usable_rect.size.x - size.x));
		position.y = CLAMP(position.y, usable_rect.position.y, usable_rect.position.y + MAX(0, usable_rect.size.y - size.y));
		int32_t mode = static_cast<int32_t>(static_cast<int64_t>(state.get("mode", Window::MODE_WINDOWED)));
		if (mode == Window::MODE_MINIMIZED) {
			mode = Window::MODE_WINDOWED;
		}
		result["screen"] = screen;
		result["position"] = position;
		result["size"] = size;
		result["mode"] = mode;
		return result;
	}

	int32_t LTEWorkspaceManager::_get_screen_for_window_rect(const Vector2i& position, const Vector2i& size, int32_t fallback_screen) const {
		DisplayServer* display_server = DisplayServer::get_singleton();
		if (!display_server) {
			return 0;
		}
		int32_t screen_count = display_server->get_screen_count();
		if (screen_count <= 0) {
			return 0;
		}
		int32_t primary_screen = display_server->get_primary_screen();
		int32_t safe_fallback_screen = fallback_screen;
		if (safe_fallback_screen < 0 || safe_fallback_screen >= screen_count) {
			safe_fallback_screen = primary_screen;
		}
		Vector2i rect_size = size;
		if (rect_size.x <= 0 || rect_size.y <= 0) {
			return safe_fallback_screen;
		}
		int64_t best_area = 0;
		int32_t best_screen = safe_fallback_screen;
		for (int32_t screen = 0; screen < screen_count; screen++) {
			Rect2i usable_rect = display_server->screen_get_usable_rect(screen);
			int32_t left = MAX(position.x, usable_rect.position.x);
			int32_t top = MAX(position.y, usable_rect.position.y);
			int32_t right = MIN(position.x + rect_size.x, usable_rect.position.x + usable_rect.size.x);
			int32_t bottom = MIN(position.y + rect_size.y, usable_rect.position.y + usable_rect.size.y);
			if (right <= left || bottom <= top) {
				continue;
			}
			int64_t area = static_cast<int64_t>(right - left) * static_cast<int64_t>(bottom - top);
			if (area > best_area) {
				best_area = area;
				best_screen = screen;
			}
		}
		return best_screen;
	}

	void LTEWorkspaceManager::_fit_floating_workspace_on_frame(const String& file_path, uint64_t root_id, int32_t remaining_frames) {
		Node* root_node = Object::cast_to<Node>(ObjectDB::get_instance(root_id));
		if (!root_node || root_node->is_queued_for_deletion()) {
			return;
		}
		Window* window = Object::cast_to<Window>(root_node->get_parent());
		if (window && !window->is_queued_for_deletion()) {
			_resize_floating_workspace_root(window);
		}
		SceneTree* tree = root_node->get_tree();
		if (remaining_frames > 0 && tree) {
			tree->connect("process_frame", callable_mp(this, &LTEWorkspaceManager::_fit_floating_workspace_on_frame).bind(file_path, root_id, remaining_frames - 1), Object::CONNECT_ONE_SHOT | Object::CONNECT_REFERENCE_COUNTED);
			return;
		}
		Dictionary layout_dict = Utils::load_json_file(file_path);
		if (!layout_dict.is_empty()) {
			_fit_to_window(layout_dict, root_node);
		}
		if (remaining_frames == 0 && tree) {
			tree->connect("process_frame", callable_mp(this, &LTEWorkspaceManager::_fit_floating_workspace_on_frame).bind(file_path, root_id, -1), Object::CONNECT_ONE_SHOT | Object::CONNECT_REFERENCE_COUNTED);
		}
	}

	void LTEWorkspaceManager::_on_floating_workspace_size_changed(const String& file_path, uint64_t window_id, uint64_t root_id) {
		Window* window = Object::cast_to<Window>(ObjectDB::get_instance(window_id));
		if (!window || window->is_queued_for_deletion()) {
			return;
		}
		_save_floating_workspace_window_state(file_path, window);
		_resize_floating_workspace_root(window);
		_fit_floating_workspace_on_frame(file_path, root_id, 1);
	}

	void LTEWorkspaceManager::_on_floating_workspace_state_changed(const String& file_path, uint64_t window_id) {
		Window* window = Object::cast_to<Window>(ObjectDB::get_instance(window_id));
		if (!window || window->is_queued_for_deletion()) {
			return;
		}
		_save_floating_workspace_window_state(file_path, window);
	}

	void LTEWorkspaceManager::_on_floating_workspace_close_requested(const String& file_path, Window* window) {
		_save_floating_workspace_window_state(file_path, window);
		String resolved_path = _resolve_workspace_file_path(file_path);
		floating_workspace_window_ids.erase(resolved_path);
		_set_floating_workspace_marker(resolved_path, false);
		emit_signal("layout_floating_changed", resolved_path, false);
		if (window && !window->is_queued_for_deletion()) {
			window->queue_free();
		}
	}

	String LTEWorkspaceManager::get_last_opened_layout() const {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String project_path = project_manager ? project_manager->get_project_path() : String();
		String layout_name = settings_config->last_opened_workspace;
		String abs_path = project_path + "/.editor/workspaces/" + layout_name + ".json";
		return abs_path;
	}

	void LTEWorkspaceManager::save_layout(const String& file_path) {
		Node* root_node = api ? api->get_editor_workspace_root() : nullptr;
		_save_layout_from_root(file_path, root_node);
	}

	void LTEWorkspaceManager::layout_fit_to_window(const String& file_path) {
		Node* root_node = api->get_editor_workspace_root();
		if (!root_node) {
			ERR_PRINT("fit_to_window: root_node is null");
			return;
		}
		Dictionary layout_dict = Utils::load_json_file(file_path);
		if (layout_dict.is_empty()) {
			ERR_PRINT("fit_to_window: layout_dict is empty");
			return;
		}
		_fit_to_window(layout_dict, root_node);
	}

	void LTEWorkspaceManager::rename_layout(const String& file_path, const String& new_name) {
		String base_dir = file_path.get_base_dir();
		String to = base_dir + "/" + new_name + ".json";
		Error err = DirAccess::rename_absolute(file_path, to);
		if (err != OK) {
			ERR_PRINT(vformat("rename_layout: Failed to rename file, error code: %s", err));
			return;
		}
		for (int64_t index = 0; index < layout_files.size(); index++) {
			String path = layout_files[index];
			if (path == file_path) {
				layout_files.set(index, to);
				break;
			}
		}
		if (get_last_opened_layout() == file_path) {
			settings_config->last_opened_workspace = new_name;
			settings_config->save_settings_config(false);
		}
		if (settings_config) {
			String old_key = file_path.get_file();
			String new_key = to.get_file();
			Array floating_workspaces = settings_config->floating_workspaces.duplicate();
			for (int64_t index = 0; index < floating_workspaces.size(); index++) {
				if (_is_same_workspace_tab(String(floating_workspaces[index]), file_path)) {
					floating_workspaces[index] = new_key;
				}
			}
			settings_config->floating_workspaces = floating_workspaces;
			if (settings_config->workspace_window_states.has(old_key)) {
				settings_config->workspace_window_states[new_key] = settings_config->workspace_window_states[old_key];
				settings_config->workspace_window_states.erase(old_key);
			}
			if (floating_workspace_window_ids.has(file_path)) {
				floating_workspace_window_ids[to] = floating_workspace_window_ids[file_path];
				floating_workspace_window_ids.erase(file_path);
			}
			settings_config->save_settings_config(false);
		}
		_save_workspace_tabs_order();
		emit_signal("layout_renamed", file_path, to);
	}

	void LTEWorkspaceManager::remove_layout(const String& file_path) {
		if (layout_files.size() == 1) {
			ERR_PRINT("remove_layout: File total is one");
			return;
		}
		Error err = DirAccess::remove_absolute(file_path);
		if (err != OK) {
			ERR_PRINT(vformat("remove_layout: Failed to remove file, error code: %s", err));
			return;
		}
		for (int64_t index = 0; index < layout_files.size(); index++) {
			String path = layout_files[index];
			if (path == file_path) {
				layout_files.remove_at(index);
				if (index - 1 >= 0) path = layout_files[index - 1];
				else if (index + 1 < layout_files.size()) path = layout_files[index + 1];
				String file_name = path.get_file().get_slice(".", 0);
				settings_config->last_opened_workspace = file_name;
				if (settings_config) {
					_set_floating_workspace_marker(file_path, false);
					settings_config->workspace_window_states.erase(file_path.get_file());
				}
				Window* floating_window = _get_floating_workspace_window(file_path);
				floating_workspace_window_ids.erase(file_path);
				if (floating_window && !floating_window->is_queued_for_deletion()) {
					floating_window->queue_free();
				}
				_save_workspace_tabs_order();
				emit_signal("layout_removed", file_path, path);
				break;
			}
		}
	}

	void LTEWorkspaceManager::_fit_to_window(const Dictionary layout_dict, Node* root_node) {
		if (!layout_dict.has("type")) {
			Array children = layout_dict.get("children", Array());
			for (int index = 0; index < children.size(); index++) {
				if (root_node->get_child_count() <= index) break;
				Dictionary child_dict = children[index];
				Node* child_node = root_node->get_child(index);
				_fit_to_window(child_dict, child_node);
			}
			return;
		}
		String type = layout_dict.get("type", String());
		if (type == "HSplitContainer" || type == "VSplitContainer") {
			Control* parent_control = Object::cast_to<Control>(root_node->get_parent());
			if (!parent_control) return;
			double ratio = layout_dict.get("ratio", 0.5);
			if (ratio < 0.0 || ratio > 1.0) ratio = 0.5;
			if (type == "HSplitContainer") {
				HSplitContainer* split_container = Object::cast_to<HSplitContainer>(root_node);
				if (!split_container) return;
				split_container->set_split_offset(ratio * parent_control->get_size().x);
			}
			else {
				VSplitContainer* split_container = Object::cast_to<VSplitContainer>(root_node);
				if (!split_container) return;
				split_container->set_split_offset(ratio * parent_control->get_size().y);
			}
		}
		Array children = layout_dict.get("children", Array());
		for (int index = 0; index < children.size(); index++) {
			if (root_node->get_child_count() <= index) break;
			Dictionary child_dict = children[index];
			Node* child_node = root_node->get_child(index);
			_fit_to_window(child_dict, child_node);
		}
	}

	void LTEWorkspaceManager::export_layout(const String& file_path, const String& export_path) {
		Dictionary layout_dict = Utils::load_json_file(file_path);
		if (layout_dict.is_empty()) {
			ERR_PRINT("export_layout: layout_dict is empty");
			return;
		}
		_erase_uuid(layout_dict);
		Utils::save_json_file(export_path, layout_dict);
	}

	void LTEWorkspaceManager::copy_layout(const String& file_path) {
		Dictionary layout_dict = Utils::load_json_file(file_path);
		if (layout_dict.is_empty()) {
			ERR_PRINT("copy_layout: layout_dict is empty");
			return;
		}
		_erase_uuid(layout_dict);
		bool too_much = false;
		int count = 2;
		String to;
		do
		{
			if (count > 100) {
				too_much = true;
				break;
			}
			to = file_path.get_base_dir() + "/" + file_path.get_file().get_slice(".", 0) + vformat("%d", count++) + ".json";
		} while (FileAccess::file_exists(to));
		if (too_much) {
			ERR_PRINT("copy_layout: Please check if you have too many files!");
			return;
		}
		Error err = Utils::save_json_file(to, layout_dict);
		if (err != OK) {
			ERR_PRINT(vformat("copy_layout: Failed to save json file, error code: %s", err));
		}
		for (int64_t index = 0; index < layout_files.size(); index++) {
			String path = layout_files[index];
			if (path == file_path) {
				layout_files.insert(index + 1, to);
				break;
			}
		}
		_save_workspace_tabs_order();
		emit_signal("layout_copyed", file_path, to);
	}

	void LTEWorkspaceManager::_erase_uuid(const Dictionary& layout_dict) {
		if (layout_dict.has("children")) {
			Array children = layout_dict["children"];
			for (int i = 0; i < children.size(); i++) {
				Dictionary child = children[i];
				if (child.has("uuid")) {
					child.erase("uuid");
				}
				_erase_uuid(child);
			}
		}
	}

	String LTEWorkspaceManager::reset_panel(const String& file_path, const String& new_panel_uuid, const String& old_uuid) {
		Dictionary layout_dict = Utils::load_json_file(file_path);
		if (layout_dict.is_empty()) {
			ERR_PRINT("export_layout: layout_dict is empty");
			return String();
		}
		Node* root_node = api->get_editor_workspace_root();
		if (!root_node) {
			ERR_PRINT("serialize_layout: root_node is null");
			return String();
		}
		String new_uuid = _find_and_reset_panel(layout_dict, root_node, new_panel_uuid, old_uuid);
		if (new_uuid.is_empty()) {
			ERR_PRINT("reset_window_uuid: uuid is empty");
			return String();
		}
		Utils::save_json_file(file_path, layout_dict);
		return new_uuid;
	}

	String LTEWorkspaceManager::_find_and_reset_panel(Dictionary& layout_dict, Node* root_node, const String& new_panel_uuid, const String& old_runtime_uuid) {
		String reset_uuid;
		std::vector<FARUStackItem> stack;
		stack.push_back({ layout_dict, root_node });
		while (!stack.empty()) {
			// 先拷贝出来，避免引用失效
			FARUStackItem item = stack.back();
			stack.pop_back();
			// 检查 window 和 uuid 都匹配
			if (item.layout_dict.has("uuid")) {
				String uuid = item.layout_dict.get("uuid", String());
				if (uuid == old_runtime_uuid) {
					item.layout_dict["panel"] = new_panel_uuid;
					String new_uuid = Utils::uuid(Utils::UUID_V7);
					item.layout_dict["uuid"] = new_uuid;
					reset_uuid = new_uuid;
					break;
				}
			}
			if (item.layout_dict.has("children")) {
				Array children = item.layout_dict.get("children", Array());
				for (int i = children.size() - 1; i >= 0; i--) {
					Dictionary child_dict = children[i];
					Node* child_node = nullptr;
					// 只有当 node 存在且索引有效时才获取子节点
					if (item.node && i < item.node->get_child_count()) {
						child_node = item.node->get_child(i);
					}
					stack.push_back({ child_dict, child_node });
				}
			}
		}
		return reset_uuid;
	}

	void LTEWorkspaceManager::change_panel(const String& file_path, Node* panel, const String& panel_uuid, const String& runtime_uuid) {
		Node* parent = panel->get_parent();
		int32_t index = panel->get_index();
		Ref<PackedScene> packed_scene = component_registry->get_component_panel_scene(panel_uuid);
		if (packed_scene.is_null()) {
			ERR_PRINT("Failed to load new window scene");
			return;
		}
		panel->queue_free();
		Node* new_panel = packed_scene->instantiate();
		parent->add_child(new_panel);
		parent->move_child(new_panel, index);
		String new_runtime_uuid = reset_panel(file_path, panel_uuid, runtime_uuid);
		if (new_panel->has_method("set_uuid")) new_panel->call("set_uuid", new_runtime_uuid);
		Array children = new_panel->find_children("PanelOptionButton");
		for (int index = 0; index < children.size(); index++) {
			Node* child = Object::cast_to<Node>(children[index]);
			if (child && child->has_method("set_current_runtime_uuid")) child->call("set_current_runtime_uuid", new_runtime_uuid);
		}
	}

	void LTEWorkspaceManager::add_panel(const String& file_path, Node* panel, const String& panel_uuid, const SplitDirection split_direction) {
		_on_add_panel(file_path, panel, panel_uuid, split_direction);
	}

	LTEWorkspaceManager::CoVoid LTEWorkspaceManager::_on_add_panel(const String file_path, Node* panel, const String panel_uuid, const SplitDirection split_direction) {
		if (split_direction < SPLIT_HORZ_LEFT || split_direction > SPLIT_VERT_BOTTOM) {
			ERR_PRINT(vformat("Invalid split direction index passed: %s", String::num(static_cast<int64_t>(split_direction))));
			co_return;
		}
		Ref<PackedScene> packed_scene = component_registry->get_component_panel_scene(panel_uuid);
		if (packed_scene.is_null()) {
			ERR_PRINT("Failed to load window");
			co_return;
		}
		Node* new_window = packed_scene->instantiate();
		SplitContainer* split_container = nullptr;
		if (split_direction == SPLIT_HORZ_LEFT || split_direction == SPLIT_HORZ_RIGHT) {
			split_container = memnew(HSplitContainer);
		}
		else {
			split_container = memnew(VSplitContainer);
		}
		if (!split_container) {
			ERR_PRINT("Failed to memnew SplitContainer!");
			co_return;
		}
		Node* workspace_root = api->get_editor_workspace_root();
		if (!workspace_root) {
			ERR_PRINT("Failed to get workspace root!");
			co_return;
		}
		bool insert_before = split_direction == SPLIT_HORZ_LEFT || split_direction == SPLIT_VERT_TOP;
		if (panel != workspace_root) {
			Node* parent = panel->get_parent();
			parent->add_child(split_container);
			parent->move_child(split_container, panel->get_index());
			parent->remove_child(panel);
			if (insert_before) {
				split_container->add_child(new_window);
				split_container->add_child(panel);
			}
			else {
				split_container->add_child(panel);
				split_container->add_child(new_window);
			}
		}
		else {
			Array children;
			for (int32_t index = 0; index < workspace_root->get_child_count(); index++) {
				Node* child = workspace_root->get_child(index);
				children.append(child);
			}
			workspace_root->add_child(split_container);
			for (int index = 0; index < children.size(); index++) {
				Node* child = Object::cast_to<Node>(children[index]);
				workspace_root->remove_child(child);
			}
			if (insert_before) {
				split_container->add_child(new_window);
			}
			for (int index = 0; index < children.size(); index++) {
				Node* child = Object::cast_to<Node>(children[index]);
				split_container->add_child(child);
			}
			if (!insert_before) {
				split_container->add_child(new_window);
			}
		}
		if (new_window->has_method("set_uuid")) new_window->call("set_uuid", Utils::uuid(Utils::UUID_V7));
		SceneTree* tree = workspace_root->get_tree();
		if (tree) co_await wait_for_signal(tree, "process_frame");
		split_container->set_split_offset(
			split_direction == SPLIT_HORZ_LEFT || split_direction == SPLIT_HORZ_RIGHT ? 
			split_container->get_size().x / 2 : 
			split_container->get_size().y / 2
		);
		split_container->connect("drag_ended", callable_mp(this, &LTEWorkspaceManager::_on_split_drag_ended).bind(file_path, workspace_root));
		save_layout(file_path);
		co_return;
	}

	void LTEWorkspaceManager::close_panel(const String& file_path, Node* panel) {
		_on_close_panel(file_path, panel);
	}

	LTEWorkspaceManager::CoVoid LTEWorkspaceManager::_on_close_panel(const String file_path, Node* panel) {
		Node* workspace_root = api->get_editor_workspace_root();
		if (!workspace_root) {
			ERR_PRINT("Failed to get workspace root!");
			co_return;
		}
		Node* parent = panel->get_parent();
		if (!parent) {
			ERR_PRINT("close_window: parent is null");
			co_return;
		}
		int32_t parent_index = parent->get_index();
		Array children;
		for (int32_t index = 0; index < parent->get_child_count(); index++) {
			Node* child = parent->get_child(index);
			if (child != panel) {
				children.append(child);
			}
		}
		panel->queue_free();
		co_await wait_for_signal(workspace_root->get_tree(), "process_frame");
		for (int index = 0; index < children.size(); index++) {
			Node* child = Object::cast_to<Node>(children[index]);
			parent->remove_child(child);
		}
		Node* grandparent = parent->get_parent();
		for (int index = 0; index < children.size(); index++) {
			Node* child = Object::cast_to<Node>(children[index]);
			grandparent->add_child(child);
			grandparent->move_child(child, parent_index);
		}
		uint64_t parent_id = parent->get_instance_id();
		uint64_t workspace_root_id = workspace_root->get_instance_id();
		if (parent_id != workspace_root_id) {
			parent->queue_free();
		}
		co_await wait_for_signal(workspace_root->get_tree(), "process_frame");
		save_layout(file_path);
	}

	void LTEWorkspaceManager::enter_panel_focus_mode(Node* panel) {
		if (!focused_panel_parent_path.is_empty()) {
			ERR_PRINT("enter_panel_focus_mode: already in panel focus mode");
			return;
		}
		Control* focused_panel = Object::cast_to<Control>(api->get_editor_focused_panel_root());
		if (!focused_panel) {
			ERR_PRINT("enter_panel_focus_mode: focused_panel is null");
			return;
		}
		Control* root_node = Object::cast_to<Control>(api->get_editor_workspace_root());
		if (!root_node) {
			ERR_PRINT("enter_panel_focus_mode: root_node is null");
			return;
		}
		Node* parent = panel->get_parent();
		focused_panel_parent_path = parent->get_path();
		focused_panel_index = panel->get_index();
		parent->remove_child(panel);
		root_node->hide();
		focused_panel->add_child(panel);
		focused_panel->show();
		emit_signal("panel_focused");
	}

	void LTEWorkspaceManager::exit_panel_focus_mode() {
		if (focused_panel_parent_path.is_empty()) {
			ERR_PRINT("exit_panel_focus_mode: already in panel focus mode");
			return;
		}
		Node* editor_instance = api->get_editor_instance();
		if (!editor_instance) {
			ERR_PRINT("exit_panel_focus_mode: editor_instance is null");
			return;
		}
		Control* focused_panel = Object::cast_to<Control>(api->get_editor_focused_panel_root());
		if (!focused_panel) {
			ERR_PRINT("exit_panel_focus_mode: focused_panel is null");
			return;
		}
		Control* root_node = Object::cast_to<Control>(api->get_editor_workspace_root());
		if (!root_node) {
			ERR_PRINT("exit_panel_focus_mode: root_node is null");
			return;
		}
		Node* panel = focused_panel->get_child(0);
		focused_panel->remove_child(panel);
		Node* parent = editor_instance->get_node_or_null(focused_panel_parent_path);
		if (!parent) {
			ERR_PRINT("exit_panel_focus_mode: parent is null");
			return;
		}
		parent->add_child(panel);
		parent->move_child(panel, focused_panel_index);
		focused_panel_parent_path = NodePath();
		focused_panel_index = -1;
		root_node->show();
		focused_panel->hide();
		emit_signal("panel_unfocused");
	}

	bool LTEWorkspaceManager::is_panel_focus_mode_active() const {
		return !focused_panel_parent_path.is_empty();
	}

	Window* LTEWorkspaceManager::create_empty_window(const Vector2i& window_size, const String& title, bool show_now, bool auto_release) {
		Node* editor_instance = api->get_editor_instance();
		if (!editor_instance) {
			ERR_PRINT("create_empty_window: editor_instance is null");
			return nullptr;
		}
		Window* window = memnew(Window);
		window->set_title(title);
		window->set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_MAIN_WINDOW_SCREEN);
		window->set_size(window_size);
		window->set_visible(false);
		window->set_wrap_controls(true);
		window->set_force_native(true);
		editor_instance->add_child(window);
		window->connect("visibility_changed", callable_mp(this, &LTEWorkspaceManager::_on_window_instance_visibility_changed).bind(window));
		if (auto_release) {
			window->connect("close_requested", callable_mp(this, &LTEWorkspaceManager::_on_window_instance_close_requested).bind(window));
		}
		if (show_now) {
			window->show();
		}
		return window;
	}

	void LTEWorkspaceManager::open_window_instance(const Vector2i& window_size, const String& panel_uuid, const String& runtime_uuid, const Dictionary& parameters) {
		Dictionary panel_info = component_registry->get_registered_panel_component_info(panel_uuid);
		if (panel_info.is_empty()) {
			ERR_PRINT("open_window_instance: panel info is empty");
			return;
		}
		String panel_name = panel_info.get("name", "Float Window");
		Ref<PackedScene> packed_scene = component_registry->get_component_panel_scene(panel_uuid);
		if (packed_scene.is_null()) {
			ERR_PRINT("open_window_instance: Failed to load packed scene");
			return;
		}
		Node* panel = packed_scene->instantiate();
		Window* window = create_empty_window(window_size, panel_name, false);
		if (!window) {
			return;
		}
		window->add_child(panel);
		Control* control_panel = Object::cast_to<Control>(panel);
		if (control_panel) {
			control_panel->set_anchors_preset(Control::PRESET_FULL_RECT);
			control_panel->set_offsets_preset(Control::PRESET_FULL_RECT);
			PanelContainer* panel_container = Object::cast_to<PanelContainer>(control_panel);
			if (panel_container) {
				panel_container->set_theme_type_variation("WindowPanel");
			}
		}
		Array children = panel->find_children("PanelOptionButton");
		for (int index = 0; index < children.size(); index++) {
			Control* child = Object::cast_to<Control>(children[index]);
			if (child) child->hide();
		}
		if (panel->has_method("set_uuid")) {
			panel->call("set_uuid", runtime_uuid.is_empty() ? Utils::uuid(Utils::UUID_V7) : runtime_uuid);
		}
		if (!parameters.is_empty() || panel->has_method("set_parameters")) {
			panel->call("set_parameters", parameters);
		}
		window->show();
	}

	void LTEWorkspaceManager::duplicate_panel_into_new_window(Node* panel, const String& panel_uuid, const String&, const String& title) {
		Node* editor_instance = api->get_editor_instance();
		if (!editor_instance) {
			ERR_PRINT("open_window_instance: editor_instance is null");
			return;
		}
		Dictionary panel_info = component_registry->get_registered_panel_component_info(panel_uuid);
		if (panel_info.is_empty()) {
			ERR_PRINT("open_window_instance: panel info is empty");
			return;
		}
		Ref<PackedScene> packed_scene = component_registry->get_component_panel_scene(panel_uuid);
		if (packed_scene.is_null()) {
			ERR_PRINT("open_window_instance: Failed to load packed scene");
			return;
		}
		String panel_name = panel_info.get("name", "Float Window");
		Control* control_panel = Object::cast_to<Control>(panel);
		Window* window = create_empty_window(
			control_panel != nullptr ? Vector2i(control_panel->get_size()) : Vector2i(1152, 648),
			title.is_empty() ? panel_name : title,
			false
		);
		if (!window) {
			return;
		}
		const String copy_runtime_uuid = Utils::uuid(Utils::UUID_V7);
		Node* copy_panel = packed_scene->instantiate();
		if (!copy_panel) {
			window->queue_free();
			return;
		}
		window->add_child(copy_panel);
		control_panel = Object::cast_to<Control>(copy_panel);
		if (control_panel) {
			control_panel->set_anchors_preset(Control::PRESET_FULL_RECT);
			control_panel->set_offsets_preset(Control::PRESET_FULL_RECT);
			PanelContainer* panel_container = Object::cast_to<PanelContainer>(control_panel);
			if (panel_container) {
				panel_container->set_theme_type_variation("WindowPanel");
			}
		}
		Array children = copy_panel->find_children("PanelOptionButton");
		for (int index = 0; index < children.size(); index++) {
			Control* child = Object::cast_to<Control>(children[index]);
			if (child) child->hide();
			if (child && child->has_method("set_current_runtime_uuid")) {
				child->call("set_current_runtime_uuid", copy_runtime_uuid);
			}
		}
		if (copy_panel->has_method("set_uuid")) {
			copy_panel->call("set_uuid", copy_runtime_uuid);
		}
		window->show();
	}
	
	void LTEWorkspaceManager::open_window_instance_from_scene(const Vector2i& window_size, const String& title, Ref<PackedScene> packed_scene) {
		if (packed_scene.is_null() || (packed_scene.is_valid() && !packed_scene->can_instantiate())) {
			ERR_PRINT("open_window_instance_from_scene: packed_scene is null");
			return;
		}
		Window* window = create_empty_window(window_size, title, false);
		Node* instantiated_scene = packed_scene->instantiate();
		window->add_child(instantiated_scene);
		Control* control = Object::cast_to<Control>(instantiated_scene);
		if (control) {
			control->set_anchors_preset(Control::PRESET_FULL_RECT);
			control->set_offsets_preset(Control::PRESET_FULL_RECT);
			PanelContainer* panel_container = Object::cast_to<PanelContainer>(control);
			if (panel_container) {
				panel_container->set_theme_type_variation("WindowPanel");
			}
		}
		window->show();
	}

	void LTEWorkspaceManager::add_scene_to_editor(Ref<PackedScene> packed_scene, const Dictionary& parameters) {
		Node* editor_instance = api->get_editor_instance();
		if (!editor_instance) {
			ERR_PRINT("add_scene_to_editor: editor_instance is null");
			return;
		}
		if (packed_scene.is_null() || !packed_scene->can_instantiate()) {
			ERR_PRINT("add_scene_to_editor: Packed scene is null");
			return;
		}
		Node* scene = packed_scene->instantiate();
		editor_instance->add_child(scene);
		if (scene->has_method("set_parameters")) {
			scene->call("set_parameters", parameters);
		}
	}

	PackedStringArray LTEWorkspaceManager::get_all_layout_runtime_uuids() {
		PackedStringArray uuids;
		for (int32_t index = 0; index < layout_files.size(); index++) {
			String file_path = layout_files[index];
			Dictionary layout_dict = Utils::load_json_file(file_path);
			Array children = layout_dict.get("children", Array());
			_find_layout_uuid(uuids, children);
		}
		return uuids;
	}

	void LTEWorkspaceManager::_find_layout_uuid(PackedStringArray& uuids, const Array& p_children) {
		for (int32_t index = 0; index < p_children.size(); index++) {
			Dictionary child = p_children[index];
			if (child.has("panel") && child.has("uuid")) {
				String uuid = child.get("uuid", "");
				uuids.append(uuid);
			}
			else if (child.has("children")) {
				Array children = child.get("children", Array());
				_find_layout_uuid(uuids, children);
			}
		}
	}

	// ── 插件面板热替换 ────────────────────────────────────

	void LTEWorkspaceManager::_connect_plugin_signals() {
		LTEPluginManager* plugin_manager = LTEPluginManager::get_singleton();
		if (!plugin_manager) {
			return;
		}
		plugin_manager->connect("plugin_disabled", callable_mp(this, &LTEWorkspaceManager::_on_plugin_disabled));
		plugin_manager->connect("plugin_enabled", callable_mp(this, &LTEWorkspaceManager::_on_plugin_enabled));
		plugin_manager->connect("plugin_uninstalled", callable_mp(this, &LTEWorkspaceManager::_on_plugin_uninstalled));
	}

	void LTEWorkspaceManager::_on_plugin_disabled(const String& plugin_id) {
		print_line(vformat("[WorkspaceManager] Plugin disabled: %s — swapping panels to UnknownPanel.", plugin_id));

		// 主布局
		if (api) {
			Node* workspace_root = api->get_editor_workspace_root();
			if (workspace_root) {
				_swap_panels_to_unknown_in_tree(workspace_root, plugin_id);
			}
		}

		// 浮动工作区窗口
		Array floating_keys = floating_workspace_window_ids.keys();
		for (int64_t i = 0; i < floating_keys.size(); i++) {
			String file_path = floating_keys[i];
			Window* window = _get_floating_workspace_window(file_path);
			if (window && !window->is_queued_for_deletion()) {
				_swap_panels_to_unknown_in_tree(window, plugin_id);
			}
		}

		// 清理该插件的 owner cache 项（面板已取消注册）
		std::vector<String> keys_to_remove;
		for (const auto& kv : _panel_owner_cache) {
			if (kv.value == plugin_id) {
				keys_to_remove.push_back(kv.key);
			}
		}
		for (int64_t i = 0; i < keys_to_remove.size(); i++) {
			_panel_owner_cache.erase(keys_to_remove[i]);
		}
	}

	void LTEWorkspaceManager::_on_plugin_enabled(const String& plugin_id) {
		print_line(vformat("[WorkspaceManager] Plugin enabled: %s — restoring panels from UnknownPanel.", plugin_id));

		// 主布局
		if (api) {
			Node* workspace_root = api->get_editor_workspace_root();
			if (workspace_root) {
				_restore_panels_in_tree(workspace_root, plugin_id);
			}
		}

		// 浮动工作区窗口
		Array floating_keys = floating_workspace_window_ids.keys();
		for (int64_t i = 0; i < floating_keys.size(); i++) {
			String file_path = floating_keys[i];
			Window* window = _get_floating_workspace_window(file_path);
			if (window && !window->is_queued_for_deletion()) {
				_restore_panels_in_tree(window, plugin_id);
			}
		}
	}

	void LTEWorkspaceManager::_on_plugin_uninstalled(const String& plugin_id) {
		print_line(vformat("[WorkspaceManager] Plugin uninstalled: %s — swapping panels to UnknownPanel and clearing tracking.", plugin_id));

		// 卸载比禁用更彻底：同样执行面板替换
		if (api) {
			Node* workspace_root = api->get_editor_workspace_root();
			if (workspace_root) {
				_swap_panels_to_unknown_in_tree(workspace_root, plugin_id);
			}
		}

		Array floating_keys = floating_workspace_window_ids.keys();
		for (int64_t i = 0; i < floating_keys.size(); i++) {
			String file_path = floating_keys[i];
			Window* window = _get_floating_workspace_window(file_path);
			if (window && !window->is_queued_for_deletion()) {
				_swap_panels_to_unknown_in_tree(window, plugin_id);
			}
		}

		// 清理热替换记录：卸载后不再恢复
		std::vector<String> hot_keys_to_remove;
		for (const auto& kv : _hot_replace_map) {
			Dictionary entry = kv.value;
			if (String(entry.get("plugin_id", "")) == plugin_id) {
				hot_keys_to_remove.push_back(kv.key);
			}
		}
		for (int64_t i = 0; i < hot_keys_to_remove.size(); i++) {
			_hot_replace_map.erase(hot_keys_to_remove[i]);
		}

		// 清理 owner cache
		std::vector<String> cache_keys_to_remove;
		for (const auto& kv : _panel_owner_cache) {
			if (kv.value == plugin_id) {
				cache_keys_to_remove.push_back(kv.key);
			}
		}
		for (int64_t i = 0; i < cache_keys_to_remove.size(); i++) {
			_panel_owner_cache.erase(cache_keys_to_remove[i]);
		}

		// 清理已知面板映射
		std::vector<String> known_keys_to_remove;
		for (const auto& kv : _known_panel_owners) {
			if (kv.value == plugin_id) {
				known_keys_to_remove.push_back(kv.key);
			}
		}
		for (int64_t i = 0; i < known_keys_to_remove.size(); i++) {
			_known_panel_owners.erase(known_keys_to_remove[i]);
		}
	}

	void LTEWorkspaceManager::_swap_panels_to_unknown_in_tree(Node* root_node, const String& plugin_id) {
		if (!root_node || !component_registry) {
			return;
		}

		// 深度优先遍历，收集需要替换的节点（避免遍历中修改树结构）
		struct SwapTarget {
			Node* panel_node;
			String runtime_uuid;
			String panel_uuid;
			String panel_name;
		};
		std::vector<SwapTarget> targets;

		std::vector<Node*> stack;
		stack.push_back(root_node);
		while (!stack.empty()) {
			Node* node = stack.back();
			stack.pop_back();
			if (!node) {
				continue;
			}

			// 跳过未知面板自身
			if (node->has_meta("unknown_panel_plugin_id")) {
				continue;
			}

			// 检查该节点是否有 runtime_uuid
			if (node->has_method("get_uuid")) {
				String runtime_uuid = String(node->call("get_uuid"));
				if (!runtime_uuid.is_empty()) {
					const String* owner = _panel_owner_cache.getptr(runtime_uuid);
					if (owner && *owner == plugin_id) {
						SwapTarget target;
						target.panel_node = node;
						target.runtime_uuid = runtime_uuid;
						// 通过 PanelOptionButton 查找 panel_uuid
						String panel_uuid;
						String panel_name;
						Array option_buttons = node->find_children("PanelOptionButton");
						for (int64_t j = 0; j < option_buttons.size(); j++) {
							Node* btn = Object::cast_to<Node>(option_buttons[j]);
							if (btn && btn->has_method("get_current_panel_uuid")) {
								panel_uuid = String(btn->call("get_current_panel_uuid"));
								break;
							}
						}
						// 回退：从 _known_panel_owners 查找（面板可能已取消注册）
						if (panel_uuid.is_empty()) {
							for (const auto& kv : _known_panel_owners) {
								if (kv.value == plugin_id) {
									panel_uuid = kv.key;
									break;
								}
							}
						}
						target.panel_name = panel_name;
						targets.push_back(target);
						continue; // 不递归进入将被替换的节点
					}
				}
			}

			// 递归子节点
			for (int32_t i = 0; i < node->get_child_count(); i++) {
				Node* child = node->get_child(i);
				if (child) {
					stack.push_back(child);
				}
			}
		}

		// 执行替换
		for (int64_t i = 0; i < targets.size(); i++) {
			_swap_single_panel_to_unknown(
				targets[i].panel_node,
				targets[i].runtime_uuid,
				targets[i].panel_uuid,
				plugin_id,
				targets[i].panel_name
			);
		}
	}

	void LTEWorkspaceManager::_swap_single_panel_to_unknown(Node* panel_node, const String& runtime_uuid, const String& panel_uuid, const String& plugin_id, const String& panel_name) {
		if (!panel_node) {
			return;
		}

		Node* parent = panel_node->get_parent();
		if (!parent) {
			return;
		}

		int32_t child_index = panel_node->get_index();

		// 获取插件名称
		String plugin_display_name = plugin_id;
		LTEPluginManager* plugin_manager = LTEPluginManager::get_singleton();
		if (plugin_manager) {
			Dictionary details = plugin_manager->get_plugin_details(plugin_id);
			String name_from_details = String(details.get("name", ""));
			if (!name_from_details.is_empty() && name_from_details != plugin_id) {
				plugin_display_name = name_from_details;
			}
		}

		// 加载 UnknownPanel 场景
		Ref<PackedScene> unknown_scene = ResourceLoader::get_singleton()->load("res://scenes/editor/unknown_panel.tscn");
		if (unknown_scene.is_null()) {
			WARN_PRINT(vformat("[WorkspaceManager] Failed to load UnknownPanel scene for plugin '%s' panel '%s'.", plugin_id, panel_uuid));
			return;
		}

		Node* unknown_panel_node = unknown_scene->instantiate();
		if (!unknown_panel_node) {
			WARN_PRINT(vformat("[WorkspaceManager] Failed to instantiate UnknownPanel for plugin '%s'.", plugin_id));
			return;
		}

		// 获取原面板的尺寸信息
		Control* original_control = Object::cast_to<Control>(panel_node);
		Control* unknown_control = Object::cast_to<Control>(unknown_panel_node);
		if (original_control && unknown_control) {
			unknown_control->set_custom_minimum_size(original_control->get_custom_minimum_size());
		}

		// 替换
		parent->remove_child(panel_node);
		parent->add_child(unknown_panel_node);
		parent->move_child(unknown_panel_node, child_index);

		// 设置 UnknownPanel 元数据
		if (unknown_panel_node->has_method("set_metadata")) {
			Dictionary payload;
			payload["panel"] = panel_uuid;
			payload["uuid"] = runtime_uuid;
			unknown_panel_node->call("set_metadata", runtime_uuid, panel_name, plugin_id, plugin_display_name, payload);
		}

		// 标记为未知面板，便于恢复时识别
		unknown_panel_node->set_meta("unknown_panel_plugin_id", plugin_id);
		unknown_panel_node->set_meta("unknown_panel_runtime_uuid", runtime_uuid);
		unknown_panel_node->set_meta("unknown_panel_panel_uuid", panel_uuid);

		// 入队销毁原面板
		panel_node->queue_free();

		// 记录热替换项
		Dictionary entry;
		entry["plugin_id"] = plugin_id;
		entry["panel_uuid"] = panel_uuid;
		entry["panel_name"] = panel_name;
		entry["runtime_uuid"] = runtime_uuid;
		entry["unknown_node_id"] = static_cast<int64_t>(unknown_panel_node->get_instance_id());
		_hot_replace_map[runtime_uuid] = entry;

		print_line(vformat("[WorkspaceManager] Swapped panel '%s' (uuid=%s) to UnknownPanel for plugin '%s'.", panel_name, runtime_uuid, plugin_id));
	}

	void LTEWorkspaceManager::_restore_panels_in_tree(Node* root_node, const String& plugin_id) {
		if (!root_node || !component_registry) {
			return;
		}

		// 收集需要恢复的 UnknownPanel 节点
		struct RestoreTarget {
			Node* unknown_node;
			String runtime_uuid;
			String panel_uuid;
			String panel_name;
		};
		std::vector<RestoreTarget> targets;

		std::vector<Node*> stack;
		stack.push_back(root_node);
		while (!stack.empty()) {
			Node* node = stack.back();
			stack.pop_back();
			if (!node) {
				continue;
			}

			if (node->has_meta("unknown_panel_plugin_id")) {
				String node_plugin_id = String(node->get_meta("unknown_panel_plugin_id"));
				if (node_plugin_id == plugin_id) {
					RestoreTarget target;
					target.unknown_node = node;
					target.runtime_uuid = String(node->get_meta("unknown_panel_runtime_uuid"));
					target.panel_uuid = String(node->get_meta("unknown_panel_panel_uuid"));

					// 从热替换记录获取面板名称
					const Dictionary* entry = _hot_replace_map.getptr(target.runtime_uuid);
					if (entry) {
						target.panel_name = String(entry->get("panel_name", ""));
					}
					targets.push_back(target);
					continue; // 不递归进入 UnknownPanel 子节点
				}
			}

			for (int32_t i = 0; i < node->get_child_count(); i++) {
				Node* child = node->get_child(i);
				if (child) {
					stack.push_back(child);
				}
			}
		}

		// 执行恢复
		for (int64_t i = 0; i < targets.size(); i++) {
			RestoreTarget& target = targets[i];
			Node* unknown_node = target.unknown_node;
			Node* parent = unknown_node->get_parent();
			if (!parent) {
				continue;
			}

			int32_t child_index = unknown_node->get_index();

			// 尝试从 component_registry 加载原始面板场景
			Ref<PackedScene> packed_scene = component_registry->get_component_panel_scene(target.panel_uuid);
			if (packed_scene.is_null()) {
				// 面板尚未重新注册（可能在插件 _enable 完成前）
				WARN_PRINT(vformat("[WorkspaceManager] Cannot restore panel '%s': scene not yet registered. Retry on next layout open.", target.panel_uuid));
				continue;
			}

			Node* new_panel_node = packed_scene->instantiate();
			if (!new_panel_node) {
				WARN_PRINT(vformat("[WorkspaceManager] Failed to instantiate panel '%s' for restore.", target.panel_uuid));
				continue;
			}

			// 获取原 UnknownPanel 的尺寸
			Control* unknown_control = Object::cast_to<Control>(unknown_node);
			Control* new_control = Object::cast_to<Control>(new_panel_node);
			if (unknown_control && new_control) {
				new_control->set_custom_minimum_size(unknown_control->get_custom_minimum_size());
			}

			// 替换
			parent->remove_child(unknown_node);
			parent->add_child(new_panel_node);
			parent->move_child(new_panel_node, child_index);

			// 恢复 runtime_uuid
			if (new_panel_node->has_method("set_uuid")) {
				new_panel_node->call("set_uuid", target.runtime_uuid);
			}
			Array option_buttons = new_panel_node->find_children("PanelOptionButton");
			for (int64_t j = 0; j < option_buttons.size(); j++) {
				Node* btn = Object::cast_to<Node>(option_buttons[j]);
				if (btn && btn->has_method("set_current_runtime_uuid")) {
					btn->call("set_current_runtime_uuid", target.runtime_uuid);
				}
			}

			// 更新 owner cache
			_panel_owner_cache[target.runtime_uuid] = plugin_id;

			// 清理
			unknown_node->queue_free();
			_hot_replace_map.erase(target.runtime_uuid);

			print_line(vformat("[WorkspaceManager] Restored panel '%s' (uuid=%s) for plugin '%s'.", target.panel_name, target.runtime_uuid, plugin_id));
		}
	}
}
