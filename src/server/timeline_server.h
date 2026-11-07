#ifndef TIMELINE_SERVER_H
#define TIMELINE_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <cstdint>
#include <vector>

#include "chart_data.h"
#include "settings_config.h"
#include "utils.h"

namespace godot
{
	class LTETimelineServer : public Object {
		GDCLASS(LTETimelineServer, Object)

	private:
		static constexpr int32_t CHART_SAVE_COALESCE_DELAY_MS = 250;
		static constexpr double DEFAULT_AUTO_SAVE_INTERVAL_SECONDS = 0.6;

		static LTETimelineServer* singleton;

		struct ChartDocument {
			String absolute_path;
			Ref<ChartData> data;
		};

		struct OpenedChart {
			String uuid;
			String path;
			String path_key;
			Array column_config;
			Ref<ChartData> data;
			Ref<ChartHelper> helper;
		};

		LTESettingsConfig* settings_config = nullptr;
		HashMap<String, ChartDocument> chart_documents;
		std::vector<OpenedChart> opened_charts;
		bool submitting_chart_change = false;

		const OpenedChart* _record_chart_data(const String& uuid, const String& chart_path);
		const OpenedChart* _find_opened_chart(const String& uuid, const String& chart_path) const;
		Dictionary _make_chart_info(const OpenedChart& opened_chart) const;
		void _apply_chart_change(Dictionary p_opened_chart, Dictionary p_target_chart);
		void _apply_chart_snapshot(String chart_path, Ref<ChartData> data, Dictionary snapshot);
		bool _is_chart_auto_save_enabled() const;
		int32_t _get_auto_save_delay_msec() const;
		void _queue_chart_save(const String& chart_path, const Ref<ChartData>& data, const int32_t delay_msec = CHART_SAVE_COALESCE_DELAY_MS);
		bool _parse_chart_save_tag(const String& tag, String& r_chart_path, int64_t& r_document_id, int64_t& r_revision) const;
		Ref<ChartData> _find_saved_document(const String& absolute_path, int64_t document_id) const;
		void _on_file_saved(const String& path, const String& tag, const int64_t revision);
		void _on_file_save_failed(const String& path, const String& tag, const int64_t revision, const int64_t error);
		int _find_single_in_point(const String& chart_path);
		String _get_project_path() const;
		String _get_absolute_chart_path(const String& chart_path) const;
		Ref<ChartHelper> _get_chart_helper_for_path(const String& chart_path) const;
		String _resolve_chart_setting_key(const Dictionary& chart_dict, const String& chart_path) const;
		// 辅助排序函数
		bool _sort_in_out_compare(const Variant& a, const Variant& b) const;
		bool _sort_markers_compare(const Variant& a, const Variant& b) const;

	protected:
		static void _bind_methods();

	public:
		LTETimelineServer();
		~LTETimelineServer();

		static LTETimelineServer* get_singleton();

		void clear_project_state();
		Dictionary fetch_timeline_config(const String& uuid, const String& chart_path) const;
		void open_chart(const String& uuid, const String& chart_path);
		Array get_opened_chart_list();
		Ref<ChartData> get_chart_data(const String& chart_path) const;
		Ref<ChartHelper> get_chart_helper(const String& uuid, const String& chart_path);
		void enable_playhead_auto_scroll(const String& uuid, const bool enable);
		void enable_show_audio_waveform(const String& uuid, const bool enable);
		void enable_smooth_audio_waveform(const String& uuid, const bool enable);
		void enable_highlight_4k_tracks(const String& uuid, const bool enable);
		void set_chart_spacing(const String& uuid, const double spacing);
		void set_chart_scroll_position(const String& uuid, const String& chart_path, const Vector2i& position);
		void set_chart_collapsed_note_tracks(const String& uuid, const String& chart_path, const PackedInt32Array& tracks);
		String get_last_opened_chart(const String& uuid) const;
		void enable_chart_snap_mode(const String& uuid, const bool enable);
		void enable_chart_coverage_mode(const String& uuid, const bool enable);
		void set_per_bars(const Array& new_per_bars);
		void set_chart_column_config(const String& uuid, const String& chart_path, const Array& new_columns);
		Dictionary get_note_group_colors(const String& chart_path) const;
		void set_note_group_color(const String& chart_path, const String& group_name, const Color& color, const Color& default_color);
		void add_chart_marker(const String& chart_path, const String& name, const double time, const String& annotation, const Color& color);
		void update_chart_marker(const String& chart_path, const int32_t index, const String& name, const double time, const String& annotation, const Color& color);
		void remove_chart_marker(const String& chart_path, const int64_t index);
		void add_chart_in_point(const String& chart_path, double current_time, bool shift_pressed = false);
		void add_chart_out_point(const String& chart_path, double current_time);
		double go_to_chart_in_point(const String& chart_path, const double current_time, const bool preview = true);
		double go_to_chart_out_point(const String& chart_path, const double current_time, const bool preview = true);
		void remove_chart_in_out_point(const String& chart_path, const int64_t index);
		void set_chart_per_bar(const String& uuid, const String& chart_path, int per_bar);
		void set_playhead(const String& uuid, const String& chart_path, const double beat);
		String get_chart_audio_abs_path(const String& chart_path) const;
		double get_time_from_playhead(const String& uuid, const String& chart_path);
		double get_chart_duration(const String& uuid, const String& chart_path);
		Dictionary submit_chart_changes(const String& uuid, const String& chart_path, const Dictionary& new_chart);
		Dictionary save_chart(const String& uuid, const String& chart_path);
		void flush_pending_chart_saves();
		Dictionary find_opened_chart_info(const String& uuid, const String& chart_path) const;
		double time_step_back(const String& chart_path, const double time) const;
		double time_step_forward(const String& chart_path, const double time) const;
		int step_back(const String& uuid, const String& chart_path, const int current_row, const int per_bar);
		int step_forward(const String& uuid, const String& chart_path, const int current_row, const int per_bar);

	};
}

#endif // !TIMELINE_SERVER_H

