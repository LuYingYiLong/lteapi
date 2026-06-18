#ifndef LTE_CHART_NOTE_HELPER_H
#define LTE_CHART_NOTE_HELPER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
	class LTEChartNoteHelper : public Object {
		GDCLASS(LTEChartNoteHelper, Object)

	private:
		static LTEChartNoteHelper* singleton;

		bool _beat_in_range(double beat, double start_beat, double end_beat) const;
		String _format_identity_float(double value) const;
		Dictionary _make_item_counts(const Array& items, const Callable& identity_callable);
		Array _get_removed_items(const Array& source_items, const Array& target_items, const Callable& identity_callable);
		void _remove_note_from_array(Array& notes, const Dictionary& target_note);
		bool _sort_notes_comparator(const Dictionary& a, const Dictionary& b) const;

	protected:
		static void _bind_methods();

	public:
		LTEChartNoteHelper();
		~LTEChartNoteHelper();

		static LTEChartNoteHelper* get_singleton();

		Dictionary normalize_note(const Dictionary& note, const Dictionary& options = Dictionary());
		Array normalize_notes(const Array& notes, const Dictionary& options = Dictionary());
		Array sort_notes(const Array& notes);
		String make_note_identity(const Dictionary& note);
		bool notes_intersect(const Dictionary& a, const Dictionary& b);
		Array find_overlaps(const Array& notes, const Dictionary& options = Dictionary());
		Dictionary make_note_delta(const Array& old_notes, const Array& new_notes);
		Dictionary apply_note_delta(const Dictionary& chart, const Dictionary& delta);
		Array filter_notes_by_range(const Array& notes, double beat_start, double beat_end, const Array& tracks = Array());
		PackedFloat64Array beats_to_times(const PackedFloat64Array& beats, const Dictionary& chart_data);
		PackedFloat64Array times_to_beats(const PackedFloat64Array& times, const Dictionary& chart_data);
	};
}

#endif // LTE_CHART_NOTE_HELPER_H
