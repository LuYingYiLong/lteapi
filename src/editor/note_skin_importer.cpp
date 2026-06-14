#include "note_skin_importer.h"

#include "note_skin_resource.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	static const char* DEFAULT_SKIN_PROJECT_SETTING = "lightech/note_skin/default_skin";

	static void append_import_option(TypedArray<Dictionary>& options, const String& name, const Variant& default_value, PropertyHint hint = PROPERTY_HINT_NONE, const String& hint_string = String()) {
		Dictionary option;
		option["name"] = name;
		option["default_value"] = default_value;
		option["property_hint"] = hint;
		option["hint_string"] = hint_string;
		option["usage"] = PROPERTY_USAGE_DEFAULT;
		options.append(option);
	}

	void LTENoteSkinImporter::_bind_methods() {
	}

	String LTENoteSkinImporter::_get_importer_name() const {
		return "lightech.note_skin";
	}

	String LTENoteSkinImporter::_get_visible_name() const {
		return "Lightech Note Skin";
	}

	PackedStringArray LTENoteSkinImporter::_get_recognized_extensions() const {
		PackedStringArray extensions;
		extensions.append("lteskin");
		return extensions;
	}

	String LTENoteSkinImporter::_get_save_extension() const {
		return "res";
	}

	String LTENoteSkinImporter::_get_resource_type() const {
		return "LTENoteSkinResource";
	}

	int32_t LTENoteSkinImporter::_get_preset_count() const {
		return 1;
	}

	String LTENoteSkinImporter::_get_preset_name(int32_t p_preset_index) const {
		return "Default";
	}

	TypedArray<Dictionary> LTENoteSkinImporter::_get_import_options(const String& p_path, int32_t p_preset_index) const {
		TypedArray<Dictionary> options;
		append_import_option(options, "set_as_default_skin", true);
		return options;
	}

	bool LTENoteSkinImporter::_get_option_visibility(const String& p_path, const StringName& p_option_name, const Dictionary& p_options) const {
		return true;
	}

	Error LTENoteSkinImporter::_import(const String& p_source_file, const String& p_save_path, const Dictionary& p_options, const TypedArray<String>& p_platform_variants, const TypedArray<String>& p_gen_files) const {
		Ref<FileAccess> file = FileAccess::open(p_source_file, FileAccess::READ);
		if (file.is_null()) {
			Error error = FileAccess::get_open_error();
			ERR_PRINT(vformat("Cannot open note skin file: %s. Error Code: %d", p_source_file, error));
			return error;
		}

		PackedByteArray skin_data = file->get_buffer(file->get_length());
		file->close();
		if (skin_data.is_empty()) {
			ERR_PRINT(vformat("Note skin file is empty: %s", p_source_file));
			return ERR_FILE_CORRUPT;
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(p_source_file);
		if (error != OK) {
			ERR_PRINT(vformat("Invalid note skin archive: %s. Error Code: %d", p_source_file, error));
			return error;
		}
		if (!reader->file_exists("manifest.json")) {
			reader->close();
			ERR_PRINT(vformat("Note skin archive has no manifest.json: %s", p_source_file));
			return ERR_FILE_CORRUPT;
		}
		PackedByteArray manifest_data = reader->read_file("manifest.json");
		reader->close();
		Variant parsed_manifest = JSON::parse_string(manifest_data.get_string_from_utf8());
		if (parsed_manifest.get_type() != Variant::DICTIONARY) {
			ERR_PRINT(vformat("Note skin manifest is not a dictionary: %s", p_source_file));
			return ERR_FILE_CORRUPT;
		}

		Ref<LTENoteSkinResource> skin_resource;
		skin_resource.instantiate();
		skin_resource->set_data(skin_data);

		String save_path = p_save_path + String(".") + _get_save_extension();
		error = ResourceSaver::get_singleton()->save(skin_resource, save_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to save note skin resource: %s. Error Code: %d", save_path, error));
			return error;
		}

		bool set_as_default_skin = bool(p_options.get("set_as_default_skin", true));
		if (set_as_default_skin) {
			ProjectSettings* project_settings = ProjectSettings::get_singleton();
			if (project_settings) {
				String source_path = p_source_file.replace("\\", "/").strip_edges().simplify_path();
				project_settings->set_setting(DEFAULT_SKIN_PROJECT_SETTING, source_path);
				project_settings->save();
			}
		}

		return OK;
	}
}
