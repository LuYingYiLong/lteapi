#ifndef FILE_SYSTEM_SERVER_H
#define FILE_SYSTEM_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <godot_cpp/variant/variant.hpp>

#include "settings_config.h"
#include "utils.h"

namespace godot
{
	class LTEFileSystemServer : public Object {
		GDCLASS(LTEFileSystemServer, Object)

	private:
		static LTEFileSystemServer* singleton;

		LTESettingsConfig* settings_config = nullptr;
		std::thread worker_thread;
		std::atomic<bool> abort_scan_flag;	// 原子布尔值，用于线程安全地通知中止
		int32_t scan_generation = 0;
		Dictionary file_fingerprints;
		Dictionary pending_known_moves;

		void _scan_worker_function(String root_path, int32_t generation);
		void _apply_scan_results(Array p_files, Array p_charts, Array p_scenes, Array p_shaders, Array p_skins, Dictionary p_track_info, int32_t generation);
		void _migrate_external_moves(const Array& scanned_paths, const Dictionary& new_file_fingerprints);
		void _migrate_references(const String& from_path, const String& to_path);
		void _migrate_settings_references(const String& from_path, const String& to_path, const String& from_relative_path, const String& to_relative_path);
		bool _migrate_settings_dictionary(Dictionary& dictionary, const String& from_relative_path, const String& to_relative_path, const String& from_absolute_path, const String& to_absolute_path) const;
		void _collect_reference_file_paths(const String& directory_path, PackedStringArray& reference_file_paths) const;
		bool _migrate_variant(Variant& value, const String& from_relative_path, const String& to_relative_path, const String& from_absolute_path, const String& to_absolute_path) const;
		String _migrate_reference_text(const String& text, const String& from_relative_path, const String& to_relative_path, const String& from_absolute_path, const String& to_absolute_path) const;
		String _replace_path_prefix(const String& text, const String& from_prefix, const String& to_prefix) const;
		String _get_project_reference_path(const String& path) const;
		Dictionary _build_file_fingerprints(const Array& paths) const;
		String _make_file_fingerprint(const String& path) const;
		void _append_path_by_fingerprint(Dictionary& paths_by_fingerprint, const String& fingerprint, const String& path) const;
		bool _is_known_move_pair(const String& from_path, const String& to_path) const;
		bool _is_project_info_path(const String& path) const;
		bool _is_reference_file_extension(const String& extension) const;
		String _normalize_path(const String& path) const;
		bool _is_filesystem_root_path(const String& path) const;
		bool _path_exists(const String& path) const;
		bool _is_path_inside(const String& path, const String& directory_path) const;
		Error _save_json_variant(const String& path, const Variant& value) const;

	protected:
		static void _bind_methods();

	public:
		LTEFileSystemServer();
		~LTEFileSystemServer();

		static LTEFileSystemServer* get_singleton();

		PackedStringArray files_path;
		Dictionary track_info;
		PackedStringArray chart_files_path;
		PackedStringArray scene_files_path;
		PackedStringArray shader_files_path;
		PackedStringArray skin_files_path;

		void clear_project_state();
		void scan_directory_completely();
		PackedStringArray get_files_path() const;
		void select_files(const String& runtime_uuid, const PackedStringArray& files);
		PackedStringArray get_selected_files(const String& runtime_uuid) const;
		void set_collapsed_folders(const String& runtime_uuid, const PackedStringArray& folders);
		PackedStringArray get_collapsed_folders(const String& runtime_uuid) const;
		void set_view_state(const String& runtime_uuid, const Dictionary& view_state);
		Dictionary get_view_state(const String& runtime_uuid) const;
		Dictionary get_track_info() const;
		PackedStringArray get_chart_files_path() const;
		PackedStringArray get_scene_files_path() const;
		PackedStringArray get_shader_files_path() const;
		PackedStringArray get_skin_files_path() const;
		void save_track_info(const Dictionary& new_track_info);
		Error save_text_file(const String& path, const String& content);
		Error restore_text_file(const String& path, const bool existed, const String& content);
		Error move_path(const String& from_path, const String& to_path, const bool move_references = true, const String& runtime_uuid = String());
	};
}

#endif // !LTEFILE_SYSTEM_SERVER

