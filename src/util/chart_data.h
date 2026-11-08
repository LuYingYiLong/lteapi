#ifndef CHART_DATA_H
#define CHART_DATA_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>

namespace godot {
	class ChartData : public RefCounted {
		GDCLASS(ChartData, RefCounted)

	private:
		struct TempoSegment {
			double beat;
			double time;
			double bpm;
		};

		Dictionary chart;
		int64_t revision = 0;
		int64_t saved_revision = 0;
		mutable std::vector<TempoSegment> tempo_segments;
		mutable double beat_total = 0.0;
		mutable double timing_duration = 0.0;
		mutable bool timing_dirty = true;

		void _ensure_timing() const;

	protected:
		static void _bind_methods();

	public:
		static Ref<ChartData> open(const String& path, bool validate_for_play = true);
		static Ref<ChartData> from_dictionary(const Dictionary& snapshot);
		static Ref<ChartData> from_timing(const Array& bpms, double duration);
		Dictionary get_snapshot() const;
		Dictionary get_metadata() const;
		Array get_bpm_events() const;
		int get_initial_key_total() const;
		int64_t get_revision() const;
		bool is_dirty() const;
		String get_audio_path() const;
		double get_duration() const;
		double get_beat_total() const;
		double get_row_total(int per_bar) const;
		double beat_to_time(double beat) const;
		double time_to_beat(double time_sec) const;
		double time_to_row(double time_sec, int per_bar) const;
		double row_to_time(double row, int per_bar) const;
		static double beat_to_row(double beat, int per_bar);
		static double row_to_beat(double row, int per_bar);
		bool matches_timing(const Array& bpms, double duration) const;
		void replace_snapshot(const Dictionary& snapshot);
		bool confirm_saved(int64_t saved_snapshot_revision);
		void mark_save_failed(int64_t failed_snapshot_revision);
	};
}

#endif // CHART_DATA_H
