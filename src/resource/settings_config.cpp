#include "settings_config.h"

#include "project_manager.h"
#include "workspace_manager.h"

#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	LTESettingsConfig* LTESettingsConfig::singleton = nullptr;

	namespace {
		const int32_t DEFAULT_TIMELINE_PANEL_PER_BARS[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 32 };

		void append_timeline_panel_per_bar(Array& target, const int32_t per_bar) {
			if (per_bar <= 0 || target.has(per_bar)) {
				return;
			}
			target.append(per_bar);
		}

		void append_timeline_panel_per_bar_variant(Array& target, const Variant& value) {
			switch (value.get_type()) {
				case Variant::INT:
					append_timeline_panel_per_bar(target, static_cast<int32_t>(static_cast<int64_t>(value)));
					break;
				case Variant::FLOAT:
					append_timeline_panel_per_bar(target, static_cast<int32_t>(static_cast<double>(value)));
					break;
				case Variant::STRING:
				case Variant::STRING_NAME:
					append_timeline_panel_per_bar(target, static_cast<int32_t>(String(value).to_int()));
					break;
				case Variant::ARRAY: {
					Array nested = value;
					for (int32_t index = 0; index < nested.size(); index++) {
						append_timeline_panel_per_bar_variant(target, nested[index]);
					}
				} break;
				case Variant::PACKED_INT32_ARRAY: {
					PackedInt32Array values = value;
					for (int32_t index = 0; index < values.size(); index++) {
						append_timeline_panel_per_bar(target, values[index]);
					}
				} break;
				case Variant::PACKED_INT64_ARRAY: {
					PackedInt64Array values = value;
					for (int32_t index = 0; index < values.size(); index++) {
						append_timeline_panel_per_bar(target, static_cast<int32_t>(values[index]));
					}
				} break;
				case Variant::PACKED_FLOAT32_ARRAY: {
					PackedFloat32Array values = value;
					for (int32_t index = 0; index < values.size(); index++) {
						append_timeline_panel_per_bar(target, static_cast<int32_t>(values[index]));
					}
				} break;
				case Variant::PACKED_FLOAT64_ARRAY: {
					PackedFloat64Array values = value;
					for (int32_t index = 0; index < values.size(); index++) {
						append_timeline_panel_per_bar(target, static_cast<int32_t>(values[index]));
					}
				} break;
				default:
					break;
			}
		}

		bool remove_unused_uuid_entries(Dictionary& dictionary, const PackedStringArray& uuids) {
			bool changed = false;
			Array keys = dictionary.keys();
			for (int32_t index = 0; index < keys.size(); index++) {
				String uuid = keys[index];
				if (!uuids.has(uuid)) {
					dictionary.erase(uuid);
					changed = true;
				}
			}
			return changed;
		}

		String get_normalized_project_root() {
			LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
			String root = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
			if (root.ends_with("/")) {
				root = root.substr(0, root.length() - 1);
			}
			return root;
		}

		String convert_project_path_string(const String& source, const String& project_root, const bool to_absolute) {
			String value = source.replace("\\", "/");
			if (project_root.is_empty() || value.is_empty()) {
				return value;
			}
			if (value.begins_with("uid://") || value.begins_with("res://") || value.begins_with("user://")) {
				return value;
			}

			if (to_absolute) {
				if (value.begins_with("/")) {
					return project_root + value;
				}
				value = value.replace("chart:/", "chart:" + project_root + "/");
				value = value.replace("scene:/", "scene:" + project_root + "/");
				value = value.replace("skin:/", "skin:" + project_root + "/");
				value = value.replace("audio:/", "audio:" + project_root + "/");
				value = value.replace("file:/", "file:" + project_root + "/");
				return value;
			}

			String project_root_lower = project_root.to_lower();
			String value_lower = value.to_lower();
			if (value_lower == project_root_lower) {
				return "/";
			}
			if (value_lower.begins_with(project_root_lower + "/")) {
				return value.substr(project_root.length());
			}

			String result = value;
			String result_lower = result.to_lower();
			int32_t position = result_lower.find(project_root_lower + "/");
			while (position >= 0) {
				result = result.substr(0, position) + result.substr(position + project_root.length());
				result_lower = result.to_lower();
				position = result_lower.find(project_root_lower + "/");
			}
			return result;
		}

		Variant convert_project_path_variant(const Variant& value, const String& project_root, const bool to_absolute);

		Variant convert_project_path_key(const Variant& key, const String& project_root, const bool to_absolute) {
			if (key.get_type() == Variant::STRING || key.get_type() == Variant::STRING_NAME) {
				return convert_project_path_string(String(key), project_root, to_absolute);
			}
			return key;
		}

		Variant convert_project_path_variant(const Variant& value, const String& project_root, const bool to_absolute) {
			switch (value.get_type()) {
				case Variant::DICTIONARY: {
					Dictionary source = value;
					Dictionary result;
					Array keys = source.keys();
					for (int32_t index = 0; index < keys.size(); index++) {
						Variant key = keys[index];
						Variant normalized_key = convert_project_path_key(key, project_root, to_absolute);
						result[normalized_key] = convert_project_path_variant(source[key], project_root, to_absolute);
					}
					return result;
				}
				case Variant::ARRAY: {
					Array source = value;
					Array result;
					for (int32_t index = 0; index < source.size(); index++) {
						result.append(convert_project_path_variant(source[index], project_root, to_absolute));
					}
					return result;
				}
				case Variant::PACKED_STRING_ARRAY: {
					PackedStringArray source = value;
					PackedStringArray result;
					for (int32_t index = 0; index < source.size(); index++) {
						result.append(convert_project_path_string(source[index], project_root, to_absolute));
					}
					return result;
				}
				case Variant::STRING:
				case Variant::STRING_NAME:
					return convert_project_path_string(String(value), project_root, to_absolute);
				default:
					return value;
			}
		}

		Dictionary convert_project_path_dictionary(const Dictionary& source, const bool to_absolute) {
			String project_root = get_normalized_project_root();
			if (project_root.is_empty()) {
				return source;
			}
			return convert_project_path_variant(source, project_root, to_absolute);
		}
	}

	void LTESettingsConfig::_bind_methods() {
		ClassDB::bind_method(D_METHOD("load_settings_config"), &LTESettingsConfig::load_settings_config);
		ClassDB::bind_method(D_METHOD("save_settings_config", "signal_emit"), &LTESettingsConfig::save_settings_config, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("settings_config_garbage_collection"), &LTESettingsConfig::settings_config_garbage_collection);
		ClassDB::bind_method(D_METHOD("get_properties_panel_view_state", "runtime_uuid"), &LTESettingsConfig::get_properties_panel_view_state);
		ClassDB::bind_method(D_METHOD("save_properties_panel_view_state", "runtime_uuid", "view_state", "signal_emit"), &LTESettingsConfig::save_properties_panel_view_state, DEFVAL(false));
		ADD_SIGNAL(MethodInfo("settings_config_loaded"));
		ADD_SIGNAL(MethodInfo("settings_config_updated"));
	}

	LTESettingsConfig::LTESettingsConfig() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTESettingsConfig::~LTESettingsConfig() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTESettingsConfig* LTESettingsConfig::get_singleton() {
		return singleton;
	}

	Array LTESettingsConfig::get_default_timeline_panel_per_bars() {
		Array result;
		for (int32_t per_bar : DEFAULT_TIMELINE_PANEL_PER_BARS) {
			result.append(per_bar);
		}
		return result;
	}

	Array LTESettingsConfig::normalize_timeline_panel_per_bars(const Array& source) {
		Array result;
		for (int32_t index = 0; index < source.size(); index++) {
			append_timeline_panel_per_bar_variant(result, source[index]);
		}
		if (result.is_empty()) {
			return get_default_timeline_panel_per_bars();
		}
		return result;
	}

	void LTESettingsConfig::load_settings_config() {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		if (!project_manager) return;
		String project_path = project_manager->get_project_path();
		if (project_path.is_empty()) return;
		Ref<ConfigFile> file;
		file.instantiate();
		Error error = file->load(project_path + "/.editor/project_settings.cfg");
		if (error == OK) {
			workspace_tabs = file->get_value("editor", "workspace_tabs", Array());
			last_opened_workspace = file->get_value("editor", "last_opened_workspace", String());
			floating_workspaces = file->get_value("editor", "floating_workspaces", Array());
			workspace_window_states = file->get_value("editor", "workspace_window_states", Dictionary());
			timeline_panel_playhead_auto_scroll = file->get_value("timeline_panel", "playhead_auto_scroll", Dictionary());
			timeline_panel_show_audio_waveform = file->get_value("timeline_panel", "show_audio_waveform", Dictionary());
			timeline_panel_smooth_audio_waveform = file->get_value("timeline_panel", "smooth_audio_waveform", Dictionary());
			timeline_panel_highlight_4k_tracks = file->get_value("timeline_panel", "highlight_4k_tracks", Dictionary());
			timeline_panel_spacing = file->get_value("timeline_panel", "spacing", Dictionary());
			timeline_panel_last_opened = file->get_value("timeline_panel", "last_opened", Dictionary());
			timeline_panel_chart_column_configs = file->get_value("timeline_panel", "chart_column_configs", Dictionary());
			timeline_panel_snap_mode = file->get_value("timeline_panel", "snap_mode", Dictionary());
			timeline_panel_coverage_mode = file->get_value("timeline_panel", "coverage_mode", Dictionary());
			timeline_panel_per_bars = normalize_timeline_panel_per_bars(file->get_value("timeline_panel", "per_bars", Array()));
			timeline_panel_chart_markers = file->get_value("timeline_panel", "chart_markers", Dictionary());
			timeline_panel_chart_in_out_points = file->get_value("timeline_panel", "chart_in_out_points", Dictionary());
			timeline_panel_chart_per_bar = file->get_value("timeline_panel", "chart_per_bar", Dictionary());
			timeline_panel_chart_playhead = file->get_value("timeline_panel", "chart_playhead", Dictionary());
			timeline_panel_chart_scroll_position = file->get_value("timeline_panel", "chart_scroll_position", Dictionary());
			timeline_panel_chart_collapsed_note_tracks = file->get_value("timeline_panel", "chart_collapsed_note_tracks", Dictionary());
			timeline_panel_note_group_colors = file->get_value("timeline_panel", "note_group_colors", Dictionary());
			composition_timeline_scene_settings = file->get_value("composition_timeline", "scene_settings", Dictionary());
			composition_timeline_last_opened = file->get_value("composition_timeline", "last_opened", Dictionary());
				scene_layers_settings = file->get_value("scene_layers_panel", "scene_settings", Dictionary());
			graph_editor_playhead_auto_scroll = file->get_value("graph_editor", "playhead_auto_scroll", Dictionary());
			graph_editor_view_settings = file->get_value("graph_editor", "view_settings", Dictionary());
			audio_visualizer_configs = file->get_value("audio_visualizer", "configs", Dictionary());
			audio_preview_chart_settings = file->get_value("audio_preview", "chart_settings", Dictionary());
			note_skin_designer_configs = file->get_value("note_skin_designer", "configs", Dictionary());
			source_monitor_last_opened = file->get_value("source_monitor", "last_opend", Dictionary());
			source_monitor_chart_time = file->get_value("source_monitor", "chart_time", Dictionary());
			source_monitor_view_judgement_line = file->get_value("source_monitor", "view_judgement_line", Dictionary());
			source_monitor_ratio = file->get_value("source_monitor", "ratio", Dictionary());
			source_monitor_show_clear_color = file->get_value("source_monitor", "show_clear_color", Dictionary());
			source_monitor_auto_play = file->get_value("source_monitor", "auto_play", Dictionary());
			source_monitor_show_debug_info = file->get_value("source_monitor", "show_debug_info", Dictionary());
			source_monitor_smart_guides = file->get_value("source_monitor", "smart_guides", Dictionary());
			source_monitor_snap_settings = file->get_value("source_monitor", "snap_settings", Dictionary());
			source_monitor_screen = file->get_value("source_monitor", "screen", Dictionary());
			source_monitor_window_mode = file->get_value("source_monitor", "window_mode", Dictionary());
			source_monitor_view_hdr_2d = file->get_value("source_monitor", "view_hdr_2d", Dictionary());
			source_monitor_game_hdr_2d = file->get_value("source_monitor", "game_hdr_2d", Dictionary());
			path_editor_view_configs = file->get_value("path_editor", "view_configs", Dictionary());
			file_system_panel_selected_file = file->get_value("file_system_panel", "selected_file", Dictionary());
			file_system_panel_collapsed_folders = file->get_value("file_system_panel", "collapsed_folders", Dictionary());
			file_system_panel_view_state = file->get_value("file_system_panel", "view_state", Dictionary());
			properties_panel_view_state = file->get_value("properties_panel", "view_state", Dictionary());

			timeline_panel_last_opened = convert_project_path_dictionary(timeline_panel_last_opened, true);
			timeline_panel_chart_column_configs = convert_project_path_dictionary(timeline_panel_chart_column_configs, true);
			timeline_panel_chart_markers = convert_project_path_dictionary(timeline_panel_chart_markers, true);
			timeline_panel_chart_in_out_points = convert_project_path_dictionary(timeline_panel_chart_in_out_points, true);
			timeline_panel_chart_per_bar = convert_project_path_dictionary(timeline_panel_chart_per_bar, true);
			timeline_panel_chart_playhead = convert_project_path_dictionary(timeline_panel_chart_playhead, true);
			timeline_panel_chart_scroll_position = convert_project_path_dictionary(timeline_panel_chart_scroll_position, true);
			timeline_panel_chart_collapsed_note_tracks = convert_project_path_dictionary(timeline_panel_chart_collapsed_note_tracks, true);
			timeline_panel_note_group_colors = convert_project_path_dictionary(timeline_panel_note_group_colors, true);
			composition_timeline_scene_settings = convert_project_path_dictionary(composition_timeline_scene_settings, true);
			composition_timeline_last_opened = convert_project_path_dictionary(composition_timeline_last_opened, true);
				scene_layers_settings = convert_project_path_dictionary(scene_layers_settings, true);
			graph_editor_view_settings = convert_project_path_dictionary(graph_editor_view_settings, true);
			audio_visualizer_configs = convert_project_path_dictionary(audio_visualizer_configs, true);
			audio_preview_chart_settings = convert_project_path_dictionary(audio_preview_chart_settings, true);
			note_skin_designer_configs = convert_project_path_dictionary(note_skin_designer_configs, true);
			source_monitor_last_opened = convert_project_path_dictionary(source_monitor_last_opened, true);
			source_monitor_chart_time = convert_project_path_dictionary(source_monitor_chart_time, true);
			file_system_panel_selected_file = convert_project_path_dictionary(file_system_panel_selected_file, true);
			file_system_panel_collapsed_folders = convert_project_path_dictionary(file_system_panel_collapsed_folders, true);
			file_system_panel_view_state = convert_project_path_dictionary(file_system_panel_view_state, true);

		}
		else {
			WARN_PRINT(vformat("Failed to load config: %s", error));
		}
		emit_signal("settings_config_loaded");
	}

	void LTESettingsConfig::save_settings_config(const bool signal_emit) {
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		if (!project_manager) return;
		String project_path = project_manager->get_project_path();
		if (project_path.is_empty()) return;
		Ref<ConfigFile> file;
		file.instantiate();
		file->set_value("editor", "workspace_tabs", workspace_tabs);
		file->set_value("editor", "last_opened_workspace", last_opened_workspace);
		file->set_value("editor", "floating_workspaces", floating_workspaces);
		file->set_value("editor", "workspace_window_states", workspace_window_states);
		file->set_value("timeline_panel", "playhead_auto_scroll", timeline_panel_playhead_auto_scroll);
		file->set_value("timeline_panel", "show_audio_waveform", timeline_panel_show_audio_waveform);
		file->set_value("timeline_panel", "smooth_audio_waveform", timeline_panel_smooth_audio_waveform);
		file->set_value("timeline_panel", "highlight_4k_tracks", timeline_panel_highlight_4k_tracks);
		file->set_value("timeline_panel", "spacing", timeline_panel_spacing);
		file->set_value("timeline_panel", "last_opened", convert_project_path_dictionary(timeline_panel_last_opened, false));
		file->set_value("timeline_panel", "chart_column_configs", convert_project_path_dictionary(timeline_panel_chart_column_configs, false));
		file->set_value("timeline_panel", "snap_mode", timeline_panel_snap_mode);
		file->set_value("timeline_panel", "coverage_mode", timeline_panel_coverage_mode);
		file->set_value("timeline_panel", "per_bars", timeline_panel_per_bars);
		file->set_value("timeline_panel", "chart_markers", convert_project_path_dictionary(timeline_panel_chart_markers, false));
		file->set_value("timeline_panel", "chart_in_out_points", convert_project_path_dictionary(timeline_panel_chart_in_out_points, false));
		file->set_value("timeline_panel", "chart_per_bar", convert_project_path_dictionary(timeline_panel_chart_per_bar, false));
		file->set_value("timeline_panel", "chart_playhead", convert_project_path_dictionary(timeline_panel_chart_playhead, false));
		file->set_value("timeline_panel", "chart_scroll_position", convert_project_path_dictionary(timeline_panel_chart_scroll_position, false));
		file->set_value("timeline_panel", "chart_collapsed_note_tracks", convert_project_path_dictionary(timeline_panel_chart_collapsed_note_tracks, false));
		file->set_value("timeline_panel", "note_group_colors", convert_project_path_dictionary(timeline_panel_note_group_colors, false));
		file->set_value("composition_timeline", "scene_settings", convert_project_path_dictionary(composition_timeline_scene_settings, false));
		file->set_value("composition_timeline", "last_opened", convert_project_path_dictionary(composition_timeline_last_opened, false));
				file->set_value("scene_layers_panel", "scene_settings", convert_project_path_dictionary(scene_layers_settings, false));
		file->set_value("graph_editor", "playhead_auto_scroll", graph_editor_playhead_auto_scroll);
		file->set_value("graph_editor", "view_settings", convert_project_path_dictionary(graph_editor_view_settings, false));
		file->set_value("audio_visualizer", "configs", convert_project_path_dictionary(audio_visualizer_configs, false));
		file->set_value("audio_preview", "chart_settings", convert_project_path_dictionary(audio_preview_chart_settings, false));
		file->set_value("note_skin_designer", "configs", convert_project_path_dictionary(note_skin_designer_configs, false));
		file->set_value("source_monitor", "last_opend", convert_project_path_dictionary(source_monitor_last_opened, false));
		file->set_value("source_monitor", "chart_time", convert_project_path_dictionary(source_monitor_chart_time, false));
		file->set_value("source_monitor", "view_judgement_line", source_monitor_view_judgement_line);
		file->set_value("source_monitor", "ratio", source_monitor_ratio);
		file->set_value("source_monitor", "show_clear_color", source_monitor_show_clear_color);
		file->set_value("source_monitor", "auto_play", source_monitor_auto_play);
		file->set_value("source_monitor", "show_debug_info", source_monitor_show_debug_info);
		file->set_value("source_monitor", "smart_guides", source_monitor_smart_guides);
		file->set_value("source_monitor", "snap_settings", source_monitor_snap_settings);
		file->set_value("source_monitor", "screen", source_monitor_screen);
		file->set_value("source_monitor", "window_mode", source_monitor_window_mode);
		file->set_value("source_monitor", "view_hdr_2d", source_monitor_view_hdr_2d);
		file->set_value("source_monitor", "game_hdr_2d", source_monitor_game_hdr_2d);
			file->set_value("path_editor", "view_configs", path_editor_view_configs);
		file->set_value("file_system_panel", "selected_file", convert_project_path_dictionary(file_system_panel_selected_file, false));
		file->set_value("file_system_panel", "collapsed_folders", convert_project_path_dictionary(file_system_panel_collapsed_folders, false));
		file->set_value("file_system_panel", "view_state", convert_project_path_dictionary(file_system_panel_view_state, false));
		file->set_value("properties_panel", "view_state", properties_panel_view_state);
		Error error = file->save(project_path + "/.editor/project_settings.cfg");
		if (error != OK) ERR_PRINT(vformat("Failed to save config: %s", error));
		if (signal_emit) emit_signal("settings_config_updated");
	}

	Dictionary LTESettingsConfig::get_properties_panel_view_state(const String& runtime_uuid) const {
		if (runtime_uuid.is_empty()) {
			return Dictionary();
		}
		Dictionary view_state = properties_panel_view_state.get(runtime_uuid, Dictionary());
		return view_state.duplicate(true);
	}

	void LTESettingsConfig::save_properties_panel_view_state(const String& runtime_uuid, const Dictionary& view_state, const bool signal_emit) {
		if (runtime_uuid.is_empty()) {
			return;
		}
		properties_panel_view_state[runtime_uuid] = view_state.duplicate(true);
		save_settings_config(signal_emit);
	}

	void LTESettingsConfig::settings_config_garbage_collection() {
		LTEWorkspaceManager* workspace_manager = LTEWorkspaceManager::get_singleton();
		if (!workspace_manager) return;
		PackedStringArray uuids = workspace_manager->get_all_layout_runtime_uuids();
		if (uuids.is_empty()) return;

		bool changed = false;
		changed = remove_unused_uuid_entries(timeline_panel_playhead_auto_scroll, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_show_audio_waveform, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_smooth_audio_waveform, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_highlight_4k_tracks, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_spacing, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_last_opened, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_chart_column_configs, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_snap_mode, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_coverage_mode, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_chart_per_bar, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_chart_playhead, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_chart_scroll_position, uuids) || changed;
		changed = remove_unused_uuid_entries(timeline_panel_chart_collapsed_note_tracks, uuids) || changed;
		changed = remove_unused_uuid_entries(composition_timeline_scene_settings, uuids) || changed;
		changed = remove_unused_uuid_entries(composition_timeline_last_opened, uuids) || changed;
				changed = remove_unused_uuid_entries(scene_layers_settings, uuids) || changed;
		changed = remove_unused_uuid_entries(graph_editor_playhead_auto_scroll, uuids) || changed;
		changed = remove_unused_uuid_entries(graph_editor_view_settings, uuids) || changed;
		changed = remove_unused_uuid_entries(audio_visualizer_configs, uuids) || changed;
		changed = remove_unused_uuid_entries(note_skin_designer_configs, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_last_opened, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_chart_time, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_view_judgement_line, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_ratio, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_show_clear_color, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_auto_play, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_show_debug_info, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_smart_guides, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_snap_settings, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_screen, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_window_mode, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_view_hdr_2d, uuids) || changed;
		changed = remove_unused_uuid_entries(source_monitor_game_hdr_2d, uuids) || changed;
			changed = remove_unused_uuid_entries(path_editor_view_configs, uuids) || changed;
		changed = remove_unused_uuid_entries(file_system_panel_selected_file, uuids) || changed;
		changed = remove_unused_uuid_entries(file_system_panel_collapsed_folders, uuids) || changed;
		changed = remove_unused_uuid_entries(file_system_panel_view_state, uuids) || changed;
		changed = remove_unused_uuid_entries(properties_panel_view_state, uuids) || changed;
		if (changed) {
			save_settings_config(false);
		}
	}
}
