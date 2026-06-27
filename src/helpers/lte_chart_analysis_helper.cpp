#include "lte_chart_analysis_helper.h"

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/core/math.hpp>
#include <cmath>

namespace godot {

LTEChartAnalysisHelper* LTEChartAnalysisHelper::singleton = nullptr;

void LTEChartAnalysisHelper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("analyze_chart", "chart", "options"), &LTEChartAnalysisHelper::analyze_chart, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("diagnose_notes", "notes", "options"), &LTEChartAnalysisHelper::diagnose_notes, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("compute_density", "notes", "duration", "window_sec", "max_rows"), &LTEChartAnalysisHelper::compute_density, DEFVAL(1.0), DEFVAL(12));
	ClassDB::bind_method(D_METHOD("compute_track_distribution", "notes", "track_count"), &LTEChartAnalysisHelper::compute_track_distribution, DEFVAL(8));
	ClassDB::bind_method(D_METHOD("compute_note_group_stats", "notes"), &LTEChartAnalysisHelper::compute_note_group_stats);
	ClassDB::bind_method(D_METHOD("_sort_density_by_nps", "a", "b"), &LTEChartAnalysisHelper::_sort_density_by_nps);
	ClassDB::bind_method(D_METHOD("_sort_note_range_by_start", "a", "b"), &LTEChartAnalysisHelper::_sort_note_range_by_start);
}

LTEChartAnalysisHelper::LTEChartAnalysisHelper() {
	if (singleton == nullptr) {
		singleton = this;
	}
}

LTEChartAnalysisHelper::~LTEChartAnalysisHelper() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

LTEChartAnalysisHelper* LTEChartAnalysisHelper::get_singleton() {
	return singleton;
}

// ——— 辅助 ———

double LTEChartAnalysisHelper::_get_note_start_beat(const Dictionary& note) const {
	String type = note.get("type", "tap");
	if (type == String("hold")) {
		return static_cast<double>(note.get("start_beat", 0.0));
	}
	return static_cast<double>(note.get("end_beat", 0.0));
}

double LTEChartAnalysisHelper::_get_note_end_beat(const Dictionary& note) const {
	return static_cast<double>(note.get("end_beat", 0.0));
}

void LTEChartAnalysisHelper::_add_diagnostic(Array& diagnostics, const String& level, const String& code, const String& message, int32_t index, double beat, int32_t key) const {
	Dictionary d;
	d["level"] = level;
	d["code"] = code;
	d["message"] = message;
	d["index"] = index;
	d["beat"] = beat;
	d["key"] = key;
	diagnostics.append(d);
}

bool LTEChartAnalysisHelper::_sort_density_by_nps(const Dictionary& a, const Dictionary& b) const {
	return static_cast<double>(a.get("nps", 0.0)) > static_cast<double>(b.get("nps", 0.0));
}

bool LTEChartAnalysisHelper::_sort_note_range_by_start(const Dictionary& a, const Dictionary& b) const {
	return static_cast<double>(a.get("start_beat", 0.0)) < static_cast<double>(b.get("start_beat", 0.0));
}

// ——— 主分析 ———

Dictionary LTEChartAnalysisHelper::analyze_chart(const Dictionary& chart, const Dictionary& options) {
	Dictionary stats;

	Dictionary metadata = chart.get("metadata", Dictionary());
	stats["metadata"] = metadata;
	stats["mode"] = metadata.get("mode", String());
	stats["duration"] = static_cast<double>(metadata.get("duration", 0.0));
	stats["audio_delay"] = static_cast<double>(metadata.get("audio_delay", 0.0));

	Array notes;
	Array chart_notes = chart.get("note", Array());
	for (int32_t i = 0; i < chart_notes.size(); ++i) {
		if (chart_notes[i].get_type() == Variant::DICTIONARY) {
			notes.append(chart_notes[i]);
		}
	}
	Array bpms = chart.get("bpm", Array());
	Array speeds = chart.get("speed", Array());
	Array roads = chart.get("road", Array());

	stats["note_quantity"] = notes.size();
	stats["bpm_quantity"] = bpms.size();
	stats["speed_quantity"] = speeds.size();
	stats["road_quantity"] = roads.size();

	// 分析 notes
	Array diagnostics;
	int32_t tap_quantity = 0;
	int32_t hold_quantity = 0;
	int32_t full_combo = 0;

	// 轨道分布
	Dictionary track_stats;
	int32_t track_count = static_cast<int32_t>(options.get("track_count", 8));
	for (int32_t t = 0; t < track_count; ++t) {
		Dictionary ts;
		ts["tap"] = 0;
		ts["hold"] = 0;
		ts["total"] = 0;
		track_stats[String::num_int64(t)] = ts;
	}

	// 密度数据
	Array event_times;
	double duration = static_cast<double>(stats["duration"]);

	// 重叠检测辅助
	Dictionary track_heads; // key -> Array of {start_beat, index, key}

	for (int32_t i = 0; i < notes.size(); ++i) {
		Dictionary note = notes[i];

		// 验证
		if (static_cast<int32_t>(note.get("key", -1)) < 0) {
			_add_diagnostic(diagnostics, "error", "note_key_missing",
				"Note key is missing or negative.", i, 0.0, -1);
			continue;
		}
		int32_t key = static_cast<int32_t>(note.get("key", -1));
		if (key >= track_count) {
			_add_diagnostic(diagnostics, "error", "note_track_out_of_range",
				"Note track is out of range.", i, 0.0, key);
			continue;
		}

		String type = note.get("type", "tap");
		double start_beat = _get_note_start_beat(note);
		double end_beat = _get_note_end_beat(note);

		// 非法 hold
		if (type == String("hold") && end_beat <= start_beat) {
			_add_diagnostic(diagnostics, "error", "hold_time_invalid",
				"Hold note has start_beat >= end_beat.", i, end_beat, key);
			continue;
		}

		// 未知类型
		if (type != String("tap") && type != String("hold")) {
			_add_diagnostic(diagnostics, "warning", "unknown_note_type",
				"Unknown note type: " + type, i, end_beat, key);
		}

		// 计数
		if (type == String("hold")) {
			hold_quantity += 1;
			full_combo += 2;
		} else {
			tap_quantity += 1;
			full_combo += 1;
		}

		// 轨道统计
		String key_str = String::num_int64(key);
		Dictionary ts = track_stats[key_str];
		if (type == String("hold")) {
			ts["hold"] = static_cast<int32_t>(ts["hold"]) + 1;
		} else {
			ts["tap"] = static_cast<int32_t>(ts["tap"]) + 1;
		}
		ts["total"] = static_cast<int32_t>(ts["total"]) + 1;
		track_stats[key_str] = ts;

		// 事件时间（用简单的 beat → time 估算，不依赖 ChartHelper）
		event_times.append(end_beat);

		// 同一逻辑轨只禁止相同 head，Hold body 重叠合法
		Dictionary head;
		head["start_beat"] = start_beat;
		head["index"] = i;
		head["key"] = key;
		Array heads = track_heads.get(key_str, Array());
		heads.append(head);
		track_heads[key_str] = heads;
	}

	stats["tap_quantity"] = tap_quantity;
	stats["hold_quantity"] = hold_quantity;
	stats["full_combo"] = full_combo;
	stats["track_stats"] = track_stats;

	// head 冲突检测
	Array track_keys = track_heads.keys();
	for (int32_t ti = 0; ti < track_keys.size(); ++ti) {
		Array heads = track_heads[track_keys[ti]];
		heads.sort_custom(Callable(this, "_sort_note_range_by_start"));

		double previous_head = -INFINITY;
		for (int32_t hi = 0; hi < heads.size(); ++hi) {
			Dictionary head = heads[hi];
			double current_head = static_cast<double>(head["start_beat"]);
			if (Math::is_equal_approx(current_head, previous_head)) {
				_add_diagnostic(diagnostics, "warning", "note_overlap",
					"Note head conflicts with another note on the same track.",
					static_cast<int32_t>(head["index"]), current_head, static_cast<int32_t>(head["key"]));
			}
			previous_head = current_head;
		}
	}

	// 重复检测
	Dictionary seen_keys;
	for (int32_t i = 0; i < notes.size(); ++i) {
		Dictionary note = notes[i];
		if (static_cast<int32_t>(note.get("key", -1)) < 0) {
			continue;
		}
		int32_t key = static_cast<int32_t>(note.get("key", -1));
		double start_beat = _get_note_start_beat(note);
		double end_beat = _get_note_end_beat(note);
		String type = note.get("type", "tap");

		char buf[128];
		snprintf(buf, sizeof(buf), "%d:%.6f:%.6f:%s", key, start_beat, end_beat, type.utf8().get_data());
		String dup_key = String(buf);
		if (seen_keys.has(dup_key)) {
			_add_diagnostic(diagnostics, "warning", "duplicate_note",
				"Duplicate note detected.", i, end_beat, key);
		}
		seen_keys[dup_key] = true;
	}

	stats["diagnostics"] = diagnostics;

	// 密度
	Dictionary density = compute_density(notes, duration);
	stats["average_nps"] = density.get("average_nps", 0.0);
	stats["peak_nps"] = density.get("peak_nps", 0.0);
	stats["density_windows"] = density.get("density_windows", Array());

	// Note groups
	stats["note_group_stats"] = compute_note_group_stats(notes);

	return stats;
}

// ——— diagnose_notes ———

Array LTEChartAnalysisHelper::diagnose_notes(const Array& notes, const Dictionary& options) {
	// 用 analyze_chart 的内部逻辑来诊断
	Dictionary dummy_chart;
	dummy_chart["note"] = notes;
	Dictionary stats = analyze_chart(dummy_chart, options);
	return stats.get("diagnostics", Array());
}

// ——— 密度 ———

Dictionary LTEChartAnalysisHelper::compute_density(const Array& notes, double duration, double window_sec, int32_t max_rows) {
	Dictionary result;

	// 收集事件时间
	Array event_times;
	for (int32_t i = 0; i < notes.size(); ++i) {
		if (notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = notes[i];
		event_times.append(_get_note_end_beat(note));
	}
	event_times.sort();

	if (event_times.is_empty() || duration <= 0.0) {
		result["average_nps"] = 0.0;
		result["peak_nps"] = 0.0;
		result["density_windows"] = Array();
		return result;
	}

	double total_notes = static_cast<double>(notes.size());
	result["average_nps"] = total_notes / duration;

	// 滑动窗口
	Array windows;
	double peak_nps = 0.0;
	int32_t left_index = 0;

	for (int32_t right_index = 0; right_index < event_times.size(); ++right_index) {
		double end_time = static_cast<double>(event_times[right_index]);
		// 收缩左侧窗口到 ≤ window_sec
		while (left_index < right_index) {
			double start_time = static_cast<double>(event_times[left_index]);
			if (end_time - start_time <= window_sec) {
				break;
			}
			left_index += 1;
		}
		int32_t note_count = right_index - left_index + 1;
		double nps = static_cast<double>(note_count) / window_sec;
		peak_nps = MAX(peak_nps, nps);

		Dictionary win;
		win["note_count"] = note_count;
		win["nps"] = Math::snapped(nps, 0.01);
		win["end_time"] = Math::snapped(end_time, 0.001);
		win["start_time"] = Math::snapped(MAX(0.0, end_time - window_sec), 0.001);
		windows.append(win);
	}

	// 取 top N
	windows.sort_custom(Callable(this, "_sort_density_by_nps"));
	int32_t limit = MIN(max_rows, windows.size());
	Array top_windows;
	for (int32_t i = 0; i < limit; ++i) {
		top_windows.append(windows[i]);
	}

	result["density_windows"] = top_windows;
	result["peak_nps"] = Math::snapped(peak_nps, 0.01);
	return result;
}

// ——— 轨道分布 ———

Dictionary LTEChartAnalysisHelper::compute_track_distribution(const Array& notes, int32_t track_count) {
	Dictionary stats;
	for (int32_t t = 0; t < track_count; ++t) {
		Dictionary ts;
		ts["tap"] = 0;
		ts["hold"] = 0;
		ts["total"] = 0;
		stats[String::num_int64(t)] = ts;
	}

	for (int32_t i = 0; i < notes.size(); ++i) {
		if (notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = notes[i];
		int32_t key = static_cast<int32_t>(note.get("key", -1));
		if (key < 0 || key >= track_count) {
			continue;
		}
		String key_str = String::num_int64(key);
		Dictionary ts = stats[key_str];
		String type = note.get("type", "tap");
		if (type == String("hold")) {
			ts["hold"] = static_cast<int32_t>(ts["hold"]) + 1;
		} else {
			ts["tap"] = static_cast<int32_t>(ts["tap"]) + 1;
		}
		ts["total"] = static_cast<int32_t>(ts["total"]) + 1;
		stats[key_str] = ts;
	}
	return stats;
}

// ——— Note Group 统计 ———

Dictionary LTEChartAnalysisHelper::compute_note_group_stats(const Array& notes) {
	Dictionary groups;

	for (int32_t i = 0; i < notes.size(); ++i) {
		if (notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = notes[i];
		String group = String(note.get("group", "")).strip_edges();

		Dictionary gs = groups.get(group, Dictionary());
		if (gs.is_empty()) {
			gs["group"] = group;
			gs["total"] = 0;
			gs["tap"] = 0;
			gs["hold"] = 0;
			gs["tracks"] = Dictionary();
			gs["first_beat"] = INFINITY;
			gs["last_beat"] = -INFINITY;
		}

		int32_t key = static_cast<int32_t>(note.get("key", -1));
		String type = note.get("type", "tap");
		double start_beat = _get_note_start_beat(note);
		double end_beat = _get_note_end_beat(note);

		gs["total"] = static_cast<int32_t>(gs["total"]) + 1;
		if (type == String("hold")) {
			gs["hold"] = static_cast<int32_t>(gs["hold"]) + 1;
		} else {
			gs["tap"] = static_cast<int32_t>(gs["tap"]) + 1;
		}

		Dictionary tracks = gs["tracks"];
		String key_str = String::num_int64(key);
		tracks[key_str] = static_cast<int32_t>(tracks.get(key_str, 0)) + 1;
		gs["tracks"] = tracks;

		double fb = static_cast<double>(gs["first_beat"]);
		double lb = static_cast<double>(gs["last_beat"]);
		gs["first_beat"] = MIN(fb, start_beat);
		gs["last_beat"] = MAX(lb, end_beat);

		groups[group] = gs;
	}
	return groups;
}

} // namespace godot
