#include "chart_data.h"

#include "utils.h"

#include <algorithm>
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

		Array read_bpm_events(const Dictionary& chart) {
			Variant events = chart.get("bpm", Variant());
			return events.get_type() == Variant::ARRAY ? Array(events) : Array();
		}

		double read_duration(const Dictionary& chart) {
			Variant metadata = chart.get("metadata", Variant());
			double duration = 0.0;
			if (metadata.get_type() == Variant::DICTIONARY && read_number(Dictionary(metadata).get("duration", 0.0), duration)) {
				return duration;
			}
			return 0.0;
		}
	}

	void ChartData::_bind_methods() {
		ClassDB::bind_static_method("ChartData", D_METHOD("open", "path", "validate_for_play"), &ChartData::open, DEFVAL(true));
		ClassDB::bind_static_method("ChartData", D_METHOD("from_dictionary", "snapshot"), &ChartData::from_dictionary);
		ClassDB::bind_static_method("ChartData", D_METHOD("from_timing", "bpms", "duration"), &ChartData::from_timing);
		ClassDB::bind_method(D_METHOD("get_snapshot"), &ChartData::get_snapshot);
		ClassDB::bind_method(D_METHOD("get_metadata"), &ChartData::get_metadata);
		ClassDB::bind_method(D_METHOD("get_bpm_events"), &ChartData::get_bpm_events);
		ClassDB::bind_method(D_METHOD("get_initial_key_total"), &ChartData::get_initial_key_total);
		ClassDB::bind_method(D_METHOD("get_revision"), &ChartData::get_revision);
		ClassDB::bind_method(D_METHOD("is_dirty"), &ChartData::is_dirty);
		ClassDB::bind_method(D_METHOD("get_audio_path"), &ChartData::get_audio_path);
		ClassDB::bind_method(D_METHOD("get_duration"), &ChartData::get_duration);
		ClassDB::bind_method(D_METHOD("get_beat_total"), &ChartData::get_beat_total);
		ClassDB::bind_method(D_METHOD("get_row_total", "per_bar"), &ChartData::get_row_total);
		ClassDB::bind_method(D_METHOD("matches_timing", "bpms", "duration"), &ChartData::matches_timing);
		ClassDB::bind_method(D_METHOD("beat_to_time", "beat"), &ChartData::beat_to_time);
		ClassDB::bind_method(D_METHOD("time_to_beat", "time_sec"), &ChartData::time_to_beat);
		ClassDB::bind_method(D_METHOD("time_to_row", "time_sec", "per_bar"), &ChartData::time_to_row);
		ClassDB::bind_method(D_METHOD("row_to_time", "row", "per_bar"), &ChartData::row_to_time);
		ClassDB::bind_static_method("ChartData", D_METHOD("beat_to_row", "beat", "per_bar"), &ChartData::beat_to_row);
		ClassDB::bind_static_method("ChartData", D_METHOD("row_to_beat", "row", "per_bar"), &ChartData::row_to_beat);
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

	Ref<ChartData> ChartData::from_timing(const Array& bpms, double duration) {
		Ref<ChartData> data;
		data.instantiate();
		Dictionary metadata;
		metadata["duration"] = duration;
		data->chart["metadata"] = metadata;
		data->chart["bpm"] = bpms.duplicate(true);
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
		return read_bpm_events(chart).duplicate(true);
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

	double ChartData::get_duration() const {
		return read_duration(chart);
	}

	bool ChartData::matches_timing(const Array& bpms, double duration) const {
		return get_duration() == duration && read_bpm_events(chart) == bpms;
	}

	void ChartData::_ensure_timing() const {
		if (!timing_dirty) {
			return;
		}
		tempo_segments.clear();
		beat_total = 0.0;
		timing_duration = get_duration();
		for (const Variant& entry : read_bpm_events(chart)) {
			if (entry.get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary bpm_event = entry;
			TempoSegment segment{ 0.0, 0.0, 120.0 };
			if (!read_number(bpm_event.get("beat", 0.0), segment.beat)) {
				segment.beat = 0.0;
			}
			if (!read_number(bpm_event.get("bpm", 120.0), segment.bpm) || segment.bpm == 0.0) {
				segment.bpm = 120.0;
			}
			if (!tempo_segments.empty()) {
				const TempoSegment& previous = tempo_segments.back();
				segment.time = previous.time + (segment.beat - previous.beat) * 60.0 / previous.bpm;
			}
			tempo_segments.push_back(segment);
		}
		if (!tempo_segments.empty()) {
			const TempoSegment& last = tempo_segments.back();
			beat_total = last.beat + std::max(timing_duration - last.time, 0.0) * last.bpm / 60.0;
		}
		timing_dirty = false;
	}

	double ChartData::get_beat_total() const {
		_ensure_timing();
		return beat_total;
	}

	double ChartData::get_row_total(int per_bar) const {
		return beat_to_row(get_beat_total(), per_bar);
	}

	double ChartData::beat_to_time(double beat) const {
		_ensure_timing();
		if (tempo_segments.empty()) {
			return 0.0;
		}
		for (size_t index = 0; index + 1 < tempo_segments.size(); ++index) {
			const TempoSegment& segment = tempo_segments[index];
			if (beat >= segment.beat && beat < tempo_segments[index + 1].beat) {
				return segment.time + (beat - segment.beat) * 60.0 / segment.bpm;
			}
		}
		const TempoSegment& last = tempo_segments.back();
		return last.time + (beat - last.beat) * 60.0 / last.bpm;
	}

	double ChartData::time_to_beat(double time_sec) const {
		_ensure_timing();
		if (tempo_segments.empty()) {
			return 0.0;
		}
		for (size_t index = 0; index + 1 < tempo_segments.size(); ++index) {
			const TempoSegment& segment = tempo_segments[index];
			if (time_sec >= segment.time && time_sec < tempo_segments[index + 1].time) {
				return segment.beat + (time_sec - segment.time) * segment.bpm / 60.0;
			}
		}
		const TempoSegment& last = tempo_segments.back();
		if (time_sec >= last.time && time_sec < timing_duration) {
			return last.beat + (time_sec - last.time) * last.bpm / 60.0;
		}
		return beat_total + (time_sec - timing_duration) * last.bpm / 60.0;
	}

	double ChartData::beat_to_row(double beat, int per_bar) {
		return beat * std::max(per_bar, 1);
	}

	double ChartData::row_to_beat(double row, int per_bar) {
		return row / std::max(per_bar, 1);
	}

	double ChartData::time_to_row(double time_sec, int per_bar) const {
		return beat_to_row(time_to_beat(time_sec), per_bar);
	}

	double ChartData::row_to_time(double row, int per_bar) const {
		return beat_to_time(row_to_beat(row, per_bar));
	}

	void ChartData::replace_snapshot(const Dictionary& snapshot) {
		if (!timing_dirty) {
			timing_dirty = !matches_timing(read_bpm_events(snapshot), read_duration(snapshot));
		}
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
