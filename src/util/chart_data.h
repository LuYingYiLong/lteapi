#ifndef CHART_DATA_H
#define CHART_DATA_H

#include "chart_helper.h"

namespace godot {
	class ChartData : public RefCounted {
		GDCLASS(ChartData, RefCounted)

	private:
		Dictionary chart;
		int64_t revision = 0;
		int64_t saved_revision = 0;

	protected:
		static void _bind_methods();

	public:
		static Ref<ChartData> open(const String& path, bool validate_for_play = true);
		static Ref<ChartData> from_dictionary(const Dictionary& snapshot);
		Dictionary get_snapshot() const;
		Dictionary get_metadata() const;
		Array get_bpm_events() const;
		int get_initial_key_total() const;
		int64_t get_revision() const;
		bool is_dirty() const;
		String get_audio_path() const;
		Ref<ChartHelper> create_helper() const;
		void refresh_helper(const Ref<ChartHelper>& helper) const;
		void replace_snapshot(const Dictionary& snapshot);
		bool confirm_saved(int64_t saved_snapshot_revision);
		void mark_save_failed(int64_t failed_snapshot_revision);
	};
}

#endif // CHART_DATA_H
