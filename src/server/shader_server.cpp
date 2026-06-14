#include "shader_server.h"

#include "file_save_server.h"
#include "file_system_server.h"
#include "preferences_manager.h"
#include "project_manager.h"
#include "utils.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTEShaderServer* LTEShaderServer::singleton = nullptr;

	void LTEShaderServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_on_file_saved", "path", "tag", "revision"), &LTEShaderServer::_on_file_saved);
		ClassDB::bind_method(D_METHOD("_on_file_save_failed", "path", "tag", "revision", "error"), &LTEShaderServer::_on_file_save_failed);
		ClassDB::bind_method(D_METHOD("normalize_shader_path", "shader_path"), &LTEShaderServer::normalize_shader_path);
		ClassDB::bind_method(D_METHOD("request_open_shader", "runtime_uuid", "shader_path"), &LTEShaderServer::request_open_shader);
		ClassDB::bind_method(D_METHOD("notify_shader_file_changed", "shader_path"), &LTEShaderServer::notify_shader_file_changed);
		ClassDB::bind_method(D_METHOD("save_shader_file", "shader_path", "content", "scan_file_system"), &LTEShaderServer::save_shader_file, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("queue_shader_file_save", "shader_path", "content", "scan_file_system", "delay_msec"), &LTEShaderServer::queue_shader_file_save, DEFVAL(true), DEFVAL(-1));
		ADD_SIGNAL(MethodInfo("shader_open_requested", PropertyInfo(Variant::STRING, "runtime_uuid"), PropertyInfo(Variant::STRING, "shader_path")));
		ADD_SIGNAL(MethodInfo("shader_file_changed", PropertyInfo(Variant::STRING, "shader_path")));
		ADD_SIGNAL(MethodInfo("shader_file_saved",
				PropertyInfo(Variant::STRING, "shader_path"),
				PropertyInfo(Variant::INT, "revision")));
		ADD_SIGNAL(MethodInfo("shader_file_save_failed",
				PropertyInfo(Variant::STRING, "shader_path"),
				PropertyInfo(Variant::INT, "revision"),
				PropertyInfo(Variant::INT, "error")));
	}

	LTEShaderServer::LTEShaderServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (file_save_server) {
			file_save_server->connect("file_saved", Callable(this, "_on_file_saved"));
			file_save_server->connect("file_save_failed", Callable(this, "_on_file_save_failed"));
		}
	}

	LTEShaderServer::~LTEShaderServer() {
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (file_save_server) {
			file_save_server->flush_pending_saves();
			Callable saved_callable = Callable(this, "_on_file_saved");
			Callable failed_callable = Callable(this, "_on_file_save_failed");
			if (file_save_server->is_connected("file_saved", saved_callable)) {
				file_save_server->disconnect("file_saved", saved_callable);
			}
			if (file_save_server->is_connected("file_save_failed", failed_callable)) {
				file_save_server->disconnect("file_save_failed", failed_callable);
			}
		}
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEShaderServer* LTEShaderServer::get_singleton() {
		return singleton;
	}

	String LTEShaderServer::normalize_shader_path(const String& shader_path) const {
		String normalized_path = shader_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_path.is_empty()) {
			return String();
		}
		if (normalized_path.begins_with("res://")) {
			return normalized_path;
		}

		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_dir = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (normalized_path.begins_with("/") && !root_dir.is_empty()) {
			return (root_dir + normalized_path).simplify_path();
		}
		if (Utils::is_absolute_path(normalized_path)) {
			return normalized_path;
		}
		if (root_dir.is_empty()) {
			return normalized_path;
		}
		return root_dir.path_join(normalized_path).replace("\\", "/").simplify_path();
	}

	void LTEShaderServer::request_open_shader(const String& runtime_uuid, const String& shader_path) {
		String normalized_path = normalize_shader_path(shader_path);
		if (normalized_path.is_empty()) {
			return;
		}
		emit_signal("shader_open_requested", runtime_uuid, normalized_path);
	}

	void LTEShaderServer::notify_shader_file_changed(const String& shader_path) {
		String normalized_path = normalize_shader_path(shader_path);
		if (normalized_path.is_empty()) {
			return;
		}
		emit_signal("shader_file_changed", normalized_path);
	}

	String LTEShaderServer::_make_shader_save_tag(const String& shader_path, const bool scan_file_system) const {
		return "shader\n" + shader_path + "\n" + (scan_file_system ? "1" : "0");
	}

	bool LTEShaderServer::_parse_shader_save_tag(const String& tag, String& r_shader_path, bool& r_scan_file_system) const {
		PackedStringArray parts = tag.split("\n", false);
		if (parts.size() != 3 || parts[0] != "shader") {
			return false;
		}
		r_shader_path = parts[1];
		r_scan_file_system = parts[2] == "1";
		return true;
	}

	int32_t LTEShaderServer::_get_auto_save_delay_msec() const {
		LTEPreferencesManager* preferences_manager = LTEPreferencesManager::get_singleton();
		double interval = preferences_manager != nullptr
			? preferences_manager->get_float_value("core.auto_save_interval", DEFAULT_AUTO_SAVE_INTERVAL_SECONDS)
			: DEFAULT_AUTO_SAVE_INTERVAL_SECONDS;
		interval = UtilityFunctions::clampf(interval, 0.1, 60.0);
		return static_cast<int32_t>(interval * 1000.0);
	}

	void LTEShaderServer::_on_file_saved(const String& path, const String& tag, const int64_t revision) {
		String shader_path;
		bool scan_file_system = false;
		if (!_parse_shader_save_tag(tag, shader_path, scan_file_system)) {
			return;
		}
		notify_shader_file_changed(shader_path);
		emit_signal("shader_file_saved", shader_path, revision);
		if (scan_file_system) {
			LTEFileSystemServer* file_system_server = LTEFileSystemServer::get_singleton();
			if (file_system_server) {
				file_system_server->scan_directory_completely();
			}
		}
	}

	void LTEShaderServer::_on_file_save_failed(const String& path, const String& tag, const int64_t revision, const int64_t error) {
		String shader_path;
		bool scan_file_system = false;
		if (!_parse_shader_save_tag(tag, shader_path, scan_file_system)) {
			return;
		}
		ERR_PRINT(vformat("Failed to save shader file: %s. Error Code: %d", path, error));
		emit_signal("shader_file_save_failed", shader_path, revision, error);
	}

	Error LTEShaderServer::save_shader_file(const String& shader_path, const String& content, const bool scan_file_system) {
		String normalized_path = normalize_shader_path(shader_path);
		if (normalized_path.is_empty()) {
			return ERR_FILE_BAD_PATH;
		}

		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (!file_save_server) {
			return ERR_UNCONFIGURED;
		}
		Error error = file_save_server->save_text_now(normalized_path, content, true);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to save shader file: %s. Error Code: %d", normalized_path, error));
			return error;
		}

		notify_shader_file_changed(normalized_path);
		if (scan_file_system) {
			LTEFileSystemServer* file_system_server = LTEFileSystemServer::get_singleton();
			if (file_system_server) {
				file_system_server->scan_directory_completely();
			}
		}
		return OK;
	}

	int64_t LTEShaderServer::queue_shader_file_save(const String& shader_path, const String& content, const bool scan_file_system, const int32_t delay_msec) {
		String normalized_path = normalize_shader_path(shader_path);
		if (normalized_path.is_empty()) {
			return -1;
		}
		LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
		if (!file_save_server) {
			return -1;
		}
		int32_t save_delay_msec = delay_msec >= 0 ? delay_msec : _get_auto_save_delay_msec();
		return file_save_server->queue_text_save(normalized_path, content, _make_shader_save_tag(normalized_path, scan_file_system), save_delay_msec, true);
	}
}
