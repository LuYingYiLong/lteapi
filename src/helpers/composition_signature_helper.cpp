#include "composition_signature_helper.h"

#include <algorithm>
#include <cmath>

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/translation_server.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTECompositionSignatureHelper* LTECompositionSignatureHelper::singleton = nullptr;

	void LTECompositionSignatureHelper::_bind_methods() {
		ClassDB::bind_method(D_METHOD("make_track_structure_signature", "layers"), &LTECompositionSignatureHelper::make_track_structure_signature);
		ClassDB::bind_method(D_METHOD("make_track_key_signatures", "layers"), &LTECompositionSignatureHelper::make_track_key_signatures);
		ClassDB::bind_method(D_METHOD("make_sequence_layer_clip_key_signature", "layer", "sequence_lane"), &LTECompositionSignatureHelper::make_sequence_layer_clip_key_signature);
		ClassDB::bind_method(D_METHOD("make_property_keyframe_track_key_signature", "layer", "property_name"), &LTECompositionSignatureHelper::make_property_keyframe_track_key_signature);
		ClassDB::bind_method(D_METHOD("make_property_modifier_track_key_signature", "layer", "property_name", "modifier_type"), &LTECompositionSignatureHelper::make_property_modifier_track_key_signature);
		ClassDB::bind_method(D_METHOD("make_keyframe_signature_data", "keyframe"), &LTECompositionSignatureHelper::make_keyframe_signature_data);
		ClassDB::bind_method(D_METHOD("make_modifier_signature_data", "modifier"), &LTECompositionSignatureHelper::make_modifier_signature_data);
		ClassDB::bind_method(D_METHOD("get_changed_property_key_track_indices", "old_signatures", "new_signatures", "track_entries"), &LTECompositionSignatureHelper::get_changed_property_key_track_indices);
		ClassDB::bind_method(D_METHOD("get_incomplete_timeline_key_track_indices", "layers", "track_entries", "timeline_panel"), &LTECompositionSignatureHelper::get_incomplete_timeline_key_track_indices);
		ClassDB::bind_method(D_METHOD("update_track_key_signatures_from_refs", "signatures", "key_refs", "layers", "track_entries"), &LTECompositionSignatureHelper::update_track_key_signatures_from_refs);
		ClassDB::bind_method(D_METHOD("sort_keyframe_signature_data", "first_keyframe", "second_keyframe"), &LTECompositionSignatureHelper::sort_keyframe_signature_data);
		ClassDB::bind_method(D_METHOD("sort_modifier_signature_data", "first_modifier", "second_modifier"), &LTECompositionSignatureHelper::sort_modifier_signature_data);
	}

	LTECompositionSignatureHelper::LTECompositionSignatureHelper() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTECompositionSignatureHelper::~LTECompositionSignatureHelper() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTECompositionSignatureHelper* LTECompositionSignatureHelper::get_singleton() {
		return singleton;
	}

	Dictionary LTECompositionSignatureHelper::_make_keyframe_signature_data(const Dictionary& keyframe) const {
		Dictionary result;
		result["id"] = String(keyframe.get("id", String()));
		result["value"] = keyframe.get("value", Variant());
		result["time"] = UtilityFunctions::snappedf(static_cast<double>(keyframe.get("time", 0.0)), 0.0001);
		result["interpolation"] = String(keyframe.get("interpolation", "linear"));
		if (keyframe.has("interpolation_options")) {
			result["interpolation_options"] = keyframe.get("interpolation_options", Dictionary());
		}
		if (keyframe.has("in_handle")) {
			result["in_handle"] = keyframe.get("in_handle", Dictionary());
		}
		if (keyframe.has("out_handle")) {
			result["out_handle"] = keyframe.get("out_handle", Dictionary());
		}
		if (keyframe.has("component_handles")) {
			result["component_handles"] = keyframe.get("component_handles", Dictionary());
		}
		return result;
	}

	Dictionary LTECompositionSignatureHelper::_make_modifier_signature_data(const Dictionary& modifier) const {
		Dictionary result;
		result["id"] = String(modifier.get("id", String()));
		result["type"] = String(modifier.get("type", String()));
		result["name"] = String(modifier.get("name", String()));
		result["graph"] = String(modifier.get("graph", String()));
		result["start_time"] = UtilityFunctions::snappedf(static_cast<double>(modifier.get("start_time", 0.0)), 0.0001);
		result["end_time"] = UtilityFunctions::snappedf(static_cast<double>(modifier.get("end_time", 0.0)), 0.0001);
		result["enabled"] = static_cast<bool>(modifier.get("enabled", true));
		result["amplitude"] = modifier.get("amplitude", Array());
		result["frequency"] = static_cast<double>(modifier.get("frequency", 0.0));
		result["seed"] = static_cast<int64_t>(modifier.get("seed", 0));
		result["bpm"] = static_cast<double>(modifier.get("bpm", 0.0));
		result["beat_interval"] = static_cast<double>(modifier.get("beat_interval", 0.0));
		result["decay"] = static_cast<double>(modifier.get("decay", 0.0));
		result["phase"] = static_cast<double>(modifier.get("phase", 0.0));
		result["fade_in"] = static_cast<double>(modifier.get("fade_in", 0.0));
		result["fade_out"] = static_cast<double>(modifier.get("fade_out", 0.0));
		return result;
	}

	Dictionary LTECompositionSignatureHelper::_find_layer_property_track(const Dictionary& layer, const String& property_name) const {
		Array property_tracks = layer.get("property_tracks", Array());
		for (int32_t i = 0; i < property_tracks.size(); ++i) {
			if (property_tracks[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary property_track = property_tracks[i];
			if (String(property_track.get("property", String())) == property_name) {
				return property_track;
			}
		}
		return Dictionary();
	}

	Array LTECompositionSignatureHelper::_get_layer_property_track_names(const Dictionary& layer) const {
		Array property_names;
		Array property_tracks = layer.get("property_tracks", Array());
		for (int32_t i = 0; i < property_tracks.size(); ++i) {
			if (property_tracks[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary property_track = property_tracks[i];
			String property_name = String(property_track.get("property", String()));
			if (!property_name.is_empty() && !property_names.has(property_name)) {
				property_names.append(property_name);
			}
		}
		return property_names;
	}

	Array LTECompositionSignatureHelper::_get_property_modifier_track_types(const Dictionary& layer, const String& property_name) const {
		Dictionary used_modifier_types;
		Dictionary property_track = _find_layer_property_track(layer, property_name);
		Array modifiers = property_track.get("modifiers", Array());
		for (int32_t i = 0; i < modifiers.size(); ++i) {
			if (modifiers[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary modifier = modifiers[i];
			String modifier_type = String(modifier.get("type", String()));
			if (modifier_type.is_empty()) {
				continue;
			}
			used_modifier_types[modifier_type] = true;
		}
		Array sorted_types = used_modifier_types.keys();
		sorted_types.sort();
		return sorted_types;
	}

	int32_t LTECompositionSignatureHelper::_get_sequence_clip_lane_count(const Dictionary& layer) const {
		int32_t max_lane = -1;
		Array clips = _get_sequence_layer_clips_with_lanes(layer);
		for (int32_t i = 0; i < clips.size(); ++i) {
			if (clips[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary clip = clips[i];
			int32_t lane = static_cast<int32_t>(clip.get("lane", 0));
			if (lane > max_lane) {
				max_lane = lane;
			}
		}
		return max_lane + 1;
	}

	Array LTECompositionSignatureHelper::_get_sequence_layer_clips_with_lanes(const Dictionary& layer) const {
		Array result;
		Array clips = layer.get("clips", Array());
		for (int32_t i = 0; i < clips.size(); ++i) {
			if (clips[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary clip = clips[i];
			if (!clip.has("lane")) {
				clip["lane"] = 0;
			}
			result.append(clip);
		}
		return result;
	}

	bool LTECompositionSignatureHelper::_is_supported_timeline_layer(const Dictionary& layer) const {
		String type = String(layer.get("type", String()));
		return type == "image" || type == "texture" || type == "text" ||
			type == "color_rect" || type == "camera" || type == "shader_layer" ||
			type == "lt_track" || type == "sequence";
	}

	String LTECompositionSignatureHelper::_get_property_track_display_name(const String& property_name) const {
		if (property_name == "position" || property_name == "offset") return "Position X";
		if (property_name == "width") return "Width";
		if (property_name == "color") return "Color";
		if (property_name == "rotation") return "Rotation";
		if (property_name == "scale") return "Scale";
		if (property_name == "opacity") return "Opacity";
		if (property_name == "visible_ratio") return "Visible Ratio";
		if (property_name == "font_size") return "Font Size";
		if (property_name == "size") return "Size";
		return property_name.capitalize();
	}

	String LTECompositionSignatureHelper::_get_modifier_type_display_name(const String& modifier_type) const {
		TranslationServer* ts = TranslationServer::get_singleton();
		if (modifier_type == "shake") {
			return ts != nullptr ? ts->translate("SHAKE_MODIFIER_NAME") : "Shake";
		}
		if (modifier_type == "beat_pulse") {
			return ts != nullptr ? ts->translate("BEAT_PULSE_MODIFIER_NAME") : "Beat Pulse";
		}
		return modifier_type.capitalize();
	}

	String LTECompositionSignatureHelper::_get_layer_type_display_name(const Dictionary& layer) const {
		String type = String(layer.get("type", String()));
		if (type == "image" || type == "texture") return "Image Layer";
		if (type == "color_rect") return "Color Rect";
		if (type == "camera") return "Camera";
		if (type == "shader_layer") return "Shader Layer";
		if (type == "lt_track") return "LT Track";
		if (type == "sequence") return "Sequence";
		return "Text Layer";
	}

	String LTECompositionSignatureHelper::make_track_structure_signature(const Array& layers) const {
		Array snapshot;
		for (int32_t layer_index = 0; layer_index < layers.size(); ++layer_index) {
			if (layers[layer_index].get_type() != Variant::DICTIONARY) continue;
			Dictionary layer = layers[layer_index];
			if (!_is_supported_timeline_layer(layer)) continue;
			String layer_id = String(layer.get("id", String()));
			if (layer_id.is_empty()) continue;
			Array property_names = _get_layer_property_track_names(layer);
			String layer_type = String(layer.get("type", String()));
			if (property_names.is_empty() && layer_type != "sequence") continue;
			String layer_name = String(layer.get("name", _get_layer_type_display_name(layer)));

			Dictionary layer_entry;
			layer_entry["kind"] = "layer";
			layer_entry["layer_id"] = layer_id;
			layer_entry["layer_index"] = layer_index;
			layer_entry["name"] = layer_name;
			layer_entry["type"] = layer_type;
			layer_entry["properties"] = property_names;
			snapshot.append(layer_entry);

			if (layer_type == "sequence") {
				int32_t lane_count = _get_sequence_clip_lane_count(layer);
				for (int32_t lane = 0; lane < lane_count; ++lane) {
					Dictionary seq_entry;
					seq_entry["kind"] = "sequence";
					seq_entry["layer_id"] = layer_id;
					seq_entry["layer_index"] = layer_index;
					seq_entry["property"] = "__sequence_clip";
					seq_entry["sequence_lane"] = lane;
					TranslationServer* ts = TranslationServer::get_singleton();
					String clip_name = ts != nullptr ? ts->translate("SEQUENCE_CLIP_NAME") : "Sequence Clip";
					seq_entry["name"] = vformat("%s %d", clip_name, lane + 1);
					snapshot.append(seq_entry);
				}
			}

			for (int32_t p = 0; p < property_names.size(); ++p) {
				String property_name = property_names[p];
				Dictionary prop_entry;
				prop_entry["kind"] = "property";
				prop_entry["layer_id"] = layer_id;
				prop_entry["layer_index"] = layer_index;
				prop_entry["property"] = property_name;
				prop_entry["name"] = _get_property_track_display_name(property_name);
				snapshot.append(prop_entry);

				Array modifier_types = _get_property_modifier_track_types(layer, property_name);
				for (int32_t m = 0; m < modifier_types.size(); ++m) {
					String modifier_type = modifier_types[m];
					Dictionary mod_entry;
					mod_entry["kind"] = "modifier";
					mod_entry["layer_id"] = layer_id;
					mod_entry["layer_index"] = layer_index;
					mod_entry["property"] = property_name;
					mod_entry["modifier_type"] = modifier_type;
					mod_entry["name"] = _get_modifier_type_display_name(modifier_type);
					snapshot.append(mod_entry);
				}
			}
		}
		if (snapshot.is_empty()) return String();
		return JSON::stringify(snapshot);
	}

	Dictionary LTECompositionSignatureHelper::make_track_key_signatures(const Array& layers) {
		Dictionary signatures;
		for (int32_t layer_index = 0; layer_index < layers.size(); ++layer_index) {
			if (layers[layer_index].get_type() != Variant::DICTIONARY) continue;
			Dictionary layer = layers[layer_index];
			if (!_is_supported_timeline_layer(layer)) continue;
			String layer_id = String(layer.get("id", String()));
			if (layer_id.is_empty()) continue;
			String layer_type = String(layer.get("type", String()));

			if (layer_type == "sequence") {
				int32_t lane_count = _get_sequence_clip_lane_count(layer);
				for (int32_t lane = 0; lane < lane_count; ++lane) {
					String key = layer_id + "\n__sequence_clip\nsequence\n" + String::num_int64(lane);
					signatures[key] = _make_sequence_layer_clip_key_signature(layer, lane);
				}
			}

			Array property_names = _get_layer_property_track_names(layer);
			for (int32_t p = 0; p < property_names.size(); ++p) {
				String property_name = property_names[p];
				String key = layer_id + "\n" + property_name + "\nproperty\n";
				signatures[key] = _make_property_keyframe_track_key_signature(layer, property_name);

				Array modifier_types = _get_property_modifier_track_types(layer, property_name);
				for (int32_t m = 0; m < modifier_types.size(); ++m) {
					String modifier_type = modifier_types[m];
					String mod_key = layer_id + "\n" + property_name + "\nmodifier\n" + modifier_type;
					signatures[mod_key] = _make_property_modifier_track_key_signature(layer, property_name, modifier_type);
				}
			}
		}
		return signatures;
	}

	String LTECompositionSignatureHelper::make_sequence_layer_clip_key_signature(const Dictionary& layer, const int32_t sequence_lane) const {
		return _make_sequence_layer_clip_key_signature(layer, sequence_lane);
	}

	String LTECompositionSignatureHelper::_make_sequence_layer_clip_key_signature(const Dictionary& layer, const int32_t sequence_lane) const {
		Array sequence_clips;
		Array all_clips = _get_sequence_layer_clips_with_lanes(layer);
		for (int32_t i = 0; i < all_clips.size(); ++i) {
			if (all_clips[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary clip = all_clips[i];
			if (static_cast<int32_t>(clip.get("lane", 0)) != sequence_lane) continue;
			Dictionary clip_data;
			clip_data["id"] = String(clip.get("id", String()));
			clip_data["start_time"] = UtilityFunctions::snappedf(static_cast<double>(clip.get("start_time", 0.0)), 0.0001);
			clip_data["duration"] = UtilityFunctions::snappedf(static_cast<double>(clip.get("duration", 0.001)), 0.0001);
			clip_data["time_offset"] = UtilityFunctions::snappedf(static_cast<double>(clip.get("time_offset", 0.0)), 0.0001);
			clip_data["time_scale"] = UtilityFunctions::snappedf(static_cast<double>(clip.get("time_scale", 1.0)), 0.0001);
			clip_data["lane"] = static_cast<int32_t>(clip.get("lane", 0));
			sequence_clips.append(clip_data);
		}
		Dictionary payload;
		payload["locked"] = static_cast<bool>(layer.get("locked", false));
		payload["clips"] = sequence_clips;
		payload["sequence_path"] = String(layer.get("sequence_path", String()));
		return JSON::stringify(payload);
	}

	String LTECompositionSignatureHelper::make_property_keyframe_track_key_signature(const Dictionary& layer, const String& property_name) {
		return _make_property_keyframe_track_key_signature(layer, property_name);
	}

	String LTECompositionSignatureHelper::_make_property_keyframe_track_key_signature(const Dictionary& layer, const String& property_name) {
		Array keyframes;
		Dictionary property_track = _find_layer_property_track(layer, property_name);
		Array raw_keyframes = property_track.get("keyframes", Array());
		for (int32_t i = 0; i < raw_keyframes.size(); ++i) {
			if (raw_keyframes[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary keyframe = raw_keyframes[i];
			keyframes.append(_make_keyframe_signature_data(keyframe));
		}
		keyframes.sort_custom(Callable(this, "sort_keyframe_signature_data"));
		Dictionary payload;
		payload["locked"] = static_cast<bool>(layer.get("locked", false));
		payload["keyframes"] = keyframes;
		return JSON::stringify(payload);
	}

	String LTECompositionSignatureHelper::make_property_modifier_track_key_signature(const Dictionary& layer, const String& property_name, const String& modifier_type) {
		return _make_property_modifier_track_key_signature(layer, property_name, modifier_type);
	}

	String LTECompositionSignatureHelper::_make_property_modifier_track_key_signature(const Dictionary& layer, const String& property_name, const String& modifier_type) {
		Array modifiers;
		Dictionary property_track = _find_layer_property_track(layer, property_name);
		Array raw_modifiers = property_track.get("modifiers", Array());
		for (int32_t i = 0; i < raw_modifiers.size(); ++i) {
			if (raw_modifiers[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary modifier = raw_modifiers[i];
			if (String(modifier.get("type", String())) != modifier_type) continue;
			modifiers.append(_make_modifier_signature_data(modifier));
		}
		modifiers.sort_custom(Callable(this, "sort_modifier_signature_data"));
		Dictionary payload;
		payload["locked"] = static_cast<bool>(layer.get("locked", false));
		payload["modifiers"] = modifiers;
		return JSON::stringify(payload);
	}

	Dictionary LTECompositionSignatureHelper::make_keyframe_signature_data(const Dictionary& keyframe) const {
		return _make_keyframe_signature_data(keyframe);
	}

	Dictionary LTECompositionSignatureHelper::make_modifier_signature_data(const Dictionary& modifier) const {
		return _make_modifier_signature_data(modifier);
	}

	Array LTECompositionSignatureHelper::get_changed_property_key_track_indices(const Dictionary& old_signatures, const Dictionary& new_signatures, const Array& track_entries) const {
		Array track_indices;
		for (int32_t track_index = 0; track_index < track_entries.size(); ++track_index) {
			if (track_entries[track_index].get_type() != Variant::DICTIONARY) continue;
			Dictionary entry = track_entries[track_index];
			String kind = String(entry.get("kind", String()));
			if (kind != "sequence" && kind != "property" && kind != "modifier") continue;
			String modifier_type = String(entry.get("modifier_type", String()));
			if (kind == "sequence") {
				modifier_type = String::num_int64(static_cast<int32_t>(entry.get("sequence_lane", 0)));
			}
			String signature_key = String(entry.get("layer_id", String())) + "\n" +
				String(entry.get("property", String())) + "\n" + kind + "\n" + modifier_type;
			String previous_signature = String(old_signatures.get(signature_key, String()));
			String new_signature = String(new_signatures.get(signature_key, String()));
			if (previous_signature != new_signature) {
				track_indices.append(track_index);
			}
		}
		return track_indices;
	}

	Array LTECompositionSignatureHelper::get_incomplete_timeline_key_track_indices(const Array& layers, const Array& track_entries, Object* timeline_panel) const {
		Array track_indices;
		if (timeline_panel == nullptr || !timeline_panel->has_method("get_key_count")) return track_indices;
		for (int32_t track_index = 0; track_index < track_entries.size(); ++track_index) {
			if (track_entries[track_index].get_type() != Variant::DICTIONARY) continue;
			Dictionary entry = track_entries[track_index];
			String kind = String(entry.get("kind", String()));
			if (kind != "sequence" && kind != "property" && kind != "modifier") continue;
			int32_t layer_index = static_cast<int32_t>(entry.get("layer_index", -1));
			if (layer_index < 0 || layer_index >= layers.size() || layers[layer_index].get_type() != Variant::DICTIONARY) continue;
			Dictionary layer = layers[layer_index];
			int32_t expected_count = 0;
			if (kind == "sequence") {
				int32_t lane = static_cast<int32_t>(entry.get("sequence_lane", 0));
				Array clips = _get_sequence_layer_clips_with_lanes(layer);
				for (int32_t i = 0; i < clips.size(); ++i) {
					if (clips[i].get_type() != Variant::DICTIONARY) continue;
					if (static_cast<int32_t>(Dictionary(clips[i]).get("lane", 0)) == lane) ++expected_count;
				}
			} else if (kind == "property") {
				String property_name = String(entry.get("property", String()));
				Dictionary property_track = _find_layer_property_track(layer, property_name);
				Array keyframes = property_track.get("keyframes", Array());
				for (int32_t i = 0; i < keyframes.size(); ++i) {
					if (keyframes[i].get_type() == Variant::DICTIONARY) ++expected_count;
				}
			} else if (kind == "modifier") {
				String property_name = String(entry.get("property", String()));
				String modifier_type = String(entry.get("modifier_type", String()));
				Dictionary property_track = _find_layer_property_track(layer, property_name);
				Array modifiers = property_track.get("modifiers", Array());
				for (int32_t i = 0; i < modifiers.size(); ++i) {
					if (modifiers[i].get_type() != Variant::DICTIONARY) continue;
					if (String(Dictionary(modifiers[i]).get("type", String())) == modifier_type) ++expected_count;
				}
			}
			int32_t actual_count = static_cast<int32_t>(timeline_panel->call("get_key_count", track_index));
			if (expected_count != actual_count) {
				track_indices.append(track_index);
			}
		}
		return track_indices;
	}

	Dictionary LTECompositionSignatureHelper::update_track_key_signatures_from_refs(const Dictionary& signatures, const Array& key_refs, const Array& layers, const Array& track_entries) {
		Dictionary updated_signatures = signatures.duplicate(false);
		for (int32_t i = 0; i < key_refs.size(); ++i) {
			if (key_refs[i].get_type() != Variant::DICTIONARY) continue;
			Dictionary key_ref = key_refs[i];
			String layer_id = String(key_ref.get("layer_id", String()));
			String property_name = String(key_ref.get("property", String()));
			String key_kind = String(key_ref.get("kind", "text_property_key"));
			// Map key_ref kind -> track kind
			String kind;
			if (key_kind == "sequence_layer_key") kind = "sequence";
			else if (key_kind == "property_modifier_key") kind = "modifier";
			else kind = "property";
			String modifier_type = String(key_ref.get("modifier_type", String()));
			if (kind == "sequence") {
				modifier_type = String::num_int64(static_cast<int32_t>(key_ref.get("sequence_lane", 0)));
			}
			String signature_key = layer_id + "\n" + property_name + "\n" + kind + "\n" + modifier_type;

			int32_t layer_index = -1;
			for (int32_t li = 0; li < layers.size(); ++li) {
				if (layers[li].get_type() != Variant::DICTIONARY) continue;
				if (String(Dictionary(layers[li]).get("id", String())) == layer_id) {
					layer_index = li;
					break;
				}
			}
			if (layer_index < 0 || layer_index >= layers.size()) continue;
			Dictionary layer = layers[layer_index];

			if (kind == "sequence") {
				updated_signatures[signature_key] = _make_sequence_layer_clip_key_signature(layer, static_cast<int32_t>(key_ref.get("sequence_lane", 0)));
			} else if (kind == "property") {
				updated_signatures[signature_key] = _make_property_keyframe_track_key_signature(layer, property_name);
			} else if (kind == "modifier") {
				updated_signatures[signature_key] = _make_property_modifier_track_key_signature(layer, property_name, modifier_type);
			}
		}
		return updated_signatures;
	}

	bool LTECompositionSignatureHelper::sort_keyframe_signature_data(const Dictionary& first_keyframe, const Dictionary& second_keyframe) {
		double first_time = static_cast<double>(first_keyframe.get("time", 0.0));
		double second_time = static_cast<double>(second_keyframe.get("time", 0.0));
		if (std::abs(first_time - second_time) > 0.0001) return first_time < second_time;
		return String(first_keyframe.get("id", String())) < String(second_keyframe.get("id", String()));
	}

	bool LTECompositionSignatureHelper::sort_modifier_signature_data(const Dictionary& first_modifier, const Dictionary& second_modifier) {
		double first_start = static_cast<double>(first_modifier.get("start_time", 0.0));
		double second_start = static_cast<double>(second_modifier.get("start_time", 0.0));
		if (std::abs(first_start - second_start) > 0.0001) return first_start < second_start;
		return String(first_modifier.get("id", String())) < String(second_modifier.get("id", String()));
	}
}
