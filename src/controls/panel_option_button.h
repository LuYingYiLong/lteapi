#ifndef PANEL_OPTION_BUTTON_H
#define PANEL_OPTION_BUTTON_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/string.hpp>
#include <vector>

#include "component_registry.h"
#include "lte_api.h"
#include "workspace_manager.h"

namespace godot {
	class PanelOptionButton : public OptionButton {
		GDCLASS(PanelOptionButton, OptionButton)

	private:
		static constexpr int32_t MENU_ID_LEFT_HORIZONTAL_SPLIT = -2;
		static constexpr int32_t MENU_ID_RIGHT_HORIZONTAL_SPLIT = -3;
		static constexpr int32_t MENU_ID_BOTTOM_VERTICAL_SPLIT = -4;
		static constexpr int32_t MENU_ID_TOP_VERTICAL_SPLIT = -5;
		static constexpr int32_t MENU_ID_FOCUS_MODE = -6;
		static constexpr int32_t MENU_ID_DUPLICATE_TO_WINDOW = -7;
		static constexpr int32_t MENU_ID_CLOSE = -8;

		LTEComponentRegistry* component_registry = nullptr;
		LTEWorkspaceManager* workspace_manager = nullptr;
		int32_t last_selected = -1;

		struct MenuItem {
			String uid_path;
			String text;
			int32_t id;
		};

		std::vector<MenuItem> items = {
			{"uid://co133xhwlhuo6", "LEFT_HORIZONTAL_SPLIT_NAME", MENU_ID_LEFT_HORIZONTAL_SPLIT},
			{"uid://cp3e1wrpx88ve", "RIGHT_HORIZONTAL_SPLIT_NAME", MENU_ID_RIGHT_HORIZONTAL_SPLIT},
			{"uid://cay6lq8rbmx0g", "BOTTOM_VERTICAL_SPLIT_NAME", MENU_ID_BOTTOM_VERTICAL_SPLIT},
			{"uid://b84nbq1ya3o0g", "TOP_VERTICAL_SPLIT_NAME", MENU_ID_TOP_VERTICAL_SPLIT},
			{"uid://cmqwkh03x1lsb", "FOCUS_MODE_NAME", MENU_ID_FOCUS_MODE},
			{"uid://dfwqi2o3ls5fg", "DUPLICATE_PANEL_TO_SEPARATE_WINDOW_NAME", MENU_ID_DUPLICATE_TO_WINDOW},
			{"uid://su61hdgslal0", "CLOSE_WINDOW_NAME", MENU_ID_CLOSE}
		};

		void _on_component_panel_registry_changed(const String& panel_uuid);
		void _on_workspace_focus_mode_changed();
		void populate_items();
		void _append_panel_item(const Dictionary& panel_info, int32_t panel_id);
		void _refresh_menu_state();
		int32_t _get_popup_item_index_by_id(int32_t item_id) const;
		Ref<Texture2D> _try_load_icon(const String& p_path);
		void _on_item_selected(int32_t index);
		void _on_add_panel_menu_callback(const Dictionary& panel_info, int32_t id);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		PanelOptionButton();
		~PanelOptionButton();

		String current_panel_uuid;
		String current_runtime_uuid;
		
		void set_current_panel_uuid(const String& p_uuid);
		String get_current_panel_uuid() const;

		void set_current_runtime_uuid(const String& p_uuid);
		String get_current_runtime_uuid() const;
	};
}

#endif // !PANEL_OPTION_BUTTON_H
