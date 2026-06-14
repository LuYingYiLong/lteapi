#include "backup_server.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTEBackupServer* LTEBackupServer::singleton = nullptr;

	void LTEBackupServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("backup_project", "project_path", "backup_name"), &LTEBackupServer::backup_project);
		ClassDB::bind_method(D_METHOD("restore_project", "project_path", "backup_name"), &LTEBackupServer::restore_project);
		ClassDB::bind_method(D_METHOD("get_backups", "project_path"), &LTEBackupServer::get_backups);
		ClassDB::bind_method(D_METHOD("delete_backup", "project_path", "backup_name"), &LTEBackupServer::delete_backup);
	}

	LTEBackupServer::LTEBackupServer() {
		singleton = this;
	}

	LTEBackupServer::~LTEBackupServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEBackupServer* LTEBackupServer::get_singleton() {
		return singleton;
	}

	bool LTEBackupServer::backup_project(const String& p_project_path, const String& p_backup_name) {
		String project_path = _normalize_path(p_project_path);
		String backup_name = _sanitize_backup_name(p_backup_name);
		if (project_path.is_empty() || backup_name.is_empty()) {
			return false;
		}
		if (!FileAccess::file_exists(project_path.path_join("info.json"))) {
			return false;
		}

		String backup_path = _get_backup_path(project_path, backup_name);
		if (backup_path.is_empty() || _path_exists(backup_path)) {
			return false;
		}

		String files_path = backup_path.path_join("files");
		Error err = DirAccess::make_dir_recursive_absolute(files_path);
		if (err != OK) {
			return false;
		}

		Dictionary result;
		result["file_count"] = 0;
		result["total_size"] = 0;
		if (!_copy_directory(project_path, files_path, project_path, _get_backup_root(project_path), result)) {
			_remove_directory_recursive(backup_path);
			return false;
		}

		Dictionary metadata;
		metadata["name"] = backup_name;
		metadata["project_name"] = project_path.get_file();
		metadata["project_path"] = project_path;
		metadata["created_time"] = Time::get_singleton()->get_unix_time_from_system();
		metadata["file_count"] = result.get("file_count", 0);
		metadata["total_size"] = result.get("total_size", 0);
		if (!_save_json(backup_path.path_join("metadata.json"), metadata)) {
			_remove_directory_recursive(backup_path);
			return false;
		}
		return true;
	}

	Dictionary LTEBackupServer::restore_project(const String& p_project_path, const String& p_backup_name) const {
		Dictionary result;
		result["ok"] = false;
		result["error"] = OK;

		String project_path = _normalize_path(p_project_path);
		String backup_name = _sanitize_backup_name(p_backup_name);
		if (project_path.is_empty() || backup_name.is_empty()) {
			result["error"] = ERR_INVALID_PARAMETER;
			return result;
		}

		String backup_path = _get_backup_path(project_path, backup_name);
		String files_path = backup_path.path_join("files");
		if (!DirAccess::dir_exists_absolute(files_path)) {
			result["error"] = ERR_FILE_NOT_FOUND;
			return result;
		}

		String safety_backup_name = _make_safety_backup_name();
		if (!const_cast<LTEBackupServer*>(this)->backup_project(project_path, safety_backup_name)) {
			result["error"] = ERR_CANT_CREATE;
			return result;
		}
		if (!_clear_project_for_restore(project_path, _get_backup_root(project_path))) {
			result["error"] = ERR_CANT_ACQUIRE_RESOURCE;
			return result;
		}

		Dictionary copy_result;
		copy_result["file_count"] = 0;
		copy_result["total_size"] = 0;
		if (!_copy_directory(files_path, project_path, files_path, String(), copy_result)) {
			result["error"] = ERR_CANT_CREATE;
			return result;
		}

		result["ok"] = true;
		result["safety_backup_name"] = safety_backup_name;
		return result;
	}

	Array LTEBackupServer::get_backups(const String& p_project_path) const {
		Array backups;
		String project_path = _normalize_path(p_project_path);
		if (project_path.is_empty()) {
			return backups;
		}

		String backup_root = _get_backup_root(project_path);
		Ref<DirAccess> dir = DirAccess::open(backup_root);
		if (dir.is_null()) {
			return backups;
		}

		dir->list_dir_begin();
		while (true) {
			String file_name = dir->get_next();
			if (file_name.is_empty()) {
				break;
			}
			if (file_name == "." || file_name == ".." || !dir->current_is_dir()) {
				continue;
			}
			String backup_path = backup_root.path_join(file_name);
			Dictionary metadata = _load_json(backup_path.path_join("metadata.json"));
			if (metadata.is_empty()) {
				metadata["name"] = file_name;
				metadata["created_time"] = 0;
			}
			metadata["name"] = String(metadata.get("name", file_name));
			metadata["path"] = backup_path;
			backups.append(metadata);
		}
		return backups;
	}

	bool LTEBackupServer::delete_backup(const String& p_project_path, const String& p_backup_name) const {
		String project_path = _normalize_path(p_project_path);
		String backup_name = _sanitize_backup_name(p_backup_name);
		if (project_path.is_empty() || backup_name.is_empty()) {
			return false;
		}
		String backup_root = _get_backup_root(project_path);
		String backup_path = _get_backup_path(project_path, backup_name);
		if (backup_path.is_empty() || backup_path == backup_root || !_is_path_inside(backup_root, backup_path)) {
			return false;
		}
		return _remove_directory_recursive(backup_path);
	}

	String LTEBackupServer::_normalize_path(const String& p_path) const {
		return String(p_path).replace("\\", "/").strip_edges().simplify_path();
	}

	String LTEBackupServer::_sanitize_backup_name(const String& p_backup_name) const {
		String clean_name = p_backup_name.strip_edges();
		static const char* invalid_characters[] = { "/", "\\", ":", "*", "?", "\"", "<", ">", "|" };
		for (const char* invalid_character : invalid_characters) {
			clean_name = clean_name.replace(invalid_character, "_");
		}
		while (clean_name.contains("..")) {
			clean_name = clean_name.replace("..", ".");
		}
		return clean_name;
	}

	String LTEBackupServer::_get_backup_storage_root() const {
		ProjectSettings* project_settings = ProjectSettings::get_singleton();
		if (!project_settings) {
			return String();
		}
		return _normalize_path(project_settings->globalize_path("user://project_backups"));
	}

	String LTEBackupServer::_get_backup_root(const String& p_project_path) const {
		String storage_root = _get_backup_storage_root();
		if (storage_root.is_empty()) {
			return String();
		}
		String project_key = _normalize_path(p_project_path).sha256_text();
		if (project_key.is_empty()) {
			return String();
		}
		return storage_root.path_join(project_key);
	}

	String LTEBackupServer::_get_backup_path(const String& p_project_path, const String& p_backup_name) const {
		String backup_name = _sanitize_backup_name(p_backup_name);
		if (p_project_path.is_empty() || backup_name.is_empty()) {
			return String();
		}
		return _get_backup_root(p_project_path).path_join(backup_name);
	}

	bool LTEBackupServer::_path_exists(const String& p_path) const {
		return FileAccess::file_exists(p_path) || DirAccess::dir_exists_absolute(p_path);
	}

	bool LTEBackupServer::_is_path_inside(const String& p_parent_path, const String& p_child_path) const {
		String parent_path = _normalize_path(p_parent_path);
		String child_path = _normalize_path(p_child_path);
		return child_path == parent_path || child_path.begins_with(parent_path + "/");
	}

	bool LTEBackupServer::_should_skip_backup_relative_path(const String& p_relative_path) const {
		String relative_path = _normalize_path(p_relative_path);
		if (relative_path.is_empty()) {
			return false;
		}
		return relative_path == ".editor/backup" || relative_path.begins_with(".editor/backup/");
	}

	bool LTEBackupServer::_copy_directory(const String& p_source_path, const String& p_target_path, const String& p_source_root, const String& p_backup_root, Dictionary& r_result) const {
		Ref<DirAccess> dir = DirAccess::open(p_source_path);
		if (dir.is_null()) {
			return false;
		}
		Error dir_err = DirAccess::make_dir_recursive_absolute(p_target_path);
		if (dir_err != OK) {
			return false;
		}

		dir->list_dir_begin();
		while (true) {
			String file_name = dir->get_next();
			if (file_name.is_empty()) {
				break;
			}
			if (file_name == "." || file_name == "..") {
				continue;
			}
			String source_child_path = _normalize_path(p_source_path.path_join(file_name));
			if (!p_backup_root.is_empty() && _is_path_inside(p_backup_root, source_child_path)) {
				continue;
			}
			String relative_path = source_child_path.trim_prefix(String(p_source_root) + "/");
			if (_should_skip_backup_relative_path(relative_path)) {
				continue;
			}
			String target_child_path = _normalize_path(p_target_path.path_join(file_name));
			if (dir->current_is_dir()) {
				if (!_copy_directory(source_child_path, target_child_path, p_source_root, p_backup_root, r_result)) {
					return false;
				}
			} else {
				Error parent_err = DirAccess::make_dir_recursive_absolute(target_child_path.get_base_dir());
				if (parent_err != OK) {
					return false;
				}
				Error copy_err = DirAccess::copy_absolute(source_child_path, target_child_path);
				if (copy_err != OK) {
					return false;
				}
				r_result["file_count"] = int64_t(r_result.get("file_count", 0)) + 1;
				r_result["total_size"] = int64_t(r_result.get("total_size", 0)) + _get_file_size(target_child_path);
			}
		}
		return true;
	}

	bool LTEBackupServer::_clear_project_for_restore(const String& p_project_path, const String& p_backup_root) const {
		Ref<DirAccess> dir = DirAccess::open(p_project_path);
		if (dir.is_null()) {
			return false;
		}
		dir->list_dir_begin();
		while (true) {
			String file_name = dir->get_next();
			if (file_name.is_empty()) {
				break;
			}
			if (file_name == "." || file_name == "..") {
				continue;
			}
			String child_path = _normalize_path(p_project_path.path_join(file_name));
			if (_is_path_inside(p_backup_root, child_path)) {
				continue;
			}
			if (dir->current_is_dir()) {
				if (file_name == ".editor") {
					if (!_clear_project_for_restore(child_path, p_backup_root)) {
						return false;
					}
					continue;
				}
				if (!_remove_directory_recursive(child_path)) {
					return false;
				}
			} else {
				Error remove_err = DirAccess::remove_absolute(child_path);
				if (remove_err != OK) {
					return false;
				}
			}
		}
		return true;
	}

	bool LTEBackupServer::_remove_directory_recursive(const String& p_directory_path) const {
		String directory_path = _normalize_path(p_directory_path);
		if (directory_path.is_empty() || !DirAccess::dir_exists_absolute(directory_path)) {
			return true;
		}
		Ref<DirAccess> dir = DirAccess::open(directory_path);
		if (dir.is_null()) {
			return false;
		}
		dir->list_dir_begin();
		while (true) {
			String file_name = dir->get_next();
			if (file_name.is_empty()) {
				break;
			}
			if (file_name == "." || file_name == "..") {
				continue;
			}
			String child_path = directory_path.path_join(file_name);
			if (dir->current_is_dir()) {
				if (!_remove_directory_recursive(child_path)) {
					return false;
				}
			} else {
				Error remove_file_err = DirAccess::remove_absolute(child_path);
				if (remove_file_err != OK) {
					return false;
				}
			}
		}
		return DirAccess::remove_absolute(directory_path) == OK;
	}

	bool LTEBackupServer::_save_json(const String& p_path, const Dictionary& p_data) const {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
		if (file.is_null()) {
			return false;
		}
		file->store_string(JSON::stringify(p_data, "\t"));
		return true;
	}

	Dictionary LTEBackupServer::_load_json(const String& p_path) const {
		if (!FileAccess::file_exists(p_path)) {
			return Dictionary();
		}
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		if (file.is_null()) {
			return Dictionary();
		}
		Variant value = JSON::parse_string(file->get_as_text());
		if (value.get_type() != Variant::DICTIONARY) {
			return Dictionary();
		}
		Dictionary metadata = value;
		return metadata;
	}

	int64_t LTEBackupServer::_get_file_size(const String& p_path) const {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		if (file.is_null()) {
			return 0;
		}
		return file->get_length();
	}

	String LTEBackupServer::_make_safety_backup_name() const {
		return "before_restore_" + String::num_int64(int64_t(Time::get_singleton()->get_unix_time_from_system()));
	}
}
