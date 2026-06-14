#ifndef BACKUP_SERVER_H
#define BACKUP_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
	class LTEBackupServer : public Object {
		GDCLASS(LTEBackupServer, Object)

	private:
		static LTEBackupServer* singleton;

		String _normalize_path(const String& p_path) const;
		String _sanitize_backup_name(const String& p_backup_name) const;
		String _get_backup_storage_root() const;
		String _get_backup_root(const String& p_project_path) const;
		String _get_backup_path(const String& p_project_path, const String& p_backup_name) const;
		bool _path_exists(const String& p_path) const;
		bool _is_path_inside(const String& p_parent_path, const String& p_child_path) const;
		bool _should_skip_backup_relative_path(const String& p_relative_path) const;
		bool _copy_directory(const String& p_source_path, const String& p_target_path, const String& p_source_root, const String& p_backup_root, Dictionary& r_result) const;
		bool _clear_project_for_restore(const String& p_project_path, const String& p_backup_root) const;
		bool _remove_directory_recursive(const String& p_directory_path) const;
		bool _save_json(const String& p_path, const Dictionary& p_data) const;
		Dictionary _load_json(const String& p_path) const;
		int64_t _get_file_size(const String& p_path) const;
		String _make_safety_backup_name() const;

	protected:
		static void _bind_methods();

	public:
		LTEBackupServer();
		~LTEBackupServer();

		static LTEBackupServer* get_singleton();

		bool backup_project(const String& p_project_path, const String& p_backup_name);
		Dictionary restore_project(const String& p_project_path, const String& p_backup_name) const;
		Array get_backups(const String& p_project_path) const;
		bool delete_backup(const String& p_project_path, const String& p_backup_name) const;
	};
}

#endif // !BACKUP_SERVER_H
