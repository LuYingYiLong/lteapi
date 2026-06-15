#include "plugin_manager.h"

#include "plugin_base.h"

#include "../server/lte_user.h"
#include "../server/project_manager.h"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	const char* LTEPluginManager::PLUGINS_BASE_DIR = "user://plugins";
	const char* LTEPluginManager::SDK_COLLECTION_BASE_DIR = "user://sdks";
	const char* LTEPluginManager::CRASH_STATE_PATH = "user://plugin_crash_state.cfg";
	const char* LTEPluginManager::SDK_MANAGER_CONFIG_PATH = "user://plugin_sdk_manager.cfg";

	LTEPluginManager* LTEPluginManager::singleton = nullptr;

	void LTEPluginManager::_bind_methods() {
		ClassDB::bind_method(D_METHOD("get_plugins"), &LTEPluginManager::get_plugins);
		ClassDB::bind_method(D_METHOD("get_enabled_plugins"), &LTEPluginManager::get_enabled_plugins);
		ClassDB::bind_method(D_METHOD("is_plugin_enabled", "plugin_id"), &LTEPluginManager::is_plugin_enabled);
		ClassDB::bind_method(D_METHOD("get_plugin_details", "plugin_id"), &LTEPluginManager::get_plugin_details);
		ClassDB::bind_method(D_METHOD("enable_plugin", "plugin_id"), &LTEPluginManager::enable_plugin);
		ClassDB::bind_method(D_METHOD("disable_plugin", "plugin_id"), &LTEPluginManager::disable_plugin);
		ClassDB::bind_method(D_METHOD("install_plugin", "zip_path"), &LTEPluginManager::install_plugin);
		ClassDB::bind_method(D_METHOD("uninstall_plugin", "plugin_id"), &LTEPluginManager::uninstall_plugin);
		ClassDB::bind_method(D_METHOD("auto_enable_safe_plugins"), &LTEPluginManager::auto_enable_safe_plugins);
		ClassDB::bind_method(D_METHOD("safe_call_plugin", "plugin_id", "method", "args"), &LTEPluginManager::safe_call_plugin, DEFVAL(Array()));
		ClassDB::bind_method(D_METHOD("reset_crash_state", "plugin_id"), &LTEPluginManager::reset_crash_state);
		ClassDB::bind_method(D_METHOD("notify_editor_ready"), &LTEPluginManager::notify_editor_ready);
		ClassDB::bind_method(D_METHOD("notify_project_opened", "project"), &LTEPluginManager::notify_project_opened);
		ClassDB::bind_method(D_METHOD("notify_project_closed"), &LTEPluginManager::notify_project_closed);
		ClassDB::bind_method(D_METHOD("get_dependency_status", "plugin_id"), &LTEPluginManager::get_dependency_status);
		ClassDB::bind_method(D_METHOD("get_dependents", "plugin_id"), &LTEPluginManager::get_dependents);
		ClassDB::bind_method(D_METHOD("can_enable_plugin", "plugin_id"), &LTEPluginManager::can_enable_plugin);
		ClassDB::bind_method(D_METHOD("can_disable_plugin", "plugin_id"), &LTEPluginManager::can_disable_plugin);
		ClassDB::bind_static_method("LTEPluginManager", D_METHOD("get_permission_info", "permission"), &LTEPluginManager::get_permission_info);
		ClassDB::bind_static_method("LTEPluginManager", D_METHOD("check_version_compatible", "plugin_version", "min_editor_version"), &LTEPluginManager::check_version_compatible);
		ClassDB::bind_static_method("LTEPluginManager", D_METHOD("get_editor_version"), &LTEPluginManager::get_editor_version);
		ClassDB::bind_method(D_METHOD("append_plugin_log", "plugin_id", "message"), &LTEPluginManager::append_plugin_log);
		ClassDB::bind_method(D_METHOD("append_plugin_log_entry", "plugin_id", "level", "event", "message", "extra"), &LTEPluginManager::append_plugin_log_entry, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("get_plugin_log", "plugin_id"), &LTEPluginManager::get_plugin_log);
		ClassDB::bind_method(D_METHOD("clear_plugin_log", "plugin_id"), &LTEPluginManager::clear_plugin_log);
		ClassDB::bind_method(D_METHOD("save_plugin_log", "plugin_id"), &LTEPluginManager::save_plugin_log);
		ClassDB::bind_method(D_METHOD("load_plugin_logs"), &LTEPluginManager::load_plugin_logs);

		ClassDB::bind_method(D_METHOD("get_sdk_collection_dir"), &LTEPluginManager::get_sdk_collection_dir);
		ClassDB::bind_method(D_METHOD("ensure_sdk_collection_dir"), &LTEPluginManager::ensure_sdk_collection_dir);
		ClassDB::bind_method(D_METHOD("validate_sdk_path", "sdk_path"), &LTEPluginManager::validate_sdk_path);
		ClassDB::bind_method(D_METHOD("scan_installed_sdks"), &LTEPluginManager::scan_installed_sdks);
		ClassDB::bind_method(D_METHOD("get_installed_sdks"), &LTEPluginManager::get_installed_sdks);
		ClassDB::bind_method(D_METHOD("install_sdk_from_zip", "zip_path"), &LTEPluginManager::install_sdk_from_zip);
		ClassDB::bind_method(D_METHOD("remove_installed_sdk", "sdk_version"), &LTEPluginManager::remove_installed_sdk);
		ClassDB::bind_method(D_METHOD("get_default_sdk_path"), &LTEPluginManager::get_default_sdk_path);
		ClassDB::bind_method(D_METHOD("set_default_sdk_path", "sdk_path"), &LTEPluginManager::set_default_sdk_path);
		ClassDB::bind_method(D_METHOD("create_plugin_project", "options"), &LTEPluginManager::create_plugin_project);

		ADD_SIGNAL(MethodInfo("sdk_installed", PropertyInfo(Variant::STRING, "sdk_version")));
		ADD_SIGNAL(MethodInfo("sdk_removed", PropertyInfo(Variant::STRING, "sdk_version")));

		ADD_SIGNAL(MethodInfo("plugin_installed", PropertyInfo(Variant::STRING, "plugin_id")));
		ADD_SIGNAL(MethodInfo("plugin_uninstalled", PropertyInfo(Variant::STRING, "plugin_id")));
		ADD_SIGNAL(MethodInfo("plugin_enabled", PropertyInfo(Variant::STRING, "plugin_id")));
		ADD_SIGNAL(MethodInfo("plugin_disabled", PropertyInfo(Variant::STRING, "plugin_id")));
	}

	LTEPluginManager::LTEPluginManager() {
		if (singleton == nullptr) {
			singleton = this;
		}
		_scan_installed_plugins();
		load_plugin_logs();
		_load_sdk_config();
	}

	LTEPluginManager::~LTEPluginManager() {
		if (singleton == this) {
			singleton = nullptr;
		}
		runtimes.clear();
	}

	LTEPluginManager* LTEPluginManager::get_singleton() {
		return singleton;
	}

	PackedStringArray LTEPluginManager::get_plugins() const {
		PackedStringArray ids;
		for (const auto& kv : runtimes) {
			ids.append(kv.key);
		}
		return ids;
	}

	PackedStringArray LTEPluginManager::get_enabled_plugins() const {
		PackedStringArray ids;
		for (const auto& kv : runtimes) {
			if (kv.value.is_valid() && kv.value->get_enabled()) {
				ids.append(kv.key);
			}
		}
		return ids;
	}

	bool LTEPluginManager::is_plugin_enabled(const String& plugin_id) const {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_valid()) {
			return runtime->get_enabled();
		}
		return false;
	}

	Dictionary LTEPluginManager::get_plugin_details(const String& plugin_id) const {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_valid()) {
			Dictionary details = runtime->get_details();
			details["dependents"] = _find_dependents(plugin_id);
			details["can_enable"] = can_enable_plugin(plugin_id);
			details["can_disable"] = can_disable_plugin(plugin_id);
			details["dependency_status"] = get_dependency_status(plugin_id);
			return details;
		}
		return _make_unknown_details(plugin_id);
	}

	void LTEPluginManager::enable_plugin(const String& plugin_id) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			ERR_PRINT(vformat("Plugin '%s' is not installed.", plugin_id));
			return;
		}
		if (runtime->get_enabled()) {
			return;
		}

		if (_is_crash_disabled(plugin_id)) {
			WARN_PRINT(vformat("Plugin '%s' was auto-disabled due to previous crashes. Reset crash state to retry.", plugin_id));
			append_plugin_log_entry(plugin_id, "warn", "enable_blocked", "Plugin blocked by crash recovery.");
			return;
		}

		Ref<LTEPluginManifest> manifest = runtime->get_manifest();
		if (manifest.is_null()) {
			ERR_PRINT(vformat("Plugin '%s' manifest is missing.", plugin_id));
			_record_fail(plugin_id, "manifest missing");
			return;
		}

		PackedStringArray missing_deps;
		if (!_check_dependencies_satisfied(plugin_id, &missing_deps)) {
			String missing_str = String(", ").join(missing_deps);
			ERR_PRINT(vformat("Plugin '%s' depends on missing or disabled plugins: %s", plugin_id, missing_str));
			append_plugin_log_entry(plugin_id, "error", "enable_blocked", vformat("Missing dependencies: %s", missing_str));
			return;
		}

		PackedStringArray visited;
		Dictionary deps = manifest->get_dependencies();
		Array dep_keys = deps.keys();
		for (int64_t i = 0; i < dep_keys.size(); i++) {
			String dep_id = String(dep_keys[i]);
			if (_detect_circular_dependency(plugin_id, dep_id, visited)) {
				ERR_PRINT(vformat("Circular dependency detected: %s <-> %s", plugin_id, dep_id));
				append_plugin_log_entry(plugin_id, "error", "enable_blocked", vformat("Circular dependency detected with: %s", dep_id));
				return;
			}
		}

		String pck_path = runtime->get_install_dir().path_join("plugin.pck");
		if (FileAccess::file_exists(pck_path)) {
			bool ok = ProjectSettings::get_singleton()->load_resource_pack(pck_path, false);
			if (!ok) {
				ERR_PRINT(vformat("Failed to mount PCK: %s", pck_path));
				_record_fail(plugin_id, vformat("PCK mount failed: %s", pck_path));
				return;
			}
		}

		String entry_path = manifest->get_entry();
		if (!ResourceLoader::get_singleton()->exists(entry_path)) {
			ERR_PRINT(vformat("Plugin entry resource not found: %s", entry_path));
			_record_fail(plugin_id, vformat("Entry resource not found: %s", entry_path));
			return;
		}

		Ref<GDScript> entry_script = ResourceLoader::get_singleton()->load(entry_path, "", ResourceLoader::CACHE_MODE_REUSE);
		if (entry_script.is_null()) {
			ERR_PRINT(vformat("Failed to load entry script: %s", entry_path));
			_record_fail(plugin_id, vformat("Failed to load entry script: %s", entry_path));
			return;
		}

		Variant instance = entry_script->call("new");
		if (instance.get_type() == Variant::NIL || !_is_plugin_instance(instance)) {
			ERR_PRINT(vformat("Plugin entry must inherit LTEPluginBase: %s", plugin_id));
			_record_fail(plugin_id, "Entry does not inherit LTEPluginBase");
			return;
		}

		runtime->set_instance(instance);

		Ref<LTEPluginContext> ctx;
		ctx.instantiate();
		ctx->set_plugin_id(plugin_id);
		_sync_context_project_root(plugin_id);
		runtime->set_context(ctx);

		Array init_args;
		init_args.append(ctx);
		_safe_call(plugin_id, "_initialize", init_args);

		_safe_call(plugin_id, "_enable");

		runtime->set_enabled(true);
		_clear_crash_state(plugin_id);
		emit_signal("plugin_enabled", plugin_id);
		_save_plugin_user_state(plugin_id, true);
		append_plugin_log_entry(plugin_id, "info", "enabled", vformat("Plugin enabled v%s", manifest->get_version()));
		print_line(vformat("Plugin enabled: %s v%s", plugin_id, manifest->get_version()));
	}

	void LTEPluginManager::disable_plugin(const String& plugin_id) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return;
		}
		if (!runtime->get_enabled()) {
			return;
		}

		PackedStringArray dependents = _find_dependents(plugin_id);
		for (int64_t i = 0; i < dependents.size(); i++) {
			String dep_id = dependents[i];
			if (is_plugin_enabled(dep_id)) {
				WARN_PRINT(vformat("Cannot disable plugin '%s': enabled plugin '%s' depends on it.", plugin_id, dep_id));
				append_plugin_log_entry(plugin_id, "warn", "disable_blocked", vformat("Enabled dependent '%s' blocks disable.", dep_id));
				return;
			}
		}

		_safe_call(plugin_id, "_disable");

		Variant ctx = runtime->get_context();
		if (ctx.get_type() != Variant::NIL) {
			Object* ctx_obj = Object::cast_to<Object>(ctx);
			if (ctx_obj) {
				ctx_obj->call("unregister_all");
			}
		}

		runtime->set_enabled(false);
		emit_signal("plugin_disabled", plugin_id);
		_save_plugin_user_state(plugin_id, false);
		append_plugin_log_entry(plugin_id, "info", "disabled", "Plugin disabled.");
		print_line(vformat("Plugin disabled: %s", plugin_id));
	}

	bool LTEPluginManager::install_plugin(const String& zip_path) {
		if (!FileAccess::file_exists(zip_path)) {
			ERR_PRINT(vformat("Plugin package not found: %s", zip_path));
			return false;
		}

		Ref<LTEPluginManifest> manifest = _read_package_manifest(zip_path);
		if (manifest.is_null()) {
			return false;
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error err = reader->open(zip_path);
		if (err != OK) {
			ERR_PRINT(vformat("Failed to open plugin package: %s (err=%d)", zip_path, err));
			return false;
		}

		String plugin_id = manifest->get_id();
		String version = manifest->get_version();
		String install_dir = String(LTEPluginManager::PLUGINS_BASE_DIR).path_join(plugin_id).path_join(version);
		String staging_dir = String(LTEPluginManager::PLUGINS_BASE_DIR).path_join(".staging").path_join(vformat("%s-%s-%d", plugin_id, version, Time::get_singleton()->get_unix_time_from_system()));

		DirAccess::make_dir_recursive_absolute(staging_dir);

		PackedStringArray files = reader->get_files();
		bool has_manifest = false;
		bool has_pck = false;
		for (int64_t i = 0; i < files.size(); i++) {
			String file_path = files[i];
			if (file_path == "manifest.json") has_manifest = true;
			if (file_path == "plugin.pck") has_pck = true;
			if (file_path.ends_with("/") || file_path.ends_with("\\")) {
				DirAccess::make_dir_recursive_absolute(staging_dir.path_join(file_path));
				continue;
			}
			PackedByteArray file_bytes = reader->read_file(file_path);
			if (file_bytes.is_empty()) {
				continue;
			}
			Ref<FileAccess> out_file = FileAccess::open(staging_dir.path_join(file_path), FileAccess::WRITE);
			if (out_file.is_null()) {
				WARN_PRINT(vformat("Failed to write file: %s", file_path));
				_delete_dir_recursive(staging_dir);
				reader->close();
				return false;
			}
			out_file->store_buffer(file_bytes);
			out_file->close();
		}
		reader->close();

		if (!has_pck) {
			ERR_PRINT("Plugin package is missing plugin.pck");
			_delete_dir_recursive(staging_dir);
			return false;
		}

		if (!has_manifest) {
			Ref<FileAccess> manifest_file = FileAccess::open(staging_dir.path_join("manifest.json"), FileAccess::WRITE);
			if (manifest_file.is_valid()) {
				String json_str = JSON::stringify(manifest->to_dict(), "\t");
				manifest_file->store_string(json_str);
				manifest_file->close();
			}
		}

		Dictionary receipt;
		receipt["installed_at"] = Time::get_singleton()->get_unix_time_from_system();
		receipt["package_name"] = zip_path.get_file();
		receipt["files"] = files;
		Ref<FileAccess> receipt_file = FileAccess::open(staging_dir.path_join("install_receipt.json"), FileAccess::WRITE);
		if (receipt_file.is_valid()) {
			receipt_file->store_string(JSON::stringify(receipt, "\t"));
			receipt_file->close();
		}

		if (DirAccess::dir_exists_absolute(install_dir)) {
			_delete_dir_recursive(install_dir);
		}
		DirAccess::make_dir_recursive_absolute(String(LTEPluginManager::PLUGINS_BASE_DIR).path_join(plugin_id));
		Error rename_err = DirAccess::rename_absolute(staging_dir, install_dir);
		if (rename_err != OK) {
			ERR_PRINT(vformat("Failed to move staging to install dir: %s -> %s (err=%d)", staging_dir, install_dir, rename_err));
			_delete_dir_recursive(staging_dir);
			return false;
		}

		_cleanup_stale_staging_dirs();

		Ref<LTEPluginRuntime> runtime;
		runtime.instantiate();
		runtime->set_plugin_id(plugin_id);
		runtime->set_manifest(manifest);
		runtime->set_install_dir(install_dir);
		runtimes[plugin_id] = runtime;

		emit_signal("plugin_installed", plugin_id);

		append_plugin_log_entry(plugin_id, "info", "installed", vformat("Plugin installed v%s from %s", version, zip_path.get_file()));
		_save_plugin_user_state(plugin_id, true);
		print_line(vformat("Plugin installed: %s v%s -> %s", plugin_id, version, install_dir));
		return true;
	}

	void LTEPluginManager::_cleanup_stale_staging_dirs() {
		String staging_root = String(PLUGINS_BASE_DIR).path_join(".staging");
		if (!DirAccess::dir_exists_absolute(staging_root)) {
			return;
		}
		Ref<DirAccess> dir = DirAccess::open(staging_root);
		if (dir.is_null()) {
			return;
		}
		dir->list_dir_begin();
		String entry = dir->get_next();
		while (!entry.is_empty()) {
			if (entry != "." && entry != "..") {
				String full_path = staging_root.path_join(entry);
				if (dir->current_is_dir()) {
					_delete_dir_recursive(full_path);
				}
			}
			entry = dir->get_next();
		}
		dir->list_dir_end();
	}

	void LTEPluginManager::uninstall_plugin(const String& plugin_id) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return;
		}

		if (runtime->get_enabled()) {
			disable_plugin(plugin_id);
		}

		_safe_call(plugin_id, "_shutdown");

		String install_dir = runtime->get_install_dir();
		if (DirAccess::dir_exists_absolute(install_dir)) {
			_delete_dir_recursive(install_dir);
		}

		_clear_crash_state(plugin_id);
		_remove_plugin_user_state(plugin_id);
		runtimes.erase(plugin_id);
		emit_signal("plugin_uninstalled", plugin_id);
		print_line(vformat("Plugin uninstalled: %s", plugin_id));
	}

	Variant LTEPluginManager::safe_call_plugin(const String& plugin_id, const String& method, const Array& args) {
		return _safe_call(plugin_id, method, args);
	}

	void LTEPluginManager::reset_crash_state(const String& plugin_id) {
		_clear_crash_state(plugin_id);
		append_plugin_log_entry(plugin_id, "info", "crash_reset", "Crash state reset by user.");
	}

	void LTEPluginManager::auto_enable_safe_plugins() {
		PackedStringArray plugin_ids = get_plugins();
		for (int64_t i = 0; i < plugin_ids.size(); i++) {
			String plugin_id = plugin_ids[i];
			if (_is_crash_disabled(plugin_id)) {
				continue;
			}
			if (_is_plugin_user_disabled(plugin_id)) {
				print_line(vformat("Plugin '%s' was previously disabled by user, skipping auto-enable.", plugin_id));
				continue;
			}
			enable_plugin(plugin_id);
		}
	}

	void LTEPluginManager::notify_editor_ready() {
		PackedStringArray plugin_ids = get_enabled_plugins();
		for (int64_t i = 0; i < plugin_ids.size(); i++) {
			_safe_call(plugin_ids[i], "_on_editor_ready");
		}
	}

	void LTEPluginManager::notify_project_opened(const Dictionary& project) {
		_sync_context_project_root("");

		PackedStringArray plugin_ids = get_enabled_plugins();
		for (int64_t i = 0; i < plugin_ids.size(); i++) {
			_sync_context_project_root(plugin_ids[i]);

			Array args;
			args.append(project);
			_safe_call(plugin_ids[i], "_on_project_opened", args);
		}
	}

	void LTEPluginManager::notify_project_closed() {
		PackedStringArray plugin_ids = get_enabled_plugins();
		for (int64_t i = 0; i < plugin_ids.size(); i++) {
			_safe_call(plugin_ids[i], "_on_project_closed");
		}
	}

	Dictionary LTEPluginManager::get_dependency_status(const String& plugin_id) const {
		Dictionary status;
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			status["ok"] = false;
			status["reason"] = "not_installed";
			return status;
		}

		Ref<LTEPluginManifest> manifest = runtime->get_manifest();
		if (manifest.is_null()) {
			status["ok"] = false;
			status["reason"] = "no_manifest";
			return status;
		}

		Dictionary deps = manifest->get_dependencies();
		PackedStringArray missing;
		Array dep_satisfied;
		Array dep_keys = deps.keys();
		for (int64_t i = 0; i < dep_keys.size(); i++) {
			String dep_id = String(dep_keys[i]);
			String required_version = String(deps[dep_id]);
			Ref<LTEPluginRuntime> dep_runtime = _get_runtime(dep_id);
			Dictionary dep_status;
			dep_status["id"] = dep_id;
			dep_status["required"] = required_version;

			if (dep_runtime.is_null()) {
				dep_status["satisfied"] = false;
				dep_status["reason"] = "not_installed";
				missing.append(dep_id);
			}
			else if (!dep_runtime->get_enabled()) {
				dep_status["satisfied"] = false;
				dep_status["reason"] = "not_enabled";
				missing.append(dep_id);
			}
			else if (!_is_dependency_version_satisfied(required_version, dep_runtime->get_manifest()->get_version())) {
				dep_status["satisfied"] = false;
				dep_status["reason"] = vformat("version_mismatch: %s required, %s installed", required_version, dep_runtime->get_manifest()->get_version());
				missing.append(dep_id);
			}
			else {
				dep_status["satisfied"] = true;
			}
			dep_satisfied.append(dep_status);
		}
		status["ok"] = missing.is_empty();
		status["dependencies"] = dep_satisfied;
		if (!missing.is_empty()) {
			status["missing"] = missing;
		}

		Dictionary opt_deps = manifest->get_optional_dependencies();
		Array opt_satisfied;
		Array opt_keys = opt_deps.keys();
		for (int64_t i = 0; i < opt_keys.size(); i++) {
			String dep_id = String(opt_keys[i]);
			String required_version = String(opt_deps[dep_id]);
			Ref<LTEPluginRuntime> dep_runtime = _get_runtime(dep_id);
			Dictionary opt_status;
			opt_status["id"] = dep_id;
			opt_status["required"] = required_version;
			opt_status["optional"] = true;

			if (dep_runtime.is_null()) {
				opt_status["satisfied"] = false;
				opt_status["reason"] = "not_installed";
			}
			else if (!dep_runtime->get_enabled()) {
				opt_status["satisfied"] = false;
				opt_status["reason"] = "not_enabled";
			}
			else if (!_is_dependency_version_satisfied(required_version, dep_runtime->get_manifest()->get_version())) {
				opt_status["satisfied"] = false;
				opt_status["reason"] = vformat("version_mismatch: %s required, %s installed", required_version, dep_runtime->get_manifest()->get_version());
			}
			else {
				opt_status["satisfied"] = true;
			}
			opt_satisfied.append(opt_status);
		}
		status["optional_dependencies"] = opt_satisfied;

		return status;
	}

	PackedStringArray LTEPluginManager::get_dependents(const String& plugin_id) const {
		return _find_dependents(plugin_id);
	}

	bool LTEPluginManager::can_enable_plugin(const String& plugin_id) const {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return false;
		}
		if (_is_crash_disabled(plugin_id)) {
			return false;
		}
		Ref<LTEPluginManifest> manifest = runtime->get_manifest();
		if (manifest.is_null()) {
			return false;
		}
		return _check_dependencies_satisfied(plugin_id, nullptr);
	}

	bool LTEPluginManager::can_disable_plugin(const String& plugin_id) const {
		if (!is_plugin_enabled(plugin_id)) {
			return false;
		}
		PackedStringArray dependents = _find_dependents(plugin_id);
		for (int64_t i = 0; i < dependents.size(); i++) {
			if (is_plugin_enabled(dependents[i])) {
				return false;
			}
		}
		return true;
	}

	Ref<LTEPluginRuntime> LTEPluginManager::_get_runtime(const String& plugin_id) const {
		const Ref<LTEPluginRuntime>* runtime = runtimes.getptr(plugin_id);
		if (runtime != nullptr) {
			return *runtime;
		}
		return Ref<LTEPluginRuntime>();
	}

	void LTEPluginManager::_scan_installed_plugins() {
		String base_dir = PLUGINS_BASE_DIR;
		if (!DirAccess::dir_exists_absolute(base_dir)) {
			return;
		}

		Ref<DirAccess> dir = DirAccess::open(base_dir);
		if (dir.is_null()) {
			return;
		}

		dir->list_dir_begin();
		String plugin_id = dir->get_next();
		while (!plugin_id.is_empty()) {
			if (plugin_id != "." && plugin_id != ".." && plugin_id != ".staging") {
				if (dir->current_is_dir() && LTEPluginManifest::is_valid_plugin_id(plugin_id)) {
					_scan_plugin_versions(plugin_id, base_dir.path_join(plugin_id));
				}
			}
			plugin_id = dir->get_next();
		}
		dir->list_dir_end();
	}

	void LTEPluginManager::_scan_plugin_versions(const String& plugin_id, const String& plugin_root) {
		Ref<DirAccess> dir = DirAccess::open(plugin_root);
		if (dir.is_null()) {
			return;
		}

		Array versions;
		dir->list_dir_begin();
		String ver = dir->get_next();
		while (!ver.is_empty()) {
			if (ver != "." && ver != ".." && dir->current_is_dir()) {
				versions.append(ver);
			}
			ver = dir->get_next();
		}
		dir->list_dir_end();

		if (versions.is_empty()) {
			return;
		}
		versions.sort();
		String latest = versions[versions.size() - 1];
		String install_dir = plugin_root.path_join(latest);
		String manifest_path = install_dir.path_join("manifest.json");

		if (!FileAccess::file_exists(manifest_path)) {
			return;
		}

		Ref<FileAccess> manifest_file = FileAccess::open(manifest_path, FileAccess::READ);
		if (manifest_file.is_null()) {
			return;
		}

		String json_string = manifest_file->get_as_text();
		manifest_file->close();

		Ref<LTEPluginManifest> manifest;
		manifest.instantiate();
		if (!manifest->parse_from_json(json_string)) {
			Dictionary result = manifest->get_parse_result();
			Array errors = result.get("errors", Array());
			for (int64_t i = 0; i < errors.size(); i++) {
				WARN_PRINT(vformat("Skipping invalid plugin %s: %s", plugin_id, String(errors[i])));
			}
			return;
		}

		Ref<LTEPluginRuntime> runtime;
		runtime.instantiate();
		runtime->set_plugin_id(plugin_id);
		runtime->set_manifest(manifest);
		runtime->set_install_dir(install_dir);
		runtimes[plugin_id] = runtime;
	}

	Ref<LTEPluginManifest> LTEPluginManager::_read_package_manifest(const String& zip_path) const {
		Ref<ZIPReader> reader;
		reader.instantiate();
		Error err = reader->open(zip_path);
		if (err != OK) {
			ERR_PRINT(vformat("Failed to open plugin package: %s (err=%d)", zip_path, err));
			return Ref<LTEPluginManifest>();
		}

		PackedByteArray manifest_bytes = reader->read_file("manifest.json");
		if (manifest_bytes.is_empty()) {
			ERR_PRINT(vformat("Plugin package '%s' is missing manifest.json", zip_path));
			reader->close();
			return Ref<LTEPluginManifest>();
		}

		Ref<LTEPluginManifest> manifest;
		manifest.instantiate();
		if (!manifest->parse_from_json(manifest_bytes.get_string_from_utf8())) {
			Dictionary result = manifest->get_parse_result();
			Array errors = result.get("errors", Array());
			for (int64_t i = 0; i < errors.size(); i++) {
				ERR_PRINT(vformat("Manifest validation failed: %s", String(errors[i])));
			}
			reader->close();
			return Ref<LTEPluginManifest>();
		}

		reader->close();
		return manifest;
	}

	Variant LTEPluginManager::_safe_call(const String& plugin_id, const String& method, const Array& args) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return Variant();
		}

		Variant instance = runtime->get_instance();
		if (instance.get_type() == Variant::NIL) {
			return Variant();
		}

		Object* obj = Object::cast_to<Object>(instance);
		if (obj == nullptr || !obj->has_method(method)) {
			return Variant();
		}

		Variant result = obj->callv(method, args);

		if (result.get_type() == Variant::OBJECT) {
			Object* res_obj = Object::cast_to<Object>(result);
			if (res_obj && res_obj->has_method("resume")) {
				WARN_PRINT(vformat("Plugin '%s' method '%s' returned a coroutine state. Async plugin callbacks are not supported.", plugin_id, method));
				return Variant();
			}
		}

		return result;
	}

	bool LTEPluginManager::_is_plugin_instance(const Variant& instance) const {
		if (instance.get_type() == Variant::NIL) {
			return false;
		}
		Object* obj = Object::cast_to<Object>(instance);
		if (obj == nullptr) {
			return false;
		}
		Ref<Script> script = obj->get_script();
		while (script.is_valid()) {
			String path = script->get_path();
			if (path.ends_with("lte_plugin_base.gd") || path.ends_with("plugin_base.gd")) {
				return true;
			}
			Ref<Script> base = script->get_base_script();
			if (base == script) {
				break;
			}
			script = base;
		}
		return false;
	}

	bool LTEPluginManager::_is_crash_disabled(const String& plugin_id) const {
		if (!FileAccess::file_exists(CRASH_STATE_PATH)) {
			return false;
		}
		Ref<ConfigFile> cfg;
		cfg.instantiate();
		cfg->load(CRASH_STATE_PATH);
		return cfg->get_value(plugin_id, "disabled_by_recovery", false);
	}

	void LTEPluginManager::_clear_crash_state(const String& plugin_id) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_valid()) {
			runtime->set_fail_count(0);
			runtime->set_last_error("");
		}
		if (!FileAccess::file_exists(CRASH_STATE_PATH)) {
			return;
		}
		Ref<ConfigFile> cfg;
		cfg.instantiate();
		cfg->load(CRASH_STATE_PATH);
		if (cfg->has_section(plugin_id)) {
			cfg->erase_section(plugin_id);
			cfg->save(CRASH_STATE_PATH);
		}
	}

	void LTEPluginManager::_save_crash_state(const String& plugin_id, int32_t fail_count, const String& last_error, bool disabled) {
		Ref<ConfigFile> cfg;
		cfg.instantiate();
		if (FileAccess::file_exists(CRASH_STATE_PATH)) {
			cfg->load(CRASH_STATE_PATH);
		}
		cfg->set_value(plugin_id, "fail_count", fail_count);
		cfg->set_value(plugin_id, "last_error", last_error);
		cfg->set_value(plugin_id, "disabled_by_recovery", disabled);
		cfg->save(CRASH_STATE_PATH);
	}

	void LTEPluginManager::_record_fail(const String& plugin_id, const String& error_msg) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return;
		}
		int32_t fc = runtime->get_fail_count() + 1;
		runtime->set_fail_count(fc);
		runtime->set_last_error(error_msg);
		_save_crash_state(plugin_id, fc, error_msg);

		Dictionary fail_extra;
		fail_extra["fail_count"] = fc;
		append_plugin_log_entry(plugin_id, "error", "fail", error_msg, fail_extra);

		if (fc >= MAX_FAIL_COUNT) {
			ERR_PRINT(vformat("Plugin '%s' has failed %d consecutive times. Auto-disabled.", plugin_id, fc));
			_save_crash_state(plugin_id, fc, error_msg, true);
			append_plugin_log_entry(plugin_id, "error", "auto_disabled", vformat("Auto-disabled after %d consecutive failures.", fc));
		}
	}

	Dictionary LTEPluginManager::_make_unknown_details(const String& plugin_id) const {
		Dictionary d;
		d["id"] = plugin_id;
		d["name"] = plugin_id;
		d["version"] = "--";
		d["api_version"] = "--";
		d["author"] = "--";
		d["description"] = "Unknown";
		d["enabled"] = false;
		d["fail_count"] = 0;
		d["last_error"] = "";
		d["dependents"] = Array();
		d["can_enable"] = false;
		d["can_disable"] = false;
		d["dependency_status"] = Dictionary();
		return d;
	}

	void LTEPluginManager::_delete_dir_recursive(const String& dir_path) {
		Ref<DirAccess> dir = DirAccess::open(dir_path);
		if (dir.is_null()) {
			return;
		}
		dir->list_dir_begin();
		String item = dir->get_next();
		while (!item.is_empty()) {
			if (item == "." || item == "..") {
				item = dir->get_next();
				continue;
			}
			String full_path = dir_path.path_join(item);
			if (dir->current_is_dir()) {
				_delete_dir_recursive(full_path);
			}
			else {
				DirAccess::remove_absolute(full_path);
			}
			item = dir->get_next();
		}
		dir->list_dir_end();
		DirAccess::remove_absolute(dir_path);
	}

	Dictionary LTEPluginManager::get_permission_info(const String& permission) {
		Dictionary info;
		if (permission == "editor_panels") {
			info["name"] = "PERM_NAME_EDITOR_PANELS";
			info["description"] = "PERM_DESC_EDITOR_PANELS";
			info["risk"] = "low";
		}
		else if (permission == "menus") {
			info["name"] = "PERM_NAME_MENUS";
			info["description"] = "PERM_DESC_MENUS";
			info["risk"] = "low";
		}
		else if (permission == "project_files") {
			info["name"] = "PERM_NAME_PROJECT_FILES";
			info["description"] = "PERM_DESC_PROJECT_FILES";
			info["risk"] = "medium";
		}
		else if (permission == "settings") {
			info["name"] = "PERM_NAME_SETTINGS";
			info["description"] = "PERM_DESC_SETTINGS";
			info["risk"] = "medium";
		}
		else if (permission == "workspace_layout") {
			info["name"] = "PERM_NAME_WORKSPACE_LAYOUT";
			info["description"] = "PERM_DESC_WORKSPACE_LAYOUT";
			info["risk"] = "medium";
		}
		else if (permission == "session") {
			info["name"] = "PERM_NAME_SESSION";
			info["description"] = "PERM_DESC_SESSION";
			info["risk"] = "medium";
		}
		else if (permission == "export") {
			info["name"] = "PERM_NAME_EXPORT";
			info["description"] = "PERM_DESC_EXPORT";
			info["risk"] = "medium";
		}
		else if (permission == "external_files") {
			info["name"] = "PERM_NAME_EXTERNAL_FILES";
			info["description"] = "PERM_DESC_EXTERNAL_FILES";
			info["risk"] = "high";
		}
		else if (permission == "network") {
			info["name"] = "PERM_NAME_NETWORK";
			info["description"] = "PERM_DESC_NETWORK";
			info["risk"] = "high";
		}
		else if (permission == "process") {
			info["name"] = "PERM_NAME_PROCESS";
			info["description"] = "PERM_DESC_PROCESS";
			info["risk"] = "high";
		}
		else if (permission == "background_service") {
			info["name"] = "PERM_NAME_BACKGROUND_SERVICE";
			info["description"] = "PERM_DESC_BACKGROUND_SERVICE";
			info["risk"] = "high";
		}
		else {
			info["name"] = "PERM_NAME_UNKNOWN";
			info["description"] = "PERM_DESC_UNKNOWN";
			info["risk"] = "unknown";
		}
		return info;
	}

	bool LTEPluginManager::check_version_compatible(const String& plugin_version, const String& min_editor_version) {
		if (min_editor_version.is_empty()) {
			return true;
		}
		return LTEPluginManifest::compare_versions(plugin_version, min_editor_version) >= 0;
	}

	String LTEPluginManager::get_editor_version() {
		return "0.0.1";
	}

	void LTEPluginManager::append_plugin_log(const String& plugin_id, const String& message) {
		append_plugin_log_entry(plugin_id, "info", "log", message);
	}

	void LTEPluginManager::append_plugin_log_entry(const String& plugin_id, const String& level, const String& event, const String& message, const Dictionary& extra) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return;
		}
		runtime->append_log_entry(level, event, message, extra);
	}

	PackedStringArray LTEPluginManager::get_plugin_log(const String& plugin_id) const {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return PackedStringArray();
		}
		return runtime->get_log();
	}

	void LTEPluginManager::clear_plugin_log(const String& plugin_id) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return;
		}
		runtime->clear_log();
	}

	void LTEPluginManager::save_plugin_log(const String& plugin_id) {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return;
		}
		runtime->save_log_to_disk();
	}

	void LTEPluginManager::load_plugin_logs() {
		for (auto& kv : runtimes) {
			if (kv.value.is_valid()) {
				kv.value->load_log_from_disk();
			}
		}
	}

	void LTEPluginManager::_save_plugin_user_state(const String& plugin_id, bool enabled) {
		LTEUser *user = LTEUser::get_singleton();
		if (user == nullptr) {
			return;
		}
		Dictionary states = user->get_plugin_enabled_states();
		states[plugin_id] = enabled;
		user->set_plugin_enabled_states(states);
		user->save_user_config();
	}

	void LTEPluginManager::_remove_plugin_user_state(const String& plugin_id) {
		LTEUser *user = LTEUser::get_singleton();
		if (user == nullptr) {
			return;
		}
		Dictionary states = user->get_plugin_enabled_states();
		if (states.has(plugin_id)) {
			states.erase(plugin_id);
			user->set_plugin_enabled_states(states);
			user->save_user_config();
		}
	}

	bool LTEPluginManager::_is_plugin_user_disabled(const String& plugin_id) const {
		LTEUser *user = LTEUser::get_singleton();
		if (user == nullptr) {
			return false;
		}
		Dictionary states = user->get_plugin_enabled_states();
		if (!states.has(plugin_id)) {
			return false;
		}
		return !bool(states[plugin_id]);
	}

	String LTEPluginManager::_get_current_project_root() const {
		LTEProjectManager* pm = LTEProjectManager::get_singleton();
		if (pm == nullptr) {
			return String();
		}
		String path = pm->get_project_path();
		if (path.is_empty()) {
			return String();
		}
		return path;
	}

	void LTEPluginManager::_sync_context_project_root(const String& plugin_id) {
		String root = _get_current_project_root();
		if (plugin_id.is_empty()) {
			for (auto& kv : runtimes) {
				if (kv.value.is_valid()) {
					Variant ctx = kv.value->get_context();
					if (ctx.get_type() != Variant::NIL) {
						Object* ctx_obj = Object::cast_to<Object>(ctx);
						if (ctx_obj && ctx_obj->has_method("set_project_root")) {
							ctx_obj->call("set_project_root", root);
						}
					}
				}
			}
		}
		else {
			Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
			if (runtime.is_valid()) {
				Variant ctx = runtime->get_context();
				if (ctx.get_type() != Variant::NIL) {
					Object* ctx_obj = Object::cast_to<Object>(ctx);
					if (ctx_obj && ctx_obj->has_method("set_project_root")) {
						ctx_obj->call("set_project_root", root);
					}
				}
			}
		}
	}

	bool LTEPluginManager::_check_dependencies_satisfied(const String& plugin_id, PackedStringArray* out_missing) const {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return false;
		}
		Ref<LTEPluginManifest> manifest = runtime->get_manifest();
		if (manifest.is_null()) {
			return false;
		}

		Dictionary deps = manifest->get_dependencies();
		Array keys = deps.keys();
		for (int64_t i = 0; i < keys.size(); i++) {
			String dep_id = String(keys[i]);
			String required_version = String(deps[dep_id]);

			Ref<LTEPluginRuntime> dep_runtime = _get_runtime(dep_id);
			if (dep_runtime.is_null() || !dep_runtime->get_enabled()) {
				if (out_missing != nullptr) {
					out_missing->append(dep_id);
				}
				return false;
			}

			Ref<LTEPluginManifest> dep_manifest = dep_runtime->get_manifest();
			if (dep_manifest.is_null()) {
				if (out_missing != nullptr) {
					out_missing->append(dep_id);
				}
				return false;
			}

			if (!_is_dependency_version_satisfied(required_version, dep_manifest->get_version())) {
				if (out_missing != nullptr) {
					out_missing->append(dep_id);
				}
				return false;
			}
		}
		return true;
	}

	bool LTEPluginManager::_check_optional_dependencies_satisfied(const String& plugin_id, PackedStringArray* out_missing) const {
		Ref<LTEPluginRuntime> runtime = _get_runtime(plugin_id);
		if (runtime.is_null()) {
			return true;
		}
		Ref<LTEPluginManifest> manifest = runtime->get_manifest();
		if (manifest.is_null()) {
			return true;
		}

		bool all_satisfied = true;
		Dictionary deps = manifest->get_optional_dependencies();
		Array keys = deps.keys();
		for (int64_t i = 0; i < keys.size(); i++) {
			String dep_id = String(keys[i]);
			String required_version = String(deps[dep_id]);

			Ref<LTEPluginRuntime> dep_runtime = _get_runtime(dep_id);
			if (dep_runtime.is_null() || !dep_runtime->get_enabled()) {
				if (out_missing != nullptr) {
					out_missing->append(dep_id);
				}
				all_satisfied = false;
				continue;
			}

			Ref<LTEPluginManifest> dep_manifest = dep_runtime->get_manifest();
			if (dep_manifest.is_null()) {
				if (out_missing != nullptr) {
					out_missing->append(dep_id);
				}
				all_satisfied = false;
				continue;
			}

			if (!_is_dependency_version_satisfied(required_version, dep_manifest->get_version())) {
				if (out_missing != nullptr) {
					out_missing->append(dep_id);
				}
				all_satisfied = false;
			}
		}
		return all_satisfied;
	}

	bool LTEPluginManager::_detect_circular_dependency(const String& plugin_id, const String& target_dep_id, PackedStringArray& visited) const {
		if (visited.has(target_dep_id)) {
			return target_dep_id == plugin_id;
		}
		if (target_dep_id == plugin_id) {
			return true;
		}

		visited.append(target_dep_id);

		Ref<LTEPluginRuntime> dep_runtime = _get_runtime(target_dep_id);
		if (dep_runtime.is_null()) {
			visited.remove_at(visited.size() - 1);
			return false;
		}

		Ref<LTEPluginManifest> dep_manifest = dep_runtime->get_manifest();
		if (dep_manifest.is_null()) {
			visited.remove_at(visited.size() - 1);
			return false;
		}

		Dictionary deps = dep_manifest->get_dependencies();
		Array keys = deps.keys();
		for (int64_t i = 0; i < keys.size(); i++) {
			String dep_id = String(keys[i]);
			if (_detect_circular_dependency(plugin_id, dep_id, visited)) {
				return true;
			}
		}

		visited.remove_at(visited.size() - 1);
		return false;
	}

	PackedStringArray LTEPluginManager::_find_dependents(const String& plugin_id) const {
		PackedStringArray dependents;
		for (const auto& kv : runtimes) {
			String other_id = kv.key;
			if (other_id == plugin_id) {
				continue;
			}
			Ref<LTEPluginRuntime> runtime = kv.value;
			if (runtime.is_null()) {
				continue;
			}
			Ref<LTEPluginManifest> manifest = runtime->get_manifest();
			if (manifest.is_null()) {
				continue;
			}
			Dictionary deps = manifest->get_dependencies();
			Dictionary opt_deps = manifest->get_optional_dependencies();
			if (deps.has(plugin_id) || opt_deps.has(plugin_id)) {
				dependents.append(other_id);
			}
		}
		return dependents;
	}

	bool LTEPluginManager::_is_dependency_version_satisfied(const String& required_version, const String& installed_version) const {
		if (required_version.is_empty()) {
			return true;
		}
		if (required_version.begins_with(">=")) {
			String req = required_version.substr(2).strip_edges();
			return LTEPluginManifest::compare_versions(installed_version, req) >= 0;
		}
		if (required_version.begins_with(">")) {
			String req = required_version.substr(1).strip_edges();
			return LTEPluginManifest::compare_versions(installed_version, req) > 0;
		}
		if (required_version.begins_with("<=")) {
			String req = required_version.substr(2).strip_edges();
			return LTEPluginManifest::compare_versions(installed_version, req) <= 0;
		}
		if (required_version.begins_with("<")) {
			String req = required_version.substr(1).strip_edges();
			return LTEPluginManifest::compare_versions(installed_version, req) < 0;
		}
		if (required_version.begins_with("^")) {
			String req = required_version.substr(1).strip_edges();
			PackedStringArray req_parts = req.split(".");
			PackedStringArray inst_parts = installed_version.split(".");
			int64_t req_major = req_parts.size() > 0 ? req_parts[0].to_int() : 0;
			int64_t inst_major = inst_parts.size() > 0 ? inst_parts[0].to_int() : 0;
			if (req_major != inst_major) {
				return false;
			}
			return LTEPluginManifest::compare_versions(installed_version, req) >= 0;
		}
		if (required_version.begins_with("~")) {
			String req = required_version.substr(1).strip_edges();
			PackedStringArray req_parts = req.split(".");
			PackedStringArray inst_parts = installed_version.split(".");
			int64_t req_major = req_parts.size() > 0 ? req_parts[0].to_int() : 0;
			int64_t inst_major = inst_parts.size() > 0 ? inst_parts[0].to_int() : 0;
			int64_t req_minor = req_parts.size() > 1 ? req_parts[1].to_int() : 0;
			int64_t inst_minor = inst_parts.size() > 1 ? inst_parts[1].to_int() : 0;
			if (req_major != inst_major || req_minor != inst_minor) {
				return false;
			}
			return LTEPluginManifest::compare_versions(installed_version, req) >= 0;
		}
		return LTEPluginManifest::compare_versions(installed_version, required_version) >= 0;
	}

	String LTEPluginManager::get_sdk_collection_dir() const {
		return String(SDK_COLLECTION_BASE_DIR);
	}

	Error LTEPluginManager::ensure_sdk_collection_dir() const {
		return DirAccess::make_dir_recursive_absolute(SDK_COLLECTION_BASE_DIR);
	}

	Dictionary LTEPluginManager::validate_sdk_path(const String& sdk_path) const {
		Dictionary result;
		Array missing;

		if (!DirAccess::dir_exists_absolute(sdk_path)) {
			result["ok"] = false;
			result["error"] = vformat("SDK directory does not exist: %s", sdk_path);
			return result;
		}

		const char* required_files[] = {
			"project.godot",
			"sdk_manifest.json",
			"tools/init_plugin.gd",
			"addons/lteapi/bin/lteapi.gdextension",
			"addons/lte_plugin_template/plugin.cfg"
		};

		for (int64_t i = 0; i < 5; i++) {
			String file_path = sdk_path.path_join(required_files[i]);
			if (!FileAccess::file_exists(file_path)) {
				missing.append(String(required_files[i]));
			}
		}

		if (!missing.is_empty()) {
			result["ok"] = false;
			result["error"] = vformat("SDK is missing required files: %s", String(", ").join(missing));
			result["missing"] = missing;
			return result;
		}

		String version = _read_sdk_manifest_version(sdk_path);
		result["ok"] = true;
		result["version"] = version;
		result["path"] = sdk_path;
		return result;
	}

	Array LTEPluginManager::scan_installed_sdks() {
		installed_sdks.clear();

		if (!DirAccess::dir_exists_absolute(SDK_COLLECTION_BASE_DIR)) {
			return installed_sdks;
		}

		Ref<DirAccess> dir = DirAccess::open(SDK_COLLECTION_BASE_DIR);
		if (dir.is_null()) {
			return installed_sdks;
		}

		dir->list_dir_begin();
		String entry = dir->get_next();
		while (!entry.is_empty()) {
			if (entry != "." && entry != ".." && dir->current_is_dir()) {
				Dictionary sdk_info = validate_sdk_path(String(SDK_COLLECTION_BASE_DIR).path_join(entry));
				if (bool(sdk_info.get("ok", false))) {
					Dictionary entry_dict;
					entry_dict["version"] = sdk_info["version"];
					entry_dict["path"] = String(SDK_COLLECTION_BASE_DIR).path_join(entry);
					installed_sdks.append(entry_dict);
				}
			}
			entry = dir->get_next();
		}
		dir->list_dir_end();

		_save_sdk_config();
		return installed_sdks;
	}

	Array LTEPluginManager::get_installed_sdks() const {
		return installed_sdks;
	}

	

	Dictionary LTEPluginManager::install_sdk_from_zip(const String& zip_path) {
		Dictionary result;

		if (!FileAccess::file_exists(zip_path)) {
			result["ok"] = false;
			result["error"] = vformat("SDK zip not found: %s", zip_path);
			return result;
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error open_err = reader->open(zip_path);
		if (open_err != OK) {
			result["ok"] = false;
			result["error"] = vformat("Failed to open SDK zip: %s (err=%d)", zip_path, open_err);
			return result;
		}

		PackedByteArray manifest_bytes = reader->read_file("sdk_manifest.json");
		if (manifest_bytes.is_empty()) {
			reader->close();
			result["ok"] = false;
			result["error"] = "SDK zip is missing sdk_manifest.json";
			return result;
		}

		String manifest_text = manifest_bytes.get_string_from_utf8();
		Ref<JSON> json;
		json.instantiate();
		Error parse_err = json->parse(manifest_text);
		if (parse_err != OK) {
			reader->close();
			result["ok"] = false;
			result["error"] = "Failed to parse sdk_manifest.json";
			return result;
		}

		Dictionary manifest = json->get_data();
		String sdk_version = manifest.get("sdk_version", "");
		if (sdk_version.is_empty()) {
			reader->close();
			result["ok"] = false;
			result["error"] = "sdk_manifest.json is missing sdk_version";
			return result;
		}

		String staging_dir = String(SDK_COLLECTION_BASE_DIR).path_join(vformat(".staging_%d", Time::get_singleton()->get_unix_time_from_system()));
		DirAccess::make_dir_recursive_absolute(staging_dir);

		PackedStringArray files = reader->get_files();
		for (int64_t i = 0; i < files.size(); i++) {
			String file_path = files[i];
			if (file_path.ends_with("/") || file_path.ends_with("\\")) {
				DirAccess::make_dir_recursive_absolute(staging_dir.path_join(file_path));
				continue;
			}
			PackedByteArray file_bytes = reader->read_file(file_path);
			if (file_bytes.is_empty()) {
				continue;
			}
			String dest_path = staging_dir.path_join(file_path);
			String dest_parent = dest_path.get_base_dir();
			DirAccess::make_dir_recursive_absolute(dest_parent);
			Ref<FileAccess> out_file = FileAccess::open(dest_path, FileAccess::WRITE);
			if (out_file.is_null()) {
				WARN_PRINT(vformat("Failed to write SDK file: %s", file_path));
				continue;
			}
			out_file->store_buffer(file_bytes);
			out_file->close();
		}
		reader->close();

		Dictionary staging_validation = validate_sdk_path(staging_dir);
		if (!bool(staging_validation.get("ok", false))) {
			_delete_dir_recursive(staging_dir);
			result["ok"] = false;
			result["error"] = String(staging_validation.get("error", "SDK validation failed after extraction"));
			return result;
		}

		String target_dir = String(SDK_COLLECTION_BASE_DIR).path_join(sdk_version);
		if (DirAccess::dir_exists_absolute(target_dir)) {
			_delete_dir_recursive(target_dir);
		}
		DirAccess::make_dir_recursive_absolute(SDK_COLLECTION_BASE_DIR);
		Error rename_err = DirAccess::rename_absolute(staging_dir, target_dir);
		if (rename_err != OK) {
			_delete_dir_recursive(staging_dir);
			result["ok"] = false;
			result["error"] = vformat("Failed to move SDK to install dir: %s (err=%d)", target_dir, rename_err);
			return result;
		}

		Dictionary entry;
		entry["version"] = sdk_version;
		entry["path"] = target_dir;
		entry["installed_at"] = Time::get_singleton()->get_unix_time_from_system();
		entry["source"] = "zip";

		for (int64_t i = 0; i < installed_sdks.size(); i++) {
			Dictionary existing = installed_sdks[i];
			if (String(existing.get("version", "")) == sdk_version) {
				installed_sdks[i] = entry;
				_save_sdk_config();
				emit_signal("sdk_installed", sdk_version);
				result["ok"] = true;
				result["version"] = sdk_version;
				result["path"] = target_dir;
				return result;
			}
		}

		installed_sdks.append(entry);
		_save_sdk_config();
		emit_signal("sdk_installed", sdk_version);
		result["ok"] = true;
		result["version"] = sdk_version;
		result["path"] = target_dir;
		return result;
	}

	Error LTEPluginManager::remove_installed_sdk(const String& sdk_version) {
		for (int64_t i = 0; i < installed_sdks.size(); i++) {
			Dictionary entry = installed_sdks[i];
			if (String(entry.get("version", "")) == sdk_version) {
				String sdk_path = entry.get("path", "");
				if (sdk_path.begins_with(String(SDK_COLLECTION_BASE_DIR)) && DirAccess::dir_exists_absolute(sdk_path)) {
					_delete_dir_recursive(sdk_path);
				}

				if (default_sdk_path == sdk_path) {
					default_sdk_path = String();
				}

				installed_sdks.remove_at(i);
				_save_sdk_config();
				emit_signal("sdk_removed", sdk_version);
				print_line(vformat("Removed SDK: %s", sdk_version));
				return OK;
			}
		}
		return ERR_DOES_NOT_EXIST;
	}

	String LTEPluginManager::get_default_sdk_path() const {
		if (!default_sdk_path.is_empty()) {
			Dictionary validation = validate_sdk_path(default_sdk_path);
			if (bool(validation.get("ok", false))) {
				return default_sdk_path;
			}
		}

		if (!installed_sdks.is_empty()) {
			Dictionary entry = installed_sdks[installed_sdks.size() - 1];
			return String(entry.get("path", ""));
		}

		return String();
	}

	Error LTEPluginManager::set_default_sdk_path(const String& sdk_path) {
		if (!sdk_path.is_empty()) {
			Dictionary validation = validate_sdk_path(sdk_path);
			if (!bool(validation.get("ok", false))) {
				ERR_PRINT(String(validation.get("error", "Invalid SDK path")));
				return ERR_INVALID_PARAMETER;
			}
		}
		default_sdk_path = sdk_path;
		_save_sdk_config();
		return OK;
	}

	Dictionary LTEPluginManager::create_plugin_project(const Dictionary& options) const {
		Dictionary result;

		String godot_path = options.get("godot_path", "");
		String sdk_path = options.get("sdk_path", "");
		String plugin_id = options.get("plugin_id", "");
		String plugin_name = options.get("plugin_name", "");
		String author = options.get("author", "");
		String description = options.get("description", "");
		String output_path = options.get("output_path", "");

		if (godot_path.is_empty()) {
			result["ok"] = false;
			result["error"] = "godot_path is required";
			return result;
		}
		if (sdk_path.is_empty()) {
			sdk_path = get_default_sdk_path();
			if (sdk_path.is_empty()) {
				result["ok"] = false;
				result["error"] = "No SDK configured. Install an SDK first.";
				return result;
			}
		}
		if (plugin_id.is_empty()) {
			result["ok"] = false;
			result["error"] = "plugin_id is required";
			return result;
		}
		if (plugin_name.is_empty()) {
			result["ok"] = false;
			result["error"] = "plugin_name is required";
			return result;
		}
		if (output_path.is_empty()) {
			result["ok"] = false;
			result["error"] = "output_path is required";
			return result;
		}

		Dictionary validation = validate_sdk_path(sdk_path);
		if (!bool(validation.get("ok", false))) {
			result["ok"] = false;
			result["error"] = String(validation.get("error", "SDK validation failed"));
			return result;
		}

		PackedStringArray args;
		args.append("--headless");
		args.append("--path");
		args.append(sdk_path);
		args.append("--script");
		args.append("res://tools/init_plugin.gd");
		args.append("--");
		args.append("--plugin-id");
		args.append(plugin_id);
		args.append("--plugin-name");
		args.append(plugin_name);
		args.append("--author");
		args.append(author);
		if (!description.is_empty()) {
			args.append("--description");
			args.append(description);
		}
		args.append("--output");
		args.append(output_path);

		int exit_code = -1;
		Array output;
		int exec_err = OS::get_singleton()->execute(godot_path, args, output, &exit_code, true);

		if (exec_err != 0) {
			result["ok"] = false;
			result["error"] = vformat("Failed to execute Godot: %s (err=%d)", godot_path, exec_err);
			result["output"] = output;
			result["exit_code"] = exit_code;
			return result;
		}

		result["ok"] = (exit_code == 0);
		result["output"] = output;
		result["exit_code"] = exit_code;
		result["output_path"] = output_path;

		if (exit_code == 0) {
			print_line(vformat("Plugin project created: %s -> %s", plugin_id, output_path));
		}
		else {
			result["error"] = vformat("init_plugin.gd exited with code %d", exit_code);
		}

		return result;
	}

	void LTEPluginManager::_load_sdk_config() {
		installed_sdks.clear();
		default_sdk_path = String();

		if (!FileAccess::file_exists(SDK_MANAGER_CONFIG_PATH)) {
			return;
		}

		Ref<ConfigFile> cfg;
		cfg.instantiate();
		Error err = cfg->load(SDK_MANAGER_CONFIG_PATH);
		if (err != OK) {
			return;
		}

		default_sdk_path = cfg->get_value("sdk", "default_path", "");

		Array sdk_list = cfg->get_value("sdk", "installed", Array());
		for (int64_t i = 0; i < sdk_list.size(); i++) {
			Variant item = sdk_list[i];
			if (item.get_type() == Variant::DICTIONARY) {
				installed_sdks.append(Dictionary(item));
			}
		}
	}

	void LTEPluginManager::_save_sdk_config() const {
		Ref<ConfigFile> cfg;
		cfg.instantiate();

		cfg->set_value("sdk", "default_path", default_sdk_path);
		cfg->set_value("sdk", "installed", installed_sdks);

		cfg->save(SDK_MANAGER_CONFIG_PATH);
	}

	String LTEPluginManager::_read_sdk_manifest_version(const String& sdk_path) const {
		String manifest_path = sdk_path.path_join("sdk_manifest.json");
		if (!FileAccess::file_exists(manifest_path)) {
			return String();
		}

		Ref<FileAccess> file = FileAccess::open(manifest_path, FileAccess::READ);
		if (file.is_null()) {
			return String();
		}

		String content = file->get_as_text();
		file->close();

		Ref<JSON> json;
		json.instantiate();
		Error err = json->parse(content);
		if (err != OK) {
			return String();
		}

		Dictionary data = json->get_data();
		return String(data.get("sdk_version", ""));
	}
}