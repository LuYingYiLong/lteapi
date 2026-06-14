#include "hotkey_tip_server.h"

#include "lte_api.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	LTEHotkeyTipServer* LTEHotkeyTipServer::singleton = nullptr;

	void LTEHotkeyTipServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("clear_hotkeys"), &LTEHotkeyTipServer::clear_hotkeys);
		ClassDB::bind_method(D_METHOD("add_hotkey", "action", "tip"), &LTEHotkeyTipServer::add_hotkey);
		ClassDB::bind_method(D_METHOD("deploy_hotkeys", "hotkeys"), &LTEHotkeyTipServer::deploy_hotkeys);
	}

	LTEHotkeyTipServer::LTEHotkeyTipServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTEHotkeyTipServer::~LTEHotkeyTipServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEHotkeyTipServer* LTEHotkeyTipServer::get_singleton() {
		return singleton;
	}

	void LTEHotkeyTipServer::clear_hotkeys() {
		LTEAPI* api = LTEAPI::get_singleton();
		if (!api) {
			return;
		}
		Node* editor_instance = api->get_editor_instance();
		if (!editor_instance || !editor_instance->has_method("clear_hotkeys")) {
			return;
		}
		editor_instance->call("clear_hotkeys");
	}

	void LTEHotkeyTipServer::add_hotkey(const StringName& action, const String& tip) {
		LTEAPI* api = LTEAPI::get_singleton();
		if (!api) {
			return;
		}
		Node* editor_instance = api->get_editor_instance();
		if (!editor_instance || !editor_instance->has_method("add_hotkey")) {
			return;
		}
		editor_instance->call("add_hotkey", action, tip);
	}

	void LTEHotkeyTipServer::deploy_hotkeys(const Array& hotkeys) {
		clear_hotkeys();
		for (int64_t index = 0; index < hotkeys.size(); index++) {
			Variant hotkey_variant = hotkeys[index];
			if (hotkey_variant.get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary hotkey = hotkey_variant;
			Variant action_variant = hotkey.get("action", Variant());
			if (action_variant.get_type() != Variant::STRING_NAME && action_variant.get_type() != Variant::STRING) {
				continue;
			}
			StringName action;
			if (action_variant.get_type() == Variant::STRING_NAME) {
				action = action_variant;
			}
			else {
				action = StringName(String(action_variant));
			}
			String tip = hotkey.get("tip", "");
			if (action == StringName() || tip.is_empty()) {
				continue;
			}
			add_hotkey(action, tip);
		}
	}
}
