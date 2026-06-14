#include "panel_option_button.h"

namespace godot {
	void PanelOptionButton::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_on_component_panel_registry_changed", "panel_uuid"), &PanelOptionButton::_on_component_panel_registry_changed);
		ClassDB::bind_method(D_METHOD("_on_workspace_focus_mode_changed"), &PanelOptionButton::_on_workspace_focus_mode_changed);
		ClassDB::bind_method(D_METHOD("_on_item_selected", "index"), &PanelOptionButton::_on_item_selected);
		ClassDB::bind_method(D_METHOD("_on_add_panel_menu_callback", "panel_info", "id"), &PanelOptionButton::_on_add_panel_menu_callback);
		ClassDB::bind_method(D_METHOD("set_current_panel_uuid", "panel_id"), &PanelOptionButton::set_current_panel_uuid);
		ClassDB::bind_method(D_METHOD("get_current_panel_uuid"), &PanelOptionButton::get_current_panel_uuid);
		ClassDB::bind_method(D_METHOD("set_current_runtime_uuid", "runtime_uuid"), &PanelOptionButton::set_current_runtime_uuid);
		ClassDB::bind_method(D_METHOD("get_current_runtime_uuid"), &PanelOptionButton::get_current_runtime_uuid);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "panel_uuid"), "set_current_panel_uuid", "get_current_panel_uuid");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "runtime_uuid"), "set_current_runtime_uuid", "get_current_runtime_uuid");
	}

	void PanelOptionButton::_notification(int p_what) {
		if (p_what == NOTIFICATION_READY) {
			set_fit_to_longest_item(false);
			set_text_overrun_behavior(TextServer::OVERRUN_TRIM_CHAR);
			populate_items();
			if (component_registry) {
				component_registry->connect("component_panel_registered", Callable(this, "_on_component_panel_registry_changed"));
				component_registry->connect("component_panel_unregistered", Callable(this, "_on_component_panel_registry_changed"));
			}
			if (workspace_manager) {
				Callable focused_callable = Callable(this, "_on_workspace_focus_mode_changed");
				if (!workspace_manager->is_connected("panel_focused", focused_callable)) {
					workspace_manager->connect("panel_focused", focused_callable);
				}
				if (!workspace_manager->is_connected("panel_unfocused", focused_callable)) {
					workspace_manager->connect("panel_unfocused", focused_callable);
				}
			}
		}
	}

	PanelOptionButton::PanelOptionButton() {
		component_registry = LTEComponentRegistry::get_singleton();
		workspace_manager = LTEWorkspaceManager::get_singleton();
	}

	PanelOptionButton::~PanelOptionButton() {
		component_registry = nullptr;
		workspace_manager = nullptr;
		items.clear();
	}

	void PanelOptionButton::_on_component_panel_registry_changed(const String& panel_uuid) {
		populate_items();
	}

	void PanelOptionButton::_on_workspace_focus_mode_changed() {
		_refresh_menu_state();
	}

	Ref<Texture2D> PanelOptionButton::_try_load_icon(const String& p_path) {
		ResourceUID* resource_uid = ResourceUID::get_singleton();
		ResourceLoader* resource_loader = ResourceLoader::get_singleton();
		String path = p_path;
		if (path.begins_with("uid://")) {
			path = resource_uid->uid_to_path(path);
		}
		if (path.begins_with("res://")) {
			Ref<Resource> icon_res = resource_loader->load(path, "Texture2D");
			Ref<Texture2D> icon = icon_res;
			if (icon.is_null()) return Ref<Texture2D>();
			return icon;
		}
		else if (path.is_absolute_path()) {
			Ref<Image> image = Image::load_from_file(path);
			if (image.is_null()) return Ref<Texture2D>();
			Ref<Texture2D> icon = ImageTexture::create_from_image(image);
			return icon;
		}
		return Ref<Texture2D>();
	}

	void PanelOptionButton::populate_items() {
		PopupMenu* popup = get_popup();
		popup->clear();
		popup->set_size(Vector2i(0, 0));
		last_selected = -1;
		Array panel_components = component_registry->get_registered_panel_components();
		Array space_types;
		for (int32_t index = 0; index < panel_components.size(); index++) {
			Dictionary panel_info = panel_components[index];
			String space_type = panel_info.get("space_type", String("GENERAL_NAME"));
			if (space_type.is_empty()) {
				space_type = "GENERAL_NAME";
			}
			if (!space_types.has(space_type)) {
				space_types.append(space_type);
			}
		}
		for (int32_t space_index = 0; space_index < space_types.size(); space_index++) {
			String space_type = space_types[space_index];
			popup->add_separator(space_type, -1);
			for (int32_t panel_index = 0; panel_index < panel_components.size(); panel_index++) {
				Dictionary panel_info = panel_components[panel_index];
				String panel_space_type = panel_info.get("space_type", String("GENERAL_NAME"));
				if (panel_space_type.is_empty()) {
					panel_space_type = "GENERAL_NAME";
				}
				if (panel_space_type == space_type) {
					_append_panel_item(panel_info, panel_index);
				}
			}
		}
		popup->add_separator("", -1);
		for (const auto& item : items) {
			Ref<Texture2D> icon = _try_load_icon(item.uid_path);
			if (icon.is_valid()) {
				popup->add_icon_item(icon, item.text, item.id);
			}
			else {
				popup->add_item(item.text, item.id);
			}
		}
		Callable selected_callable = Callable(this, "_on_item_selected");
		if (!is_connected("item_selected", selected_callable)) {
			connect("item_selected", selected_callable);
		}
		_refresh_menu_state();
	}

	void PanelOptionButton::_append_panel_item(const Dictionary& panel_info, int32_t panel_id) {
		PopupMenu* popup = get_popup();
		String panel_name = panel_info.get("name", "Unnamed Panel");
		String panel_uuid = panel_info.get("uuid", "");
		popup->add_item(panel_name, panel_id);
		int32_t item_index = popup->get_item_count() - 1;
		if (panel_uuid == current_panel_uuid) {
			set_deferred("selected", item_index);
			last_selected = item_index;
		}
		if (panel_info.has("icon")) {
			String icon_path = panel_info.get("icon", "");
			Ref<Texture2D> icon = _try_load_icon(icon_path);
			popup->set_item_icon(item_index, icon);
		}
		popup->set_item_metadata(item_index, panel_uuid);
	}

	void PanelOptionButton::_refresh_menu_state() {
		PopupMenu* popup = get_popup();
		int32_t focus_mode_index = _get_popup_item_index_by_id(MENU_ID_FOCUS_MODE);
		if (focus_mode_index >= 0 && workspace_manager) {
			popup->set_item_disabled(focus_mode_index, workspace_manager->is_panel_focus_mode_active());
		}
	}

	int32_t PanelOptionButton::_get_popup_item_index_by_id(int32_t item_id) const {
		PopupMenu* popup = get_popup();
		for (int32_t index = 0; index < popup->get_item_count(); index++) {
			if (popup->get_item_id(index) == item_id) {
				return index;
			}
		}
		return -1;
	}

	void PanelOptionButton::_on_item_selected(int32_t index) {
		int32_t id = get_item_id(index);
		if (id >= 0) {
			PopupMenu* popup = get_popup();
			String uuid = popup->get_item_metadata(index);
			workspace_manager->change_panel(workspace_manager->get_last_opened_layout(), get_owner(), uuid, current_runtime_uuid);
			last_selected = index;
		}
		else {
			set("selected", last_selected);
			if (id >= MENU_ID_TOP_VERTICAL_SPLIT && id <= MENU_ID_LEFT_HORIZONTAL_SPLIT) {
				ResourceUID* resource_uid = ResourceUID::get_singleton();
				ResourceLoader* resource_loader = ResourceLoader::get_singleton();
				if (resource_uid->has_id(resource_uid->text_to_id("uid://hwxq2tvcaeoa"))) {
					String path = resource_uid->uid_to_path("uid://hwxq2tvcaeoa");
					Ref<Resource> res = resource_loader->load(path);
					Ref<PackedScene> packed_scene = res;
					if (packed_scene.is_null()) {
						return;
					}
					Node* add_panel_menu = packed_scene->instantiate();
					LTEAPI* api = LTEAPI::get_singleton();
					Node* root_node = api ? api->get_editor_instance() : nullptr;
					if (!root_node) {
						memdelete(add_panel_menu);
						return;
					}
					root_node->add_child(add_panel_menu);
					add_panel_menu->connect("callback", Callable(this, "_on_add_panel_menu_callback").bind(id), Object::CONNECT_ONE_SHOT);
				}
			}
			else {
				switch (id) {
				case MENU_ID_FOCUS_MODE: workspace_manager->enter_panel_focus_mode(get_owner()); break;
				case MENU_ID_DUPLICATE_TO_WINDOW: workspace_manager->duplicate_panel_into_new_window(get_owner(), current_panel_uuid, current_runtime_uuid); break;
				case MENU_ID_CLOSE: workspace_manager->close_panel(workspace_manager->get_last_opened_layout(), get_owner()); break;
				}
				_refresh_menu_state();
			}
		}
	}

	void PanelOptionButton::_on_add_panel_menu_callback(const Dictionary& panel_info, int32_t id) {
		if (panel_info.is_empty()) return;
		String uuid = panel_info.get("uuid", "");
		LTEWorkspaceManager::SplitDirection split_direction = LTEWorkspaceManager::SPLIT_HORZ_RIGHT;
		switch (id) {
		case MENU_ID_LEFT_HORIZONTAL_SPLIT:
			split_direction = LTEWorkspaceManager::SPLIT_HORZ_LEFT;
			break;
		case MENU_ID_RIGHT_HORIZONTAL_SPLIT:
			split_direction = LTEWorkspaceManager::SPLIT_HORZ_RIGHT;
			break;
		case MENU_ID_BOTTOM_VERTICAL_SPLIT:
			split_direction = LTEWorkspaceManager::SPLIT_VERT_BOTTOM;
			break;
		case MENU_ID_TOP_VERTICAL_SPLIT:
			split_direction = LTEWorkspaceManager::SPLIT_VERT_TOP;
			break;
		}
		workspace_manager->add_panel(
			workspace_manager->get_last_opened_layout(),
			get_owner(),
			uuid,
			split_direction
		);
	}

	void PanelOptionButton::set_current_panel_uuid(const String& p_uuid) { current_panel_uuid = p_uuid; }
	String PanelOptionButton::get_current_panel_uuid() const { return current_panel_uuid; }

	void PanelOptionButton::set_current_runtime_uuid(const String& p_uuid) { current_runtime_uuid = p_uuid; }
	String PanelOptionButton::get_current_runtime_uuid() const { return current_runtime_uuid; }
}
