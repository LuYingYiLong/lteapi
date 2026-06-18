#include "lte_chart_note_helper.h"

#include "util/chart_helper.h"

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/core/math.hpp>

#include <cmath>
#include <cstdio>

namespace godot {

LTEChartNoteHelper* LTEChartNoteHelper::singleton = nullptr;

void LTEChartNoteHelper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("normalize_note", "note", "options"), &LTEChartNoteHelper::normalize_note, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("normalize_notes", "notes", "options"), &LTEChartNoteHelper::normalize_notes, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("sort_notes", "notes"), &LTEChartNoteHelper::sort_notes);
	ClassDB::bind_method(D_METHOD("make_note_identity", "note"), &LTEChartNoteHelper::make_note_identity);
	ClassDB::bind_method(D_METHOD("notes_intersect", "a", "b"), &LTEChartNoteHelper::notes_intersect);
	ClassDB::bind_method(D_METHOD("find_overlaps", "notes", "options"), &LTEChartNoteHelper::find_overlaps, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("make_note_delta", "old_notes", "new_notes"), &LTEChartNoteHelper::make_note_delta);
	ClassDB::bind_method(D_METHOD("apply_note_delta", "chart", "delta"), &LTEChartNoteHelper::apply_note_delta);
	ClassDB::bind_method(D_METHOD("filter_notes_by_range", "notes", "beat_start", "beat_end", "tracks"), &LTEChartNoteHelper::filter_notes_by_range, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("beats_to_times", "beats", "chart_data"), &LTEChartNoteHelper::beats_to_times);
	ClassDB::bind_method(D_METHOD("times_to_beats", "times", "chart_data"), &LTEChartNoteHelper::times_to_beats);
	ClassDB::bind_method(D_METHOD("_sort_notes_comparator", "a", "b"), &LTEChartNoteHelper::_sort_notes_comparator);
}

LTEChartNoteHelper::LTEChartNoteHelper() {
	if (singleton == nullptr) {
		singleton = this;
	}
}

LTEChartNoteHelper::~LTEChartNoteHelper() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

LTEChartNoteHelper* LTEChartNoteHelper::get_singleton() {
	return singleton;
}

// ——— 规范化 ———

Dictionary LTEChartNoteHelper::normalize_note(const Dictionary& note, const Dictionary& options) {
	Dictionary normalized = note.duplicate(true);

	String type = String(normalized.get("type", "tap"));
	normalized["type"] = type;
	normalized["key"] = static_cast<int32_t>(normalized.get("key", -1));
	normalized["end_beat"] = Math::snapped(static_cast<double>(normalized.get("end_beat", 0.0)), 0.000001);

	if (type == String("tap")) {
		if (normalized.has("start_beat")) {
			normalized.erase("start_beat");
		}
		if (normalized.has("head")) {
			normalized.erase("head");
		}
	} else {
		normalized["start_beat"] = Math::snapped(static_cast<double>(normalized.get("start_beat", 0.0)), 0.000001);
		if (!normalized.has("head")) {
			normalized["head"] = "tap";
		}
	}

	double early_val = static_cast<double>(normalized.get("early", 0.0));
	if (Math::is_zero_approx(early_val)) {
		if (normalized.has("early")) {
			normalized.erase("early");
		}
	} else {
		normalized["early"] = early_val;
	}

	double speed_val = static_cast<double>(normalized.get("speed", 1.0));
	if (Math::is_equal_approx(speed_val, 1.0)) {
		if (normalized.has("speed")) {
			normalized.erase("speed");
		}
	} else {
		normalized["speed"] = speed_val;
	}

	String group = String(normalized.get("group", "")).strip_edges();
	if (group.is_empty()) {
		if (normalized.has("group")) {
			normalized.erase("group");
		}
	} else {
		normalized["group"] = group;
	}

	return normalized;
}

Array LTEChartNoteHelper::normalize_notes(const Array& notes, const Dictionary& options) {
	Array result;
	for (int32_t i = 0; i < notes.size(); ++i) {
		if (notes[i].get_type() == Variant::DICTIONARY) {
			Dictionary note = notes[i];
			result.append(normalize_note(note, options));
		}
	}
	return result;
}

// ——— 排序 ———

Array LTEChartNoteHelper::sort_notes(const Array& notes) {
	Array sorted = notes.duplicate(true);
	sorted.sort_custom(Callable(this, "_sort_notes_comparator"));
	return sorted;
}

bool LTEChartNoteHelper::_sort_notes_comparator(const Dictionary& a, const Dictionary& b) const {
	String a_type = a.get("type", "tap");
	String b_type = b.get("type", "tap");
	double a_beat = (a_type == String("tap")) ? static_cast<double>(a.get("end_beat", 0.0)) : static_cast<double>(a.get("start_beat", 0.0));
	double b_beat = (b_type == String("tap")) ? static_cast<double>(b.get("end_beat", 0.0)) : static_cast<double>(b.get("start_beat", 0.0));
	if (Math::is_equal_approx(a_beat, b_beat)) {
		return static_cast<int32_t>(a.get("key", 0)) < static_cast<int32_t>(b.get("key", 0));
	}
	return a_beat < b_beat;
}

// ——— 标识 ———

String LTEChartNoteHelper::_format_identity_float(double value) const {
	// 使用 String::num 匹配 GDScript 的 str(snappedf(value, 0.000001))
	double snapped = Math::snapped(value, 0.000001);
	return String::num(snapped);
}

String LTEChartNoteHelper::make_note_identity(const Dictionary& note) {
	Dictionary normalized = normalize_note(note);
	String type = normalized.get("type", "tap");
	String str_key = String::num_int64(static_cast<int32_t>(normalized.get("key", -1)));
	String str_end = _format_identity_float(static_cast<double>(normalized.get("end_beat", 0.0)));

	PackedStringArray parts;
	parts.append(type);
	parts.append(str_key);
	parts.append(str_end);

	if (type == String("hold")) {
		parts.append(_format_identity_float(static_cast<double>(normalized.get("start_beat", 0.0))));
		parts.append(String(normalized.get("head", "tap")));
	}
	if (normalized.has("early")) {
		parts.append("early=" + _format_identity_float(static_cast<double>(normalized.get("early", 0.0))));
	}
	if (normalized.has("speed")) {
		parts.append("speed=" + _format_identity_float(static_cast<double>(normalized.get("speed", 0.0))));
	}
	if (normalized.has("group")) {
		parts.append("group=" + String(normalized.get("group", "")));
	}

	return String("|").join(parts);
}

// ——— 碰撞检测 ———

bool LTEChartNoteHelper::_beat_in_range(double beat, double start_beat, double end_beat) const {
	bool after_start = (beat > start_beat) || Math::is_equal_approx(beat, start_beat);
	bool before_end = (beat < end_beat) || Math::is_equal_approx(beat, end_beat);
	return after_start && before_end;
}

bool LTEChartNoteHelper::notes_intersect(const Dictionary& a, const Dictionary& b) {
	if (static_cast<int32_t>(a.get("key", -1)) != static_cast<int32_t>(b.get("key", -1))) {
		return false;
	}

	String a_type = a.get("type", "tap");
	String b_type = b.get("type", "tap");

	if (a_type == String("tap") && b_type == String("tap")) {
		return Math::is_equal_approx(
			static_cast<double>(a.get("end_beat", 0.0)),
			static_cast<double>(b.get("end_beat", 0.0))
		);
	}
	if (a_type == String("tap")) {
		return _beat_in_range(
			static_cast<double>(a.get("end_beat", 0.0)),
			static_cast<double>(b.get("start_beat", 0.0)),
			static_cast<double>(b.get("end_beat", 0.0))
		);
	}
	if (b_type == String("tap")) {
		return _beat_in_range(
			static_cast<double>(b.get("end_beat", 0.0)),
			static_cast<double>(a.get("start_beat", 0.0)),
			static_cast<double>(a.get("end_beat", 0.0))
		);
	}
	// both hold
	double a_start = static_cast<double>(a.get("start_beat", 0.0));
	double a_end = static_cast<double>(a.get("end_beat", 0.0));
	double b_start = static_cast<double>(b.get("start_beat", 0.0));
	double b_end = static_cast<double>(b.get("end_beat", 0.0));
	return MAX(a_start, b_start) < MIN(a_end, b_end);
}

Array LTEChartNoteHelper::find_overlaps(const Array& notes, const Dictionary& options) {
	Array overlaps;
	int32_t note_count = notes.size();
	if (note_count < 2) {
		return overlaps;
	}

	// 按 key 分组建索引
	Dictionary by_track;
	for (int32_t i = 0; i < note_count; ++i) {
		if (notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = notes[i];
		int32_t key = static_cast<int32_t>(note.get("key", -1));
		Array track_notes = by_track.get(key, Array());
		Dictionary entry;
		entry["note"] = note;
		entry["index"] = i;
		track_notes.append(entry);
		by_track[key] = track_notes;
	}

	Array track_keys = by_track.keys();
	for (int32_t ti = 0; ti < track_keys.size(); ++ti) {
		Array track_notes = by_track[track_keys[ti]];
		int32_t track_count = track_notes.size();
		for (int32_t i = 0; i < track_count; ++i) {
			Dictionary entry_a = track_notes[i];
			for (int32_t j = i + 1; j < track_count; ++j) {
				Dictionary entry_b = track_notes[j];
				if (notes_intersect(entry_a["note"], entry_b["note"])) {
					Dictionary overlap;
					overlap["note_a"] = Dictionary(entry_a["note"]).duplicate(true);
					overlap["note_b"] = Dictionary(entry_b["note"]).duplicate(true);
					overlap["index_a"] = entry_a["index"];
					overlap["index_b"] = entry_b["index"];
					overlap["type"] = "note_overlap";
					overlaps.append(overlap);
				}
			}
		}
	}
	return overlaps;
}

// ——— 差异计算 ———

Dictionary LTEChartNoteHelper::_make_item_counts(const Array& items, const Callable& identity_callable) {
	Dictionary counts;
	for (int32_t i = 0; i < items.size(); ++i) {
		if (items[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary item = items[i];
		String identity = String(identity_callable.call(item));
		int32_t count = static_cast<int32_t>(counts.get(identity, 0));
		counts[identity] = count + 1;
	}
	return counts;
}

Array LTEChartNoteHelper::_get_removed_items(const Array& source_items, const Array& target_items, const Callable& identity_callable) {
	Dictionary target_counts = _make_item_counts(target_items, identity_callable);
	Array removed_items;
	for (int32_t i = 0; i < source_items.size(); ++i) {
		if (source_items[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary item = source_items[i];
		String identity = String(identity_callable.call(item));
		int32_t count = static_cast<int32_t>(target_counts.get(identity, 0));
		if (count > 0) {
			target_counts[identity] = count - 1;
		} else {
			removed_items.append(item.duplicate(true));
		}
	}
	return removed_items;
}

Dictionary LTEChartNoteHelper::make_note_delta(const Array& old_notes, const Array& new_notes) {
	Callable identity_callable = Callable(this, "make_note_identity");
	Array removed = _get_removed_items(old_notes, new_notes, identity_callable);
	Array added = _get_removed_items(new_notes, old_notes, identity_callable);
	Dictionary result;
	result["added"] = added;
	result["removed"] = removed;
	return result;
}

void LTEChartNoteHelper::_remove_note_from_array(Array& notes, const Dictionary& target_note) {
	Callable identity_callable = Callable(this, "make_note_identity");
	String target_identity = String(identity_callable.call(target_note));
	for (int32_t i = notes.size() - 1; i >= 0; --i) {
		if (notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = notes[i];
		String identity = String(identity_callable.call(note));
		if (identity == target_identity) {
			notes.remove_at(i);
			return;
		}
	}
}

Dictionary LTEChartNoteHelper::apply_note_delta(const Dictionary& chart, const Dictionary& delta) {
	Dictionary result = chart.duplicate(true);
	Array notes;
	Array chart_notes = result.get("note", Array());
	for (int32_t i = 0; i < chart_notes.size(); ++i) {
		if (chart_notes[i].get_type() == Variant::DICTIONARY) {
			notes.append(chart_notes[i]);
		}
	}

	// 移除
	Array removed_notes = delta.get("removed", Array());
	for (int32_t i = 0; i < removed_notes.size(); ++i) {
		if (removed_notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = removed_notes[i];
		_remove_note_from_array(notes, note);
	}

	// 添加
	Array added_notes = delta.get("added", Array());
	for (int32_t i = 0; i < added_notes.size(); ++i) {
		if (added_notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = added_notes[i];
		if (!note.has("type")) {
			continue;
		}
		notes.append(normalize_note(note));
	}

	// 重排序
	notes = sort_notes(notes);
	result["note"] = notes;
	return result;
}

// ——— 过滤 ———

Array LTEChartNoteHelper::filter_notes_by_range(const Array& notes, double beat_start, double beat_end, const Array& tracks) {
	Array result;
	for (int32_t i = 0; i < notes.size(); ++i) {
		if (notes[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary note = notes[i];
		int32_t key = static_cast<int32_t>(note.get("key", -1));
		if (!tracks.is_empty() && !tracks.has(key)) {
			continue;
		}
		String note_type = note.get("type", "tap");
		double n_start = (note_type == String("hold"))
			? static_cast<double>(note.get("start_beat", 0.0))
			: static_cast<double>(note.get("end_beat", 0.0));
		double n_end = static_cast<double>(note.get("end_beat", 0.0));
		if (n_end < beat_start || n_start > beat_end) {
			continue;
		}
		result.append(note.duplicate(true));
	}
	return result;
}

// ——— 批量节拍/时间转换 ———

PackedFloat64Array LTEChartNoteHelper::beats_to_times(const PackedFloat64Array& beats, const Dictionary& chart_data) {
	PackedFloat64Array result;
	if (beats.is_empty()) {
		return result;
	}

	Ref<ChartHelper> helper;
	helper.instantiate();
	helper->set_bpms(chart_data.get("bpm", Array()));
	helper->set_per_bar(static_cast<int32_t>(chart_data.get("per_bar", 1)));
	helper->set_duration(static_cast<double>(chart_data.get("duration", 0.0)));

	result.resize(beats.size());
	for (int32_t i = 0; i < beats.size(); ++i) {
		result[i] = helper->beat_to_time(beats[i]);
	}
	return result;
}

PackedFloat64Array LTEChartNoteHelper::times_to_beats(const PackedFloat64Array& times, const Dictionary& chart_data) {
	PackedFloat64Array result;
	if (times.is_empty()) {
		return result;
	}

	Ref<ChartHelper> helper;
	helper.instantiate();
	helper->set_bpms(chart_data.get("bpm", Array()));
	helper->set_per_bar(static_cast<int32_t>(chart_data.get("per_bar", 1)));
	helper->set_duration(static_cast<double>(chart_data.get("duration", 0.0)));

	result.resize(times.size());
	for (int32_t i = 0; i < times.size(); ++i) {
		result[i] = helper->time_to_beat(times[i]);
	}
	return result;
}

} // namespace godot
