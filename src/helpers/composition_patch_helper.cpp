#include "composition_patch_helper.h"

#include <cmath>

#include <godot_cpp/variant/utility_functions.hpp>
#include "util/utils.h"
#include "server/project_manager.h"

namespace godot {
	LTECompositionPatchHelper* LTECompositionPatchHelper::singleton = nullptr;

	void LTECompositionPatchHelper::_bind_methods() {
		ClassDB::bind_method(D_METHOD("find_keyframe_in_composition", "source_composition", "key_ref"), &LTECompositionPatchHelper::find_keyframe_in_composition);
		ClassDB::bind_method(D_METHOD("find_keyframe_by_ref", "keyframes", "key_ref"), &LTECompositionPatchHelper::find_keyframe_by_ref);
		ClassDB::bind_method(D_METHOD("keyframe_matches_ref", "keyframe", "key_ref"), &LTECompositionPatchHelper::keyframe_matches_ref);
		ClassDB::bind_method(D_METHOD("find_keyframe_index_by_ref", "keyframes", "key_ref"), &LTECompositionPatchHelper::find_keyframe_index_by_ref);
		ClassDB::bind_method(D_METHOD("find_modifier_index_by_ref", "modifiers", "key_ref", "modifier_type"), &LTECompositionPatchHelper::find_modifier_index_by_ref, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("ensure_keyframe_id", "keyframe"), &LTECompositionPatchHelper::ensure_keyframe_id);
		ClassDB::bind_method(D_METHOD("ensure_modifier_id", "modifier"), &LTECompositionPatchHelper::ensure_modifier_id);
		ClassDB::bind_method(D_METHOD("find_property_track_index", "property_tracks", "property_name"), &LTECompositionPatchHelper::find_property_track_index);
		ClassDB::bind_method(D_METHOD("find_property_keyframe_index_for_ref", "keyframes", "key_ref"), &LTECompositionPatchHelper::find_property_keyframe_index_for_ref);
		ClassDB::bind_method(D_METHOD("find_matching_key_ref", "refs", "key_ref"), &LTECompositionPatchHelper::find_matching_key_ref);
		ClassDB::bind_method(D_METHOD("key_ref_scope_matches", "ref", "key_ref"), &LTECompositionPatchHelper::key_ref_scope_matches);
		ClassDB::bind_method(D_METHOD("merge_key_refs", "first_refs", "second_refs"), &LTECompositionPatchHelper::merge_key_refs);
		ClassDB::bind_method(D_METHOD("has_matching_key_ref", "refs", "key_ref"), &LTECompositionPatchHelper::has_matching_key_ref);
		ClassDB::bind_method(D_METHOD("sort_sequence_clips", "first_clip", "second_clip"), &LTECompositionPatchHelper::sort_sequence_clips);
		ClassDB::bind_method(D_METHOD("sort_keyframes", "first_keyframe", "second_keyframe"), &LTECompositionPatchHelper::sort_keyframes);
		ClassDB::bind_method(D_METHOD("sort_modifiers", "first_modifier", "second_modifier"), &LTECompositionPatchHelper::sort_modifiers);
		ClassDB::bind_method(D_METHOD("sort_property_tracks", "first_track", "second_track"), &LTECompositionPatchHelper::sort_property_tracks);
		ClassDB::bind_method(D_METHOD("normalize_path", "path"), &LTECompositionPatchHelper::normalize_path);
		ClassDB::bind_method(D_METHOD("is_same_path", "first_path", "second_path"), &LTECompositionPatchHelper::is_same_path);
	}

	LTECompositionPatchHelper::LTECompositionPatchHelper() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTECompositionPatchHelper::~LTECompositionPatchHelper() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTECompositionPatchHelper* LTECompositionPatchHelper::get_singleton() {
		return singleton;
	}

	Dictionary LTECompositionPatchHelper::_find_layer_property_track(const Dictionary& layer, const String& property_name) const {
		Array property_tracks = layer.get("property_tracks", Array());
		for (int32_t i = 0; i < property_tracks.size(); ++i) {
			if (property_tracks[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary pt = property_tracks[i];
			if (String(pt.get("property", String())) == property_name) return pt;
		}
		return Dictionary();
	}

	bool LTECompositionPatchHelper::_keyframe_matches_ref(const Dictionary& keyframe, const Dictionary& key_ref) const {
		String ref_id = String(key_ref.get("id", String()));
		if (!ref_id.is_empty() && ref_id == String(keyframe.get("id", String()))) return true;
		double kf_time = UtilityFunctions::snappedf(static_cast<double>(keyframe.get("time", 0.0)), 0.0001);
		double ref_time = static_cast<double>(key_ref.get("time", 0.0));
		return std::abs(kf_time - ref_time) <= 0.0001;
	}

	Dictionary LTECompositionPatchHelper::find_keyframe_in_composition(const Dictionary& source_composition, const Dictionary& key_ref) const {
		String kind = String(key_ref.get("kind", String()));
		if (kind != "text_property_key") return Dictionary();
		Array layers = source_composition.get("layers", Array());
		String layer_id = String(key_ref.get("layer_id", String()));
		String property_name = String(key_ref.get("property", String()));
		for (int32_t i = 0; i < layers.size(); ++i) {
			if (layers[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary layer = layers[i];
			if (String(layer.get("id", String())) != layer_id) continue;
			Dictionary pt = _find_layer_property_track(layer, property_name);
			if (pt.is_empty()) return Dictionary();
			return find_keyframe_by_ref(pt.get("keyframes", Array()), key_ref);
		}
		return Dictionary();
	}

	Dictionary LTECompositionPatchHelper::find_keyframe_by_ref(const Array& keyframes, const Dictionary& key_ref) const {
		for (int32_t i = 0; i < keyframes.size(); ++i) {
			if (keyframes[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary kf = keyframes[i];
			if (_keyframe_matches_ref(kf, key_ref)) return kf;
		}
		return Dictionary();
	}

	bool LTECompositionPatchHelper::keyframe_matches_ref(const Dictionary& keyframe, const Dictionary& key_ref) const {
		return _keyframe_matches_ref(keyframe, key_ref);
	}

	int32_t LTECompositionPatchHelper::find_keyframe_index_by_ref(const Array& keyframes, const Dictionary& key_ref) const {
		for (int32_t i = 0; i < keyframes.size(); ++i) {
			if (keyframes[i].get_type() != Variant::DICTIONARY) continue;
			if (_keyframe_matches_ref(Dictionary(keyframes[i]), key_ref)) return i;
		}
		return -1;
	}

	int32_t LTECompositionPatchHelper::find_modifier_index_by_ref(const Array& modifiers, const Dictionary& key_ref, const String& modifier_type) const {
		String ref_id = String(key_ref.get("id", String()));
		double ref_time = UtilityFunctions::snappedf(static_cast<double>(key_ref.get("time", 0.0)), 0.0001);
		String target_type = modifier_type;
		if (target_type.is_empty()) target_type = String(key_ref.get("modifier_type", String()));
		for (int32_t i = 0; i < modifiers.size(); ++i) {
			if (modifiers[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary mod = modifiers[i];
			if (!target_type.is_empty() && String(mod.get("type", String())) != target_type) continue;
			if (!ref_id.is_empty() && ref_id == String(mod.get("id", String()))) return i;
			double mod_start = UtilityFunctions::snappedf(static_cast<double>(mod.get("start_time", 0.0)), 0.0001);
			if (std::abs(mod_start - ref_time) <= 0.0001) return i;
		}
		return -1;
	}

	Dictionary LTECompositionPatchHelper::ensure_keyframe_id(const Dictionary& keyframe) const {
		Dictionary normalized = keyframe.duplicate(true);
		if (String(normalized.get("id", String())).is_empty()) {
			normalized["id"] = Utils::uuid(Utils::UUID_V7);
		}
		return normalized;
	}

	Dictionary LTECompositionPatchHelper::ensure_modifier_id(const Dictionary& modifier) const {
		Dictionary normalized = modifier.duplicate(true);
		if (String(normalized.get("id", String())).is_empty()) {
			normalized["id"] = Utils::uuid(Utils::UUID_V7);
		}
		return normalized;
	}

	int32_t LTECompositionPatchHelper::find_property_track_index(const Array& property_tracks, const String& property_name) const {
		for (int32_t i = 0; i < property_tracks.size(); ++i) {
			if (property_tracks[i].get_type() != Variant::DICTIONARY) continue;
			if (String(Dictionary(property_tracks[i]).get("property", String())) == property_name) return i;
		}
		return -1;
	}

	int32_t LTECompositionPatchHelper::find_property_keyframe_index_for_ref(const Array& keyframes, const Dictionary& key_ref) const {
		for (int32_t i = 0; i < keyframes.size(); ++i) {
			if (keyframes[i].get_type() != Variant::DICTIONARY) continue;
			if (_keyframe_matches_ref(Dictionary(keyframes[i]), key_ref)) return i;
		}
		return -1;
	}

	Dictionary LTECompositionPatchHelper::find_matching_key_ref(const Array& refs, const Dictionary& key_ref) const {
		for (int32_t i = 0; i < refs.size(); ++i) {
			if (refs[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary ref = refs[i];
			String ref_kind = String(ref.get("kind", String()));
			String key_ref_kind = String(key_ref.get("kind", String()));
			if (!ref_kind.is_empty() && !key_ref_kind.is_empty() && ref_kind != key_ref_kind) continue;
			if (String(ref.get("layer_id", String())) != String(key_ref.get("layer_id", String()))) continue;
			if (String(ref.get("property", String())) != String(key_ref.get("property", String()))) continue;
			if (!key_ref_scope_matches(ref, key_ref)) continue;
			String ref_id = String(ref.get("id", String()));
			if (!ref_id.is_empty() && ref_id == String(key_ref.get("id", String()))) return ref;
			if (std::abs(static_cast<double>(ref.get("time", 0.0)) - static_cast<double>(key_ref.get("time", 0.0))) <= 0.0001) return ref;
		}
		return Dictionary();
	}

	bool LTECompositionPatchHelper::key_ref_scope_matches(const Dictionary& ref, const Dictionary& key_ref) const {
		String kind = String(ref.get("kind", key_ref.get("kind", String())));
		if (kind == "property_modifier_key") {
			String ref_mt = String(ref.get("modifier_type", String()));
			String key_ref_mt = String(key_ref.get("modifier_type", String()));
			if (ref_mt.is_empty() || key_ref_mt.is_empty()) {
				return !String(ref.get("id", String())).is_empty() && !String(key_ref.get("id", String())).is_empty();
			}
			return ref_mt == key_ref_mt;
		}
		if (kind == "sequence_layer_key" && ref.has("sequence_lane") && key_ref.has("sequence_lane")) {
			return static_cast<int32_t>(ref.get("sequence_lane", 0)) == static_cast<int32_t>(key_ref.get("sequence_lane", 0));
		}
		return true;
	}

	Array LTECompositionPatchHelper::merge_key_refs(const Array& first_refs, const Array& second_refs) const {
		Array refs;
		for (int32_t i = 0; i < first_refs.size(); ++i) {
			if (first_refs[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary kr = first_refs[i];
			if (!has_matching_key_ref(refs, kr)) refs.append(kr.duplicate(true));
		}
		for (int32_t i = 0; i < second_refs.size(); ++i) {
			if (second_refs[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary kr = second_refs[i];
			if (!has_matching_key_ref(refs, kr)) refs.append(kr.duplicate(true));
		}
		return refs;
	}

	bool LTECompositionPatchHelper::has_matching_key_ref(const Array& refs, const Dictionary& key_ref) const {
		return !find_matching_key_ref(refs, key_ref).is_empty();
	}

	bool LTECompositionPatchHelper::sort_sequence_clips(const Dictionary& first_clip, const Dictionary& second_clip) const {
		double ft = static_cast<double>(first_clip.get("start_time", 0.0));
		double st = static_cast<double>(second_clip.get("start_time", 0.0));
		if (std::abs(ft - st) > 0.0001) return ft < st;
		return String(first_clip.get("id", String())) < String(second_clip.get("id", String()));
	}

	bool LTECompositionPatchHelper::sort_keyframes(const Dictionary& first_keyframe, const Dictionary& second_keyframe) const {
		double ft = static_cast<double>(first_keyframe.get("time", 0.0));
		double st = static_cast<double>(second_keyframe.get("time", 0.0));
		if (std::abs(ft - st) > 0.0001) return ft < st;
		return String(first_keyframe.get("id", String())) < String(second_keyframe.get("id", String()));
	}

	bool LTECompositionPatchHelper::sort_modifiers(const Dictionary& first_modifier, const Dictionary& second_modifier) const {
		double fs = static_cast<double>(first_modifier.get("start_time", 0.0));
		double ss = static_cast<double>(second_modifier.get("start_time", 0.0));
		if (std::abs(fs - ss) > 0.0001) return fs < ss;
		return String(first_modifier.get("id", String())) < String(second_modifier.get("id", String()));
	}

	bool LTECompositionPatchHelper::sort_property_tracks(const Dictionary& first_track, const Dictionary& second_track) const {
		String fp = String(first_track.get("property", String()));
		String sp = String(second_track.get("property", String()));
		return fp < sp;
	}

	String LTECompositionPatchHelper::normalize_path(const String& path) const {
		if (path.is_empty()) return String();
		String np = path.replace("\\", "/");
		LTEProjectManager* pm = LTEProjectManager::get_singleton();
		String root = pm != nullptr ? pm->get_project_path() : String();
		if (np.begins_with("/") && !root.is_empty()) {
			np = root + np;
		} else if (!np.is_absolute_path() && !root.is_empty()) {
			np = root.path_join(np);
		}
		return np.replace("\\", "/").simplify_path();
	}

	bool LTECompositionPatchHelper::is_same_path(const String& first_path, const String& second_path) const {
		String nf = normalize_path(first_path);
		if (nf.is_empty()) return second_path.is_empty();
		return nf == normalize_path(second_path);
	}
}
