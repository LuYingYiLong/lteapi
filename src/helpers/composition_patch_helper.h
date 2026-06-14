#ifndef COMPOSITION_PATCH_HELPER_H
#define COMPOSITION_PATCH_HELPER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
	class LTECompositionPatchHelper : public Object {
		GDCLASS(LTECompositionPatchHelper, Object)

	private:
		static LTECompositionPatchHelper* singleton;

		bool _keyframe_matches_ref(const Dictionary& keyframe, const Dictionary& key_ref) const;
		Dictionary _find_layer_property_track(const Dictionary& layer, const String& property_name) const;

	protected:
		static void _bind_methods();

	public:
		LTECompositionPatchHelper();
		~LTECompositionPatchHelper();

		static LTECompositionPatchHelper* get_singleton();

		Dictionary find_keyframe_in_composition(const Dictionary& source_composition, const Dictionary& key_ref) const;
		Dictionary find_keyframe_by_ref(const Array& keyframes, const Dictionary& key_ref) const;
		bool keyframe_matches_ref(const Dictionary& keyframe, const Dictionary& key_ref) const;
		int32_t find_keyframe_index_by_ref(const Array& keyframes, const Dictionary& key_ref) const;
		int32_t find_modifier_index_by_ref(const Array& modifiers, const Dictionary& key_ref, const String& modifier_type) const;
		Dictionary ensure_keyframe_id(const Dictionary& keyframe) const;
		Dictionary ensure_modifier_id(const Dictionary& modifier) const;
		int32_t find_property_track_index(const Array& property_tracks, const String& property_name) const;
		int32_t find_property_keyframe_index_for_ref(const Array& keyframes, const Dictionary& key_ref) const;
		Dictionary find_matching_key_ref(const Array& refs, const Dictionary& key_ref) const;
		bool key_ref_scope_matches(const Dictionary& ref, const Dictionary& key_ref) const;
		Array merge_key_refs(const Array& first_refs, const Array& second_refs) const;
		bool has_matching_key_ref(const Array& refs, const Dictionary& key_ref) const;
		bool sort_sequence_clips(const Dictionary& first_clip, const Dictionary& second_clip) const;
		bool sort_keyframes(const Dictionary& first_keyframe, const Dictionary& second_keyframe) const;
		bool sort_modifiers(const Dictionary& first_modifier, const Dictionary& second_modifier) const;
		bool sort_property_tracks(const Dictionary& first_track, const Dictionary& second_track) const;
		String normalize_path(const String& path) const;
		bool is_same_path(const String& first_path, const String& second_path) const;
	};
}

#endif // COMPOSITION_PATCH_HELPER_H
