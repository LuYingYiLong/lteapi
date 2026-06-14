#include "lte_api.h"

#include "audio_visualizer_server.h"
#include "backup_server.h"
#include "component_registry.h"
#include "composition_server.h"
#include "helpers/composition_signature_helper.h"
#include "helpers/composition_patch_helper.h"
#include "file_system_server.h"
#include "file_save_server.h"
#include "graph_editor_server.h"
#include "hotkey_tip_server.h"
#include "note_skin_server.h"
#include "plugin_manager.h"
#include "preferences_manager.h"
#include "project_manager.h"
#include "properties_server.h"
#include "settings_config.h"
#include "shader_server.h"
#include "source_monitor_server.h"
#include "timeline_server.h"
#include "undo_redo_server.h"
#include "workspace_manager.h"
#include "lte_user.h"

#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTEAPI* LTEAPI::singleton = nullptr;

	void LTEAPI::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_editor_instance", "instance"), &LTEAPI::set_editor_instance);
		ClassDB::bind_method(D_METHOD("get_editor_instance"), &LTEAPI::get_editor_instance);
		ClassDB::bind_method(D_METHOD("get_editor_workspace_root"), &LTEAPI::get_editor_workspace_root);
		ClassDB::bind_method(D_METHOD("get_editor_focused_panel_root"), &LTEAPI::get_editor_focused_panel_root);
		ClassDB::bind_method(D_METHOD("get_editor_basic_workspace_files"), &LTEAPI::get_editor_basic_workspace_files);
		ClassDB::bind_method(D_METHOD("get_backup_server"), &LTEAPI::get_backup_server);
		ClassDB::bind_method(D_METHOD("clear_hotkeys"), &LTEAPI::clear_hotkeys);
		ClassDB::bind_method(D_METHOD("add_hotkey", "action", "tip"), &LTEAPI::add_hotkey);
	}

	LTEAPI* LTEAPI::get_singleton() {
		return singleton;
	}

	LTEAPI::LTEAPI() {
		ERR_FAIL_COND(singleton != nullptr);
		singleton = this;

		settings_config = memnew(LTESettingsConfig);
		component_registry = memnew(LTEComponentRegistry);
		preferences_manager = memnew(LTEPreferencesManager);
		project_manager = memnew(LTEProjectManager);
		workspace_manager = memnew(LTEWorkspaceManager);
		file_system_server = memnew(LTEFileSystemServer);
		file_save_server = memnew(LTEFileSaveServer);
		undo_redo = memnew(LTEUndoRedo);
		timeline_server = memnew(LTETimelineServer);
		graph_editor_server = memnew(LTEGraphEditorServer);
		source_monitor_server = memnew(LTESourceMonitorServer);
		properties_server = memnew(LTEPropertiesServer);
		composition_server = memnew(LTECompositionServer);
		composition_signature_helper = memnew(LTECompositionSignatureHelper);
		composition_patch_helper = memnew(LTECompositionPatchHelper);
		hotkey_tip_server = memnew(LTEHotkeyTipServer);
		shader_server = memnew(LTEShaderServer);
		audio_visualizer_server = memnew(LTEAudioVisualizerServer);
		note_skin_server = memnew(LTENoteSkinServer);
		backup_server = memnew(LTEBackupServer);
		user = memnew(LTEUser);
		plugin_manager = memnew(LTEPluginManager);
	}

	LTEAPI::~LTEAPI() {
		ERR_FAIL_COND(singleton != this);
		editor_instance = nullptr;

		if (plugin_manager) {
			memdelete(plugin_manager);
			plugin_manager = nullptr;
		}
		if (properties_server) {
			memdelete(properties_server);
			properties_server = nullptr;
		}
		if (composition_patch_helper) {
			memdelete(composition_patch_helper);
			composition_patch_helper = nullptr;
		}
		if (composition_signature_helper) {
			memdelete(composition_signature_helper);
			composition_signature_helper = nullptr;
		}
		if (composition_server) {
			memdelete(composition_server);
			composition_server = nullptr;
		}
		if (hotkey_tip_server) {
			memdelete(hotkey_tip_server);
			hotkey_tip_server = nullptr;
		}
		if (shader_server) {
			memdelete(shader_server);
			shader_server = nullptr;
		}
		if (audio_visualizer_server) {
			memdelete(audio_visualizer_server);
			audio_visualizer_server = nullptr;
		}
		if (note_skin_server) {
			memdelete(note_skin_server);
			note_skin_server = nullptr;
		}
		if (backup_server) {
			memdelete(backup_server);
			backup_server = nullptr;
		}
		if (user) {
			memdelete(user);
			user = nullptr;
		}
		if (source_monitor_server) {
			memdelete(source_monitor_server);
			source_monitor_server = nullptr;
		}
		if (graph_editor_server) {
			memdelete(graph_editor_server);
			graph_editor_server = nullptr;
		}
		if (timeline_server) {
			memdelete(timeline_server);
			timeline_server = nullptr;
		}
		if (undo_redo) {
			memdelete(undo_redo);
			undo_redo = nullptr;
		}
		if (file_system_server) {
			memdelete(file_system_server);
			file_system_server = nullptr;
		}
		if (file_save_server) {
			memdelete(file_save_server);
			file_save_server = nullptr;
		}
		if (workspace_manager) {
			memdelete(workspace_manager);
			workspace_manager = nullptr;
		}
		if (project_manager) {
			memdelete(project_manager);
			project_manager = nullptr;
		}
		if (preferences_manager) {
			memdelete(preferences_manager);
			preferences_manager = nullptr;
		}
		if (component_registry) {
			memdelete(component_registry);
			component_registry = nullptr;
		}
		if (settings_config) {
			memdelete(settings_config);
			settings_config = nullptr;
		}
		singleton = nullptr;
	}

	void LTEAPI::set_editor_instance(Node* instance) {
		editor_instance = instance;
	}

	Node* LTEAPI::get_editor_instance() const {
		return editor_instance;
	}

	Node* LTEAPI::get_editor_workspace_root() const {
		if (!editor_instance || !editor_instance->has_method("get_workspace_root")) {
			return nullptr;
		}
		return Object::cast_to<Node>(editor_instance->call("get_workspace_root"));
	}

	Node* LTEAPI::get_editor_focused_panel_root() const {
		if (!editor_instance || !editor_instance->has_method("get_focused_panel_root")) {
			return nullptr;
		}
		return Object::cast_to<Node>(editor_instance->call("get_focused_panel_root"));
	}

	PackedStringArray LTEAPI::get_editor_basic_workspace_files() const {
		if (!editor_instance || !editor_instance->has_method("get_basic_workspace_files")) {
			return PackedStringArray();
		}
		return editor_instance->call("get_basic_workspace_files");
	}

	void LTEAPI::clear_hotkeys() {
		if (!hotkey_tip_server) {
			return;
		}
		hotkey_tip_server->clear_hotkeys();
	}

	void LTEAPI::add_hotkey(const StringName& action, const String& tip) {
		if (!hotkey_tip_server) {
			return;
		}
		hotkey_tip_server->add_hotkey(action, tip);
	}

	LTESettingsConfig* LTEAPI::get_settings_config() const {
		return settings_config;
	}

	LTEComponentRegistry* LTEAPI::get_component_registry() const {
		return component_registry;
	}

	LTEPreferencesManager* LTEAPI::get_preferences_manager() const {
		return preferences_manager;
	}

	LTEPluginManager* LTEAPI::get_plugin_manager() const {
		return plugin_manager;
	}

	LTEProjectManager* LTEAPI::get_project_manager() const {
		return project_manager;
	}

	LTEWorkspaceManager* LTEAPI::get_workspace_manager() const {
		return workspace_manager;
	}

	LTEUndoRedo* LTEAPI::get_undo_redo() const {
		return undo_redo;
	}

	LTETimelineServer* LTEAPI::get_timeline_server() const {
		return timeline_server;
	}

	LTEGraphEditorServer* LTEAPI::get_graph_editor_server() const {
		return graph_editor_server;
	}

	LTESourceMonitorServer* LTEAPI::get_source_monitor_server() const {
		return source_monitor_server;
	}

	LTEPropertiesServer* LTEAPI::get_properties_server() const {
		return properties_server;
	}

	LTEFileSystemServer* LTEAPI::get_file_system_server() const {
		return file_system_server;
	}

	LTEFileSaveServer* LTEAPI::get_file_save_server() const {
		return file_save_server;
	}

	LTECompositionServer* LTEAPI::get_composition_server() const {
		return composition_server;
	}

	LTEHotkeyTipServer* LTEAPI::get_hotkey_tip_server() const {
		return hotkey_tip_server;
	}

	LTEShaderServer* LTEAPI::get_shader_server() const {
		return shader_server;
	}

	LTEAudioVisualizerServer* LTEAPI::get_audio_visualizer_server() const {
		return audio_visualizer_server;
	}

	LTENoteSkinServer* LTEAPI::get_note_skin_server() const {
		return note_skin_server;
	}

	LTEBackupServer* LTEAPI::get_backup_server() const {
		return backup_server;
	}

	LTEUser* LTEAPI::get_user() const {
		return user;
	}

	LTECompositionSignatureHelper* LTEAPI::get_composition_signature_helper() const {
		return composition_signature_helper;
	}

	LTECompositionPatchHelper* LTEAPI::get_composition_patch_helper() const {
		return composition_patch_helper;
	}
}
