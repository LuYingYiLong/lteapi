#include "editor_panel_container.h"

#include "hotkey_tip_server.h"
#include "lte_api.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	EditorPanelContainer::EditorPanelContainer() {
		set_process_input(true);
		set_focus_mode(FOCUS_ALL);
	}

	void EditorPanelContainer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("process_panel_input", "event"), &EditorPanelContainer::process_panel_input);
		ClassDB::bind_method(D_METHOD("set_hotkey_tips", "new_hotkey_tips"), &EditorPanelContainer::set_hotkey_tips);
		ClassDB::bind_method(D_METHOD("get_hotkey_tips"), &EditorPanelContainer::get_hotkey_tips);
		ClassDB::bind_method(D_METHOD("clear_all_hotkey_tips"), &EditorPanelContainer::clear_all_hotkey_tips);
		ClassDB::bind_method(D_METHOD("set_presence_panel_name", "name"), &EditorPanelContainer::set_presence_panel_name);
		ClassDB::bind_method(D_METHOD("get_presence_panel_name"), &EditorPanelContainer::get_presence_panel_name);

		ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "hotkey_tips", PROPERTY_HINT_DICTIONARY_TYPE, "StringName;String"), "set_hotkey_tips", "get_hotkey_tips");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "presence_panel_name"), "set_presence_panel_name", "get_presence_panel_name");
	}

	void EditorPanelContainer::_input(const Ref<InputEvent>& p_event) {
		process_panel_input(p_event);
	}

	void EditorPanelContainer::process_panel_input(const Ref<InputEvent>& p_event) {
		if (!_can_process_panel_input()) {
			return;
		}
		Ref<InputEventMouseButton> mouse_button = p_event;
		if (mouse_button.is_null() || !mouse_button->is_pressed()) {
			return;
		}
		const StringName theme_type = get_theme_type_variation();
		if (theme_type == StringName("WindowPanel")) {
			return;
		}
		const Rect2 panel_rect(Vector2(), get_size());
		if (panel_rect.has_point(get_local_mouse_position())) {
			if (theme_type.is_empty()) {
				set_theme_type_variation(StringName("FocusPanelContainer"));
			}
			grab_focus();
			_deploy_hotkey_tips();
			return;
		}
		if (theme_type == StringName("FocusPanelContainer")) {
			set_theme_type_variation(StringName());
		}
	}

	void EditorPanelContainer::set_hotkey_tips(const Dictionary& p_hotkey_tips) {
		hotkey_tips = p_hotkey_tips;
	}

	Dictionary EditorPanelContainer::get_hotkey_tips() const {
		return hotkey_tips;
	}

	void EditorPanelContainer::clear_all_hotkey_tips() {
		hotkey_tips.clear();
	}

	void EditorPanelContainer::set_presence_panel_name(const String& p_name) {
		presence_panel_name = p_name;
	}

	String EditorPanelContainer::get_presence_panel_name() const {
		return presence_panel_name;
	}

	bool EditorPanelContainer::_can_process_panel_input() const {
		return is_visible_in_tree() && _is_inside_editor_focused_panel();
	}

	bool EditorPanelContainer::_is_editor_focus_mode_active() const {
		LTEAPI* api = LTEAPI::get_singleton();
		if (!api) {
			return false;
		}
		Node* focused_panel_node = api->get_editor_focused_panel_root();
		Control* focused_panel = Object::cast_to<Control>(focused_panel_node);
		return focused_panel && focused_panel->is_visible_in_tree() && focused_panel->get_child_count() > 0;
	}

	bool EditorPanelContainer::_is_inside_editor_focused_panel() const {
		if (!_is_editor_focus_mode_active()) {
			return true;
		}
		LTEAPI* api = LTEAPI::get_singleton();
		if (!api) {
			return true;
		}
		Node* focused_panel_node = api->get_editor_focused_panel_root();
		if (!focused_panel_node) {
			return true;
		}
		return focused_panel_node == this || focused_panel_node->is_ancestor_of(const_cast<EditorPanelContainer*>(this));
	}

	void EditorPanelContainer::_deploy_hotkey_tips() const {
		LTEHotkeyTipServer* hotkey_tip_server = LTEHotkeyTipServer::get_singleton();
		if (!hotkey_tip_server) {
			return;
		}
		hotkey_tip_server->clear_hotkeys();
		Array keys = hotkey_tips.keys();
		for (int64_t index = 0; index < keys.size(); index++) {
			Variant action_key = keys[index];
			if (action_key.get_type() != Variant::STRING_NAME && action_key.get_type() != Variant::STRING) {
				continue;
			}
			StringName action = action_key.get_type() == Variant::STRING_NAME ? StringName(action_key) : StringName(String(action_key));
			String tip = hotkey_tips.get(action_key, "");
			if (action == StringName() || tip.is_empty()) {
				continue;
			}
			hotkey_tip_server->add_hotkey(action, tip);
		}

		if (presence_panel_name.is_empty()) {
			return;
		}
		LTEAPI *api = LTEAPI::get_singleton();
		if (!api) {
			return;
		}
		SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
		if (!tree) {
			return;
		}
		Node* editor_session = tree->get_root()->get_node_or_null(NodePath("EditorSession"));
		if (!editor_session || !editor_session->has_method("update_local_presence")) {
			return;
		}
		Node *editor_instance = api->get_editor_instance();
		String layout_id;
		if (editor_instance && editor_instance->has_method("get_current_presence_layout_id")) {
			layout_id = editor_instance->call("get_current_presence_layout_id");
		}
		Control *workspace_root = Object::cast_to<Control>(api->get_editor_workspace_root());
		Vector2 pointer_ratio;
		if (workspace_root) {
			Vector2 workspace_size = workspace_root->get_size();
			Vector2 workspace_position = workspace_root->get_local_mouse_position();
			if (workspace_size.x > 0.0) {
				pointer_ratio.x = CLAMP(workspace_position.x / workspace_size.x, 0.0, 1.0);
			}
			if (workspace_size.y > 0.0) {
				pointer_ratio.y = CLAMP(workspace_position.y / workspace_size.y, 0.0, 1.0);
			}
		}
		Dictionary presence;
		presence["panel"] = presence_panel_name;
		presence["action"] = "viewing";
		presence["description"] = TranslationServer::get_singleton()->translate(presence_panel_name);
		if (!layout_id.is_empty()) {
			presence["layout_id"] = layout_id;
			presence["panel_active"] = false;
			presence["pointer_rx"] = pointer_ratio.x;
			presence["pointer_ry"] = pointer_ratio.y;
			presence["pointer_visible"] = true;
		}
		editor_session->call("update_local_presence", presence);
	}
}
