#ifndef PATH_EDITOR_SERVER_H
#define PATH_EDITOR_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
	class LTESettingsConfig;

	class LTEPathEditorServer : public Object {
		GDCLASS(LTEPathEditorServer, Object)

	private:
		static LTEPathEditorServer* singleton;
		LTESettingsConfig* settings_config = nullptr;
		ObjectID active_editor_id;

	protected:
		static void _bind_methods();

	public:
		LTEPathEditorServer();
		~LTEPathEditorServer();

		static LTEPathEditorServer* get_singleton();

		void request_open_path(const Dictionary& context);
		void register_editor(Object* editor);
		void unregister_editor(Object* editor);
		void set_active_editor(Object* editor);
		Object* get_active_editor() const;

		Dictionary get_view_config(const String& uuid) const;
		void set_view_config(const String& uuid, const Dictionary& config);
		void clear_project_state();
	};
}

#endif
