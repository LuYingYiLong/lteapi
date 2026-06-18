#ifndef LTE_API_H
#define LTE_API_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
	class LTESettingsConfig;
	class LTEComponentRegistry;
	class LTEPreferencesManager;
	class LTEPluginManager;
	class LTEWorkspaceManager;
	class LTETimelineServer;
	class LTEGraphEditorServer;
	class LTESourceMonitorServer;
	class LTEPropertiesServer;
	class LTEFileSystemServer;
	class LTEFileSaveServer;
	class LTEProjectManager;
	class LTEUndoRedo;
	class LTECompositionServer;
		class LTESceneLayersServer;
	class LTEHotkeyTipServer;
	class LTEShaderServer;
	class LTEAudioVisualizerServer;
	class LTENoteSkinServer;
	class LTEBackupServer;
	class LTEUser;
	class LTECompositionSignatureHelper;
	class LTECompositionPatchHelper;
	class LTEChartNoteHelper;
	class LTEChartAnalysisHelper;
	class LTESpeedCurveHelper;

	class LTEAPI : public Object {
		GDCLASS(LTEAPI, Object)

	private:
		static LTEAPI* singleton;

		Node* editor_instance = nullptr;

		LTESettingsConfig* settings_config = nullptr;
		LTEComponentRegistry* component_registry = nullptr;
		LTEPreferencesManager* preferences_manager = nullptr;
		LTEPluginManager* plugin_manager = nullptr;
		LTEWorkspaceManager* workspace_manager = nullptr;
		LTEUndoRedo* undo_redo = nullptr;
		LTETimelineServer* timeline_server = nullptr;
		LTEGraphEditorServer* graph_editor_server = nullptr;
		LTESourceMonitorServer* source_monitor_server = nullptr;
		LTEPropertiesServer* properties_server = nullptr;
		LTEFileSystemServer* file_system_server = nullptr;
		LTEFileSaveServer* file_save_server = nullptr;
		LTEProjectManager* project_manager = nullptr;
		LTECompositionServer* composition_server = nullptr;
			LTESceneLayersServer* scene_layers_server = nullptr;
		LTEHotkeyTipServer* hotkey_tip_server = nullptr;
		LTEShaderServer* shader_server = nullptr;
		LTEAudioVisualizerServer* audio_visualizer_server = nullptr;
		LTENoteSkinServer* note_skin_server = nullptr;
		LTEBackupServer* backup_server = nullptr;
		LTEUser* user = nullptr;
		LTECompositionSignatureHelper* composition_signature_helper = nullptr;
		LTECompositionPatchHelper* composition_patch_helper = nullptr;
		LTEChartNoteHelper* chart_note_helper = nullptr;
		LTEChartAnalysisHelper* chart_analysis_helper = nullptr;
		LTESpeedCurveHelper* speed_curve_helper = nullptr;

	protected:
		static void _bind_methods();

	public:
		LTEAPI();
		~LTEAPI();

		static LTEAPI* get_singleton();

		void set_editor_instance(Node* instance);
		Node* get_editor_instance() const;
		Node* get_editor_workspace_root() const;
		Node* get_editor_focused_panel_root() const;
		PackedStringArray get_editor_basic_workspace_files() const;
		void clear_hotkeys();
		void add_hotkey(const StringName& action, const String& tip);

		LTESettingsConfig* get_settings_config() const;
		LTEComponentRegistry* get_component_registry() const;
		LTEPreferencesManager* get_preferences_manager() const;
		LTEPluginManager* get_plugin_manager() const;
		LTEProjectManager* get_project_manager() const;
		LTEWorkspaceManager* get_workspace_manager() const;
		LTEUndoRedo* get_undo_redo() const;
		LTETimelineServer* get_timeline_server() const;
		LTEGraphEditorServer* get_graph_editor_server() const;
		LTESourceMonitorServer* get_source_monitor_server() const;
		LTEPropertiesServer* get_properties_server() const;
		LTEFileSystemServer* get_file_system_server() const;
		LTEFileSaveServer* get_file_save_server() const;
		LTECompositionServer* get_composition_server() const;
			LTESceneLayersServer* get_scene_layers_server() const;
		LTEHotkeyTipServer* get_hotkey_tip_server() const;
		LTEShaderServer* get_shader_server() const;
		LTEAudioVisualizerServer* get_audio_visualizer_server() const;
		LTENoteSkinServer* get_note_skin_server() const;
		LTEBackupServer* get_backup_server() const;
		LTEUser* get_user() const;
		LTECompositionSignatureHelper* get_composition_signature_helper() const;
		LTECompositionPatchHelper* get_composition_patch_helper() const;
		LTEChartNoteHelper* get_chart_note_helper() const;
		LTEChartAnalysisHelper* get_chart_analysis_helper() const;
		LTESpeedCurveHelper* get_speed_curve_helper() const;
	};
}

#endif
