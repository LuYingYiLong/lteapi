#include "register_types.h"

#include "lte_api.h"
#include "audio_visualizer_server.h"
#include "backup_server.h"
#include "classes/base_random.h"
#include "classes/crypto_random.h"
#include "classes/build_in_random.h"
#include "classes/preference_config_handle.h"
#include "classes/preference_config_schema.h"
#include "controls/auto_adjust_popup_menu.h"
#include "controls/editor_panel_container.h"
#include "controls/confirm_dialog.h"
#include "controls/inspector_view_base.h"
#include "controls/panel_option_button.h"
#include "controls/stauts_label.h"
#include "controls/time_scrubber.h"
#include "utils.h"
#include "chart_helper.h"
#include "zip_helper.h"
#include "settings_config.h"
#include "shader_server.h"
#include "component_registry.h"
#include "composition_server.h"
#include "scene_layers_server.h"
#include "helpers/composition_signature_helper.h"
#include "helpers/composition_patch_helper.h"
#include "helpers/lte_chart_note_helper.h"
#include "helpers/lte_chart_analysis_helper.h"
#include "helpers/lte_speed_curve_helper.h"
#include "preferences_manager.h"
#include "plugin_manager.h"
#include "project_manager.h"
#include "workspace_manager.h"
#include "timeline_server.h"
#include "graph_editor_server.h"
#include "hotkey_tip_server.h"
#include "note_skin_editor_plugin.h"
#include "note_skin_importer.h"
#include "note_skin_resource.h"
#include "note_skin_server.h"
#include "path_editor_server.h"
#include "inspector_registry.h"
#include "source_monitor_server.h"
#include "properties_server.h"
#include "file_system_server.h"
#include "file_save_server.h"
#include "undo_redo_server.h"
#include "lte_user.h"
#include "plugin_base.h"
#include "plugin_manifest.h"
#include "plugin_context.h"
#include "plugin_runtime.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static LTEAPI* lte_api_instance = nullptr;
static const char* DEFAULT_SKIN_PROJECT_SETTING = "lightech/note_skin/default_skin";
static const char* DEFAULT_SKIN_PATH = "res://datas/vanilla_skin.lteskin";
static bool note_skin_editor_plugin_registered = false;

static void add_lte_project_setting(const String& p_name, const Variant& p_default_value, const Variant::Type p_type, const PropertyHint p_hint = PROPERTY_HINT_NONE, const String& p_hint_string = String()) {
	ProjectSettings* settings = ProjectSettings::get_singleton();
	ERR_FAIL_COND(!settings);

	if (!settings->has_setting(p_name)) {
		settings->set_setting(p_name, p_default_value);
	}
	settings->set_initial_value(p_name, p_default_value);

	Dictionary property_info;
	property_info["name"] = p_name;
	property_info["type"] = p_type;
	if (p_hint != PROPERTY_HINT_NONE) {
		property_info["hint"] = p_hint;
		property_info["hint_string"] = p_hint_string;
	}
	settings->add_property_info(property_info);
}

void initialize_example_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_CLASS(LTENoteSkinImporter);
		GDREGISTER_CLASS(LTENoteSkinEditorPlugin);
		if (!note_skin_editor_plugin_registered) {
			EditorPlugins::add_by_type<LTENoteSkinEditorPlugin>();
			note_skin_editor_plugin_registered = true;
		}
		add_lte_project_setting(DEFAULT_SKIN_PROJECT_SETTING, String(DEFAULT_SKIN_PATH), Variant::STRING, PROPERTY_HINT_FILE, "*.lteskin");
		return;
	}

	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(BaseRandom);
	GDREGISTER_CLASS(CryptoRandom);
	GDREGISTER_CLASS(BuildInRandom);
	GDREGISTER_CLASS(LTEPreferenceConfigHandle);
	GDREGISTER_CLASS(LTEPreferenceConfigSchema);
	GDREGISTER_CLASS(Utils);
	GDREGISTER_CLASS(ChartHelper);
	GDREGISTER_CLASS(ZipHelper);
	GDREGISTER_CLASS(AutoAdjustPopupMenu);
	GDREGISTER_CLASS(EditorPanelContainer);
	GDREGISTER_CLASS(LTEConfirmDialog);
	GDREGISTER_CLASS(LTEInspectorViewBase);
	GDREGISTER_CLASS(PanelOptionButton);
	GDREGISTER_CLASS(StautsLabel);
	GDREGISTER_CLASS(TimeScrubber);
	GDREGISTER_CLASS(LTESettingsConfig);
	GDREGISTER_CLASS(LTEComponentRegistry);
	GDREGISTER_CLASS(LTEPreferencesManager);
	GDREGISTER_CLASS(LTEPluginManager);
	GDREGISTER_CLASS(LTEProjectManager);
	GDREGISTER_CLASS(LTEWorkspaceManager);
	GDREGISTER_CLASS(LTEUndoRedo);
	GDREGISTER_CLASS(LTETimelineServer);
	GDREGISTER_CLASS(LTEGraphEditorServer);
	GDREGISTER_CLASS(LTESourceMonitorServer);
	GDREGISTER_CLASS(LTEPropertiesServer);
	GDREGISTER_CLASS(LTEFileSystemServer);
	GDREGISTER_CLASS(LTEFileSaveServer);
	GDREGISTER_CLASS(LTECompositionServer);
	GDREGISTER_CLASS(LTESceneLayersServer);
	GDREGISTER_CLASS(LTECompositionSignatureHelper);
	GDREGISTER_CLASS(LTECompositionPatchHelper);
	GDREGISTER_CLASS(LTEChartNoteHelper);
	GDREGISTER_CLASS(LTEChartAnalysisHelper);
	GDREGISTER_CLASS(LTESpeedCurveHelper);
	GDREGISTER_CLASS(LTEHotkeyTipServer);
	GDREGISTER_CLASS(LTEShaderServer);
	GDREGISTER_CLASS(LTEAudioVisualizerServer);
	GDREGISTER_CLASS(LTENoteSkinResource);
	GDREGISTER_CLASS(LTENoteSkinServer);
	GDREGISTER_CLASS(LTEBackupServer);
	GDREGISTER_CLASS(LTEPathEditorServer);
	GDREGISTER_CLASS(LTEInspectorRegistry);
	GDREGISTER_CLASS(LTEUser);
	GDREGISTER_CLASS(LTEPluginBase);
	GDREGISTER_CLASS(LTEPluginManifest);
	GDREGISTER_CLASS(LTEPluginContext);
	GDREGISTER_CLASS(LTEPluginRuntime);

	GDREGISTER_CLASS(LTEAPI);
	lte_api_instance = memnew(LTEAPI);
	Engine::get_singleton()->register_singleton("LTEAPI", lte_api_instance);
	Engine::get_singleton()->register_singleton("LTESettingsConfig", lte_api_instance->get_settings_config());
	Engine::get_singleton()->register_singleton("LTEComponentRegistry", lte_api_instance->get_component_registry());
	Engine::get_singleton()->register_singleton("LTEPreferencesManager", lte_api_instance->get_preferences_manager());
	Engine::get_singleton()->register_singleton("LTEPluginManager", lte_api_instance->get_plugin_manager());
	Engine::get_singleton()->register_singleton("LTEWorkspaceManager", lte_api_instance->get_workspace_manager());
	Engine::get_singleton()->register_singleton("LTEUndoRedo", lte_api_instance->get_undo_redo());
	Engine::get_singleton()->register_singleton("LTETimelineServer", lte_api_instance->get_timeline_server());
	Engine::get_singleton()->register_singleton("LTEGraphEditorServer", lte_api_instance->get_graph_editor_server());
	Engine::get_singleton()->register_singleton("LTESourceMonitorServer", lte_api_instance->get_source_monitor_server());
	Engine::get_singleton()->register_singleton("LTEPropertiesServer", lte_api_instance->get_properties_server());
	Engine::get_singleton()->register_singleton("LTEFileSystemServer", lte_api_instance->get_file_system_server());
	Engine::get_singleton()->register_singleton("LTEFileSaveServer", lte_api_instance->get_file_save_server());
	Engine::get_singleton()->register_singleton("LTECompositionServer", lte_api_instance->get_composition_server());
	Engine::get_singleton()->register_singleton("LTESceneLayersServer", lte_api_instance->get_scene_layers_server());
	Engine::get_singleton()->register_singleton("LTECompositionSignatureHelper", lte_api_instance->get_composition_signature_helper());
	Engine::get_singleton()->register_singleton("LTECompositionPatchHelper", lte_api_instance->get_composition_patch_helper());
	Engine::get_singleton()->register_singleton("LTEChartNoteHelper", lte_api_instance->get_chart_note_helper());
	Engine::get_singleton()->register_singleton("LTEChartAnalysisHelper", lte_api_instance->get_chart_analysis_helper());
	Engine::get_singleton()->register_singleton("LTESpeedCurveHelper", lte_api_instance->get_speed_curve_helper());
	Engine::get_singleton()->register_singleton("LTEHotkeyTipServer", lte_api_instance->get_hotkey_tip_server());
	Engine::get_singleton()->register_singleton("LTEShaderServer", lte_api_instance->get_shader_server());
	Engine::get_singleton()->register_singleton("LTEAudioVisualizerServer", lte_api_instance->get_audio_visualizer_server());
	Engine::get_singleton()->register_singleton("LTENoteSkinServer", lte_api_instance->get_note_skin_server());
	Engine::get_singleton()->register_singleton("LTEBackupServer", lte_api_instance->get_backup_server());
	Engine::get_singleton()->register_singleton("LTEPathEditorServer", lte_api_instance->get_path_editor_server());
	Engine::get_singleton()->register_singleton("LTEInspectorRegistry", lte_api_instance->get_inspector_registry());
	Engine::get_singleton()->register_singleton("LTEUser", lte_api_instance->get_user());
	Engine::get_singleton()->register_singleton("LTEProjectManager", lte_api_instance->get_project_manager());
}

void uninitialize_example_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		if (note_skin_editor_plugin_registered) {
			EditorPlugins::remove_by_type<LTENoteSkinEditorPlugin>();
			note_skin_editor_plugin_registered = false;
		}
		return;
	}

	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (lte_api_instance) {
		// 先从 Engine 移除
		Engine::get_singleton()->unregister_singleton("LTEProjectManager");
		Engine::get_singleton()->unregister_singleton("LTEUser");
		Engine::get_singleton()->unregister_singleton("LTEBackupServer");
		Engine::get_singleton()->unregister_singleton("LTEPathEditorServer");
		Engine::get_singleton()->unregister_singleton("LTEInspectorRegistry");
		Engine::get_singleton()->unregister_singleton("LTENoteSkinServer");
		Engine::get_singleton()->unregister_singleton("LTEAudioVisualizerServer");
		Engine::get_singleton()->unregister_singleton("LTEShaderServer");
		Engine::get_singleton()->unregister_singleton("LTEHotkeyTipServer");
		Engine::get_singleton()->unregister_singleton("LTECompositionPatchHelper");
		Engine::get_singleton()->unregister_singleton("LTECompositionSignatureHelper");
		Engine::get_singleton()->unregister_singleton("LTEChartNoteHelper");
		Engine::get_singleton()->unregister_singleton("LTEChartAnalysisHelper");
		Engine::get_singleton()->unregister_singleton("LTESpeedCurveHelper");
				Engine::get_singleton()->unregister_singleton("LTESceneLayersServer");
		Engine::get_singleton()->unregister_singleton("LTECompositionServer");
		Engine::get_singleton()->unregister_singleton("LTEFileSaveServer");
		Engine::get_singleton()->unregister_singleton("LTEFileSystemServer");
		Engine::get_singleton()->unregister_singleton("LTEPropertiesServer");
		Engine::get_singleton()->unregister_singleton("LTESourceMonitorServer");
		Engine::get_singleton()->unregister_singleton("LTEGraphEditorServer");
		Engine::get_singleton()->unregister_singleton("LTETimelineServer");
		Engine::get_singleton()->unregister_singleton("LTEUndoRedo");
		Engine::get_singleton()->unregister_singleton("LTEWorkspaceManager");
		Engine::get_singleton()->unregister_singleton("LTEPluginManager");
		Engine::get_singleton()->unregister_singleton("LTEPreferencesManager");
		Engine::get_singleton()->unregister_singleton("LTEComponentRegistry");
		Engine::get_singleton()->unregister_singleton("LTESettingsConfig");
		Engine::get_singleton()->unregister_singleton("LTEAPI");
		// 再释放内存
		memdelete(lte_api_instance);
		lte_api_instance = nullptr;
	}
}

extern "C" {
	// Initialization.
	GDExtensionBool GDE_EXPORT example_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization) {
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_example_module);
		init_obj.register_terminator(uninitialize_example_module);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
