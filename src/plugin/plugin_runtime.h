#ifndef PLUGIN_RUNTIME_H
#define PLUGIN_RUNTIME_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "plugin_manifest.h"

namespace godot {
	class LTEPluginRuntime : public RefCounted {
		GDCLASS(LTEPluginRuntime, RefCounted)

	private:
		String plugin_id;
		Ref<LTEPluginManifest> manifest;
		Variant instance;
		Variant context;
		bool enabled;
		String install_dir;
		int32_t fail_count;
		String last_error;
		PackedStringArray log_entries;
		int32_t log_sequence;

		static const int32_t MAX_MEMORY_LOG_ENTRIES = 200;
		static const int32_t MAX_DISK_LOG_ENTRIES = 1000;
		static const char* LOG_BASE_DIR;

		String _get_log_dir() const;
		String _get_log_file_path() const;
		void _trim_disk_log() const;

	protected:
		static void _bind_methods();

	public:
		LTEPluginRuntime();
		~LTEPluginRuntime();

		void set_plugin_id(const String& pid);
		String get_plugin_id() const;

		void set_manifest(const Ref<LTEPluginManifest>& m);
		Ref<LTEPluginManifest> get_manifest() const;

		void set_instance(const Variant& inst);
		Variant get_instance() const;

		void set_context(const Variant& ctx);
		Variant get_context() const;

		void set_enabled(bool e);
		bool get_enabled() const;

		void set_install_dir(const String& dir);
		String get_install_dir() const;

		void set_fail_count(int32_t fc);
		int32_t get_fail_count() const;

		void set_last_error(const String& le);
		String get_last_error() const;

		void append_log(const String& message);
		void append_log_entry(const String& level, const String& event, const String& message, const Dictionary& extra = Dictionary());
		PackedStringArray get_log() const;
		void clear_log();

		void save_log_to_disk();
		void load_log_from_disk();
		String get_log_file_path() const;

		Dictionary get_details() const;
	};
}

#endif // !PLUGIN_RUNTIME_H
