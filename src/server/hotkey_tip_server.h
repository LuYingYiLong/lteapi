#ifndef HOTKEY_TIP_SERVER_H
#define HOTKEY_TIP_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
	class LTEHotkeyTipServer : public Object {
		GDCLASS(LTEHotkeyTipServer, Object)

	private:
		static LTEHotkeyTipServer* singleton;

	protected:
		static void _bind_methods();

	public:
		LTEHotkeyTipServer();
		~LTEHotkeyTipServer();

		static LTEHotkeyTipServer* get_singleton();

		void clear_hotkeys();
		void add_hotkey(const StringName& action, const String& tip);
		void deploy_hotkeys(const Array& hotkeys);
	};
}

#endif // HOTKEY_TIP_SERVER_H
