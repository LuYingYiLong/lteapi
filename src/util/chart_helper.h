#ifndef CHART_HELPER_H
#define CHART_HELPER_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot
{
	class ChartHelper : public RefCounted {
		GDCLASS(ChartHelper, RefCounted)
		
	protected:
		static void _bind_methods();

		Array beat_map;
		Array time_map;

		void _build_time_to_beat_map();
		void _build_beat_to_time_map();
		void _calculate_beat_total();
		void _calculate_row_total();

	public:
		ChartHelper();
		~ChartHelper();

		Array bpms;
		int per_bar = 1;
		double duration = 0.0;
		double beat_total = 0.0;
		double row_total = 0.0;

		static Ref<ChartHelper> open(const String& path);
		double time_to_row(const double time_sec);
		double row_to_time(const double row);
		double time_to_beat(const double time_sec);
		double beat_to_time(const double beat);
		double row_to_beat(const double row);
		double beat_to_row(const double beat);
		static double row_to_beat_absolute(const double row, const int per_bar);
		static double beat_to_row_absolute(const double beat, const int per_bar);

		void set_bpms(const Array& new_bpms);
		Array get_bpms() const;

		void set_per_bar(const int new_per_bar);
		int get_per_bar() const;

		void set_duration(const double new_duration);
		double get_duration() const;

		double get_beat_total() const;
		double get_row_total() const;
	};
}

#endif // !CHART_HELPER_H
