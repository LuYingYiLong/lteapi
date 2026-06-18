#include "lte_speed_curve_helper.h"

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/core/math.hpp>
#include <cmath>

namespace godot {

LTESpeedCurveHelper* LTESpeedCurveHelper::singleton = nullptr;

void LTESpeedCurveHelper::_bind_methods() {
	ClassDB::bind_method(D_METHOD("sample_ease_in", "curve_type", "weight"), &LTESpeedCurveHelper::sample_ease_in);
	ClassDB::bind_method(D_METHOD("sample_bounce_out", "weight"), &LTESpeedCurveHelper::sample_bounce_out);
	ClassDB::bind_method(D_METHOD("apply_easing", "curve_type", "easing", "weight"), &LTESpeedCurveHelper::apply_easing);
	ClassDB::bind_method(D_METHOD("sample_bezier_axis", "start_value", "control_a", "control_b", "end_value", "weight"), &LTESpeedCurveHelper::sample_bezier_axis);
	ClassDB::bind_method(D_METHOD("solve_bezier_weight_for_x_scalar", "weight", "control_a_x", "control_b_x"), &LTESpeedCurveHelper::solve_bezier_weight_for_x_scalar);
	ClassDB::bind_method(D_METHOD("cubic_bezier_point", "a", "b", "c", "d", "weight"), &LTESpeedCurveHelper::cubic_bezier_point);
	ClassDB::bind_method(D_METHOD("solve_bezier_weight_for_x_vec2", "a", "b", "c", "d", "target_x"), &LTESpeedCurveHelper::solve_bezier_weight_for_x_vec2);
	ClassDB::bind_method(D_METHOD("normalize_speed", "speed", "options"), &LTESpeedCurveHelper::normalize_speed, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("normalize_speeds", "speeds", "options"), &LTESpeedCurveHelper::normalize_speeds, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("sample_speed", "speeds", "beat", "interpolation_infos"), &LTESpeedCurveHelper::sample_speed, DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("sample_speeds_batch", "speeds", "beats", "interpolation_infos"), &LTESpeedCurveHelper::sample_speeds_batch, DEFVAL(Dictionary()));
}

LTESpeedCurveHelper::LTESpeedCurveHelper() {
	if (singleton == nullptr) {
		singleton = this;
	}
}

LTESpeedCurveHelper::~LTESpeedCurveHelper() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

LTESpeedCurveHelper* LTESpeedCurveHelper::get_singleton() {
	return singleton;
}

// ——— 缓动曲线 ———

double LTESpeedCurveHelper::sample_ease_in(const String& curve_type, double weight) const {
	if (curve_type == String("sinusoidal")) {
		return 1.0 - std::cos(weight * Math_PI * 0.5);
	}
	if (curve_type == String("quadratic")) {
		return weight * weight;
	}
	if (curve_type == String("cubic")) {
		return weight * weight * weight;
	}
	if (curve_type == String("quartic")) {
		return std::pow(weight, 4.0);
	}
	if (curve_type == String("quintic")) {
		return std::pow(weight, 5.0);
	}
	if (curve_type == String("exponential")) {
		if (Math::is_zero_approx(weight)) {
			return 0.0;
		}
		return std::pow(2.0, 10.0 * weight - 10.0);
	}
	if (curve_type == String("circular")) {
		return 1.0 - std::sqrt(1.0 - weight * weight);
	}
	if (curve_type == String("back")) {
		double overshoot = 1.70158;
		return (overshoot + 1.0) * weight * weight * weight - overshoot * weight * weight;
	}
	if (curve_type == String("bounce")) {
		return 1.0 - sample_bounce_out(1.0 - weight);
	}
	if (curve_type == String("elastic")) {
		if (Math::is_zero_approx(weight) || Math::is_equal_approx(weight, 1.0)) {
			return weight;
		}
		return -std::pow(2.0, 10.0 * weight - 10.0) * std::sin((weight * 10.0 - 10.75) * Math_TAU / 3.0);
	}
	return weight;
}

double LTESpeedCurveHelper::sample_bounce_out(double weight) const {
	double bounce_scale = 7.5625;
	double bounce_step = 2.75;
	if (weight < 1.0 / bounce_step) {
		return bounce_scale * weight * weight;
	}
	if (weight < 2.0 / bounce_step) {
		double shifted = weight - 1.5 / bounce_step;
		return bounce_scale * shifted * shifted + 0.75;
	}
	if (weight < 2.5 / bounce_step) {
		double shifted = weight - 2.25 / bounce_step;
		return bounce_scale * shifted * shifted + 0.9375;
	}
	double shifted = weight - 2.625 / bounce_step;
	return bounce_scale * shifted * shifted + 0.984375;
}

double LTESpeedCurveHelper::apply_easing(const String& curve_type, const String& easing, double weight) const {
	double clamped = CLAMP(weight, 0.0, 1.0);
	if (easing == String("ease_in")) {
		return sample_ease_in(curve_type, clamped);
	}
	if (easing == String("ease_out")) {
		return 1.0 - sample_ease_in(curve_type, 1.0 - clamped);
	}
	if (easing == String("ease_in_out")) {
		if (clamped < 0.5) {
			return sample_ease_in(curve_type, clamped * 2.0) * 0.5;
		}
		return 1.0 - sample_ease_in(curve_type, 2.0 - clamped * 2.0) * 0.5;
	}
	return sample_ease_in(curve_type, clamped);
}

// ——— Bezier 数学 ———

double LTESpeedCurveHelper::sample_bezier_axis(double start_value, double control_a, double control_b, double end_value, double weight) const {
	double inv = 1.0 - weight;
	return inv * inv * inv * start_value
		+ 3.0 * inv * inv * weight * control_a
		+ 3.0 * inv * weight * weight * control_b
		+ weight * weight * weight * end_value;
}

double LTESpeedCurveHelper::solve_bezier_weight_for_x_scalar(double weight, double control_a_x, double control_b_x) const {
	double low = 0.0;
	double high = 1.0;
	for (int32_t i = 0; i < 16; ++i) {
		double middle = (low + high) * 0.5;
		double x = sample_bezier_axis(0.0, control_a_x, control_b_x, 1.0, middle);
		if (x < weight) {
			low = middle;
		} else {
			high = middle;
		}
	}
	return (low + high) * 0.5;
}

Vector2 LTESpeedCurveHelper::cubic_bezier_point(const Vector2& a, const Vector2& b, const Vector2& c, const Vector2& d, double weight) const {
	double inv = 1.0 - weight;
	return a * inv * inv * inv
		+ b * 3.0 * inv * inv * weight
		+ c * 3.0 * inv * weight * weight
		+ d * weight * weight * weight;
}

double LTESpeedCurveHelper::solve_bezier_weight_for_x_vec2(const Vector2& a, const Vector2& b, const Vector2& c, const Vector2& d, double target_x) const {
	double low = 0.0;
	double high = 1.0;
	for (int32_t i = 0; i < 18; ++i) {
		double middle = (low + high) * 0.5;
		Vector2 position = cubic_bezier_point(a, b, c, d, middle);
		if (position.x < target_x) {
			low = middle;
		} else {
			high = middle;
		}
	}
	return (low + high) * 0.5;
}

// ——— 速度标准化 ———

Dictionary LTESpeedCurveHelper::normalize_speed(const Dictionary& speed, const Dictionary& options) {
	double min_speed = static_cast<double>(options.get("min_speed", 0.0));
	Dictionary normalized = speed.duplicate(true);
	normalized["beat"] = Math::snapped(static_cast<double>(normalized.get("beat", 0.0)), 0.000001);
	normalized["speed"] = MAX(Math::snapped(static_cast<double>(normalized.get("speed", normalized.get("bpm", 1.0))), 0.001), min_speed);
	String interpolation = String(normalized.get("interpolation", "linear"));
	normalized["interpolation"] = interpolation;
	if (normalized.has("bpm")) {
		normalized.erase("bpm");
	}
	if (normalized.has("update_mode")) {
		normalized.erase("update_mode");
	}
	if (interpolation != String("bezier")) {
		if (normalized.has("in_handle")) {
			normalized.erase("in_handle");
		}
		if (normalized.has("out_handle")) {
			normalized.erase("out_handle");
		}
	}
	return normalized;
}

Array LTESpeedCurveHelper::normalize_speeds(const Array& speeds, const Dictionary& options) {
	Array normalized_items;
	for (int32_t i = 0; i < speeds.size(); ++i) {
		if (speeds[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		Dictionary speed = speeds[i];
		normalized_items.append(normalize_speed(speed, options));
	}
	// Sort by beat ascending (bubble sort — speed arrays are typically small)
	for (int32_t i = 0; i < normalized_items.size(); ++i) {
		for (int32_t j = i + 1; j < normalized_items.size(); ++j) {
			double bi = static_cast<double>(Dictionary(normalized_items[i]).get("beat", 0.0));
			double bj = static_cast<double>(Dictionary(normalized_items[j]).get("beat", 0.0));
			if (bi > bj) {
				Variant tmp = normalized_items[i];
				normalized_items[i] = normalized_items[j];
				normalized_items[j] = tmp;
			}
		}
	}
	// Erase in_handle for first item and after non-bezier segments
	for (int32_t i = 0; i < normalized_items.size(); ++i) {
		Dictionary current = normalized_items[i];
		if (i == 0) {
			if (current.has("in_handle")) {
				current.erase("in_handle");
				normalized_items[i] = current;
			}
			continue;
		}
		Dictionary previous = normalized_items[i - 1];
		if (String(previous.get("interpolation", "linear")) != String("bezier")) {
			if (current.has("in_handle")) {
				current.erase("in_handle");
				normalized_items[i] = current;
			}
		}
	}
	return normalized_items;
}

// ——— 速度采样 ———

Dictionary LTESpeedCurveHelper::_get_speed_handle(const Dictionary& speed, const String& handle_name) const {
	Variant value = speed.get(handle_name, Variant());
	if (value.get_type() == Variant::DICTIONARY) {
		return value;
	}
	return Dictionary();
}

double LTESpeedCurveHelper::_sample_bezier_speed(const Dictionary& segment, double start_speed, double end_speed, double weight) const {
	Dictionary out_handle = _get_speed_handle(segment, "out_handle");
	Dictionary in_handle = _get_speed_handle(segment, "in_handle");

	Vector2 a = Vector2(0.0, start_speed);
	Vector2 b = Vector2(
		CLAMP(static_cast<double>(out_handle.get("x", 1.0 / 3.0)), 0.001, 0.999),
		start_speed + static_cast<double>(out_handle.get("y", (end_speed - start_speed) / 3.0))
	);
	Vector2 c = Vector2(
		1.0 + CLAMP(static_cast<double>(in_handle.get("x", -1.0 / 3.0)), -0.999, -0.001),
		end_speed + static_cast<double>(in_handle.get("y", (start_speed - end_speed) / 3.0))
	);
	Vector2 d = Vector2(1.0, end_speed);

	double solved = solve_bezier_weight_for_x_vec2(a, b, c, d, weight);
	return cubic_bezier_point(a, b, c, d, solved).y;
}

int32_t LTESpeedCurveHelper::_find_speed_segment_for_beat(const Array& speeds, double beat) const {
	if (speeds.is_empty()) {
		return -1;
	}
	int32_t low = 0;
	int32_t high = speeds.size() - 1;
	while (low < high) {
		int32_t mid = (low + high + 1) / 2;
		Dictionary speed = speeds[mid];
		if (static_cast<double>(speed.get("beat", 0.0)) <= beat) {
			low = mid;
		} else {
			high = mid - 1;
		}
	}
	return low;
}

double LTESpeedCurveHelper::sample_speed(const Array& speeds, double beat, const Dictionary& interpolation_infos) const {
	if (speeds.is_empty()) {
		return 1.0;
	}

	int32_t seg_idx = _find_speed_segment_for_beat(speeds, beat);
	if (seg_idx < 0) {
		return 1.0;
	}

	Dictionary segment = speeds[seg_idx];
	int32_t next_idx = MIN(seg_idx + 1, speeds.size() - 1);
	Dictionary next_segment = speeds[next_idx];

	double start_speed = static_cast<double>(segment.get("speed", 1.0));
	double end_speed = static_cast<double>(next_segment.get("speed", start_speed));
	double from_beat = static_cast<double>(segment.get("beat", 0.0));
	double to_beat = static_cast<double>(next_segment.get("beat", from_beat));
	String interpolation = String(segment.get("interpolation", "linear"));

	if (Math::is_equal_approx(from_beat, to_beat)) {
		return end_speed;
	}

	double weight = CLAMP((beat - from_beat) / (to_beat - from_beat), 0.0, 1.0);

	if (interpolation == String("constant")) {
		return start_speed;
	}
	if (interpolation == String("linear")) {
		return Math::lerp(start_speed, end_speed, weight);
	}
	if (interpolation == String("bezier")) {
		return _sample_bezier_speed(segment, start_speed, end_speed, weight);
	}

	// 缓动曲线
	String curve_type = String(interpolation_infos.get(interpolation, interpolation));
	Dictionary options = Dictionary(segment.get("interpolation_options", Dictionary()));
	String easing = String(options.get("easing", "ease_in_out"));
	double eased = apply_easing(curve_type, easing, weight);
	return Math::lerp(start_speed, end_speed, eased);
}

PackedFloat64Array LTESpeedCurveHelper::sample_speeds_batch(const Array& speeds, const PackedFloat64Array& beats, const Dictionary& interpolation_infos) const {
	PackedFloat64Array result;
	if (beats.is_empty()) {
		return result;
	}
	result.resize(beats.size());
	for (int32_t i = 0; i < beats.size(); ++i) {
		result[i] = sample_speed(speeds, beats[i], interpolation_infos);
	}
	return result;
}

} // namespace godot
