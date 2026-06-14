#ifndef COMPOSITION_SIGNATURE_HELPER_H
#define COMPOSITION_SIGNATURE_HELPER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
	class LTECompositionSignatureHelper : public Object {
		GDCLASS(LTECompositionSignatureHelper, Object)

	private:
		static LTECompositionSignatureHelper* singleton;

		Dictionary _make_keyframe_signature_data(const Dictionary& keyframe) const;
		Dictionary _make_modifier_signature_data(const Dictionary& modifier) const;
		String _make_property_keyframe_track_key_signature(const Dictionary& layer, const String& property_name);
		String _make_property_modifier_track_key_signature(const Dictionary& layer, const String& property_name, const String& modifier_type);
		String _make_sequence_layer_clip_key_signature(const Dictionary& layer, const int32_t sequence_lane) const;
		Dictionary _find_layer_property_track(const Dictionary& layer, const String& property_name) const;
		Array _get_layer_property_track_names(const Dictionary& layer) const;
		Array _get_property_modifier_track_types(const Dictionary& layer, const String& property_name) const;
		int32_t _get_sequence_clip_lane_count(const Dictionary& layer) const;
		Array _get_sequence_layer_clips_with_lanes(const Dictionary& layer) const;
		bool _is_supported_timeline_layer(const Dictionary& layer) const;
		String _get_property_track_display_name(const String& property_name) const;
		String _get_modifier_type_display_name(const String& modifier_type) const;
		String _get_layer_type_display_name(const Dictionary& layer) const;

	protected:
		static void _bind_methods();

	public:
		LTECompositionSignatureHelper();
		~LTECompositionSignatureHelper();

		static LTECompositionSignatureHelper* get_singleton();

		String make_track_structure_signature(const Array& layers) const;
		Dictionary make_track_key_signatures(const Array& layers);
		String make_sequence_layer_clip_key_signature(const Dictionary& layer, const int32_t sequence_lane) const;
		String make_property_keyframe_track_key_signature(const Dictionary& layer, const String& property_name);
		String make_property_modifier_track_key_signature(const Dictionary& layer, const String& property_name, const String& modifier_type);
		Dictionary make_keyframe_signature_data(const Dictionary& keyframe) const;
		Dictionary make_modifier_signature_data(const Dictionary& modifier) const;
		Array get_changed_property_key_track_indices(const Dictionary& old_signatures, const Dictionary& new_signatures, const Array& track_entries) const;
		Array get_incomplete_timeline_key_track_indices(const Array& layers, const Array& track_entries, Object* timeline_panel) const;
		Dictionary update_track_key_signatures_from_refs(const Dictionary& signatures, const Array& key_refs, const Array& layers, const Array& track_entries);
		bool sort_keyframe_signature_data(const Dictionary& first_keyframe, const Dictionary& second_keyframe);
		bool sort_modifier_signature_data(const Dictionary& first_modifier, const Dictionary& second_modifier);
	};
}

#endif // COMPOSITION_SIGNATURE_HELPER_H
