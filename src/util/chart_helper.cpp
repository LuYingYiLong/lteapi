#include "chart_helper.h"
#include "utils.h"

namespace godot {
	void ChartHelper::_bind_methods() {
		ClassDB::bind_static_method("ChartHelper", D_METHOD("open", "path"), &ChartHelper::open);
		ClassDB::bind_method(D_METHOD("time_to_row", "time_sec"), &ChartHelper::time_to_row);
		ClassDB::bind_method(D_METHOD("row_to_time", "row"), &ChartHelper::row_to_time);
		ClassDB::bind_method(D_METHOD("time_to_beat", "time_sec"), &ChartHelper::time_to_beat);
		ClassDB::bind_method(D_METHOD("beat_to_time", "beat"), &ChartHelper::beat_to_time);
		ClassDB::bind_method(D_METHOD("row_to_beat", "row"), &ChartHelper::row_to_beat);
		ClassDB::bind_method(D_METHOD("beat_to_row", "beat"), &ChartHelper::beat_to_row);
		ClassDB::bind_static_method("ChartHelper", D_METHOD("row_to_beat_absolute", "row", "per_bar"), &ChartHelper::row_to_beat_absolute);
		ClassDB::bind_static_method("ChartHelper", D_METHOD("beat_to_row_absolute", "beat", "per_bar"), &ChartHelper::beat_to_row_absolute);
		ClassDB::bind_method(D_METHOD("set_bpms", "new_bpms"), &ChartHelper::set_bpms);
		ClassDB::bind_method(D_METHOD("get_bpms"), &ChartHelper::get_bpms);
		ClassDB::bind_method(D_METHOD("set_per_bar", "new_per_bar"), &ChartHelper::set_per_bar);
		ClassDB::bind_method(D_METHOD("get_per_bar"), &ChartHelper::get_per_bar);
		ClassDB::bind_method(D_METHOD("set_duration", "new_duration"), &ChartHelper::set_duration);
		ClassDB::bind_method(D_METHOD("get_duration"), &ChartHelper::get_duration);
		ClassDB::bind_method(D_METHOD("get_row_total"), &ChartHelper::get_row_total);
		ClassDB::bind_method(D_METHOD("get_beat_total"), &ChartHelper::get_beat_total);
		
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "bpms"), "set_bpms", "get_bpms");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "per_bar"), "set_per_bar", "get_per_bar");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");
	}

	ChartHelper::ChartHelper() {

	}

	ChartHelper::~ChartHelper() {

	}

	void ChartHelper::_build_time_to_beat_map() {
		Array map;
		Array sorted_beats;
		for (int index = 0; index < bpms.size(); ++index) {
			Dictionary current_bpm = bpms[index];
			sorted_beats.append(current_bpm.get("beat", 0.0));
		}
		double current_sec = 0.0;
		for (int index = 0; index < sorted_beats.size(); ++index) {
			Dictionary current_bpm = bpms[index];
			double from_beat = sorted_beats[index];
			double to_beat = INFINITY;
			double bpm = current_bpm.get("bpm", 120.0);

			if (bpm == 0.0) bpm = 120.0;

			// 最后一段用剩余秒数反推拍数
			double sec_left = duration - current_sec;
			if (index == sorted_beats.size() - 1) {
				to_beat = beat_total;
				Dictionary result = Dictionary();
				result["from_sec"] = current_sec;
				result["to_sec"] = duration;
				result["from_beat"] = from_beat;
				result["to_beat"] = to_beat;
				result["bpm"] = bpm;
				map.append(result);
				break;
			}

			// 普通段
			to_beat = sorted_beats[index + 1];
			double beats_in_seg = to_beat - from_beat;
			double sec_in_seg = beats_in_seg * 60.0 / bpm;
			Dictionary result = Dictionary();
			result["from_sec"] = current_sec;
			result["to_sec"] = current_sec + sec_in_seg;
			result["from_beat"] = from_beat;
			result["to_beat"] = to_beat;
			result["bpm"] = bpm;
			map.append(result);
			current_sec += sec_in_seg;
		}
		beat_map = map;
	}

	void ChartHelper::_build_beat_to_time_map() {
		Array map;
		Array sorted_beats;
		for (int index = 0; index < bpms.size(); ++index) {
			Dictionary current_bpm = bpms[index];
			sorted_beats.append(current_bpm.get("beat", 0.0));
		}
		double current_map_end_beat = beat_total;
		double current_sec = 0.0;
		for (int index = 0; index < sorted_beats.size(); ++index) {
			double from_beat = sorted_beats[index];
			Dictionary current_bpm = bpms[index];
			double bpm = current_bpm.get("bpm", 120.0);

			if (bpm == 0.0) bpm = 120.0;

			// 最后一段
			if (index == sorted_beats.size() - 1) {
				Dictionary result = Dictionary();
				result["from_sec"] = current_sec;
				result["from_beat"] = from_beat;
				result["to_beat"] = current_map_end_beat;
				result["bpm"] = bpm;
				map.append(result);
				break;
			}
			double to_beat_in_seg = sorted_beats[index + 1];
			double beats_in_seg = to_beat_in_seg - from_beat;
			double sec_in_seg = beats_in_seg * 60.0 / bpm;
			Dictionary result = Dictionary();
			result["from_sec"] = current_sec;
			result["from_beat"] = from_beat;
			result["to_beat"] = to_beat_in_seg;
			result["bpm"] = bpm;
			map.append(result);
			current_sec += sec_in_seg;
		}
		time_map = map;
	}

	void ChartHelper::_calculate_beat_total() {
		if (bpms.is_empty()) {
			beat_total = 0.0;
			return;
		}
		double current_sec = 0.0;
		double current_beat = 0.0;
		for (int index = 0; index < bpms.size(); ++index) {
			Dictionary current_bpm = bpms[index];
			double from_beat = current_bpm.get("beat", 0.0);
			double bpm = current_bpm.get("bpm", 120.0);

			if (bpm == 0.0) bpm = 120.0;

			// 用剩余秒数反推拍数
			if (index == bpms.size() - 1) {
				double sec_left = duration - current_sec;

				// 防止精度误差导致负数
				if (sec_left < 0) sec_left = 0;
				double beats_left = sec_left * bpm / 60.0;
				beat_total = from_beat + beats_left;
				break;
			}

			// 普通段
			Dictionary next_bpm = bpms[index + 1];
			double to_beat = next_bpm.get("beat", 0.0);
			double beats_seg = to_beat - from_beat;
			if (bpm == 0.0) bpm = 120.0;
			double sec_seg = beats_seg * 60.0 / bpm;
			current_sec += sec_seg;
			current_beat = to_beat;
		}
	}

	void ChartHelper::_calculate_row_total() {
		row_total = get_beat_total() * per_bar;
	}

	Ref<ChartHelper> ChartHelper::open(const String& path) {
		Dictionary chart = Utils::load_json_file(path);
		if (chart.is_empty()) return nullptr;
		Ref<ChartHelper> chart_helper;
		chart_helper.instantiate();
		chart_helper->bpms = chart.get("bpm", Array());
		Dictionary metadata = chart.get("metadata", Dictionary());
		chart_helper->duration = metadata.get("duration", 0.0);
		chart_helper->_calculate_beat_total();
		chart_helper->_calculate_row_total();
		chart_helper->_build_time_to_beat_map();
		chart_helper->_build_beat_to_time_map();
		return chart_helper;
	}

	double ChartHelper::time_to_row(const double time_sec) {
		return time_to_beat(time_sec) * per_bar;
	}

	double ChartHelper::row_to_time(const double row) {
		return beat_to_time(row / per_bar);
	}

	double ChartHelper::time_to_beat(const double time_sec) {
		Array seg_map = beat_map;
		for (int index = 0; index < seg_map.size(); ++index) {
			Dictionary seg = seg_map[index];
			double from_sec = seg.get("from_sec", 0.0);
			double to_sec = seg.get("to_sec", 0.0);
			double from_beat = seg.get("from_beat", 0.0);
			double to_beat = seg.get("to_beat", 0.0);
			if (time_sec >= from_sec && time_sec < to_sec) {
				double time = (time_sec - from_sec) / (to_sec - from_sec);
				return UtilityFunctions::lerpf(from_beat, to_beat, time);
			}
		}

		// 超尾
		if (seg_map.is_empty()) return 0.0;
		Dictionary last = seg_map[seg_map.size() - 1];
		double to_sec = last.get("to_sec", 0.0);
		double to_beat = last.get("to_beat", 0.0);
		double bpm = last.get("bpm", 0.0);

		if (bpm == 0.0) bpm = 120.0;

		double extra_sec = time_sec - to_sec;
		double extra_beat_cout = extra_sec * bpm / 60.0;
		return to_beat + extra_beat_cout;
	}

	double ChartHelper::beat_to_time(const double beat) {
		Array seg_map = time_map;
		for (int index = 0; index < seg_map.size(); ++index) {
			Dictionary seg = seg_map[index];
			double from_sec = seg.get("from_sec", 0.0);
			double from_beat = seg.get("from_beat", 0.0);
			double to_beat = seg.get("to_beat", 0.0);
			double bpm = seg.get("bpm", 0.0);
			if (bpm == 0.0) bpm = 120.0;
			if (beat < from_beat) continue;
			if (beat >= to_beat) continue;
			double time = (beat - from_beat) / (to_beat - from_beat);
			return UtilityFunctions::lerpf(from_sec, from_sec + (to_beat - from_beat) * 60.0 / bpm, time);
		}

		// 超尾
		if (seg_map.is_empty()) return 0.0;
		Dictionary last = seg_map[seg_map.size() - 1];
		double from_sec = last.get("from_sec", 0.0);
		double from_beat = last.get("from_beat", 0.0);
		double bpm = last.get("bpm", 0.0);
		if (bpm == 0.0) bpm = 120.0;
		double extra_beats = beat - from_beat;
		double extra_sec = extra_beats * 60.0 / bpm;
		return from_sec + extra_sec;
	}

	double ChartHelper::row_to_beat(const double row) {
		return row / per_bar;
	}

	double ChartHelper::beat_to_row(const double beat) {
		return beat * per_bar;
	}

	double ChartHelper::row_to_beat_absolute(const double row, const int per_bar) {
		return row / per_bar;
	}

	double ChartHelper::beat_to_row_absolute(const double beat, const int per_bar) {
		return beat * per_bar;
	}

	void ChartHelper::set_bpms(const Array& new_bpms) {
		bpms = new_bpms;
		_calculate_beat_total();
		_calculate_row_total();
		_build_time_to_beat_map();
		_build_beat_to_time_map();
	}

	Array ChartHelper::get_bpms() const {
		return bpms;
	}

	void ChartHelper::set_per_bar(const int new_per_bar) {
		per_bar = new_per_bar;
		_calculate_row_total();
	}

	int ChartHelper::get_per_bar() const {
		return per_bar;
	}

	void ChartHelper::set_duration(const double new_duration) {
		duration = new_duration;
		_calculate_beat_total();
		_calculate_row_total();
		_build_time_to_beat_map();
		_build_beat_to_time_map();
	}

	double ChartHelper::get_duration() const {
		return duration;
	}

	double ChartHelper::get_beat_total() const {
		return beat_total;
	}

	double ChartHelper::get_row_total() const {
		return row_total;
	}

}
