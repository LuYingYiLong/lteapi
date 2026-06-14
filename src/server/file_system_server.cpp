#include "file_system_server.h"

#include "project_manager.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTEFileSystemServer* LTEFileSystemServer::singleton = nullptr;

	void LTEFileSystemServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_apply_scan_results", "files", "charts", "scenes", "shaders", "skins", "track_info", "generation"), &LTEFileSystemServer::_apply_scan_results);

		ClassDB::bind_method(D_METHOD("scan_directory_completely"), &LTEFileSystemServer::scan_directory_completely);
		ClassDB::bind_method(D_METHOD("get_files_path"), &LTEFileSystemServer::get_files_path);
		ClassDB::bind_method(D_METHOD("select_files", "runtime_uuid", "files"), &LTEFileSystemServer::select_files);
		ClassDB::bind_method(D_METHOD("get_selected_files", "runtime_uuid"), &LTEFileSystemServer::get_selected_files);
		ClassDB::bind_method(D_METHOD("set_collapsed_folders", "runtime_uuid", "folders"), &LTEFileSystemServer::set_collapsed_folders);
		ClassDB::bind_method(D_METHOD("get_collapsed_folders", "runtime_uuid"), &LTEFileSystemServer::get_collapsed_folders);
		ClassDB::bind_method(D_METHOD("set_view_state", "runtime_uuid", "view_state"), &LTEFileSystemServer::set_view_state);
		ClassDB::bind_method(D_METHOD("get_view_state", "runtime_uuid"), &LTEFileSystemServer::get_view_state);
		ClassDB::bind_method(D_METHOD("get_track_info"), &LTEFileSystemServer::get_track_info);
		ClassDB::bind_method(D_METHOD("get_chart_files_path"), &LTEFileSystemServer::get_chart_files_path);
		ClassDB::bind_method(D_METHOD("get_scene_files_path"), &LTEFileSystemServer::get_scene_files_path);
		ClassDB::bind_method(D_METHOD("get_shader_files_path"), &LTEFileSystemServer::get_shader_files_path);
		ClassDB::bind_method(D_METHOD("get_skin_files_path"), &LTEFileSystemServer::get_skin_files_path);
		ClassDB::bind_method(D_METHOD("save_track_info", "new_track_info"), &LTEFileSystemServer::save_track_info);
		ClassDB::bind_method(D_METHOD("save_text_file", "path", "content"), &LTEFileSystemServer::save_text_file);
		ClassDB::bind_method(D_METHOD("restore_text_file", "path", "existed", "content"), &LTEFileSystemServer::restore_text_file);
		ClassDB::bind_method(D_METHOD("move_path", "from_path", "to_path", "move_references", "runtime_uuid"), &LTEFileSystemServer::move_path, DEFVAL(true), DEFVAL(String()));
		ADD_SIGNAL(MethodInfo("scan_completed"));
		ADD_SIGNAL(MethodInfo("path_moved",
			PropertyInfo(Variant::STRING, "from_path"),
			PropertyInfo(Variant::STRING, "to_path")));
		ADD_SIGNAL(MethodInfo("files_selected", PropertyInfo(Variant::STRING, "runtime_uuid"), PropertyInfo(Variant::PACKED_STRING_ARRAY, "files")));
		ADD_SIGNAL(MethodInfo("track_info_changed"));
	}

	LTEFileSystemServer::LTEFileSystemServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
		settings_config = LTESettingsConfig::get_singleton();
		abort_scan_flag.store(false);
	}

	LTEFileSystemServer::~LTEFileSystemServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
		settings_config = nullptr;
		abort_scan_flag.store(true);
		if (worker_thread.joinable()) {
			worker_thread.join();
		}
	}

	LTEFileSystemServer* LTEFileSystemServer::get_singleton() {
		return singleton;
	}

	void LTEFileSystemServer::clear_project_state() {
		scan_generation++;
		if (worker_thread.joinable()) {
			abort_scan_flag.store(true);
			worker_thread.join();
		}
		files_path.clear();
		chart_files_path.clear();
		scene_files_path.clear();
		shader_files_path.clear();
		skin_files_path.clear();
		file_fingerprints.clear();
		pending_known_moves.clear();
		track_info.clear();
		abort_scan_flag.store(false);
	}

	void LTEFileSystemServer::scan_directory_completely() {
		// 如果已有线程在运行，先发出中止信号，并等待它结束
		if (worker_thread.joinable()) {
			abort_scan_flag.store(true); // 通知旧线程停下
			worker_thread.join();        // 等待旧线程真正退出
		}
		// 重置标记
		abort_scan_flag.store(false);
		scan_generation++;
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_path = project_manager ? _normalize_path(project_manager->get_project_path()) : String();
		if (root_path.is_empty() || _is_filesystem_root_path(root_path) || !FileAccess::file_exists(root_path.path_join("info.json"))) return;
		worker_thread = std::thread(&LTEFileSystemServer::_scan_worker_function, this, root_path, scan_generation);
	}

	void LTEFileSystemServer::_scan_worker_function(String root_path, int32_t generation) {
		root_path = _normalize_path(root_path);
		if (root_path.is_empty() || _is_filesystem_root_path(root_path) || !FileAccess::file_exists(root_path.path_join("info.json"))) {
			return;
		}
		// 定义局部变量，绝对不要触碰类的成员变量
		Array local_files;
		Array local_charts;
		Array local_scenes;
		Array local_shaders;
		Array local_skins;
		Dictionary local_track_info;

		// 用于记录待扫描的目录栈
		Array dir_stack;
		dir_stack.push_back(root_path);
		PackedStringArray scanned_files;

		while (!(dir_stack.is_empty())) {
			// 检查点：如果主线程要求中止（比如发起了新扫描），立即退出
			if (abort_scan_flag.load()) return;
			String current_dir = dir_stack.pop_back();
			Ref<DirAccess> dir_access = DirAccess::open(current_dir);
			if (dir_access.is_valid()) {
				dir_access->list_dir_begin();
				String file_name = dir_access->get_next();
				while (file_name != String()) {
					// 检查点：频繁检查，保证响应速度
					if (abort_scan_flag.load()) return;

					if (file_name != ".editor") {
						String full_path = current_dir.path_join(file_name);
						if (dir_access->current_is_dir()) {
							// 如果是目录，加入栈中以便后续扫描
							scanned_files.append(full_path);
							dir_stack.push_back(full_path);
						}
						else {
							// 如果是文件，记录下来
							scanned_files.append(full_path);
							String ext = file_name.get_extension().to_lower();
							if (ext == "chart") local_charts.append(full_path);
							if (ext == "scn" || ext == "seq") local_scenes.append(full_path);
							if (ext == "shader") local_shaders.append(full_path);
							if (ext == "lteskin") local_skins.append(full_path);
						}
					}
					file_name = dir_access->get_next();
				}
				dir_access->list_dir_end();
			}
		}
		local_files = scanned_files;
		local_files.sort();
		local_skins.sort();

		if (FileAccess::file_exists(root_path + "/info.json")) {
			// 最后一次检查
			if (abort_scan_flag.load()) return;

			local_track_info = Utils::load_json_file(root_path + "/info.json");
		}

		// 扫描完成且未被中止：回到主线程提交数据
		// 使用 call_deferred 确保 _apply_scan_results 在主线程运行
		if (!abort_scan_flag.load()) {
			call_deferred("_apply_scan_results", local_files, local_charts, local_scenes, local_shaders, local_skins, local_track_info, generation);
		}
	}

	void LTEFileSystemServer::_apply_scan_results(Array p_files, Array p_charts, Array p_scenes, Array p_shaders, Array p_skins, Dictionary p_track_info, int32_t generation) {
		// 再次检查标记（防止极端情况：线程刚发deferred就被析构了）
		if (abort_scan_flag.load()) return;
		if (generation != scan_generation) return;

		// 只有回到主线程，才允许修改成员变量
		Dictionary new_file_fingerprints = _build_file_fingerprints(p_files);
		_migrate_external_moves(p_files, new_file_fingerprints);
		files_path = p_files;
		chart_files_path = p_charts;
		scene_files_path = p_scenes;
		shader_files_path = p_shaders;
		skin_files_path = p_skins;
		file_fingerprints = new_file_fingerprints;
		track_info = p_track_info;
		pending_known_moves.clear();

		emit_signal("scan_completed");
	}

	PackedStringArray LTEFileSystemServer::get_files_path() const {
		return files_path;
	}

	void LTEFileSystemServer::select_files(const String& runtime_uuid, const PackedStringArray& files) {
		if (!settings_config) return;
		settings_config->file_system_panel_selected_file[runtime_uuid] = files;
		settings_config->save_settings_config(false);
		emit_signal("files_selected", runtime_uuid, files);
	}

	PackedStringArray LTEFileSystemServer::get_selected_files(const String& runtime_uuid) const {
		if (!settings_config) return PackedStringArray();
		PackedStringArray selected_files = settings_config->file_system_panel_selected_file.get(runtime_uuid, PackedStringArray());
		return selected_files;
	}

	void LTEFileSystemServer::set_collapsed_folders(const String& runtime_uuid, const PackedStringArray& folders) {
		if (!settings_config || runtime_uuid.is_empty()) return;
		settings_config->file_system_panel_collapsed_folders[runtime_uuid] = folders;
		settings_config->save_settings_config(false);
	}

	PackedStringArray LTEFileSystemServer::get_collapsed_folders(const String& runtime_uuid) const {
		if (!settings_config || runtime_uuid.is_empty()) return PackedStringArray();
		PackedStringArray collapsed_folders = settings_config->file_system_panel_collapsed_folders.get(runtime_uuid, PackedStringArray());
		return collapsed_folders;
	}

	void LTEFileSystemServer::set_view_state(const String& runtime_uuid, const Dictionary& view_state) {
		if (!settings_config || runtime_uuid.is_empty()) return;
		settings_config->file_system_panel_view_state[runtime_uuid] = view_state;
		settings_config->save_settings_config(false);
	}

	Dictionary LTEFileSystemServer::get_view_state(const String& runtime_uuid) const {
		if (!settings_config || runtime_uuid.is_empty()) return Dictionary();
		Dictionary view_state = settings_config->file_system_panel_view_state.get(runtime_uuid, Dictionary());
		return view_state;
	}

	Dictionary LTEFileSystemServer::get_track_info() const {
		return track_info;
	}

	PackedStringArray LTEFileSystemServer::get_chart_files_path() const {
		return chart_files_path;
	}

	PackedStringArray LTEFileSystemServer::get_scene_files_path() const {
		return scene_files_path;
	}

	PackedStringArray LTEFileSystemServer::get_shader_files_path() const {
		return shader_files_path;
	}

	PackedStringArray LTEFileSystemServer::get_skin_files_path() const {
		return skin_files_path;
	}

	void LTEFileSystemServer::save_track_info(const Dictionary& new_track_info) {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		if (!project_manager || project_manager->get_project_path().is_empty()) return;
		String save_path = project_manager->get_project_path() + "/info.json";
		track_info = new_track_info;
		Utils::save_json_file(save_path, track_info);
		emit_signal("track_info_changed");
	}

	Error LTEFileSystemServer::save_text_file(const String& path, const String& content) {
		if (path.is_empty()) {
			return ERR_FILE_BAD_PATH;
		}

		String dir_path = path.get_base_dir();
		Ref<DirAccess> dir_access = DirAccess::open(dir_path);
		if (dir_access.is_null()) {
			dir_access = DirAccess::open("user://");
		}
		if (dir_access.is_null()) {
			return ERR_CANT_OPEN;
		}

		Error error = dir_access->make_dir_recursive(dir_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to create directory: %s", dir_path));
			return error;
		}

		Ref<FileAccess> file_access = FileAccess::open(path, FileAccess::WRITE);
		if (file_access.is_null()) {
			error = FileAccess::get_open_error();
			ERR_PRINT(vformat("Failed to open file: %s. Error Code: %d", path, error));
			return error;
		}

		if (!file_access->store_string(content)) {
			ERR_PRINT(vformat("Failed to write file: %s", path));
			return ERR_FILE_CANT_WRITE;
		}
		file_access->close();
		scan_directory_completely();
		return OK;
	}

	Error LTEFileSystemServer::restore_text_file(const String& path, const bool existed, const String& content) {
		if (existed) {
			return save_text_file(path, content);
		}

		if (FileAccess::file_exists(path)) {
			Error error = DirAccess::remove_absolute(path);
			if (error != OK) {
				ERR_PRINT(vformat("Failed to remove file: %s. Error Code: %d", path, error));
				return error;
			}
		}
		scan_directory_completely();
		return OK;
	}

	Error LTEFileSystemServer::move_path(const String& from_path, const String& to_path, const bool move_references, const String& runtime_uuid) {
		String normalized_from_path = _normalize_path(from_path);
		String normalized_to_path = _normalize_path(to_path);
		if (normalized_from_path.is_empty() || normalized_to_path.is_empty()) {
			return ERR_FILE_BAD_PATH;
		}
		if (normalized_from_path == normalized_to_path) {
			return OK;
		}
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String project_info_path = project_manager ? _normalize_path(project_manager->get_project_path().path_join("info.json")) : String();
		if (!project_info_path.is_empty() && normalized_from_path == project_info_path) {
			ERR_PRINT(vformat("move_path: Project info file can not be moved: %s", normalized_from_path));
			return ERR_UNAUTHORIZED;
		}
		if (!_path_exists(normalized_from_path)) {
			ERR_PRINT(vformat("move_path: Source does not exist: %s", normalized_from_path));
			return ERR_DOES_NOT_EXIST;
		}
		if (_path_exists(normalized_to_path)) {
			ERR_PRINT(vformat("move_path: Destination already exists: %s", normalized_to_path));
			return ERR_ALREADY_EXISTS;
		}
		if (DirAccess::dir_exists_absolute(normalized_from_path) && _is_path_inside(normalized_to_path, normalized_from_path)) {
			ERR_PRINT(vformat("move_path: Cannot move a directory into itself: %s", normalized_from_path + " -> " + normalized_to_path));
			return ERR_INVALID_PARAMETER;
		}
		Error error = DirAccess::make_dir_recursive_absolute(normalized_to_path.get_base_dir());
		if (error != OK) {
			ERR_PRINT(vformat("move_path: Failed to create destination directory: %s", normalized_to_path.get_base_dir()));
			return error;
		}
		error = DirAccess::rename_absolute(normalized_from_path, normalized_to_path);
		if (error != OK) {
			ERR_PRINT(vformat("move_path: Failed to move path: %s -> %s. Error: %d", normalized_from_path, normalized_to_path, error));
			return error;
		}
		if (move_references) {
			_migrate_references(normalized_from_path, normalized_to_path);
		}
		pending_known_moves[normalized_from_path] = normalized_to_path;
		if (!runtime_uuid.is_empty()) {
			PackedStringArray selected_files;
			selected_files.append(normalized_to_path);
			select_files(runtime_uuid, selected_files);
		}
		emit_signal("path_moved", normalized_from_path, normalized_to_path);
		scan_directory_completely();
		return OK;
	}

	void LTEFileSystemServer::_migrate_external_moves(const Array& scanned_paths, const Dictionary& new_file_fingerprints) {
		if (files_path.is_empty() || file_fingerprints.is_empty() || new_file_fingerprints.is_empty()) {
			return;
		}

		Dictionary old_path_lookup;
		for (int64_t index = 0; index < files_path.size(); index++) {
			old_path_lookup[_normalize_path(files_path[index])] = true;
		}

		Dictionary new_path_lookup;
		for (int64_t index = 0; index < scanned_paths.size(); index++) {
			new_path_lookup[_normalize_path(scanned_paths[index])] = true;
		}

		Dictionary removed_by_fingerprint;
		Array old_paths = file_fingerprints.keys();
		for (int64_t index = 0; index < old_paths.size(); index++) {
			String old_path = _normalize_path(old_paths[index]);
			if (old_path.is_empty() || new_path_lookup.has(old_path) || _is_project_info_path(old_path)) {
				continue;
			}
			String fingerprint = String(file_fingerprints[old_paths[index]]);
			_append_path_by_fingerprint(removed_by_fingerprint, fingerprint, old_path);
		}

		Dictionary added_by_fingerprint;
		Array new_paths = new_file_fingerprints.keys();
		for (int64_t index = 0; index < new_paths.size(); index++) {
			String new_path = _normalize_path(new_paths[index]);
			if (new_path.is_empty() || old_path_lookup.has(new_path) || _is_project_info_path(new_path)) {
				continue;
			}
			String fingerprint = String(new_file_fingerprints[new_paths[index]]);
			_append_path_by_fingerprint(added_by_fingerprint, fingerprint, new_path);
		}

		Array fingerprints = removed_by_fingerprint.keys();
		for (int64_t index = 0; index < fingerprints.size(); index++) {
			String fingerprint = String(fingerprints[index]);
			if (!added_by_fingerprint.has(fingerprint)) {
				continue;
			}
			Array removed_paths = removed_by_fingerprint[fingerprint];
			Array added_paths = added_by_fingerprint[fingerprint];
			if (removed_paths.size() != 1 || added_paths.size() != 1) {
				continue;
			}
			String from_path = _normalize_path(removed_paths[0]);
			String to_path = _normalize_path(added_paths[0]);
			if (from_path.is_empty() || to_path.is_empty() || from_path == to_path) {
				continue;
			}
			if (_is_known_move_pair(from_path, to_path)) {
				continue;
			}
			_migrate_references(from_path, to_path);
			emit_signal("path_moved", from_path, to_path);
		}
	}

	void LTEFileSystemServer::_migrate_references(const String& from_path, const String& to_path) {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_path = project_manager ? _normalize_path(project_manager->get_project_path()) : String();
		if (root_path.is_empty()) {
			return;
		}
		String from_relative_path = _get_project_reference_path(from_path);
		String to_relative_path = _get_project_reference_path(to_path);
		if (from_relative_path.is_empty() || to_relative_path.is_empty()) {
			return;
		}
		_migrate_settings_references(from_path, to_path, from_relative_path, to_relative_path);
		PackedStringArray reference_file_paths;
		String info_path = root_path.path_join("info.json");
		if (FileAccess::file_exists(info_path)) {
			reference_file_paths.append(info_path);
		}
		_collect_reference_file_paths(root_path, reference_file_paths);
		reference_file_paths.sort();
		for (int64_t index = 0; index < reference_file_paths.size(); index++) {
			String reference_file_path = _normalize_path(reference_file_paths[index]);
			Ref<FileAccess> file = FileAccess::open(reference_file_path, FileAccess::READ);
			if (file.is_null()) {
				continue;
			}
			String content = file->get_as_text();
			file->close();
			Variant value = JSON::parse_string(content);
			Variant::Type type = value.get_type();
			if (type != Variant::DICTIONARY && type != Variant::ARRAY) {
				continue;
			}
			bool changed = _migrate_variant(value, from_relative_path, to_relative_path, from_path, to_path);
			if (!changed) {
				continue;
			}
			_save_json_variant(reference_file_path, value);
		}
	}

	void LTEFileSystemServer::_collect_reference_file_paths(const String& directory_path, PackedStringArray& reference_file_paths) const {
		Ref<DirAccess> dir_access = DirAccess::open(directory_path);
		if (dir_access.is_null()) {
			return;
		}
		dir_access->list_dir_begin();
		String entry_name = dir_access->get_next();
		while (!entry_name.is_empty()) {
			if (entry_name == "." || entry_name == ".." || entry_name == ".editor") {
				entry_name = dir_access->get_next();
				continue;
			}
			String child_path = _normalize_path(directory_path.path_join(entry_name));
			if (dir_access->current_is_dir()) {
				_collect_reference_file_paths(child_path, reference_file_paths);
			}
			else {
				String extension = entry_name.get_extension().to_lower();
				if (_is_reference_file_extension(extension)) {
					reference_file_paths.append(child_path);
				}
			}
			entry_name = dir_access->get_next();
		}
		dir_access->list_dir_end();
	}

	void LTEFileSystemServer::_migrate_settings_references(const String& from_path, const String& to_path, const String& from_relative_path, const String& to_relative_path) {
		if (!settings_config) {
			return;
		}
		bool changed = false;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_playhead_auto_scroll, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_show_audio_waveform, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_smooth_audio_waveform, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_spacing, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_last_opened, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_snap_mode, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_coverage_mode, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_chart_column_configs, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_chart_markers, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_chart_in_out_points, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_chart_per_bar, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_chart_playhead, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->timeline_panel_chart_scroll_position, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->composition_timeline_scene_settings, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->graph_editor_playhead_auto_scroll, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->graph_editor_view_settings, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->audio_visualizer_configs, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->audio_preview_chart_settings, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->note_skin_designer_configs, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_last_opened, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_chart_time, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_view_judgement_line, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_ratio, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_show_clear_color, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_auto_play, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_show_debug_info, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_screen, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->source_monitor_window_mode, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->file_system_panel_selected_file, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->file_system_panel_collapsed_folders, from_relative_path, to_relative_path, from_path, to_path) || changed;
		changed = _migrate_settings_dictionary(settings_config->file_system_panel_view_state, from_relative_path, to_relative_path, from_path, to_path) || changed;
		if (changed) {
			settings_config->save_settings_config(false);
		}
	}

	bool LTEFileSystemServer::_migrate_settings_dictionary(Dictionary& dictionary, const String& from_relative_path, const String& to_relative_path, const String& from_absolute_path, const String& to_absolute_path) const {
		Variant value = dictionary;
		if (!_migrate_variant(value, from_relative_path, to_relative_path, from_absolute_path, to_absolute_path)) {
			return false;
		}
		if (value.get_type() != Variant::DICTIONARY) {
			return false;
		}
		dictionary = value;
		return true;
	}

	bool LTEFileSystemServer::_migrate_variant(Variant& value, const String& from_relative_path, const String& to_relative_path, const String& from_absolute_path, const String& to_absolute_path) const {
		Variant::Type type = value.get_type();
		if (type == Variant::DICTIONARY) {
			Dictionary dictionary = value;
			Array keys = dictionary.keys();
			Dictionary migrated_dictionary;
			bool changed = false;
			for (int64_t index = 0; index < keys.size(); index++) {
				Variant key = keys[index];
				Variant child = dictionary[key];
				Variant migrated_key = key;
				if (key.get_type() == Variant::STRING) {
					String key_text = key;
					String migrated_key_text = _migrate_reference_text(key_text, from_relative_path, to_relative_path, from_absolute_path, to_absolute_path);
					if (migrated_key_text != key_text) {
						migrated_key = migrated_key_text;
						changed = true;
					}
				}
				if (_migrate_variant(child, from_relative_path, to_relative_path, from_absolute_path, to_absolute_path)) {
					changed = true;
				}
				migrated_dictionary[migrated_key] = child;
			}
			value = migrated_dictionary;
			return changed;
		}
		if (type == Variant::ARRAY) {
			Array array = value;
			bool changed = false;
			for (int64_t index = 0; index < array.size(); index++) {
				Variant child = array[index];
				if (_migrate_variant(child, from_relative_path, to_relative_path, from_absolute_path, to_absolute_path)) {
					array[index] = child;
					changed = true;
				}
			}
			value = array;
			return changed;
		}
		if (type == Variant::STRING) {
			String text = value;
			String migrated_text = _migrate_reference_text(text, from_relative_path, to_relative_path, from_absolute_path, to_absolute_path);
			if (migrated_text != text) {
				value = migrated_text;
				return true;
			}
		}
		return false;
	}

	String LTEFileSystemServer::_migrate_reference_text(const String& text, const String& from_relative_path, const String& to_relative_path, const String& from_absolute_path, const String& to_absolute_path) const {
		String normalized_text = _normalize_path(text);
		String migrated_text = _replace_path_prefix(normalized_text, from_relative_path, to_relative_path);
		if (migrated_text != normalized_text) {
			return migrated_text;
		}
		return _replace_path_prefix(normalized_text, from_absolute_path, to_absolute_path);
	}

	String LTEFileSystemServer::_replace_path_prefix(const String& text, const String& from_prefix, const String& to_prefix) const {
		if (from_prefix.is_empty()) {
			return text;
		}
		if (text == from_prefix) {
			return to_prefix;
		}
		if (text.begins_with(from_prefix + String("/"))) {
			return to_prefix + text.substr(from_prefix.length());
		}
		return text;
	}

	String LTEFileSystemServer::_get_project_reference_path(const String& path) const {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_path = project_manager ? _normalize_path(project_manager->get_project_path()) : String();
		String normalized_path = _normalize_path(path);
		if (root_path.is_empty() || normalized_path.is_empty()) {
			return String();
		}
		if (normalized_path == root_path) {
			return "/";
		}
		if (!normalized_path.begins_with(root_path + "/")) {
			return String();
		}
		return "/" + normalized_path.trim_prefix(root_path + "/");
	}

	Dictionary LTEFileSystemServer::_build_file_fingerprints(const Array& paths) const {
		Dictionary fingerprints;
		for (int64_t index = 0; index < paths.size(); index++) {
			String path = _normalize_path(paths[index]);
			if (path.is_empty() || !FileAccess::file_exists(path) || _is_project_info_path(path)) {
				continue;
			}
			String fingerprint = _make_file_fingerprint(path);
			if (fingerprint.is_empty()) {
				continue;
			}
			fingerprints[path] = fingerprint;
		}
		return fingerprints;
	}

	String LTEFileSystemServer::_make_file_fingerprint(const String& path) const {
		String normalized_path = _normalize_path(path);
		if (normalized_path.is_empty() || !FileAccess::file_exists(normalized_path)) {
			return String();
		}
		Ref<FileAccess> file = FileAccess::open(normalized_path, FileAccess::READ);
		if (file.is_null()) {
			return String();
		}
		uint64_t length = file->get_length();
		file->close();
		uint64_t modified_time = FileAccess::get_modified_time(normalized_path);
		return normalized_path.get_file().to_lower() + "|" + String::num_uint64(length) + "|" + String::num_uint64(modified_time);
	}

	void LTEFileSystemServer::_append_path_by_fingerprint(Dictionary& paths_by_fingerprint, const String& fingerprint, const String& path) const {
		if (fingerprint.is_empty() || path.is_empty()) {
			return;
		}
		Array paths = paths_by_fingerprint.get(fingerprint, Array());
		paths.append(path);
		paths_by_fingerprint[fingerprint] = paths;
	}

	bool LTEFileSystemServer::_is_known_move_pair(const String& from_path, const String& to_path) const {
		Array known_from_paths = pending_known_moves.keys();
		for (int64_t index = 0; index < known_from_paths.size(); index++) {
			String known_from_path = _normalize_path(known_from_paths[index]);
			String known_to_path = _normalize_path(pending_known_moves[known_from_paths[index]]);
			if (known_from_path.is_empty() || known_to_path.is_empty()) {
				continue;
			}
			if (from_path == known_from_path && to_path == known_to_path) {
				return true;
			}
			if (from_path.begins_with(known_from_path + "/") && to_path == known_to_path + from_path.substr(known_from_path.length())) {
				return true;
			}
		}
		return false;
	}

	bool LTEFileSystemServer::_is_project_info_path(const String& path) const {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String project_info_path = project_manager ? _normalize_path(project_manager->get_project_path().path_join("info.json")) : String();
		return !project_info_path.is_empty() && _normalize_path(path) == project_info_path;
	}

	bool LTEFileSystemServer::_is_reference_file_extension(const String& extension) const {
		String normalized_extension = extension.to_lower();
		return normalized_extension == "chart" || normalized_extension == "scn" || normalized_extension == "seq";
	}

	String LTEFileSystemServer::_normalize_path(const String& path) const {
		return path.replace("\\", "/").strip_edges().simplify_path();
	}

	bool LTEFileSystemServer::_is_filesystem_root_path(const String& path) const {
		String normalized_path = _normalize_path(path);
		while (normalized_path.length() > 1 && normalized_path.ends_with("/")) {
			normalized_path = normalized_path.substr(0, normalized_path.length() - 1);
		}
		return normalized_path.is_empty() ||
				normalized_path == "." ||
				normalized_path == "/" ||
				(normalized_path.length() == 2 && normalized_path.ends_with(":"));
	}

	bool LTEFileSystemServer::_path_exists(const String& path) const {
		return FileAccess::file_exists(path) || DirAccess::dir_exists_absolute(path);
	}

	bool LTEFileSystemServer::_is_path_inside(const String& path, const String& directory_path) const {
		String normalized_path = _normalize_path(path);
		String normalized_directory_path = _normalize_path(directory_path);
		return normalized_path == normalized_directory_path || normalized_path.begins_with(normalized_directory_path + "/");
	}

	Error LTEFileSystemServer::_save_json_variant(const String& path, const Variant& value) const {
		Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
		if (file.is_null()) {
			Error error = FileAccess::get_open_error();
			ERR_PRINT(vformat("Failed to open file: %s. Error Code: %d", path, error));
			return error;
		}
		if (!file->store_string(JSON::stringify(value, "\t"))) {
			file->close();
			return ERR_FILE_CANT_WRITE;
		}
		file->close();
		return OK;
	}
}
