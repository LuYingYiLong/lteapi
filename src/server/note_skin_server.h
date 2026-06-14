#ifndef NOTE_SKIN_SERVER_H
#define NOTE_SKIN_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	class LTENoteSkinServer : public Object {
		GDCLASS(LTENoteSkinServer, Object)

	private:
		static LTENoteSkinServer* singleton;

		Dictionary skin_revisions;

		Dictionary _normalize_manifest(const Dictionary& manifest) const;
		Array _normalize_targets(const Variant& value) const;
		Dictionary _normalize_textures(const Variant& value) const;
		Dictionary _normalize_texture_entry(const String& texture_path, const Variant& value) const;
		Dictionary _normalize_sounds(const Variant& value) const;
		Dictionary _normalize_sound_entry(const String& sound_path, const Variant& value) const;
		Dictionary _normalize_hit_effects(const Variant& value) const;
		Dictionary _normalize_hit_effect(const String& effect_id, const Variant& value) const;
		Array _normalize_hit_effect_layers(const Variant& value) const;
		Dictionary _normalize_hit_effect_layer(const Dictionary& layer, const int index) const;
		Dictionary _normalize_hit_sounds(const Variant& value) const;
		Dictionary _normalize_hit_sound(const String& sound_id, const Variant& value) const;
		Dictionary _normalize_hit_events(const Variant& value) const;
		Dictionary _normalize_hit_event(const String& event_id, const Variant& value) const;
		Dictionary _normalize_note_structures(const Variant& value, const Variant& fallback = Variant()) const;
		Dictionary _normalize_note_structure(const Variant& value) const;
		Dictionary _normalize_parts(const Variant& value) const;
		Dictionary _normalize_part(const String& id, const Variant& value) const;
		Array _normalize_layers(const Variant& value) const;
		Dictionary _normalize_layer(const Dictionary& layer, const int index) const;
		Dictionary _normalize_designer_config(const Dictionary& config) const;
		Dictionary _make_default_note_structure() const;
		Dictionary _make_default_part(const String& id, const String& name, const String& role, const double height, const double time_anchor_y) const;
		Dictionary _make_default_layer(const String& id, const String& name, const bool recolor) const;
		Dictionary _make_default_hit_effects() const;
		Dictionary _make_default_hit_sounds() const;
		Dictionary _make_default_hit_events() const;
		String _normalize_view_type(const String& view_type) const;
		bool _is_safe_zip_path(const String& file_path) const;
		bool _is_texture_file(const String& file_path) const;
		bool _is_sound_file(const String& file_path) const;
		PackedByteArray _read_file_bytes(const String& file_path) const;
		PackedByteArray _read_skin_archive_bytes(const String& skin_path) const;
		String _get_skin_archive_reader_path(const String& skin_path) const;
		PackedStringArray _read_zip_file_list(const String& skin_path) const;
		String _make_unique_zip_path(const String& preferred_path, const PackedStringArray& existing_files) const;
		Error _rewrite_skin_archive(const String& skin_path, const Dictionary& files) const;
		void _mark_skin_file_changed(const String& normalized_skin_path);

	protected:
		static void _bind_methods();

	public:
		LTENoteSkinServer();
		~LTENoteSkinServer();

		static LTENoteSkinServer* get_singleton();

		void clear_project_state();
		String normalize_skin_path(const String& skin_path) const;
		String get_default_skin_path() const;
		void set_default_skin_path(const String& skin_path) const;
		Dictionary make_default_skin_manifest(const Dictionary& metadata = Dictionary()) const;
		Error create_skin_file(const String& skin_path, const Dictionary& manifest, const bool scan_file_system = true);
		Error save_skin_manifest(const String& skin_path, const Dictionary& manifest, const PackedStringArray& removed_files = PackedStringArray());
		int64_t get_skin_revision(const String& skin_path) const;
		Dictionary load_skin_manifest(const String& skin_path) const;
		PackedByteArray read_skin_file(const String& skin_path, const String& file_path) const;
		PackedStringArray get_skin_file_list(const String& skin_path) const;
		PackedStringArray get_skin_texture_files(const String& skin_path) const;
		PackedStringArray get_skin_sound_files(const String& skin_path) const;
		String import_skin_texture(const String& skin_path, const String& source_path);
		String rename_skin_texture(const String& skin_path, const String& old_texture_path, const String& new_texture_path);
		String import_skin_sound(const String& skin_path, const String& source_path);
		String rename_skin_sound(const String& skin_path, const String& old_sound_path, const String& new_sound_path);
		void request_open_skin(const String& runtime_uuid, const String& skin_path);
		Dictionary get_designer_config(const String& runtime_uuid) const;
		void save_designer_config(const String& runtime_uuid, const Dictionary& config);
		void set_designer_view_type(const String& runtime_uuid, const String& view_type);
	};
}

#endif
