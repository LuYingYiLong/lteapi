#include "plugin_runtime.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	const char* LTEPluginRuntime::LOG_BASE_DIR = "user://logs/plugins";

	void LTEPluginRuntime::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_plugin_id", "plugin_id"), &LTEPluginRuntime::set_plugin_id);
		ClassDB::bind_method(D_METHOD("get_plugin_id"), &LTEPluginRuntime::get_plugin_id);
		ClassDB::bind_method(D_METHOD("set_manifest", "manifest"), &LTEPluginRuntime::set_manifest);
		ClassDB::bind_method(D_METHOD("get_manifest"), &LTEPluginRuntime::get_manifest);
		ClassDB::bind_method(D_METHOD("set_instance", "instance"), &LTEPluginRuntime::set_instance);
		ClassDB::bind_method(D_METHOD("get_instance"), &LTEPluginRuntime::get_instance);
		ClassDB::bind_method(D_METHOD("set_context", "context"), &LTEPluginRuntime::set_context);
		ClassDB::bind_method(D_METHOD("get_context"), &LTEPluginRuntime::get_context);
		ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &LTEPluginRuntime::set_enabled);
		ClassDB::bind_method(D_METHOD("get_enabled"), &LTEPluginRuntime::get_enabled);
		ClassDB::bind_method(D_METHOD("set_install_dir", "install_dir"), &LTEPluginRuntime::set_install_dir);
		ClassDB::bind_method(D_METHOD("get_install_dir"), &LTEPluginRuntime::get_install_dir);
		ClassDB::bind_method(D_METHOD("set_fail_count", "fail_count"), &LTEPluginRuntime::set_fail_count);
		ClassDB::bind_method(D_METHOD("get_fail_count"), &LTEPluginRuntime::get_fail_count);
		ClassDB::bind_method(D_METHOD("set_last_error", "last_error"), &LTEPluginRuntime::set_last_error);
		ClassDB::bind_method(D_METHOD("get_last_error"), &LTEPluginRuntime::get_last_error);
		ClassDB::bind_method(D_METHOD("append_log", "message"), &LTEPluginRuntime::append_log);
		ClassDB::bind_method(D_METHOD("append_log_entry", "level", "event", "message", "extra"), &LTEPluginRuntime::append_log_entry, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("get_log"), &LTEPluginRuntime::get_log);
		ClassDB::bind_method(D_METHOD("clear_log"), &LTEPluginRuntime::clear_log);
		ClassDB::bind_method(D_METHOD("save_log_to_disk"), &LTEPluginRuntime::save_log_to_disk);
		ClassDB::bind_method(D_METHOD("load_log_from_disk"), &LTEPluginRuntime::load_log_from_disk);
		ClassDB::bind_method(D_METHOD("get_log_file_path"), &LTEPluginRuntime::get_log_file_path);
		ClassDB::bind_method(D_METHOD("get_details"), &LTEPluginRuntime::get_details);
	}

	LTEPluginRuntime::LTEPluginRuntime() {
		enabled = false;
		fail_count = 0;
		log_sequence = 0;
	}

	LTEPluginRuntime::~LTEPluginRuntime() {
	}

	void LTEPluginRuntime::set_plugin_id(const String& pid) { plugin_id = pid; }
	String LTEPluginRuntime::get_plugin_id() const { return plugin_id; }

	void LTEPluginRuntime::set_manifest(const Ref<LTEPluginManifest>& m) { manifest = m; }
	Ref<LTEPluginManifest> LTEPluginRuntime::get_manifest() const { return manifest; }

	void LTEPluginRuntime::set_instance(const Variant& inst) { instance = inst; }
	Variant LTEPluginRuntime::get_instance() const { return instance; }

	void LTEPluginRuntime::set_context(const Variant& ctx) { context = ctx; }
	Variant LTEPluginRuntime::get_context() const { return context; }

	void LTEPluginRuntime::set_enabled(bool e) { enabled = e; }
	bool LTEPluginRuntime::get_enabled() const { return enabled; }

	void LTEPluginRuntime::set_install_dir(const String& dir) { install_dir = dir; }
	String LTEPluginRuntime::get_install_dir() const { return install_dir; }

	void LTEPluginRuntime::set_fail_count(int32_t fc) { fail_count = fc; }
	int32_t LTEPluginRuntime::get_fail_count() const { return fail_count; }

	void LTEPluginRuntime::set_last_error(const String& le) { last_error = le; }
	String LTEPluginRuntime::get_last_error() const { return last_error; }

	String LTEPluginRuntime::_get_log_dir() const {
		return String(LOG_BASE_DIR).path_join(plugin_id);
	}

	String LTEPluginRuntime::_get_log_file_path() const {
		return _get_log_dir().path_join("plugin.log");
	}

	void LTEPluginRuntime::append_log(const String& message) {
		append_log_entry("info", "log", message);
	}

	void LTEPluginRuntime::append_log_entry(const String& level, const String& event, const String& message, const Dictionary& extra) {
		Dictionary entry;
		entry["time"] = Time::get_singleton()->get_unix_time_from_system();
		entry["level"] = level;
		entry["event"] = event;
		entry["message"] = message;
		entry["seq"] = log_sequence++;

		Array extra_keys = extra.keys();
		for (int64_t i = 0; i < extra_keys.size(); i++) {
			String key = String(extra_keys[i]);
			entry[key] = extra[key];
		}

		String line = JSON::stringify(entry, "");
		log_entries.append(line);

		if (log_entries.size() > MAX_MEMORY_LOG_ENTRIES) {
			log_entries.remove_at(0);
		}

		save_log_to_disk();
	}

	PackedStringArray LTEPluginRuntime::get_log() const {
		return log_entries;
	}

	void LTEPluginRuntime::clear_log() {
		log_entries.clear();
		String log_path = _get_log_file_path();
		if (FileAccess::file_exists(log_path)) {
			DirAccess::remove_absolute(log_path);
		}
	}

	void LTEPluginRuntime::save_log_to_disk() {
		String log_dir = _get_log_dir();
		DirAccess::make_dir_recursive_absolute(log_dir);

		Ref<FileAccess> file = FileAccess::open(_get_log_file_path(), FileAccess::WRITE);
		if (file.is_null()) {
			return;
		}

		for (int64_t i = 0; i < log_entries.size(); i++) {
			file->store_line(log_entries[i]);
		}
		file->close();

		_trim_disk_log();
	}

	void LTEPluginRuntime::load_log_from_disk() {
		String log_path = _get_log_file_path();
		if (!FileAccess::file_exists(log_path)) {
			return;
		}

		Ref<FileAccess> file = FileAccess::open(log_path, FileAccess::READ);
		if (file.is_null()) {
			return;
		}

		log_entries.clear();
		log_sequence = 0;
		while (!file->eof_reached()) {
			String line = file->get_line().strip_edges();
			if (line.is_empty()) {
				continue;
			}
			log_entries.append(line);
			log_sequence++;
		}
		file->close();

		while (log_entries.size() > MAX_MEMORY_LOG_ENTRIES) {
			log_entries.remove_at(0);
		}
	}

	String LTEPluginRuntime::get_log_file_path() const {
		return _get_log_file_path();
	}

	void LTEPluginRuntime::_trim_disk_log() const {
		if (log_entries.size() <= MAX_DISK_LOG_ENTRIES) {
			return;
		}

		String log_path = _get_log_file_path();
		if (!FileAccess::file_exists(log_path)) {
			return;
		}

		Ref<FileAccess> file = FileAccess::open(log_path, FileAccess::READ);
		if (file.is_null()) {
			return;
		}

		PackedStringArray all_lines;
		while (!file->eof_reached()) {
			String line = file->get_line().strip_edges();
			if (!line.is_empty()) {
				all_lines.append(line);
			}
		}
		file->close();

		while (all_lines.size() > MAX_DISK_LOG_ENTRIES) {
			all_lines.remove_at(0);
		}

		Ref<FileAccess> out = FileAccess::open(log_path, FileAccess::WRITE);
		if (out.is_null()) {
			return;
		}
		for (int64_t i = 0; i < all_lines.size(); i++) {
			out->store_line(all_lines[i]);
		}
		out->close();
	}

	Dictionary LTEPluginRuntime::get_details() const {
		Dictionary details;
		if (manifest.is_valid()) {
			details = manifest->to_dict();
		}
		else {
			details["id"] = plugin_id;
			details["name"] = plugin_id;
			details["version"] = "--";
			details["api_version"] = "--";
			details["author"] = "--";
			details["description"] = "--";
		}
		details["enabled"] = enabled;
		details["fail_count"] = fail_count;
		details["last_error"] = last_error;
		return details;
	}
}
