#ifndef PLUGIN_CONTEXT_H
#define PLUGIN_CONTEXT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {
	class LTEPluginContext : public RefCounted {
		GDCLASS(LTEPluginContext, RefCounted)

	private:
		String plugin_id;
		Array registrations;
		String project_root;

		String _get_user_config_dir() const;
		String _get_user_config_path() const;
		String _get_project_config_dir() const;
		String _get_project_config_path() const;
		void _save_config_file(const String& path, const Dictionary& config);
		Dictionary _load_config_file(const String& path) const;

	protected:
		static void _bind_methods();

	public:
		LTEPluginContext();
		~LTEPluginContext();

		void set_plugin_id(const String& pid);
		String get_plugin_id() const;

		void set_project_root(const String& root);
		String get_project_root() const;

		void register_panel(const String& panel_uuid, const String& panel_name, const String& scene_path, const String& icon_path = String(), const String& space_type = String());
		void register_menu(const String& menu_id, const String& menu_name, const String& icon_path = String(), const String& tooltip = String());
		void register_menu_item(const String& menu_id, int32_t item_id, const String& item_name, const String& icon_path = String(), const Dictionary& metadata = Dictionary());
		void register_command(const String& command_name, const Callable& callable);

		void save_user_config(const Dictionary& config);
		Dictionary load_user_config() const;
		void save_project_config(const Dictionary& config);
		Dictionary load_project_config() const;
		String get_cache_dir() const;
		void clear_cache();

		void unregister_all();
	};
}

#endif // !PLUGIN_CONTEXT_H
