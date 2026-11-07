#include "chart_data.h"

namespace godot {
	void ChartData::_bind_methods() {
		ClassDB::bind_static_method("ChartData", D_METHOD("from_dictionary", "snapshot"), &ChartData::from_dictionary);
		ClassDB::bind_method(D_METHOD("get_snapshot"), &ChartData::get_snapshot);
		ClassDB::bind_method(D_METHOD("get_revision"), &ChartData::get_revision);
		ClassDB::bind_method(D_METHOD("is_dirty"), &ChartData::is_dirty);
		ClassDB::bind_method(D_METHOD("get_audio_path"), &ChartData::get_audio_path);
	}

	Ref<ChartData> ChartData::from_dictionary(const Dictionary& snapshot) {
		Ref<ChartData> data;
		data.instantiate();
		data->chart = snapshot.duplicate(true);
		return data;
	}

	Dictionary ChartData::get_snapshot() const {
		return chart.duplicate(true);
	}

	int64_t ChartData::get_revision() const {
		return revision;
	}

	bool ChartData::is_dirty() const {
		return revision != saved_revision;
	}

	String ChartData::get_audio_path() const {
		Dictionary metadata = chart.get("metadata", Dictionary());
		return metadata.get("audio", String());
	}

	Ref<ChartHelper> ChartData::create_helper() const {
		return ChartHelper::from_dictionary(chart);
	}

	void ChartData::refresh_helper(const Ref<ChartHelper>& helper) const {
		helper->update_from_chart(chart);
	}

	void ChartData::replace_snapshot(const Dictionary& snapshot) {
		chart = snapshot.duplicate(true);
		++revision;
	}

	bool ChartData::confirm_saved(int64_t saved_snapshot_revision) {
		if (saved_snapshot_revision != revision) {
			return false;
		}
		saved_revision = saved_snapshot_revision;
		return true;
	}

	void ChartData::mark_save_failed(int64_t failed_snapshot_revision) {
		if (failed_snapshot_revision == revision) {
			saved_revision = -1;
		}
	}
}
