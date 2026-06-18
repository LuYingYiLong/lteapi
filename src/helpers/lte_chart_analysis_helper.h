#ifndef LTE_CHART_ANALYSIS_HELPER_H
#define LTE_CHART_ANALYSIS_HELPER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
	class LTEChartAnalysisHelper : public Object {
		GDCLASS(LTEChartAnalysisHelper, Object)

	private:
		static LTEChartAnalysisHelper* singleton;

		double _get_note_start_beat(const Dictionary& note) const;
		double _get_note_end_beat(const Dictionary& note) const;
		void _add_diagnostic(Array& diagnostics, const String& level, const String& code, const String& message, int32_t index, double beat, int32_t key) const;
		bool _sort_density_by_nps(const Dictionary& a, const Dictionary& b) const;
		bool _sort_note_range_by_start(const Dictionary& a, const Dictionary& b) const;

	protected:
		static void _bind_methods();

	public:
		LTEChartAnalysisHelper();
		~LTEChartAnalysisHelper();

		static LTEChartAnalysisHelper* get_singleton();

		Dictionary analyze_chart(const Dictionary& chart, const Dictionary& options = Dictionary());
		Array diagnose_notes(const Array& notes, const Dictionary& options = Dictionary());
		Dictionary compute_density(const Array& notes, double duration, double window_sec = 1.0, int32_t max_rows = 12);
		Dictionary compute_track_distribution(const Array& notes, int32_t track_count = 8);
		Dictionary compute_note_group_stats(const Array& notes);
	};
}

#endif // LTE_CHART_ANALYSIS_HELPER_H
