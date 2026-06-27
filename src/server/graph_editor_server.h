#ifndef GRAPH_EDITOR_SERVER_H
#define GRAPH_EDITOR_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {
	class LTEGraphEditorServer : public Object {
		GDCLASS(LTEGraphEditorServer, Object)

	private:
		static LTEGraphEditorServer* singleton;

		Dictionary active_chart_paths;

		String _get_absolute_chart_path(const String& chart_path) const;
		bool _is_chart_in_current_project(const String& chart_path) const;
		Dictionary _find_opened_chart_by_path(const String& chart_path) const;
		Dictionary _get_latest_opened_chart() const;
		Dictionary _make_empty_model(const String& uuid) const;
		Dictionary _make_model(const String& uuid, const Dictionary& opened_chart) const;
		Dictionary _normalize_speed(const Dictionary& speed) const;
		Array _normalize_speeds(const Array& speeds) const;

	protected:
		static void _bind_methods();

	public:
		LTEGraphEditorServer();
		~LTEGraphEditorServer();

		static LTEGraphEditorServer* get_singleton();

		void clear_project_state();
		Dictionary fetch_graph_editor_config(const String& uuid) const;
		void set_playhead_auto_scroll(const String& uuid, const bool enable);
		void set_split_ratio(const String& uuid, const double ratio);
		void set_curve_view(const String& uuid, const String& curve_id, const Vector2& scroll_offset, const Vector2& content_scale);
		void set_curve_tree_scroll(const String& uuid, const Vector2& scroll);
		double find_adjacent_keyframe_time(const Array& keyframe_times, const double current_time, const bool forward) const;
		Dictionary fetch_speed_graph_model(const String& uuid);
		Dictionary set_active_chart_path(const String& uuid, const String& chart_path);
		Dictionary submit_speed_curve(const String& uuid, const String& chart_path, const Array& speeds);
	};
}

#endif
