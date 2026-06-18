#ifndef COMPOSITION_SERVER_H
#define COMPOSITION_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {
	class LTECompositionServer : public Object {
		GDCLASS(LTECompositionServer, Object)

	private:
		static constexpr double DEFAULT_AUTO_SAVE_INTERVAL_SECONDS = 0.6;

		static LTECompositionServer* singleton;

		Array opened_compositions;
		mutable Dictionary evaluation_composition_cache;
		int64_t composition_save_revision_counter = 0;

		String _get_absolute_scene_path(const String& scene_path) const;
		bool _is_opened_composition_in_current_project(const Dictionary& opened_composition) const;
		Variant _load_composition_variant(const String& scene_path) const;
		int _find_opened_composition_index(const String& uuid, const String& scene_path) const;
		int _find_first_opened_composition_index(const String& scene_path) const;
		Dictionary _record_composition_data(const String& uuid, const String& scene_path);
		Dictionary _load_composition(const String& scene_path) const;
		Dictionary _get_composition_for_evaluation(const String& scene_path) const;
		Error _save_composition(const String& scene_path, const Dictionary& composition) const;
		void _queue_composition_save(const String& scene_path, const Dictionary& composition, const int64_t revision);
		int32_t _get_auto_save_delay_msec() const;
		String _make_composition_save_tag(const String& scene_path, const int64_t revision) const;
		bool _parse_composition_save_tag(const String& tag, String& r_scene_path, int64_t& r_revision) const;
		void _on_file_saved(const String& path, const String& tag, const int64_t revision);
		void _on_file_save_failed(const String& path, const String& tag, const int64_t revision, const int64_t error);
		Dictionary _make_default_composition(const String& type = "scene") const;
		Dictionary _make_default_game_layer() const;
		Dictionary _normalize_composition_data(const Variant& value) const;
		Dictionary _normalize_embedded_sequence_composition(const Variant& value) const;
		Dictionary _normalize_layer(const Variant& value) const;
		Array _normalize_layers(const Variant& value, const bool require_game_layer = true) const;
		bool _has_game_layer(const Array& layers) const;
		bool _value_needs_game_layer_repair(const Variant& value) const;
		Array _normalize_property_tracks(const Variant& value) const;
		Dictionary _normalize_property_track(const Variant& value) const;
		Array _normalize_keyframes(const Variant& value) const;
		Dictionary _normalize_keyframe(const Variant& value) const;
		Array _normalize_modifiers(const Variant& value) const;
		Dictionary _normalize_modifier(const Variant& value) const;
		Array _normalize_vector2_array(const Variant& value, const double fallback_x, const double fallback_y) const;
		String _normalize_color_string(const Variant& value, const String& fallback) const;
		bool _is_number(const Variant& value) const;
		bool _try_get_color(const Variant& value, Color& color) const;
		Variant _make_color_variant_like(const Variant& source, const Color& color) const;
		void _apply_property_tracks(Dictionary& state, const Array& tracks, const String& layer_id, const double time) const;
		Variant _sample_keyframes(const Array& keyframes, const double time, const Variant& fallback) const;
		Variant _apply_property_modifiers(const String& property_name, const Variant& value, const Array& modifiers, const double time) const;
		Variant _apply_shake_modifier(const Variant& value, const Dictionary& modifier, const double time) const;
		Variant _apply_beat_pulse_modifier(const Variant& value, const Dictionary& modifier, const double time) const;
		Variant _apply_vector_offset(const Variant& value, const double offset_x, const double offset_y) const;
		double _sample_shake_value(const double local_time, const double frequency, const int seed, const int axis) const;
		double _sample_shake_noise(const double sample_index, const int seed, const int axis) const;
		double _sample_modifier_fade_weight(const Dictionary& modifier, const double time) const;
		Variant _lerp_variant(const Variant& from, const Variant& to, const double weight) const;
		Variant _sample_bezier_variant(const Variant& from, const Variant& to, const Dictionary& previous_keyframe, const Dictionary& next_keyframe, const double weight) const;
		double _sample_bezier_number(const double from, const double to, const Dictionary& previous_keyframe, const Dictionary& next_keyframe, const double weight, const int component_index = -1) const;
		double _sample_interpolation_weight(const String& interpolation, const Dictionary& keyframe, const double weight) const;
		double _apply_easing(const String& curve_type, const String& easing, const double weight) const;
		double _sample_ease_in(const String& curve_type, const double weight) const;
		double _sample_bounce_out(const double weight) const;
		double _solve_bezier_weight_for_x(const Vector2& point_a, const Vector2& point_b, const Vector2& point_c, const Vector2& point_d, const double target_x) const;
		Vector2 _cubic_bezier_point(const Vector2& point_a, const Vector2& point_b, const Vector2& point_c, const Vector2& point_d, const double weight) const;
		Dictionary _get_keyframe_handle(const Dictionary& keyframe, const String& handle_name, const int component_index = -1) const;
		Dictionary _get_interpolation_options(const Dictionary& keyframe) const;
		String _get_interpolation_curve_type(const String& interpolation) const;
		String _get_component_handle_key(const int component_index) const;
		Array _evaluate_composition_internal(const String& scene_path, const double time, Array visited_paths) const;
		Array _evaluate_composition_data_internal(const Dictionary& composition, const String& owner_path, const double time, Array visited_paths) const;
		void _append_sequence_child_states(Array& states, const Dictionary& state, const double time, Array visited_paths) const;
		void _apply_sequence_parent_state(Dictionary& child_state, const Dictionary& parent_state) const;
		Dictionary _submit_composition_changes_internal(const String& uuid, const String& scene_path, const Dictionary& new_composition, const bool normalize);
		bool _apply_composition_keyframe_deltas(Dictionary& composition, const Array& deltas, const bool use_after) const;
		bool _apply_composition_keyframe_deltas_batched(Dictionary& composition, const Array& deltas, const bool use_after) const;
		bool _apply_composition_keyframe_delta(Dictionary& composition, const Dictionary& delta, const bool use_after) const;
		int32_t _find_layer_index_by_id(const Array& layers, const String& layer_id) const;
		int32_t _find_property_track_index(const Array& property_tracks, const String& property_name) const;
		int32_t _find_keyframe_index(const Array& keyframes, const String& keyframe_id, const double fallback_time, const bool use_fallback_time) const;

	protected:
		static void _bind_methods();

	public:
		LTECompositionServer();
		~LTECompositionServer();

		static LTECompositionServer* get_singleton();

		void clear_project_state();
		void open_composition(const String& uuid, const String& scene_path);
		Array get_opened_composition_list() const;
		Dictionary find_opened_composition_info(const String& uuid, const String& scene_path) const;
		Dictionary get_composition(const String& scene_path) const;
		Array get_composition_layers(const String& scene_path) const;
		Dictionary set_composition_layers(const String& scene_path, const Array& layers);
		Dictionary submit_composition_changes(const String& uuid, const String& scene_path, const Dictionary& new_composition);
		Dictionary submit_normalized_composition_changes(const String& uuid, const String& scene_path, const Dictionary& new_composition);
		Dictionary apply_composition_keyframe_deltas(const String& uuid, const String& scene_path, const Array& deltas, const bool use_after = true);
		Dictionary save_composition(const String& uuid, const String& scene_path);
		Array evaluate_composition(const String& scene_path, const double time) const;
		Dictionary normalize_composition(const Variant& value) const;
		double compute_interpolation_weight(const String& interpolation, const Dictionary& keyframe_data, const double weight) const;
		String get_last_opened_composition(const String& uuid) const;
		void set_last_opened_composition(const String& uuid, const String& scene_path);
		Dictionary fetch_timeline_config(const String& uuid, const String& scene_path) const;
		PackedStringArray get_scene_layers_collapsed_items(const String& uuid, const String& scene_path) const;
		void set_scene_layers_collapsed_items(const String& uuid, const String& scene_path, const PackedStringArray& collapsed_items);
		Vector2 get_scene_layers_scroll(const String& uuid, const String& scene_path) const;
		void set_scene_layers_scroll(const String& uuid, const String& scene_path, const Vector2& scroll);
		void set_timeline_snap_mode(const String& uuid, const String& scene_path, const bool enable);
		void set_timeline_step(const String& uuid, const String& scene_path, const double step);
		void set_timeline_counting_unit(const String& uuid, const String& scene_path, const String& counting_unit);
		void set_timeline_per_bar(const String& uuid, const String& scene_path, const int per_bar);
		void set_timeline_playhead_auto_scroll(const String& uuid, const String& scene_path, const bool enable);
		void set_timeline_auto_insert_keyframes_mode(const String& uuid, const String& scene_path, const bool enable);
		void set_timeline_view(const String& uuid, const String& scene_path, const double scale, const int h_scroll, const int v_scroll);
	};
}

#endif // COMPOSITION_SERVER_H
