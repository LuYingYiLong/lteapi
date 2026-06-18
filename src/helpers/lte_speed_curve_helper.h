#ifndef LTE_SPEED_CURVE_HELPER_H
#define LTE_SPEED_CURVE_HELPER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {
	class LTESpeedCurveHelper : public Object {
		GDCLASS(LTESpeedCurveHelper, Object)

	private:
		static LTESpeedCurveHelper* singleton;

		Dictionary _get_speed_handle(const Dictionary& speed, const String& handle_name) const;
		double _sample_bezier_speed(const Dictionary& segment, double start_speed, double end_speed, double weight) const;
		int32_t _find_speed_segment_for_beat(const Array& speeds, double beat) const;

	protected:
		static void _bind_methods();

	public:
		LTESpeedCurveHelper();
		~LTESpeedCurveHelper();

		static LTESpeedCurveHelper* get_singleton();

		// Easing
		double sample_ease_in(const String& curve_type, double weight) const;
		double sample_bounce_out(double weight) const;
		double apply_easing(const String& curve_type, const String& easing, double weight) const;

		// Bezier math
		double sample_bezier_axis(double start_value, double control_a, double control_b, double end_value, double weight) const;
		double solve_bezier_weight_for_x_scalar(double weight, double control_a_x, double control_b_x) const;
		Vector2 cubic_bezier_point(const Vector2& a, const Vector2& b, const Vector2& c, const Vector2& d, double weight) const;
		double solve_bezier_weight_for_x_vec2(const Vector2& a, const Vector2& b, const Vector2& c, const Vector2& d, double target_x) const;

		// Speed normalization
		Dictionary normalize_speed(const Dictionary& speed, const Dictionary& options = Dictionary());
		Array normalize_speeds(const Array& speeds, const Dictionary& options = Dictionary());

		// Speed sampling
		double sample_speed(const Array& speeds, double beat, const Dictionary& interpolation_infos = Dictionary()) const;
		PackedFloat64Array sample_speeds_batch(const Array& speeds, const PackedFloat64Array& beats, const Dictionary& interpolation_infos = Dictionary()) const;
	};
}

#endif // LTE_SPEED_CURVE_HELPER_H
