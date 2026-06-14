#ifndef SHADER_SERVER_H
#define SHADER_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>

namespace godot {
	class LTEShaderServer : public Object {
		GDCLASS(LTEShaderServer, Object)

	private:
		static constexpr int32_t SHADER_SAVE_COALESCE_DELAY_MS = 600;
		static constexpr double DEFAULT_AUTO_SAVE_INTERVAL_SECONDS = 0.6;

		static LTEShaderServer* singleton;

		String _make_shader_save_tag(const String& shader_path, const bool scan_file_system) const;
		bool _parse_shader_save_tag(const String& tag, String& r_shader_path, bool& r_scan_file_system) const;
		int32_t _get_auto_save_delay_msec() const;
		void _on_file_saved(const String& path, const String& tag, const int64_t revision);
		void _on_file_save_failed(const String& path, const String& tag, const int64_t revision, const int64_t error);

	protected:
		static void _bind_methods();

	public:
		LTEShaderServer();
		~LTEShaderServer();

		static LTEShaderServer* get_singleton();

		String normalize_shader_path(const String& shader_path) const;
		void request_open_shader(const String& runtime_uuid, const String& shader_path);
		void notify_shader_file_changed(const String& shader_path);
		Error save_shader_file(const String& shader_path, const String& content, const bool scan_file_system = true);
		int64_t queue_shader_file_save(const String& shader_path, const String& content, const bool scan_file_system = true, const int32_t delay_msec = -1);
	};
}

#endif
