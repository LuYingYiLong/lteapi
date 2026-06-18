#ifndef SETTINGS_CONFIG_H
#define SETTINGS_CONFIG_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	class LTESettingsConfig : public Object {
		GDCLASS(LTESettingsConfig, Object)

	private:
		static LTESettingsConfig* singleton;

	protected:
		static void _bind_methods();

	public:
		LTESettingsConfig();
		~LTESettingsConfig();

		static LTESettingsConfig* get_singleton();
		static Array get_default_timeline_panel_per_bars();
		static Array normalize_timeline_panel_per_bars(const Array& source);

		Array workspace_tabs;
		String last_opened_workspace;
		Array floating_workspaces;
		Dictionary workspace_window_states;
		
		Dictionary timeline_panel_playhead_auto_scroll;
		Dictionary timeline_panel_show_audio_waveform;
		Dictionary timeline_panel_smooth_audio_waveform;
		Dictionary timeline_panel_highlight_4k_tracks;
		Dictionary timeline_panel_spacing;
		Dictionary timeline_panel_last_opened;
		Dictionary timeline_panel_snap_mode;
		Dictionary timeline_panel_coverage_mode;
		Array timeline_panel_per_bars;
		Dictionary timeline_panel_chart_column_configs;
		Dictionary timeline_panel_chart_markers;
		Dictionary timeline_panel_chart_in_out_points;
		Dictionary timeline_panel_chart_per_bar;
		Dictionary timeline_panel_chart_playhead;
		Dictionary timeline_panel_chart_scroll_position;
		Dictionary timeline_panel_note_group_colors;

		Dictionary composition_timeline_scene_settings;
		Dictionary composition_timeline_last_opened;

		Dictionary scene_layers_settings;

		Dictionary graph_editor_playhead_auto_scroll;
		Dictionary graph_editor_view_settings;

		Dictionary audio_visualizer_configs;
		Dictionary audio_preview_chart_settings;
		Dictionary note_skin_designer_configs;

		Dictionary source_monitor_last_opened;
		Dictionary source_monitor_chart_time;
		Dictionary source_monitor_view_judgement_line;
		Dictionary source_monitor_ratio;
		Dictionary source_monitor_show_clear_color;
		Dictionary source_monitor_auto_play;
		Dictionary source_monitor_show_debug_info;
		Dictionary source_monitor_smart_guides;
		Dictionary source_monitor_snap_settings;
		Dictionary source_monitor_screen;
		Dictionary source_monitor_window_mode;
		
		Dictionary file_system_panel_selected_file;
		Dictionary file_system_panel_collapsed_folders;
		Dictionary file_system_panel_view_state;
		Dictionary properties_panel_view_state;

		void load_settings_config();								// 加载配置
		void save_settings_config(const bool signal_emit = true);	// 保存配置
		void settings_config_garbage_collection();
		Dictionary get_properties_panel_view_state(const String& runtime_uuid) const;
		void save_properties_panel_view_state(const String& runtime_uuid, const Dictionary& view_state, const bool signal_emit = false);
	};
}

#endif // !SETTINGS_CONFIG_H
