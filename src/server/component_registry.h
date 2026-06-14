#ifndef COMPONENT_REGISTRY_H
#define COMPONENT_REGISTRY_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "utils.h"

namespace godot {
	class LTEComponentRegistry : public Object {
		GDCLASS(LTEComponentRegistry, Object)

	private:
		static LTEComponentRegistry* singleton;

		Dictionary editor_registry;
		void _register_core_interpolations();
		Array _make_easing_option_schema(const String& default_easing = "ease_in_out") const;

	protected:
		static void _bind_methods();

	public:
		LTEComponentRegistry();
		~LTEComponentRegistry();

		static LTEComponentRegistry* get_singleton();

		Ref<PackedScene> get_component_panel_scene(const String& uuid);
		Array get_registered_panel_components();
		Dictionary get_registered_panel_components_by_space_type();
		Dictionary get_registered_panel_component_info(const String& uuid);
		bool has_component_panel(const String& panel_uuid);
		String get_panel_owner(const String& panel_uuid);
		void register_component_panel(const String& panel_uuid, const String& icon_path, const String& panel_name, const String& scene_path, const String& space_type = "", const String& owner_plugin_id = "");
		void unregister_component_panel(const String& panel_uuid);
		Array get_registered_editor_menus();
		Dictionary get_registered_editor_menu_info(const String& menu_id);
		void register_editor_menu(const String& menu_id, const String& menu_name, const Dictionary& options = Dictionary());
		void unregister_editor_menu(const String& menu_id);
		Array get_registered_editor_menu_items(const String& menu_id);
		Dictionary get_registered_editor_menu_item_info(const String& menu_id, const int32_t item_id);
		void register_editor_menu_item(const String& menu_id, const int32_t item_id, const String& item_name, const String& icon_path = "", const Dictionary& options = Dictionary());
		void unregister_editor_menu_item(const String& menu_id, const int32_t item_id);
		Array get_registered_timeline_note_modifiers();
		Dictionary get_timeline_note_modifier_info(const String& modifier_id);
		void register_timeline_note_modifier(const String& modifier_id, const String& modifier_name, const String& scene_path, const String& icon_path = "", const Dictionary& options = Dictionary());
		void unregister_timeline_note_modifier(const String& modifier_id);
		Array get_registered_composition_modifiers();
		Dictionary get_composition_modifier_info(const String& modifier_id);
		void register_composition_modifier(const String& modifier_id, const String& modifier_name, const String& scene_path, const String& icon_path = "", const Dictionary& options = Dictionary());
		void unregister_composition_modifier(const String& modifier_id);
		Array get_registered_interpolations();
		Dictionary get_interpolation_info(const String& interpolation_id);
		void register_interpolation(const String& interpolation_id, const String& interpolation_name, const Array& options = Array(), const String& curve_type = "linear");
		void unregister_interpolation(const String& interpolation_id);
		Array get_registered_speed_interpolations();
		Dictionary get_speed_interpolation_info(const String& interpolation_id);
		void register_speed_interpolation(const String& interpolation_id, const String& interpolation_name, const Array& options = Array(), const String& curve_type = "linear");
		void unregister_speed_interpolation(const String& interpolation_id);
	};
}

#endif // !COMPONENT_REGISTRY_H
