#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/gd_script.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include "zip_helper.h"
#include "plugin_manifest.h"
#include "plugin_runtime.h"
#include "plugin_context.h"

namespace godot {
	class LTEPluginManager : public Object {
		GDCLASS(LTEPluginManager, Object)

	private:
		static LTEPluginManager* singleton;

		static const int MAX_FAIL_COUNT = 3;
		static const char* PLUGINS_BASE_DIR;
		static const char* CRASH_STATE_PATH;

		HashMap<String, Ref<LTEPluginRuntime>> runtimes;

		void _scan_installed_plugins();
		void _scan_plugin_versions(const String& plugin_id, const String& plugin_root);
		Ref<LTEPluginManifest> _read_package_manifest(const String& zip_path) const;
		Ref<LTEPluginRuntime> _get_runtime(const String& plugin_id) const;
		bool _is_crash_disabled(const String& plugin_id) const;
		void _clear_crash_state(const String& plugin_id);
		void _save_crash_state(const String& plugin_id, int32_t fail_count, const String& last_error, bool disabled = false);
		Variant _safe_call(const String& plugin_id, const String& method, const Array& args = Array());
		bool _is_plugin_instance(const Variant& instance) const;
		Dictionary _make_unknown_details(const String& plugin_id) const;
		void _delete_dir_recursive(const String& dir_path);
		void _cleanup_stale_staging_dirs();
		void _record_fail(const String& plugin_id, const String& error_msg);
		void _save_plugin_user_state(const String& plugin_id, bool enabled);
		void _remove_plugin_user_state(const String& plugin_id);
		bool _is_plugin_user_disabled(const String& plugin_id) const;
		String _get_current_project_root() const;

		bool _check_dependencies_satisfied(const String& plugin_id, PackedStringArray* out_missing) const;
		bool _check_optional_dependencies_satisfied(const String& plugin_id, PackedStringArray* out_missing) const;
		bool _detect_circular_dependency(const String& plugin_id, const String& target_dep_id, PackedStringArray& visited) const;
		PackedStringArray _find_dependents(const String& plugin_id) const;
		bool _is_dependency_version_satisfied(const String& required_version, const String& installed_version) const;
		void _sync_context_project_root(const String& plugin_id);

	protected:
		static void _bind_methods();

	public:
		LTEPluginManager();
		~LTEPluginManager();

		static LTEPluginManager* get_singleton();

		PackedStringArray get_plugins() const;
		PackedStringArray get_enabled_plugins() const;
		bool is_plugin_enabled(const String& plugin_id) const;
		Dictionary get_plugin_details(const String& plugin_id) const;
		void enable_plugin(const String& plugin_id);
		void disable_plugin(const String& plugin_id);
		bool install_plugin(const String& zip_path);
		void uninstall_plugin(const String& plugin_id);
		void auto_enable_safe_plugins();
		Variant safe_call_plugin(const String& plugin_id, const String& method, const Array& args = Array());
		void reset_crash_state(const String& plugin_id);
		void notify_editor_ready();
		void notify_project_opened(const Dictionary& project);
		void notify_project_closed();

		Dictionary get_dependency_status(const String& plugin_id) const;
		PackedStringArray get_dependents(const String& plugin_id) const;
		bool can_enable_plugin(const String& plugin_id) const;
		bool can_disable_plugin(const String& plugin_id) const;

		static Dictionary get_permission_info(const String& permission);
		static bool check_version_compatible(const String& plugin_version, const String& min_editor_version);
		static String get_editor_version();
		void append_plugin_log(const String& plugin_id, const String& message);
		void append_plugin_log_entry(const String& plugin_id, const String& level, const String& event, const String& message, const Dictionary& extra = Dictionary());
		PackedStringArray get_plugin_log(const String& plugin_id) const;
		void clear_plugin_log(const String& plugin_id);
		void save_plugin_log(const String& plugin_id);
		void load_plugin_logs();
	};
}

#endif // !PLUGIN_MANAGER_H
