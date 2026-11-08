#include "chart_data.h"

#include "utils.h"

#include <initializer_list>

namespace godot {
	namespace {
		bool read_number(const Variant& value, double& number) {
			switch (value.get_type()) {
				case Variant::INT:
				case Variant::FLOAT:
					number = value;
					break;
				case Variant::STRING: {
					String text = value;
					if (!text.is_valid_float()) {
						return false;
					}
					number = text.to_float();
					break;
				}
				default:
					return false;
			}
			return std::isfinite(number);
		}

		bool has_valid_numbers(const Dictionary& record, std::initializer_list<const char*> fields) {
			for (const char* field : fields) {
				double number = 0.0;
				if (record.has(field) && !read_number(record[field], number)) {
					return false;
				}
			}
			return true;
		}

		bool has_valid_entries(const Dictionary& chart, const String& section, std::initializer_list<const char*> fields, bool require_positive_bpm = false) {
			Variant stored = chart.get(section, Array());
			if (stored.get_type() != Variant::ARRAY) {
				return false;
			}
			for (const Variant& entry : Array(stored)) {
				if (entry.get_type() != Variant::DICTIONARY) {
					return false;
				}
				Dictionary record = entry;
				if (!has_valid_numbers(record, fields)) {
					return false;
				}
				double bpm = 0.0;
				if (require_positive_bpm && (!read_number(record.get("bpm", 0.0), bpm) || bpm <= 0.0)) {
					return false;
				}
			}
			return true;
		}

		bool is_playable_chart(const Dictionary& chart) {
			Variant stored = chart.get("metadata", Variant());
			if (stored.get_type() != Variant::DICTIONARY) {
				return false;
			}
			Dictionary metadata = stored;
			return !metadata.is_empty() &&
					has_valid_numbers(metadata, { "duration", "level", "audio_delay" }) &&
					(!metadata.has("audio") || metadata["audio"].get_type() == Variant::STRING) &&
					has_valid_entries(chart, "bpm", { "beat", "bpm" }, true) &&
					has_valid_entries(chart, "note", { "start_beat", "end_beat", "key", "early" }) &&
					has_valid_entries(chart, "speed", { "beat", "speed" }) &&
					has_valid_entries(chart, "road", { "beat", "key_total" });
		}
	}

	void ChartData::_bind_methods() {
		ClassDB::bind_static_method("ChartData", D_METHOD("open", "path", "validate_for_play"), &ChartData::open, DEFVAL(true));
		ClassDB::bind_static_method("ChartData", D_METHOD("from_dictionary", "snapshot"), &ChartData::from_dictionary);
		ClassDB::bind_method(D_METHOD("get_snapshot"), &ChartData::get_snapshot);
		ClassDB::bind_method(D_METHOD("get_metadata"), &ChartData::get_metadata);
		ClassDB::bind_method(D_METHOD("get_bpm_events"), &ChartData::get_bpm_events);
		ClassDB::bind_method(D_METHOD("get_initial_key_total"), &ChartData::get_initial_key_total);
		ClassDB::bind_method(D_METHOD("get_revision"), &ChartData::get_revision);
		ClassDB::bind_method(D_METHOD("is_dirty"), &ChartData::is_dirty);
		ClassDB::bind_method(D_METHOD("get_audio_path"), &ChartData::get_audio_path);
	}

	Ref<ChartData> ChartData::open(const String& path, bool validate_for_play) {
		if (path.is_empty()) {
			return nullptr;
		}
		Dictionary snapshot = Utils::load_json_file(path, false);
		if (snapshot.is_empty() || (validate_for_play && !is_playable_chart(snapshot))) {
			return nullptr;
		}
		Ref<ChartData> data;
		data.instantiate();
		data->chart = snapshot;
		return data;
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

	Dictionary ChartData::get_metadata() const {
		Variant metadata = chart.get("metadata", Variant());
		return metadata.get_type() == Variant::DICTIONARY ? Dictionary(metadata).duplicate(true) : Dictionary();
	}

	Array ChartData::get_bpm_events() const {
		Variant bpms = chart.get("bpm", Variant());
		return bpms.get_type() == Variant::ARRAY ? Array(bpms).duplicate(true) : Array();
	}

	int ChartData::get_initial_key_total() const {
		Variant roads = chart.get("road", Variant());
		if (roads.get_type() != Variant::ARRAY) {
			return 4;
		}
		for (const Variant& road : Array(roads)) {
			if (road.get_type() != Variant::DICTIONARY) {
				continue;
			}
			double keys = 4.0;
			if (!read_number(Dictionary(road).get("key_total", 4), keys)) {
				return 4;
			}
			keys = std::trunc(keys);
			if (keys <= 4.0) {
				return 4;
			}
			return keys <= 6.0 ? 6 : 8;
		}
		return 4;
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
