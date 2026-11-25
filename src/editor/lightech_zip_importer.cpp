#include "lightech_zip_importer.h"

#include "lightech_zip_resource.h"
#include "lightech_zip_reader.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	void LightechZipImporter::_bind_methods() {
	}

	String LightechZipImporter::_get_importer_name() const {
		return "lightech.zip_package";
	}

	String LightechZipImporter::_get_visible_name() const {
		return "Lightech Zip Package";
	}

	PackedStringArray LightechZipImporter::_get_recognized_extensions() const {
		PackedStringArray extensions;
		extensions.append("ltz");
		return extensions;
	}

	String LightechZipImporter::_get_save_extension() const {
		return "res";
	}

	String LightechZipImporter::_get_resource_type() const {
		return "LightechZipResource";
	}

	int32_t LightechZipImporter::_get_preset_count() const {
		return 1;
	}

	String LightechZipImporter::_get_preset_name(int32_t p_preset_index) const {
		return "Default";
	}

	TypedArray<Dictionary> LightechZipImporter::_get_import_options(const String& p_path, int32_t p_preset_index) const {
		TypedArray<Dictionary> options;
		return options;
	}

	bool LightechZipImporter::_get_option_visibility(const String& p_path, const StringName& p_option_name, const Dictionary& p_options) const {
		return true;
	}

	Error LightechZipImporter::_import(const String& p_source_file, const String& p_save_path, const Dictionary& p_options, const TypedArray<String>& p_platform_variants, const TypedArray<String>& p_gen_files) const {
		Ref<LightechZipReader> validation = LightechZipReader::open(p_source_file);
		if (validation.is_null()) {
			ERR_PRINT(vformat("Invalid Lightech Zip package: %s", p_source_file));
			return ERR_FILE_CORRUPT;
		}

		Ref<FileAccess> file = FileAccess::open(p_source_file, FileAccess::READ);
		if (file.is_null()) {
			Error error = FileAccess::get_open_error();
			ERR_PRINT(vformat("Cannot open Lightech Zip package: %s. Error Code: %d", p_source_file, error));
			return error;
		}
		PackedByteArray archive_data = file->get_buffer(file->get_length());
		file->close();
		if (archive_data.is_empty()) {
			ERR_PRINT(vformat("Lightech Zip package is empty: %s", p_source_file));
			return ERR_FILE_CORRUPT;
		}

		Ref<LightechZipResource> resource;
		resource.instantiate();
		resource->set_data(archive_data);
		resource->set_source_path(p_source_file);
		String save_path = p_save_path + String(".") + _get_save_extension();
		Error error = ResourceSaver::get_singleton()->save(resource, save_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to save Lightech Zip resource: %s. Error Code: %d", save_path, error));
			return error;
		}
		return OK;
	}
}
