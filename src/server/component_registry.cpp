#include "component_registry.h"

namespace godot {
	LTEComponentRegistry* LTEComponentRegistry::singleton = nullptr;

	void LTEComponentRegistry::_bind_methods() {
		ClassDB::bind_method(D_METHOD("get_component_panel_scene", "uuid"), &LTEComponentRegistry::get_component_panel_scene);
		ClassDB::bind_method(D_METHOD("get_registered_panel_components"), &LTEComponentRegistry::get_registered_panel_components);
		ClassDB::bind_method(D_METHOD("get_registered_panel_components_by_space_type"), &LTEComponentRegistry::get_registered_panel_components_by_space_type);
		ClassDB::bind_method(D_METHOD("get_registered_panel_component_info", "uuid"), &LTEComponentRegistry::get_registered_panel_component_info);
		ClassDB::bind_method(D_METHOD("register_component_panel", "panel_uuid", "icon_path", "panel_name", "scene_path", "space_type", "owner_plugin_id"), &LTEComponentRegistry::register_component_panel, DEFVAL(String("")), DEFVAL(String("")));
		ClassDB::bind_method(D_METHOD("unregister_component_panel", "panel_uuid"), &LTEComponentRegistry::unregister_component_panel);
		ClassDB::bind_method(D_METHOD("has_component_panel", "panel_uuid"), &LTEComponentRegistry::has_component_panel);
		ClassDB::bind_method(D_METHOD("get_panel_owner", "panel_uuid"), &LTEComponentRegistry::get_panel_owner);
		ClassDB::bind_method(D_METHOD("get_registered_editor_menus"), &LTEComponentRegistry::get_registered_editor_menus);
		ClassDB::bind_method(D_METHOD("get_registered_editor_menu_info", "menu_id"), &LTEComponentRegistry::get_registered_editor_menu_info);
		ClassDB::bind_method(D_METHOD("register_editor_menu", "menu_id", "menu_name", "options"), &LTEComponentRegistry::register_editor_menu, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("unregister_editor_menu", "menu_id"), &LTEComponentRegistry::unregister_editor_menu);
		ClassDB::bind_method(D_METHOD("get_registered_editor_menu_items", "menu_id"), &LTEComponentRegistry::get_registered_editor_menu_items);
		ClassDB::bind_method(D_METHOD("get_registered_editor_menu_item_info", "menu_id", "item_id"), &LTEComponentRegistry::get_registered_editor_menu_item_info);
		ClassDB::bind_method(D_METHOD("register_editor_menu_item", "menu_id", "item_id", "item_name", "icon_path", "options"), &LTEComponentRegistry::register_editor_menu_item, DEFVAL(String()), DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("unregister_editor_menu_item", "menu_id", "item_id"), &LTEComponentRegistry::unregister_editor_menu_item);
		ClassDB::bind_method(D_METHOD("get_registered_timeline_note_modifiers"), &LTEComponentRegistry::get_registered_timeline_note_modifiers);
		ClassDB::bind_method(D_METHOD("get_timeline_note_modifier_info", "modifier_id"), &LTEComponentRegistry::get_timeline_note_modifier_info);
		ClassDB::bind_method(D_METHOD("register_timeline_note_modifier", "modifier_id", "modifier_name", "scene_path", "icon_path", "options"), &LTEComponentRegistry::register_timeline_note_modifier, DEFVAL(String()), DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("unregister_timeline_note_modifier", "modifier_id"), &LTEComponentRegistry::unregister_timeline_note_modifier);
		ClassDB::bind_method(D_METHOD("get_registered_composition_modifiers"), &LTEComponentRegistry::get_registered_composition_modifiers);
		ClassDB::bind_method(D_METHOD("get_composition_modifier_info", "modifier_id"), &LTEComponentRegistry::get_composition_modifier_info);
		ClassDB::bind_method(D_METHOD("register_composition_modifier", "modifier_id", "modifier_name", "scene_path", "icon_path", "options"), &LTEComponentRegistry::register_composition_modifier, DEFVAL(String()), DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("unregister_composition_modifier", "modifier_id"), &LTEComponentRegistry::unregister_composition_modifier);
		ClassDB::bind_method(D_METHOD("get_registered_interpolations"), &LTEComponentRegistry::get_registered_interpolations);
		ClassDB::bind_method(D_METHOD("get_interpolation_info", "interpolation_id"), &LTEComponentRegistry::get_interpolation_info);
		ClassDB::bind_method(D_METHOD("register_interpolation", "interpolation_id", "interpolation_name", "options", "curve_type"), &LTEComponentRegistry::register_interpolation, DEFVAL(Array()), DEFVAL(String("linear")));
		ClassDB::bind_method(D_METHOD("unregister_interpolation", "interpolation_id"), &LTEComponentRegistry::unregister_interpolation);
		ClassDB::bind_method(D_METHOD("get_registered_speed_interpolations"), &LTEComponentRegistry::get_registered_speed_interpolations);
		ClassDB::bind_method(D_METHOD("get_speed_interpolation_info", "interpolation_id"), &LTEComponentRegistry::get_speed_interpolation_info);
		ClassDB::bind_method(D_METHOD("register_speed_interpolation", "interpolation_id", "interpolation_name", "options", "curve_type"), &LTEComponentRegistry::register_speed_interpolation, DEFVAL(Array()), DEFVAL(String("linear")));
		ClassDB::bind_method(D_METHOD("unregister_speed_interpolation", "interpolation_id"), &LTEComponentRegistry::unregister_speed_interpolation);
		
		ADD_SIGNAL(MethodInfo("component_panel_registered", PropertyInfo(Variant::STRING, "panel_uuid")));
		ADD_SIGNAL(MethodInfo("component_panel_unregistered", PropertyInfo(Variant::STRING, "panel_uuid")));
		ADD_SIGNAL(MethodInfo("editor_menu_registered", PropertyInfo(Variant::STRING, "menu_id")));
		ADD_SIGNAL(MethodInfo("editor_menu_unregistered", PropertyInfo(Variant::STRING, "menu_id")));
		ADD_SIGNAL(MethodInfo("editor_menu_item_registered", PropertyInfo(Variant::STRING, "menu_id"), PropertyInfo(Variant::INT, "item_id")));
		ADD_SIGNAL(MethodInfo("editor_menu_item_unregistered", PropertyInfo(Variant::STRING, "menu_id"), PropertyInfo(Variant::INT, "item_id")));
		ADD_SIGNAL(MethodInfo("editor_menu_item_pressed", PropertyInfo(Variant::STRING, "menu_id"), PropertyInfo(Variant::INT, "item_id"), PropertyInfo(Variant::DICTIONARY, "item_info")));
		ADD_SIGNAL(MethodInfo("timeline_note_modifier_registered", PropertyInfo(Variant::STRING, "modifier_id")));
		ADD_SIGNAL(MethodInfo("timeline_note_modifier_unregistered", PropertyInfo(Variant::STRING, "modifier_id")));
		ADD_SIGNAL(MethodInfo("composition_modifier_registered", PropertyInfo(Variant::STRING, "modifier_id")));
		ADD_SIGNAL(MethodInfo("composition_modifier_unregistered", PropertyInfo(Variant::STRING, "modifier_id")));
		ADD_SIGNAL(MethodInfo("interpolation_registered", PropertyInfo(Variant::STRING, "interpolation_id")));
		ADD_SIGNAL(MethodInfo("interpolation_unregistered", PropertyInfo(Variant::STRING, "interpolation_id")));
		ADD_SIGNAL(MethodInfo("speed_interpolation_registered", PropertyInfo(Variant::STRING, "interpolation_id")));
		ADD_SIGNAL(MethodInfo("speed_interpolation_unregistered", PropertyInfo(Variant::STRING, "interpolation_id")));
	}

	LTEComponentRegistry::LTEComponentRegistry() {
		if (singleton == nullptr) {
			singleton = this;
		}
		register_component_panel(
			"0198e50b-c5be-74e9-a19e-3fb78c3b035a",
			"uid://dgchkaoieqq07",
			"TIMELINE_PANEL_NAME",
			"uid://cfgu2jb4ufb55",
			"GENERAL_NAME"
		);
		register_component_panel(
			"019e0da6-3257-7a9f-9d3a-776dc3f575fe",
			"uid://byl07ah7u5lxo",
			"COMPOSITION_TIMELINE_PANEL_NAME",
			"uid://7vfk7tybcdkc",
			"GENERAL_NAME"
		);
		register_component_panel(
			"0198e50c-a9ae-766c-b28f-8e28f729ea03",
			"uid://87am8n87i0ag",
			"SOURCE_MONITOR_NAME",
			"uid://b8vpr00ybvvyv",
			"GENERAL_NAME"
		);
		register_component_panel(
			"0198e50c-ef88-70dd-813a-c1191de05f43",
			"uid://cnaj4bycsf4am",
			"SCENE_LAYERS_PANEL_NAME",
			"uid://0785rk67ajnh",
			"GENERAL_NAME"
		);
		register_component_panel(
			"019e24dd-d8d3-7945-bd27-e711d5c54c23",
			"uid://dm1ld531drulq",
			"NOTE_SKIN_DESIGNER_NAME",
			"uid://xee1s81umxlq",
			"GENERAL_NAME"
		);
		register_component_panel(
			"019a76fd-5439-7c3d-84eb-4fa99d9caa23",
			"uid://b4m5nhuf75k7d",
			"SHADER_EDITOR_NAME",
			"uid://dbhqe4dkasxe1",
			"SCRIPTING_NAME"
		);
		register_component_panel(
			"0198e50c-418c-7071-b784-91b01fa8246e",
			"uid://dsmuxqkkwf8jj",
			"FILE_SYSTEM_NAME",
			"uid://ccj4r6u6ftdjw",
			"DATA_NAME"
		);
		register_component_panel(
			"019b5e8b-6150-71c1-8e3e-1c896f6825c8",
			"uid://bqpv5q2eonk57",
			"PROPERTIES_PANEL_NAME",
			"uid://b5jusugaldoeg",
			"DATA_NAME"
		);
		register_component_panel(
			"019b8ed9-5387-795f-9762-a97e82e98034",
			"uid://b6swooeo05iup",
			"GRAPH_EDITOR_NAME",
			"uid://chn04hs7jcent",
			"DATA_NAME"
		);
		register_component_panel(
			"019f15b8-7c42-7a31-9b6c-2b3f8c4e51d0",
			"uid://c7j6lx0i8o6vo",
			"PATH_EDITOR_NAME",
			"uid://ctjn7bkoxxr2t",
			"DATA_NAME"
		);
		register_component_panel(
			"019e1c2a-7f03-7f68-9352-20f9f0041c06",
			"uid://cvn7s8bowxe2o",
			"AUDIO_VISUALIZER_NAME",
			"uid://dcohf1ake8qan",
			"GENERAL_NAME"
		);
		register_component_panel(
			"019bc075-969c-7748-a060-1e9aa7b1c15c",
			"uid://h31ib8dk4ux8",
			"PREFERENCES_NAME",
			"uid://cdqrrchg5e1p3",
			"DATA_NAME"
		);

		Dictionary files_menu_options;
		files_menu_options["order"] = 0;
		files_menu_options["node_name"] = "FilesMenuButton";
		register_editor_menu("files", "FILES_NAME", files_menu_options);

		Dictionary edit_menu_options;
		edit_menu_options["order"] = 1;
		edit_menu_options["node_name"] = "EditButton";
		register_editor_menu("edit", "EDIT_NAME", edit_menu_options);

		Dictionary windows_menu_options;
		windows_menu_options["order"] = 2;
		windows_menu_options["node_name"] = "WindowsMenuButton";
		register_editor_menu("windows", "WINDOWS_NAME", windows_menu_options);

		Dictionary plugin_menu_options;
		plugin_menu_options["order"] = 3;
		plugin_menu_options["node_name"] = "PluginMenuButton";
		register_editor_menu("plugin", "PLUGIN_NAME", plugin_menu_options);

		Dictionary help_menu_options;
		help_menu_options["order"] = 4;
		help_menu_options["node_name"] = "HelpMenuButton";
		register_editor_menu("help", "HELP_NAME", help_menu_options);

		Dictionary editor_handler_options;
		editor_handler_options["handled_by_editor"] = true;


		Dictionary open_user_data_folder_options;
		open_user_data_folder_options["handled_by_editor"] = true;
		open_user_data_folder_options["order"] = 2;

		Dictionary open_video_export_folder_options;
		open_video_export_folder_options["handled_by_editor"] = true;
		open_video_export_folder_options["order"] = 3;

		Dictionary export_project_options;
		export_project_options["handled_by_editor"] = true;
		export_project_options["order"] = 4;

		Dictionary video_export_options;
		video_export_options["handled_by_editor"] = true;
		video_export_options["order"] = 5;

		Dictionary backup_current_project_options;
		backup_current_project_options["handled_by_editor"] = true;
		backup_current_project_options["order"] = 6;

		Dictionary restore_from_backup_options;
		restore_from_backup_options["handled_by_editor"] = true;
		restore_from_backup_options["order"] = 7;

		register_editor_menu_item("files", 2, "OPEN_USER_DATA_FOLDER_NAME", "uid://dcwq3bvau3j4x", open_user_data_folder_options);
		register_editor_menu_item("files", 5, "OPEN_VIDEO_EXPORT_FOLDER_NAME", "uid://kehph71ddxp8", open_video_export_folder_options);
		register_editor_menu_item("files", 3, "EXPORT_CHART_NAME", "uid://cq52fkc2ma86k", export_project_options);
		register_editor_menu_item("files", 4, "VIDEO_EXPORT_NAME", "uid://cyusnjrpfq80w", video_export_options);
		register_editor_menu_item("files", 6, "BACKUP_CURRENT_PROJECT_NAME", "uid://bh031vfc3253b", backup_current_project_options);
		register_editor_menu_item("files", 7, "RESTORE_FROM_BACKUP_NAME", "uid://bh031vfc3253b", restore_from_backup_options);


		register_editor_menu_item("edit", 0, "UNDO_NAME", "uid://rh2nmwty5e4q", editor_handler_options);
		register_editor_menu_item("edit", 1, "REDO_NAME", "uid://dnbfahdjlycbg", editor_handler_options);

		Dictionary show_status_options;
		show_status_options["handled_by_editor"] = true;
		show_status_options["checkable"] = true;
		show_status_options["checked"] = true;
		register_editor_menu_item("windows", 0, "SHOW_STAUTS_BAR_NAME", "uid://c6spywd6arp5y", show_status_options);

		Dictionary fullscreen_options;
		fullscreen_options["handled_by_editor"] = true;
		fullscreen_options["checkable"] = true;
		register_editor_menu_item("windows", 1, "FULLSCREEN_MODE_NAME", "uid://cmqwkh03x1lsb", fullscreen_options);

		Dictionary windows_separator_options;
		windows_separator_options["separator"] = true;
		register_editor_menu_item("windows", 2, "", "", windows_separator_options);
		register_editor_menu_item("windows", 3, "EXPORT_CURRENT_LAYOUT_NAME", "uid://cq52fkc2ma86k", editor_handler_options);
		register_editor_menu_item("windows", 4, "LEFT_HORIZONTAL_SPLIT_NAME", "uid://co133xhwlhuo6", editor_handler_options);
		register_editor_menu_item("windows", 5, "RIGHT_HORIZONTAL_SPLIT_NAME", "uid://cp3e1wrpx88ve", editor_handler_options);
		register_editor_menu_item("windows", 6, "BOTTOM_VERTICAL_SPLIT_NAME", "uid://cay6lq8rbmx0g", editor_handler_options);
		register_editor_menu_item("windows", 7, "TOP_VERTICAL_SPLIT_NAME", "uid://b84nbq1ya3o0g", editor_handler_options);

		Dictionary plugin_manager_options;
		plugin_manager_options["scene"] = "uid://3jhqmtu4dd68";
		register_editor_menu_item("plugin", 0, "PLUGIN_MANAGER_NAME", "uid://ch10e8esaoha0", plugin_manager_options);

		Dictionary documents_options;
		documents_options["handled_by_editor"] = true;
		documents_options["order"] = 0;
		register_editor_menu_item("help", 0, "DOCUMENTS_NAME", "uid://celrpvlge4fuw", documents_options);

		Dictionary reset_onboarding_options;
		reset_onboarding_options["handled_by_editor"] = true;
		reset_onboarding_options["order"] = 1;
		register_editor_menu_item("help", 1, "RESET_ONBOARDING_NAME", "uid://c26ha45pdtntx", reset_onboarding_options);

		Dictionary uuid_builder_options;
		uuid_builder_options["scene"] = "uid://c1tmim5gb04q5";
		uuid_builder_options["order"] = 0;
		register_editor_menu_item("tools", 0, "UUID_BUILDER_NAME", "", uuid_builder_options);

		Dictionary bpm_measurer_options;
		bpm_measurer_options["scene"] = "uid://ce1woep0ct8i4";
		bpm_measurer_options["order"] = 1;
		register_editor_menu_item("tools", 1, "BPM_MEASURER_NAME", "", bpm_measurer_options);

		Dictionary array_modifier_options;
		array_modifier_options["order"] = 0;
		register_timeline_note_modifier("array", "ARRAY_NAME", "res://scenes/editor/timeline_panel/modifiers/array_modifier.tscn", "", array_modifier_options);

		Dictionary stair_modifier_options;
		stair_modifier_options["order"] = 1;
		register_timeline_note_modifier("stair", "STAIR_NAME", "res://scenes/editor/timeline_panel/modifiers/stair_modifier.tscn", "", stair_modifier_options);

		Dictionary spiral_modifier_options;
		spiral_modifier_options["order"] = 2;
		register_timeline_note_modifier("spiral", "SPIRAL_NAME", "res://scenes/editor/timeline_panel/modifiers/spiral_modifier.tscn", "", spiral_modifier_options);

		Dictionary random_modifier_options;
		random_modifier_options["order"] = 3;
		register_timeline_note_modifier("random", "RANDOM_NAME", "res://scenes/editor/timeline_panel/modifiers/random_modifier.tscn", "", random_modifier_options);

		Dictionary mirror_modifier_options;
		mirror_modifier_options["order"] = 4;
		register_timeline_note_modifier("mirror", "MIRROR_NAME", "res://scenes/editor/timeline_panel/modifiers/mirror_modifier.tscn", "", mirror_modifier_options);

		Dictionary reverse_tracks_modifier_options;
		reverse_tracks_modifier_options["order"] = 5;
		register_timeline_note_modifier("reverse_tracks", "REVERSE_TRACKS_NAME", "res://scenes/editor/timeline_panel/modifiers/reverse_tracks_modifier.tscn", "", reverse_tracks_modifier_options);

		Dictionary quantize_modifier_options;
		quantize_modifier_options["order"] = 6;
		register_timeline_note_modifier("quantize", "QUANTIZE_NAME", "res://scenes/editor/timeline_panel/modifiers/quantize_modifier.tscn", "", quantize_modifier_options);

		Dictionary random_perturb_modifier_options;
		random_perturb_modifier_options["order"] = 7;
		register_timeline_note_modifier("random_perturb", "RANDOM_PERTURB_NAME", "res://scenes/editor/timeline_panel/modifiers/random_perturb_modifier.tscn", "", random_perturb_modifier_options);

		Dictionary remap_track_modifier_options;
		remap_track_modifier_options["order"] = 8;
		register_timeline_note_modifier("remap_track", "REMAP_TRACK_NAME", "res://scenes/editor/timeline_panel/modifiers/remap_track_modifier.tscn", "", remap_track_modifier_options);

		Dictionary connect_modifier_options;
		connect_modifier_options["order"] = 9;
		register_timeline_note_modifier("connect", "CONNECT_NAME", "res://scenes/editor/timeline_panel/modifiers/connect_modifier.tscn", "", connect_modifier_options);

		Array position_modifier_properties;
		position_modifier_properties.append("position");
		Dictionary shake_composition_modifier_options;
		shake_composition_modifier_options["order"] = 0;
		shake_composition_modifier_options["graph"] = "TRANSFORM_GRAPH_NAME";
		shake_composition_modifier_options["properties"] = position_modifier_properties;
		register_composition_modifier("shake", "SHAKE_MODIFIER_NAME", "", "", shake_composition_modifier_options);

		Dictionary beat_pulse_composition_modifier_options;
		beat_pulse_composition_modifier_options["order"] = 1;
		beat_pulse_composition_modifier_options["graph"] = "RHYTHM_GRAPH_NAME";
		beat_pulse_composition_modifier_options["properties"] = position_modifier_properties;
		register_composition_modifier("beat_pulse", "BEAT_PULSE_MODIFIER_NAME", "", "", beat_pulse_composition_modifier_options);

		Dictionary sine_swing_composition_modifier_options;
		sine_swing_composition_modifier_options["order"] = 2;
		sine_swing_composition_modifier_options["graph"] = "TRANSFORM_GRAPH_NAME";
		sine_swing_composition_modifier_options["properties"] = position_modifier_properties;
		register_composition_modifier("sine_swing", "SINE_SWING_MODIFIER_NAME", "", "", sine_swing_composition_modifier_options);

		Dictionary circular_motion_composition_modifier_options;
		circular_motion_composition_modifier_options["order"] = 3;
		circular_motion_composition_modifier_options["graph"] = "TRANSFORM_GRAPH_NAME";
		circular_motion_composition_modifier_options["properties"] = position_modifier_properties;
		register_composition_modifier("circular_motion", "CIRCULAR_MOTION_MODIFIER_NAME", "", "", circular_motion_composition_modifier_options);

		Dictionary spring_impact_composition_modifier_options;
		spring_impact_composition_modifier_options["order"] = 4;
		spring_impact_composition_modifier_options["graph"] = "TRANSFORM_GRAPH_NAME";
		spring_impact_composition_modifier_options["properties"] = position_modifier_properties;
		register_composition_modifier("spring_impact", "SPRING_IMPACT_MODIFIER_NAME", "", "", spring_impact_composition_modifier_options);

		Dictionary noise_drift_composition_modifier_options;
		noise_drift_composition_modifier_options["order"] = 5;
		noise_drift_composition_modifier_options["graph"] = "TRANSFORM_GRAPH_NAME";
		noise_drift_composition_modifier_options["properties"] = position_modifier_properties;
		register_composition_modifier("noise_drift", "NOISE_DRIFT_MODIFIER_NAME", "", "", noise_drift_composition_modifier_options);

		Dictionary path_following_composition_modifier_options;
		path_following_composition_modifier_options["order"] = 6;
		path_following_composition_modifier_options["graph"] = "TRANSFORM_GRAPH_NAME";
		path_following_composition_modifier_options["properties"] = position_modifier_properties;
		register_composition_modifier("path_following", "PATH_FOLLOWING_MODIFIER_NAME", "", "", path_following_composition_modifier_options);

		_register_core_interpolations();
	}

	LTEComponentRegistry::~LTEComponentRegistry() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEComponentRegistry* LTEComponentRegistry::get_singleton() {
		return singleton;
	}

	Ref<PackedScene> LTEComponentRegistry::get_component_panel_scene(const String& uuid) {
		Dictionary component = get_registered_panel_component_info(uuid);
		if (component.is_empty()) {
			ERR_PRINT(vformat("Component not found for uuid: %s", uuid));
			return nullptr;
		}
		String scene_uid = component.get("scene", String());
		if (scene_uid.is_empty()) {
			ERR_PRINT(vformat("Empty scene uid for uuid: %s", uuid));
			return nullptr;
		}
		String scene_path = ResourceUID::get_singleton()->uid_to_path(scene_uid);
		if (scene_path.is_empty()) {
			ERR_PRINT(vformat("Failed to resolve uid to path: %s", scene_uid));
			return nullptr;
		}
		Ref<Resource> res = ResourceLoader::get_singleton()->load(scene_path);
		if (res.is_null()) {
			ERR_PRINT(vformat("Failed to load resource: %s", scene_path));
			return nullptr;
		}
		Ref<PackedScene> packed_scene = res;
		if (packed_scene.is_null()) {
			ERR_PRINT(vformat("Resource is not a PackedScene: %s", scene_path));
			return nullptr;
		}
		return packed_scene;
	}

	Array LTEComponentRegistry::get_registered_panel_components() {
		Array components = editor_registry.get("panel", Array());
		return components;
	}

	Dictionary LTEComponentRegistry::get_registered_panel_components_by_space_type() {
		Array components = get_registered_panel_components();
		Dictionary components_by_space;
		for (int32_t index = 0; index < components.size(); index++) {
			Dictionary component = components[index];
			String space_type = component.get("space_type", "");
			Array space_components = components_by_space.get(space_type, Array());
			space_components.append(component);
			components_by_space[space_type] = space_components;
		}
		return components_by_space;
	}

	Dictionary LTEComponentRegistry::get_registered_panel_component_info(const String& uuid) {
		Array components = get_registered_panel_components();
		for (int32_t index = 0; index < components.size(); index++) {
			Dictionary component = components[index];
			String component_uuid = component.get("uuid", "");
			if (component_uuid == uuid) return component;
		}
		return Dictionary();
	}

	void LTEComponentRegistry::register_component_panel(const String& panel_uuid, const String& icon_path, const String& panel_name, const String& scene_path, const String& space_type, const String& owner_plugin_id) {
		Dictionary panel_info;
		panel_info["uuid"] = panel_uuid;
		panel_info["icon"] = icon_path;
		panel_info["name"] = panel_name;
		panel_info["scene"] = scene_path;
		panel_info["space_type"] = space_type;
		panel_info["owner_plugin_id"] = owner_plugin_id;
		Array panels = editor_registry.get("panel", Array());
		panels.append(panel_info);
		editor_registry["panel"] = panels;
		emit_signal("component_panel_registered", panel_uuid);
	}

	bool LTEComponentRegistry::has_component_panel(const String& panel_uuid) {
		Array panels = editor_registry.get("panel", Array());
		for (int32_t index = 0; index < panels.size(); index++) {
			Dictionary panel_info = panels[index];
			if (String(panel_info.get("uuid", "")) == panel_uuid) {
				return true;
			}
		}
		return false;
	}

	String LTEComponentRegistry::get_panel_owner(const String& panel_uuid) {
		Array panels = editor_registry.get("panel", Array());
		for (int32_t index = 0; index < panels.size(); index++) {
			Dictionary panel_info = panels[index];
			if (String(panel_info.get("uuid", "")) == panel_uuid) {
				return panel_info.get("owner_plugin_id", "");
			}
		}
		return "";
	}

	void LTEComponentRegistry::unregister_component_panel(const String& panel_uuid) {
		Array panels = editor_registry.get("panel", Array());
		for (int32_t index = 0; index < panels.size(); index++) {
			Dictionary panel_info = panels[index];
			String uuid = panel_info.get("uuid", "");
			if (uuid == panel_uuid) {
				panels.remove_at(index);
				editor_registry["panel"] = panels;
				emit_signal("component_panel_unregistered", panel_uuid);
				break;
			}
		}
	}

	Array LTEComponentRegistry::get_registered_editor_menus() {
		Array menus = editor_registry.get("menu_button", Array());
		return menus;
	}

	Dictionary LTEComponentRegistry::get_registered_editor_menu_info(const String& menu_id) {
		Array menus = get_registered_editor_menus();
		for (int32_t index = 0; index < menus.size(); index++) {
			Dictionary menu_info = menus[index];
			if (String(menu_info.get("id", "")) == menu_id) {
				return menu_info;
			}
		}
		return Dictionary();
	}

	void LTEComponentRegistry::register_editor_menu(const String& menu_id, const String& menu_name, const Dictionary& options) {
		if (menu_id.is_empty()) {
			ERR_PRINT("Editor menu id cannot be empty.");
			return;
		}

		Dictionary menu_info = options.duplicate(true);
		menu_info["id"] = menu_id;
		menu_info["name"] = menu_name;

		Array menus = editor_registry.get("menu_button", Array());
		if (!menu_info.has("order")) {
			menu_info["order"] = menus.size();
		}
		for (int32_t index = 0; index < menus.size(); index++) {
			Dictionary existing_menu_info = menus[index];
			if (String(existing_menu_info.get("id", "")) == menu_id) {
				menus[index] = menu_info;
				editor_registry["menu_button"] = menus;
				emit_signal("editor_menu_registered", menu_id);
				return;
			}
		}

		menus.append(menu_info);
		editor_registry["menu_button"] = menus;
		emit_signal("editor_menu_registered", menu_id);
	}

	void LTEComponentRegistry::unregister_editor_menu(const String& menu_id) {
		Array menus = editor_registry.get("menu_button", Array());
		for (int32_t index = 0; index < menus.size(); index++) {
			Dictionary menu_info = menus[index];
			if (String(menu_info.get("id", "")) == menu_id) {
				menus.remove_at(index);
				editor_registry["menu_button"] = menus;
				emit_signal("editor_menu_unregistered", menu_id);
				return;
			}
		}
	}

	Array LTEComponentRegistry::get_registered_editor_menu_items(const String& menu_id) {
		Dictionary menus = editor_registry.get("menu", Dictionary());
		Array menu_items = menus.get(menu_id, Array());
		return menu_items;
	}

	Dictionary LTEComponentRegistry::get_registered_editor_menu_item_info(const String& menu_id, const int32_t item_id) {
		Array menu_items = get_registered_editor_menu_items(menu_id);
		for (int32_t index = 0; index < menu_items.size(); index++) {
			Dictionary item = menu_items[index];
			if (int32_t(item.get("id", -1)) == item_id) {
				return item;
			}
		}
		return Dictionary();
	}

	void LTEComponentRegistry::register_editor_menu_item(const String& menu_id, const int32_t item_id, const String& item_name, const String& icon_path, const Dictionary& options) {
		if (menu_id.is_empty()) {
			ERR_PRINT("Editor menu id cannot be empty.");
			return;
		}

		Dictionary item_info = options.duplicate(true);
		item_info["id"] = item_id;
		item_info["name"] = item_name;
		item_info["icon"] = icon_path;

		Dictionary menus = editor_registry.get("menu", Dictionary());
		Array menu_items = menus.get(menu_id, Array());
		if (!item_info.has("order")) {
			item_info["order"] = menu_items.size();
		}
		for (int32_t index = 0; index < menu_items.size(); index++) {
			Dictionary item = menu_items[index];
			if (int32_t(item.get("id", -1)) == item_id) {
				menu_items[index] = item_info;
				menus[menu_id] = menu_items;
				editor_registry["menu"] = menus;
				emit_signal("editor_menu_item_registered", menu_id, item_id);
				return;
			}
		}

		menu_items.append(item_info);
		menus[menu_id] = menu_items;
		editor_registry["menu"] = menus;
		emit_signal("editor_menu_item_registered", menu_id, item_id);
	}

	void LTEComponentRegistry::unregister_editor_menu_item(const String& menu_id, const int32_t item_id) {
		Dictionary menus = editor_registry.get("menu", Dictionary());
		Array menu_items = menus.get(menu_id, Array());
		for (int32_t index = 0; index < menu_items.size(); index++) {
			Dictionary item = menu_items[index];
			if (int32_t(item.get("id", -1)) == item_id) {
				menu_items.remove_at(index);
				menus[menu_id] = menu_items;
				editor_registry["menu"] = menus;
				emit_signal("editor_menu_item_unregistered", menu_id, item_id);
				return;
			}
		}
	}

	Array LTEComponentRegistry::get_registered_timeline_note_modifiers() {
		return editor_registry.get("timeline_note_modifier", Array());
	}

	Dictionary LTEComponentRegistry::get_timeline_note_modifier_info(const String& modifier_id) {
		Array modifiers = get_registered_timeline_note_modifiers();
		for (int32_t index = 0; index < modifiers.size(); index++) {
			Dictionary modifier = modifiers[index];
			if (String(modifier.get("id", "")) == modifier_id) {
				return modifier;
			}
		}
		return Dictionary();
	}

	void LTEComponentRegistry::register_timeline_note_modifier(const String& modifier_id, const String& modifier_name, const String& scene_path, const String& icon_path, const Dictionary& options) {
		if (modifier_id.is_empty()) {
			ERR_PRINT("Timeline note modifier id cannot be empty.");
			return;
		}
		if (scene_path.is_empty()) {
			ERR_PRINT("Timeline note modifier scene path cannot be empty.");
			return;
		}

		Dictionary modifier_info = options.duplicate(true);
		modifier_info["id"] = modifier_id;
		modifier_info["name"] = modifier_name;
		modifier_info["scene"] = scene_path;
		modifier_info["icon"] = icon_path;

		Array modifiers = editor_registry.get("timeline_note_modifier", Array());
		if (!modifier_info.has("order")) {
			modifier_info["order"] = modifiers.size();
		}
		for (int32_t index = 0; index < modifiers.size(); index++) {
			Dictionary existing_modifier = modifiers[index];
			if (String(existing_modifier.get("id", "")) == modifier_id) {
				modifiers[index] = modifier_info;
				editor_registry["timeline_note_modifier"] = modifiers;
				emit_signal("timeline_note_modifier_registered", modifier_id);
				return;
			}
		}

		modifiers.append(modifier_info);
		editor_registry["timeline_note_modifier"] = modifiers;
		emit_signal("timeline_note_modifier_registered", modifier_id);
	}

	void LTEComponentRegistry::unregister_timeline_note_modifier(const String& modifier_id) {
		Array modifiers = editor_registry.get("timeline_note_modifier", Array());
		for (int32_t index = 0; index < modifiers.size(); index++) {
			Dictionary modifier = modifiers[index];
			if (String(modifier.get("id", "")) == modifier_id) {
				modifiers.remove_at(index);
				editor_registry["timeline_note_modifier"] = modifiers;
				emit_signal("timeline_note_modifier_unregistered", modifier_id);
				return;
			}
		}
	}

	Array LTEComponentRegistry::get_registered_composition_modifiers() {
		return editor_registry.get("composition_modifier", Array());
	}

	Dictionary LTEComponentRegistry::get_composition_modifier_info(const String& modifier_id) {
		Array modifiers = get_registered_composition_modifiers();
		for (int32_t index = 0; index < modifiers.size(); index++) {
			Dictionary modifier = modifiers[index];
			if (String(modifier.get("id", "")) == modifier_id) {
				return modifier;
			}
		}
		return Dictionary();
	}

	void LTEComponentRegistry::register_composition_modifier(const String& modifier_id, const String& modifier_name, const String& scene_path, const String& icon_path, const Dictionary& options) {
		if (modifier_id.is_empty()) {
			ERR_PRINT("Composition modifier id cannot be empty.");
			return;
		}

		Dictionary modifier_info = options.duplicate(true);
		modifier_info["id"] = modifier_id;
		modifier_info["name"] = modifier_name;
		modifier_info["scene"] = scene_path;
		modifier_info["icon"] = icon_path;
		if (!modifier_info.has("graph")) {
			modifier_info["graph"] = "GENERAL_NAME";
		}
		if (modifier_info.get("properties", Variant()).get_type() != Variant::ARRAY) {
			Array properties;
			modifier_info["properties"] = properties;
		}

		Array modifiers = editor_registry.get("composition_modifier", Array());
		if (!modifier_info.has("order")) {
			modifier_info["order"] = modifiers.size();
		}
		for (int32_t index = 0; index < modifiers.size(); index++) {
			Dictionary existing_modifier = modifiers[index];
			if (String(existing_modifier.get("id", "")) == modifier_id) {
				modifiers[index] = modifier_info;
				editor_registry["composition_modifier"] = modifiers;
				emit_signal("composition_modifier_registered", modifier_id);
				return;
			}
		}

		modifiers.append(modifier_info);
		editor_registry["composition_modifier"] = modifiers;
		emit_signal("composition_modifier_registered", modifier_id);
	}

	void LTEComponentRegistry::unregister_composition_modifier(const String& modifier_id) {
		Array modifiers = editor_registry.get("composition_modifier", Array());
		for (int32_t index = 0; index < modifiers.size(); index++) {
			Dictionary modifier = modifiers[index];
			if (String(modifier.get("id", "")) == modifier_id) {
				modifiers.remove_at(index);
				editor_registry["composition_modifier"] = modifiers;
				emit_signal("composition_modifier_unregistered", modifier_id);
				return;
			}
		}
	}

	void LTEComponentRegistry::_register_core_interpolations() {
		Array empty_options;
		Array easing_options = _make_easing_option_schema();
		register_interpolation("constant", "CONSTANT_NAME", empty_options, "constant");
		register_interpolation("linear", "LINEAR_NAME", empty_options, "linear");
		register_interpolation("bezier", "BEZIER_NAME", empty_options, "bezier");
		register_interpolation("sinusoidal", "SINUSOIDAL_NAME", easing_options, "sinusoidal");
		register_interpolation("quadratic", "QUADRATIC_NAME", easing_options, "quadratic");
		register_interpolation("cubic", "CUBIC_NAME", easing_options, "cubic");
		register_interpolation("quartic", "QUARTIC_NAME", easing_options, "quartic");
		register_interpolation("quintic", "QUINTIC_NAME", easing_options, "quintic");
		register_interpolation("exponential", "EXPONENTIAL_NAME", easing_options, "exponential");
		register_interpolation("circular", "CIRCULAR_NAME", easing_options, "circular");
		register_interpolation("back", "BACK_NAME2", easing_options, "back");
		register_interpolation("bounce", "BOUNCE_NAME", easing_options, "bounce");
		register_interpolation("elastic", "ELASTIC_NAME", easing_options, "elastic");
	}

	Array LTEComponentRegistry::_make_easing_option_schema(const String& default_easing) const {
		Array easing_items;
		Dictionary ease_in_item;
		ease_in_item["id"] = "ease_in";
		ease_in_item["name"] = "EASE_IN_NAME";
		easing_items.append(ease_in_item);

		Dictionary ease_out_item;
		ease_out_item["id"] = "ease_out";
		ease_out_item["name"] = "EASE_OUT_NAME";
		easing_items.append(ease_out_item);

		Dictionary ease_in_out_item;
		ease_in_out_item["id"] = "ease_in_out";
		ease_in_out_item["name"] = "EASE_IN_OUT_NAME";
		easing_items.append(ease_in_out_item);

		Dictionary easing_option;
		easing_option["key"] = "easing";
		easing_option["name"] = "EASING_NAME";
		easing_option["type"] = "enum";
		easing_option["default"] = default_easing;
		easing_option["items"] = easing_items;

		Array options;
		options.append(easing_option);
		return options;
	}

	Array LTEComponentRegistry::get_registered_interpolations() {
		return editor_registry.get("interpolation", Array());
	}

	Dictionary LTEComponentRegistry::get_interpolation_info(const String& interpolation_id) {
		Array interpolations = get_registered_interpolations();
		for (int32_t index = 0; index < interpolations.size(); index++) {
			Dictionary interpolation = interpolations[index];
			if (String(interpolation.get("id", "")) == interpolation_id) {
				return interpolation;
			}
		}
		return Dictionary();
	}

	void LTEComponentRegistry::register_interpolation(const String& interpolation_id, const String& interpolation_name, const Array& options, const String& curve_type) {
		if (interpolation_id.is_empty()) {
			ERR_PRINT("Interpolation id cannot be empty.");
			return;
		}

		Dictionary interpolation_info;
		interpolation_info["id"] = interpolation_id;
		interpolation_info["name"] = interpolation_name;
		interpolation_info["options"] = options.duplicate(true);
		interpolation_info["curve_type"] = curve_type.is_empty() ? interpolation_id : curve_type;

		Array interpolations = editor_registry.get("interpolation", Array());
		for (int32_t index = 0; index < interpolations.size(); index++) {
			Dictionary interpolation = interpolations[index];
			if (String(interpolation.get("id", "")) == interpolation_id) {
				interpolations[index] = interpolation_info;
				editor_registry["interpolation"] = interpolations;
				emit_signal("interpolation_registered", interpolation_id);
				return;
			}
		}

		interpolations.append(interpolation_info);
		editor_registry["interpolation"] = interpolations;
		emit_signal("interpolation_registered", interpolation_id);
	}

	void LTEComponentRegistry::unregister_interpolation(const String& interpolation_id) {
		Array interpolations = editor_registry.get("interpolation", Array());
		for (int32_t index = 0; index < interpolations.size(); index++) {
			Dictionary interpolation_info = interpolations[index];
			if (String(interpolation_info.get("id", "")) == interpolation_id) {
				interpolations.remove_at(index);
				editor_registry["interpolation"] = interpolations;
				emit_signal("interpolation_unregistered", interpolation_id);
				return;
			}
		}
	}

	Array LTEComponentRegistry::get_registered_speed_interpolations() {
		return get_registered_interpolations();
	}

	Dictionary LTEComponentRegistry::get_speed_interpolation_info(const String& interpolation_id) {
		return get_interpolation_info(interpolation_id);
	}

	void LTEComponentRegistry::register_speed_interpolation(const String& interpolation_id, const String& interpolation_name, const Array& options, const String& curve_type) {
		register_interpolation(interpolation_id, interpolation_name, options, curve_type);
		emit_signal("speed_interpolation_registered", interpolation_id);
	}

	void LTEComponentRegistry::unregister_speed_interpolation(const String& interpolation_id) {
		unregister_interpolation(interpolation_id);
		emit_signal("speed_interpolation_unregistered", interpolation_id);
	}

} // namespace godot
