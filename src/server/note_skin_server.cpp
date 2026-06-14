#include "note_skin_server.h"

#include "file_system_server.h"
#include "note_skin_resource.h"
#include "project_manager.h"
#include "settings_config.h"
#include "utils.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_uid.hpp>
#include <godot_cpp/classes/zip_packer.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTENoteSkinServer* LTENoteSkinServer::singleton = nullptr;
	static const char* DEFAULT_SKIN_PROJECT_SETTING = "lightech/note_skin/default_skin";
	static const char* DEFAULT_SKIN_PATH = "res://datas/vanilla_skin.lteskin";

	void LTENoteSkinServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("normalize_skin_path", "skin_path"), &LTENoteSkinServer::normalize_skin_path);
		ClassDB::bind_method(D_METHOD("get_default_skin_path"), &LTENoteSkinServer::get_default_skin_path);
		ClassDB::bind_method(D_METHOD("set_default_skin_path", "skin_path"), &LTENoteSkinServer::set_default_skin_path);
		ClassDB::bind_method(D_METHOD("make_default_skin_manifest", "metadata"), &LTENoteSkinServer::make_default_skin_manifest, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("create_skin_file", "skin_path", "manifest", "scan_file_system"), &LTENoteSkinServer::create_skin_file, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("save_skin_manifest", "skin_path", "manifest", "removed_files"), &LTENoteSkinServer::save_skin_manifest, DEFVAL(PackedStringArray()));
		ClassDB::bind_method(D_METHOD("get_skin_revision", "skin_path"), &LTENoteSkinServer::get_skin_revision);
		ClassDB::bind_method(D_METHOD("load_skin_manifest", "skin_path"), &LTENoteSkinServer::load_skin_manifest);
		ClassDB::bind_method(D_METHOD("read_skin_file", "skin_path", "file_path"), &LTENoteSkinServer::read_skin_file);
		ClassDB::bind_method(D_METHOD("get_skin_file_list", "skin_path"), &LTENoteSkinServer::get_skin_file_list);
		ClassDB::bind_method(D_METHOD("get_skin_texture_files", "skin_path"), &LTENoteSkinServer::get_skin_texture_files);
		ClassDB::bind_method(D_METHOD("get_skin_sound_files", "skin_path"), &LTENoteSkinServer::get_skin_sound_files);
		ClassDB::bind_method(D_METHOD("import_skin_texture", "skin_path", "source_path"), &LTENoteSkinServer::import_skin_texture);
		ClassDB::bind_method(D_METHOD("rename_skin_texture", "skin_path", "old_texture_path", "new_texture_path"), &LTENoteSkinServer::rename_skin_texture);
		ClassDB::bind_method(D_METHOD("import_skin_sound", "skin_path", "source_path"), &LTENoteSkinServer::import_skin_sound);
		ClassDB::bind_method(D_METHOD("rename_skin_sound", "skin_path", "old_sound_path", "new_sound_path"), &LTENoteSkinServer::rename_skin_sound);
		ClassDB::bind_method(D_METHOD("request_open_skin", "runtime_uuid", "skin_path"), &LTENoteSkinServer::request_open_skin);
		ClassDB::bind_method(D_METHOD("get_designer_config", "runtime_uuid"), &LTENoteSkinServer::get_designer_config);
		ClassDB::bind_method(D_METHOD("save_designer_config", "runtime_uuid", "config"), &LTENoteSkinServer::save_designer_config);
		ClassDB::bind_method(D_METHOD("set_designer_view_type", "runtime_uuid", "view_type"), &LTENoteSkinServer::set_designer_view_type);

		ADD_SIGNAL(MethodInfo("skin_open_requested",
				PropertyInfo(Variant::STRING, "runtime_uuid"),
				PropertyInfo(Variant::STRING, "skin_path")));
		ADD_SIGNAL(MethodInfo("skin_file_changed", PropertyInfo(Variant::STRING, "skin_path")));
		ADD_SIGNAL(MethodInfo("designer_config_changed",
				PropertyInfo(Variant::STRING, "runtime_uuid"),
				PropertyInfo(Variant::DICTIONARY, "config")));
	}

	LTENoteSkinServer::LTENoteSkinServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTENoteSkinServer::~LTENoteSkinServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
		skin_revisions.clear();
	}

	LTENoteSkinServer* LTENoteSkinServer::get_singleton() {
		return singleton;
	}

	void LTENoteSkinServer::clear_project_state() {
		skin_revisions.clear();
	}

	void LTENoteSkinServer::_mark_skin_file_changed(const String& normalized_skin_path) {
		if (normalized_skin_path.is_empty()) {
			return;
		}
		const int64_t revision = skin_revisions.get(normalized_skin_path, 0);
		skin_revisions[normalized_skin_path] = revision + 1;
		emit_signal("skin_file_changed", normalized_skin_path);
	}

	String LTENoteSkinServer::normalize_skin_path(const String& skin_path) const {
		String normalized_path = skin_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_path.is_empty()) {
			return String();
		}
		if (normalized_path.begins_with("uid://")) {
			ResourceUID* resource_uid = ResourceUID::get_singleton();
			if (resource_uid) {
				String uid_path = resource_uid->uid_to_path(normalized_path).replace("\\", "/").strip_edges().simplify_path();
				if (!uid_path.is_empty()) {
					normalized_path = uid_path;
				}
			}
			if (normalized_path.begins_with("uid://")) {
				return String();
			}
		}
		if (normalized_path.get_extension().to_lower() != "lteskin") {
			normalized_path = normalized_path.get_basename() + ".lteskin";
		}
		if (normalized_path.begins_with("res://")) {
			return normalized_path;
		}

		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_dir = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (normalized_path.begins_with("/") && !root_dir.is_empty()) {
			return (root_dir + normalized_path).simplify_path();
		}
		if (Utils::is_absolute_path(normalized_path)) {
			return normalized_path;
		}
		if (root_dir.is_empty()) {
			return normalized_path;
		}
		return root_dir.path_join(normalized_path).replace("\\", "/").simplify_path();
	}

	String LTENoteSkinServer::get_default_skin_path() const {
		ProjectSettings* project_settings = ProjectSettings::get_singleton();
		if (!project_settings || !project_settings->has_setting(DEFAULT_SKIN_PROJECT_SETTING)) {
			return String(DEFAULT_SKIN_PATH);
		}
		String skin_path = String(project_settings->get_setting(DEFAULT_SKIN_PROJECT_SETTING));
		skin_path = skin_path.replace("\\", "/").strip_edges().simplify_path();
		if (skin_path.is_empty()) {
			return String(DEFAULT_SKIN_PATH);
		}
		String normalized_path = normalize_skin_path(skin_path);
		return normalized_path.is_empty() ? String(DEFAULT_SKIN_PATH) : normalized_path;
	}

	void LTENoteSkinServer::set_default_skin_path(const String& skin_path) const {
		ProjectSettings* project_settings = ProjectSettings::get_singleton();
		if (!project_settings) {
			return;
		}
		String normalized_path = skin_path.replace("\\", "/").strip_edges().simplify_path();
		project_settings->set_setting(DEFAULT_SKIN_PROJECT_SETTING, normalized_path);
	}

	Dictionary LTENoteSkinServer::make_default_skin_manifest(const Dictionary& metadata) const {
		Dictionary manifest;
		manifest["format"] = "lightech.note_skin";
		manifest["format_version"] = 2;
		manifest["id"] = Utils::uuid(Utils::UUID_V7);
		manifest["name"] = "New Skin";
		manifest["author"] = String();
		manifest["version"] = "1.0";
		manifest["description"] = String();
		manifest["targets"] = _normalize_targets(Array());
		manifest["preview"] = String();
		manifest["textures"] = Dictionary();
		manifest["sounds"] = Dictionary();
		manifest["hit_effects"] = _make_default_hit_effects();
		manifest["hit_sounds"] = _make_default_hit_sounds();
		manifest["hit_events"] = _make_default_hit_events();
		manifest["notes"] = _normalize_note_structures(Dictionary());

		Array keys = metadata.keys();
		for (int index = 0; index < keys.size(); ++index) {
			Variant key = keys[index];
			manifest[key] = metadata.get(key, Variant());
		}
		return _normalize_manifest(manifest);
	}

	Error LTENoteSkinServer::create_skin_file(const String& skin_path, const Dictionary& manifest, const bool scan_file_system) {
		String normalized_path = normalize_skin_path(skin_path);
		if (normalized_path.is_empty()) {
			return ERR_FILE_BAD_PATH;
		}
		if (FileAccess::file_exists(normalized_path)) {
			ERR_PRINT(vformat("Skin file already exists: %s", normalized_path));
			return ERR_ALREADY_EXISTS;
		}

		String dir_path = normalized_path.get_base_dir();
		Ref<DirAccess> dir_access = DirAccess::open(dir_path);
		if (dir_access.is_null()) {
			dir_access = DirAccess::open("user://");
		}
		if (dir_access.is_null()) {
			return ERR_CANT_OPEN;
		}
		Error error = dir_access->make_dir_recursive(dir_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to create skin directory: %s. Error Code: %d", dir_path, error));
			return error;
		}

		Dictionary normalized_manifest = _normalize_manifest(manifest);
		String content = JSON::stringify(normalized_manifest, "\t");
		Ref<ZIPPacker> packer;
		packer.instantiate();
		error = packer->open(normalized_path, ZIPPacker::APPEND_CREATE);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to create skin file: %s. Error Code: %d", normalized_path, error));
			return error;
		}
		error = packer->start_file("manifest.json");
		if (error == OK) {
			error = packer->write_file(content.to_utf8_buffer());
		}
		if (error == OK) {
			error = packer->close_file();
		}
		Error close_error = packer->close();
		if (error == OK) {
			error = close_error;
		}
		if (error != OK) {
			ERR_PRINT(vformat("Failed to write skin manifest: %s. Error Code: %d", normalized_path, error));
			return error;
		}

		_mark_skin_file_changed(normalized_path);
		if (scan_file_system) {
			LTEFileSystemServer* file_system_server = LTEFileSystemServer::get_singleton();
			if (file_system_server) {
				file_system_server->scan_directory_completely();
			}
		}
		return OK;
	}

	Error LTENoteSkinServer::save_skin_manifest(const String& skin_path, const Dictionary& manifest, const PackedStringArray& removed_files) {
		String normalized_path = normalize_skin_path(skin_path);
		if (normalized_path.is_empty()) {
			return ERR_FILE_BAD_PATH;
		}
		if (!FileAccess::file_exists(normalized_path)) {
			ERR_PRINT(vformat("Skin file does not exist: %s", normalized_path));
			return ERR_FILE_NOT_FOUND;
		}

		Dictionary archive_files;
		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(normalized_path);
		if (error == OK) {
			PackedStringArray existing_files = reader->get_files();
			for (int index = 0; index < existing_files.size(); ++index) {
				String file_path = existing_files[index].replace("\\", "/").strip_edges().simplify_path();
				if (file_path.is_empty() || !_is_safe_zip_path(file_path) || removed_files.has(file_path)) {
					continue;
				}
				archive_files[file_path] = reader->read_file(file_path);
			}
			reader->close();
		}

		Dictionary normalized_manifest = _normalize_manifest(manifest);
		archive_files["manifest.json"] = JSON::stringify(normalized_manifest, "\t").to_utf8_buffer();
		error = _rewrite_skin_archive(normalized_path, archive_files);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to save skin manifest: %s. Error Code: %d", normalized_path, error));
			return error;
		}

		_mark_skin_file_changed(normalized_path);
		return OK;
	}

	int64_t LTENoteSkinServer::get_skin_revision(const String& skin_path) const {
		String normalized_path = normalize_skin_path(skin_path);
		if (normalized_path.is_empty()) {
			return 0;
		}
		return skin_revisions.get(normalized_path, 0);
	}

	Dictionary LTENoteSkinServer::load_skin_manifest(const String& skin_path) const {
		String normalized_path = normalize_skin_path(skin_path);
		String reader_path = _get_skin_archive_reader_path(normalized_path);
		if (reader_path.is_empty()) {
			return Dictionary();
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(reader_path);
		if (error == OK && reader->file_exists("manifest.json")) {
			PackedByteArray buffer = reader->read_file("manifest.json");
			reader->close();
			Variant parsed = JSON::parse_string(buffer.get_string_from_utf8());
			if (parsed.get_type() == Variant::DICTIONARY) {
				Dictionary parsed_manifest = parsed;
				Dictionary manifest = _normalize_manifest(parsed_manifest);
				manifest["path"] = normalized_path;
				return manifest;
			}
			return Dictionary();
		}
		if (error == OK) {
			reader->close();
		}

		Ref<FileAccess> file = FileAccess::open(reader_path, FileAccess::READ);
		if (file.is_null()) {
			return Dictionary();
		}
		String text = file->get_as_text();
		file->close();
		Variant parsed = JSON::parse_string(text);
		if (parsed.get_type() != Variant::DICTIONARY) {
			return Dictionary();
		}
		Dictionary parsed_manifest = parsed;
		Dictionary manifest = _normalize_manifest(parsed_manifest);
		manifest["path"] = normalized_path;
		return manifest;
	}

	PackedByteArray LTENoteSkinServer::read_skin_file(const String& skin_path, const String& file_path) const {
		String normalized_path = normalize_skin_path(skin_path);
		String reader_path = _get_skin_archive_reader_path(normalized_path);
		String normalized_file_path = file_path.replace("\\", "/").strip_edges().simplify_path();
		if (reader_path.is_empty() || normalized_file_path.is_empty() || !_is_safe_zip_path(normalized_file_path)) {
			return PackedByteArray();
		}
		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(reader_path);
		if (error != OK) {
			return PackedByteArray();
		}
		PackedByteArray buffer;
		if (reader->file_exists(normalized_file_path)) {
			buffer = reader->read_file(normalized_file_path);
		}
		reader->close();
		return buffer;
	}

	PackedStringArray LTENoteSkinServer::get_skin_file_list(const String& skin_path) const {
		return _read_zip_file_list(skin_path);
	}

	PackedStringArray LTENoteSkinServer::get_skin_texture_files(const String& skin_path) const {
		PackedStringArray result;
		Dictionary manifest = load_skin_manifest(skin_path);
		Dictionary textures = manifest.get("textures", Dictionary());
		Array texture_keys = textures.keys();
		for (int index = 0; index < texture_keys.size(); ++index) {
			String texture_path = String(texture_keys[index]).replace("\\", "/").strip_edges().simplify_path();
			if (!texture_path.is_empty() && _is_texture_file(texture_path) && !result.has(texture_path)) {
				result.append(texture_path);
			}
		}

		PackedStringArray files = _read_zip_file_list(normalize_skin_path(skin_path));
		for (int index = 0; index < files.size(); ++index) {
			String file_path = files[index].replace("\\", "/").strip_edges().simplify_path();
			if (file_path.begins_with("textures/") && _is_texture_file(file_path) && !result.has(file_path)) {
				result.append(file_path);
			}
		}
		result.sort();
		return result;
	}

	PackedStringArray LTENoteSkinServer::get_skin_sound_files(const String& skin_path) const {
		PackedStringArray result;
		Dictionary manifest = load_skin_manifest(skin_path);
		Dictionary sounds = manifest.get("sounds", Dictionary());
		Array sound_keys = sounds.keys();
		for (int index = 0; index < sound_keys.size(); ++index) {
			String sound_path = String(sound_keys[index]).replace("\\", "/").strip_edges().simplify_path();
			if (!sound_path.is_empty() && _is_sound_file(sound_path) && !result.has(sound_path)) {
				result.append(sound_path);
			}
		}

		PackedStringArray files = _read_zip_file_list(normalize_skin_path(skin_path));
		for (int index = 0; index < files.size(); ++index) {
			String file_path = files[index].replace("\\", "/").strip_edges().simplify_path();
			if (file_path.begins_with("sounds/") && _is_sound_file(file_path) && !result.has(file_path)) {
				result.append(file_path);
			}
		}
		result.sort();
		return result;
	}

	String LTENoteSkinServer::import_skin_texture(const String& skin_path, const String& source_path) {
		String normalized_skin_path = normalize_skin_path(skin_path);
		String normalized_source_path = source_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_skin_path.is_empty() || !FileAccess::file_exists(normalized_skin_path)) {
			ERR_PRINT(vformat("Skin file does not exist: %s", normalized_skin_path));
			return String();
		}
		if (normalized_source_path.is_empty() || !FileAccess::file_exists(normalized_source_path)) {
			ERR_PRINT(vformat("Texture file does not exist: %s", normalized_source_path));
			return String();
		}
		if (!_is_texture_file(normalized_source_path)) {
			ERR_PRINT(vformat("Unsupported texture file: %s", normalized_source_path));
			return String();
		}

		PackedByteArray source_buffer = _read_file_bytes(normalized_source_path);
		if (source_buffer.is_empty()) {
			ERR_PRINT(vformat("Failed to read texture file: %s", normalized_source_path));
			return String();
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(normalized_skin_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to open skin file: %s. Error Code: %d", normalized_skin_path, error));
			return String();
		}

		Dictionary archive_files;
		PackedStringArray existing_files = reader->get_files();
		for (int index = 0; index < existing_files.size(); ++index) {
			String file_path = existing_files[index].replace("\\", "/").strip_edges().simplify_path();
			if (file_path.is_empty() || !_is_safe_zip_path(file_path)) {
				continue;
			}
			archive_files[file_path] = reader->read_file(file_path);
		}
		reader->close();

		String target_path = _make_unique_zip_path("textures/" + normalized_source_path.get_file(), existing_files);
		archive_files[target_path] = source_buffer;

		Dictionary manifest = load_skin_manifest(normalized_skin_path);
		if (manifest.is_empty()) {
			manifest = make_default_skin_manifest(Dictionary());
		}
		Dictionary textures = manifest.get("textures", Dictionary());
		Dictionary texture_entry;
		texture_entry["path"] = target_path;
		texture_entry["source_name"] = normalized_source_path.get_file();
		textures[target_path] = texture_entry;
		manifest["textures"] = textures;
		archive_files["manifest.json"] = JSON::stringify(_normalize_manifest(manifest), "\t").to_utf8_buffer();

		error = _rewrite_skin_archive(normalized_skin_path, archive_files);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to import texture: %s. Error Code: %d", normalized_source_path, error));
			return String();
		}

		_mark_skin_file_changed(normalized_skin_path);
		return target_path;
	}

	String LTENoteSkinServer::rename_skin_texture(const String& skin_path, const String& old_texture_path, const String& new_texture_path) {
		String normalized_skin_path = normalize_skin_path(skin_path);
		String old_path = old_texture_path.replace("\\", "/").strip_edges().simplify_path();
		String requested_path = new_texture_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_skin_path.is_empty() || !FileAccess::file_exists(normalized_skin_path)) {
			ERR_PRINT(vformat("Skin file does not exist: %s", normalized_skin_path));
			return String();
		}
		if (old_path.is_empty() || requested_path.is_empty() || !_is_safe_zip_path(old_path) || !_is_safe_zip_path(requested_path)) {
			return String();
		}
		if (!_is_texture_file(old_path)) {
			return String();
		}

		String old_extension = old_path.get_extension();
		if (old_extension.is_empty()) {
			return String();
		}
		String requested_base_name = requested_path.get_file().get_basename().strip_edges();
		if (requested_base_name.is_empty()) {
			return String();
		}
		String requested_dir = requested_path.get_base_dir();
		if (requested_dir.is_empty()) {
			requested_dir = old_path.get_base_dir();
		}
		requested_path = requested_dir.path_join(requested_base_name + "." + old_extension).replace("\\", "/").simplify_path();
		if (requested_path == old_path) {
			return old_path;
		}
		if (!_is_safe_zip_path(requested_path) || !_is_texture_file(requested_path)) {
			return String();
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(normalized_skin_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to open skin file: %s. Error Code: %d", normalized_skin_path, error));
			return String();
		}

		Dictionary archive_files;
		PackedStringArray existing_files = reader->get_files();
		for (int index = 0; index < existing_files.size(); ++index) {
			String file_path = existing_files[index].replace("\\", "/").strip_edges().simplify_path();
			if (file_path.is_empty() || !_is_safe_zip_path(file_path)) {
				continue;
			}
			archive_files[file_path] = reader->read_file(file_path);
		}
		reader->close();

		if (!archive_files.has(old_path)) {
			ERR_PRINT(vformat("Texture file does not exist in skin: %s", old_path));
			return String();
		}
		PackedByteArray texture_buffer = archive_files[old_path];
		archive_files.erase(old_path);

		PackedStringArray remaining_files;
		Array archive_keys = archive_files.keys();
		for (int index = 0; index < archive_keys.size(); ++index) {
			remaining_files.append(String(archive_keys[index]));
		}
		String final_path = _make_unique_zip_path(requested_path, remaining_files);
		archive_files[final_path] = texture_buffer;

		Dictionary manifest = load_skin_manifest(normalized_skin_path);
		if (manifest.is_empty()) {
			manifest = make_default_skin_manifest(Dictionary());
		}
		Dictionary textures = manifest.get("textures", Dictionary());
		Dictionary texture_entry;
		if (textures.has(old_path) && textures[old_path].get_type() == Variant::DICTIONARY) {
			texture_entry = textures[old_path];
		}
		textures.erase(old_path);
		texture_entry["path"] = final_path;
		texture_entry["source_name"] = final_path.get_file();
		textures[final_path] = texture_entry;
		manifest["textures"] = textures;

		Dictionary notes = manifest.get("notes", Dictionary());
		Array note_keys = notes.keys();
		for (int note_index = 0; note_index < note_keys.size(); ++note_index) {
			String note_key = String(note_keys[note_index]);
			if (notes[note_key].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary note = notes[note_key];
			Dictionary parts = note.get("parts", Dictionary());
			Array part_keys = parts.keys();
			for (int part_index = 0; part_index < part_keys.size(); ++part_index) {
				String part_id = String(part_keys[part_index]);
				if (parts[part_id].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary part = parts[part_id];
				Array layers = part.get("layers", Array());
				for (int layer_index = 0; layer_index < layers.size(); ++layer_index) {
					if (layers[layer_index].get_type() != Variant::DICTIONARY) {
						continue;
					}
					Dictionary layer = layers[layer_index];
					if (String(layer.get("texture", String())) == old_path) {
						layer["texture"] = final_path;
						layers[layer_index] = layer;
					}
				}
				part["layers"] = layers;
				parts[part_id] = part;
			}
			note["parts"] = parts;
			notes[note_key] = note;
		}
		manifest["notes"] = notes;

		archive_files["manifest.json"] = JSON::stringify(_normalize_manifest(manifest), "\t").to_utf8_buffer();
		error = _rewrite_skin_archive(normalized_skin_path, archive_files);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to rename texture: %s. Error Code: %d", old_path, error));
			return String();
		}

		_mark_skin_file_changed(normalized_skin_path);
		return final_path;
	}

	String LTENoteSkinServer::import_skin_sound(const String& skin_path, const String& source_path) {
		String normalized_skin_path = normalize_skin_path(skin_path);
		String normalized_source_path = source_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_skin_path.is_empty() || !FileAccess::file_exists(normalized_skin_path)) {
			ERR_PRINT(vformat("Skin file does not exist: %s", normalized_skin_path));
			return String();
		}
		if (normalized_source_path.is_empty() || !FileAccess::file_exists(normalized_source_path)) {
			ERR_PRINT(vformat("Sound file does not exist: %s", normalized_source_path));
			return String();
		}
		if (!_is_sound_file(normalized_source_path)) {
			ERR_PRINT(vformat("Unsupported sound file: %s", normalized_source_path));
			return String();
		}

		PackedByteArray source_buffer = _read_file_bytes(normalized_source_path);
		if (source_buffer.is_empty()) {
			ERR_PRINT(vformat("Failed to read sound file: %s", normalized_source_path));
			return String();
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(normalized_skin_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to open skin file: %s. Error Code: %d", normalized_skin_path, error));
			return String();
		}

		Dictionary archive_files;
		PackedStringArray existing_files = reader->get_files();
		for (int index = 0; index < existing_files.size(); ++index) {
			String file_path = existing_files[index].replace("\\", "/").strip_edges().simplify_path();
			if (file_path.is_empty() || !_is_safe_zip_path(file_path)) {
				continue;
			}
			archive_files[file_path] = reader->read_file(file_path);
		}
		reader->close();

		String target_path = _make_unique_zip_path("sounds/" + normalized_source_path.get_file(), existing_files);
		archive_files[target_path] = source_buffer;

		Dictionary manifest = load_skin_manifest(normalized_skin_path);
		if (manifest.is_empty()) {
			manifest = make_default_skin_manifest(Dictionary());
		}
		Dictionary sounds = manifest.get("sounds", Dictionary());
		Dictionary sound_entry;
		sound_entry["path"] = target_path;
		sound_entry["source_name"] = normalized_source_path.get_file();
		sounds[target_path] = sound_entry;
		manifest["sounds"] = sounds;
		archive_files["manifest.json"] = JSON::stringify(_normalize_manifest(manifest), "\t").to_utf8_buffer();

		error = _rewrite_skin_archive(normalized_skin_path, archive_files);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to import sound: %s. Error Code: %d", normalized_source_path, error));
			return String();
		}

		_mark_skin_file_changed(normalized_skin_path);
		return target_path;
	}

	String LTENoteSkinServer::rename_skin_sound(const String& skin_path, const String& old_sound_path, const String& new_sound_path) {
		String normalized_skin_path = normalize_skin_path(skin_path);
		String old_path = old_sound_path.replace("\\", "/").strip_edges().simplify_path();
		String requested_path = new_sound_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_skin_path.is_empty() || !FileAccess::file_exists(normalized_skin_path)) {
			ERR_PRINT(vformat("Skin file does not exist: %s", normalized_skin_path));
			return String();
		}
		if (old_path.is_empty() || requested_path.is_empty() || !_is_safe_zip_path(old_path) || !_is_safe_zip_path(requested_path)) {
			return String();
		}
		if (!_is_sound_file(old_path)) {
			return String();
		}

		String old_extension = old_path.get_extension();
		if (old_extension.is_empty()) {
			return String();
		}
		String requested_base_name = requested_path.get_file().get_basename().strip_edges();
		if (requested_base_name.is_empty()) {
			return String();
		}
		String requested_dir = requested_path.get_base_dir();
		if (requested_dir.is_empty()) {
			requested_dir = old_path.get_base_dir();
		}
		requested_path = requested_dir.path_join(requested_base_name + "." + old_extension).replace("\\", "/").simplify_path();
		if (requested_path == old_path) {
			return old_path;
		}
		if (!_is_safe_zip_path(requested_path) || !_is_sound_file(requested_path)) {
			return String();
		}

		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(normalized_skin_path);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to open skin file: %s. Error Code: %d", normalized_skin_path, error));
			return String();
		}

		Dictionary archive_files;
		PackedStringArray existing_files = reader->get_files();
		for (int index = 0; index < existing_files.size(); ++index) {
			String file_path = existing_files[index].replace("\\", "/").strip_edges().simplify_path();
			if (file_path.is_empty() || !_is_safe_zip_path(file_path)) {
				continue;
			}
			archive_files[file_path] = reader->read_file(file_path);
		}
		reader->close();

		if (!archive_files.has(old_path)) {
			ERR_PRINT(vformat("Sound file does not exist in skin: %s", old_path));
			return String();
		}
		PackedByteArray sound_buffer = archive_files[old_path];
		archive_files.erase(old_path);

		PackedStringArray remaining_files;
		Array archive_keys = archive_files.keys();
		for (int index = 0; index < archive_keys.size(); ++index) {
			remaining_files.append(String(archive_keys[index]));
		}
		String final_path = _make_unique_zip_path(requested_path, remaining_files);
		archive_files[final_path] = sound_buffer;

		Dictionary manifest = load_skin_manifest(normalized_skin_path);
		if (manifest.is_empty()) {
			manifest = make_default_skin_manifest(Dictionary());
		}
		Dictionary sounds = manifest.get("sounds", Dictionary());
		Dictionary sound_entry;
		if (sounds.has(old_path) && sounds[old_path].get_type() == Variant::DICTIONARY) {
			sound_entry = sounds[old_path];
		}
		sounds.erase(old_path);
		sound_entry["path"] = final_path;
		sound_entry["source_name"] = final_path.get_file();
		sounds[final_path] = sound_entry;
		manifest["sounds"] = sounds;

		Dictionary hit_sounds = manifest.get("hit_sounds", Dictionary());
		Array hit_sound_keys = hit_sounds.keys();
		for (int hit_sound_index = 0; hit_sound_index < hit_sound_keys.size(); ++hit_sound_index) {
			String hit_sound_id = String(hit_sound_keys[hit_sound_index]);
			if (hit_sounds[hit_sound_id].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary hit_sound = hit_sounds[hit_sound_id];
			Array variants = hit_sound.get("variants", Array());
			for (int variant_index = 0; variant_index < variants.size(); ++variant_index) {
				if (String(variants[variant_index]) == old_path) {
					variants[variant_index] = final_path;
				}
			}
			hit_sound["variants"] = variants;
			if (String(hit_sound.get("sound", String())) == old_path) {
				hit_sound["sound"] = final_path;
			}
			hit_sounds[hit_sound_id] = hit_sound;
		}
		manifest["hit_sounds"] = hit_sounds;

		Dictionary hit_effects = manifest.get("hit_effects", Dictionary());
		Array effect_keys = hit_effects.keys();
		for (int effect_index = 0; effect_index < effect_keys.size(); ++effect_index) {
			String effect_id = String(effect_keys[effect_index]);
			if (hit_effects[effect_id].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary effect = hit_effects[effect_id];
			Array layers = effect.get("layers", Array());
			for (int layer_index = 0; layer_index < layers.size(); ++layer_index) {
				if (layers[layer_index].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary layer = layers[layer_index];
				if (String(layer.get("sound", String())) == old_path) {
					layer["sound"] = final_path;
					layers[layer_index] = layer;
				}
			}
			effect["layers"] = layers;
			hit_effects[effect_id] = effect;
		}
		manifest["hit_effects"] = hit_effects;

		archive_files["manifest.json"] = JSON::stringify(_normalize_manifest(manifest), "\t").to_utf8_buffer();
		error = _rewrite_skin_archive(normalized_skin_path, archive_files);
		if (error != OK) {
			ERR_PRINT(vformat("Failed to rename sound: %s. Error Code: %d", old_path, error));
			return String();
		}

		_mark_skin_file_changed(normalized_skin_path);
		return final_path;
	}

	void LTENoteSkinServer::request_open_skin(const String& runtime_uuid, const String& skin_path) {
		String normalized_path = normalize_skin_path(skin_path);
		if (normalized_path.is_empty()) {
			return;
		}
		emit_signal("skin_open_requested", runtime_uuid, normalized_path);
	}

	Dictionary LTENoteSkinServer::get_designer_config(const String& runtime_uuid) const {
		Dictionary default_config = _normalize_designer_config(Dictionary());
		if (runtime_uuid.is_empty()) {
			return default_config;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return default_config;
		}
		Dictionary config = settings_config->note_skin_designer_configs.get(runtime_uuid, Dictionary());
		return _normalize_designer_config(config);
	}

	void LTENoteSkinServer::save_designer_config(const String& runtime_uuid, const Dictionary& config) {
		if (runtime_uuid.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		Dictionary normalized_config = _normalize_designer_config(config);
		settings_config->note_skin_designer_configs[runtime_uuid] = normalized_config;
		settings_config->save_settings_config();
		emit_signal("designer_config_changed", runtime_uuid, normalized_config);
	}

	void LTENoteSkinServer::set_designer_view_type(const String& runtime_uuid, const String& view_type) {
		Dictionary config = get_designer_config(runtime_uuid);
		config["view_type"] = _normalize_view_type(view_type);
		save_designer_config(runtime_uuid, config);
	}

	Dictionary LTENoteSkinServer::_normalize_manifest(const Dictionary& manifest) const {
		Dictionary normalized = manifest.duplicate(true);
		normalized["format"] = "lightech.note_skin";
		normalized["format_version"] = 2;
		if (String(normalized.get("id", String())).is_empty()) {
			normalized["id"] = Utils::uuid(Utils::UUID_V7);
		}
		String skin_name = String(normalized.get("name", "New Skin")).strip_edges();
		normalized["name"] = skin_name.is_empty() ? String("New Skin") : skin_name;
		normalized["author"] = String(normalized.get("author", String()));
		normalized["version"] = String(normalized.get("version", "1.0"));
		normalized["description"] = String(normalized.get("description", String()));
		normalized["targets"] = _normalize_targets(normalized.get("targets", Array()));
		normalized["preview"] = String(normalized.get("preview", String()));
		normalized["textures"] = _normalize_textures(normalized.get("textures", Dictionary()));
		normalized["sounds"] = _normalize_sounds(normalized.get("sounds", Dictionary()));
		normalized["hit_effects"] = _normalize_hit_effects(normalized.has("hit_effects") ? normalized.get("hit_effects", Dictionary()) : Variant());
		normalized["hit_sounds"] = _normalize_hit_sounds(normalized.has("hit_sounds") ? normalized.get("hit_sounds", Dictionary()) : Variant());
		normalized["hit_events"] = _normalize_hit_events(normalized.has("hit_events") ? normalized.get("hit_events", Dictionary()) : Variant());
		normalized["notes"] = _normalize_note_structures(normalized.get("notes", Dictionary()), normalized.get("note", Dictionary()));
		normalized.erase("assets");
		normalized.erase("slots");
		normalized.erase("metrics");
		normalized.erase("events");
		normalized.erase("note");
		normalized.erase("path");
		return normalized;
	}

	Array LTENoteSkinServer::_normalize_targets(const Variant& value) const {
		Array targets;
		if (value.get_type() == Variant::ARRAY) {
			Array source = value;
			for (int index = 0; index < source.size(); ++index) {
				String target = String(source[index]).strip_edges().to_lower();
				if ((target == "key" || target == "lt") && !targets.has(target)) {
					targets.append(target);
				}
			}
		}
		if (targets.is_empty()) {
			targets.append("key");
			targets.append("lt");
		}
		return targets;
	}

	Dictionary LTENoteSkinServer::_normalize_textures(const Variant& value) const {
		Dictionary textures;
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary source = value;
			Array keys = source.keys();
			for (int index = 0; index < keys.size(); ++index) {
				String texture_path = String(keys[index]).replace("\\", "/").strip_edges().simplify_path();
				if (texture_path.is_empty() || !_is_safe_zip_path(texture_path) || !_is_texture_file(texture_path)) {
					continue;
				}
				textures[texture_path] = _normalize_texture_entry(texture_path, source[keys[index]]);
			}
		} else if (value.get_type() == Variant::ARRAY) {
			Array source = value;
			for (int index = 0; index < source.size(); ++index) {
				if (source[index].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary entry = source[index];
				String texture_path = String(entry.get("path", String())).replace("\\", "/").strip_edges().simplify_path();
				if (texture_path.is_empty() || !_is_safe_zip_path(texture_path) || !_is_texture_file(texture_path)) {
					continue;
				}
				textures[texture_path] = _normalize_texture_entry(texture_path, entry);
			}
		}
		return textures;
	}

	Dictionary LTENoteSkinServer::_normalize_texture_entry(const String& texture_path, const Variant& value) const {
		Dictionary entry;
		if (value.get_type() == Variant::DICTIONARY) {
			entry = value;
		}
		entry["path"] = texture_path;
		entry["source_name"] = String(entry.get("source_name", texture_path.get_file()));
		return entry;
	}

	Dictionary LTENoteSkinServer::_normalize_sounds(const Variant& value) const {
		Dictionary sounds;
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary source = value;
			Array keys = source.keys();
			for (int index = 0; index < keys.size(); ++index) {
				String sound_path = String(keys[index]).replace("\\", "/").strip_edges().simplify_path();
				if (sound_path.is_empty() || !_is_safe_zip_path(sound_path) || !_is_sound_file(sound_path)) {
					continue;
				}
				sounds[sound_path] = _normalize_sound_entry(sound_path, source[keys[index]]);
			}
		} else if (value.get_type() == Variant::ARRAY) {
			Array source = value;
			for (int index = 0; index < source.size(); ++index) {
				if (source[index].get_type() != Variant::DICTIONARY) {
					continue;
				}
				Dictionary entry = source[index];
				String sound_path = String(entry.get("path", String())).replace("\\", "/").strip_edges().simplify_path();
				if (sound_path.is_empty() || !_is_safe_zip_path(sound_path) || !_is_sound_file(sound_path)) {
					continue;
				}
				sounds[sound_path] = _normalize_sound_entry(sound_path, entry);
			}
		}
		return sounds;
	}

	Dictionary LTENoteSkinServer::_normalize_sound_entry(const String& sound_path, const Variant& value) const {
		Dictionary entry;
		if (value.get_type() == Variant::DICTIONARY) {
			entry = value;
		}
		entry["path"] = sound_path;
		entry["source_name"] = String(entry.get("source_name", sound_path.get_file()));
		return entry;
	}

	Dictionary LTENoteSkinServer::_normalize_hit_effects(const Variant& value) const {
		Dictionary effects;
		if (value.get_type() != Variant::DICTIONARY) {
			return _make_default_hit_effects();
		}
		Dictionary source = value;
		Array keys = source.keys();
		for (int index = 0; index < keys.size(); ++index) {
			String effect_id = String(keys[index]).strip_edges();
			if (effect_id.is_empty()) {
				continue;
			}
			effects[effect_id] = _normalize_hit_effect(effect_id, source[keys[index]]);
		}
		return effects;
	}

	Dictionary LTENoteSkinServer::_normalize_hit_effect(const String& effect_id, const Variant& value) const {
		Dictionary effect;
		effect["id"] = effect_id;
		effect["name"] = effect_id.capitalize();
		effect["duration"] = 0.25;
		effect["layers"] = Array();
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary source = value;
			effect["id"] = String(source.get("id", effect_id)).strip_edges();
			if (String(effect["id"]).is_empty()) {
				effect["id"] = effect_id;
			}
			effect["name"] = String(source.get("name", String(effect["id"]).capitalize()));
			effect["duration"] = double(source.get("duration", effect["duration"]));
			effect["layers"] = _normalize_hit_effect_layers(source.get("layers", effect["layers"]));
		}
		return effect;
	}

	Array LTENoteSkinServer::_normalize_hit_effect_layers(const Variant& value) const {
		Array layers;
		if (value.get_type() != Variant::ARRAY) {
			return layers;
		}
		Array source = value;
		for (int index = 0; index < source.size(); ++index) {
			if (source[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary layer = source[index];
			layers.append(_normalize_hit_effect_layer(layer, index));
		}
		return layers;
	}

	Dictionary LTENoteSkinServer::_normalize_hit_effect_layer(const Dictionary& layer, const int index) const {
		Dictionary normalized;
		normalized["id"] = vformat("effect_layer_%d", index);
		normalized["name"] = vformat("Effect Layer %d", index + 1);
		normalized["type"] = "sprite";
		normalized["texture"] = String();
		normalized["sound"] = String();
		normalized["offset_x"] = 0.0;
		normalized["offset_y"] = 0.0;
		normalized["width"] = 0.0;
		normalized["height"] = 0.0;
		normalized["tint"] = "#ffffffff";
		normalized["visible"] = true;
		normalized["recolor"] = false;
		normalized["delay"] = 0.0;
		normalized["volume_db"] = 0.0;
		normalized["pitch"] = 1.0;
		normalized["pitch_random"] = 0.0;
		normalized["duration"] = 0.25;
		normalized["count"] = 8;
		Array keys = layer.keys();
		for (int key_index = 0; key_index < keys.size(); ++key_index) {
			Variant key = keys[key_index];
			normalized[key] = layer.get(key, Variant());
		}
		normalized["id"] = String(normalized.get("id", vformat("effect_layer_%d", index))).strip_edges();
		if (String(normalized["id"]).is_empty()) {
			normalized["id"] = vformat("effect_layer_%d", index);
		}
		normalized["name"] = String(normalized.get("name", String(normalized["id"]).capitalize()));
		String type = String(normalized.get("type", "sprite")).strip_edges().to_lower();
		if (type != "sprite" && type != "ring" && type != "particles" && type != "sound") {
			type = "sprite";
		}
		normalized["type"] = type;
		normalized["texture"] = String(normalized.get("texture", String()));
		normalized["sound"] = String(normalized.get("sound", String()));
		normalized["offset_x"] = double(normalized.get("offset_x", 0.0));
		normalized["offset_y"] = double(normalized.get("offset_y", 0.0));
		normalized["width"] = double(normalized.get("width", 0.0));
		normalized["height"] = double(normalized.get("height", 0.0));
		normalized["tint"] = String(normalized.get("tint", "#ffffffff"));
		normalized["visible"] = bool(normalized.get("visible", true));
		normalized["recolor"] = bool(normalized.get("recolor", false));
		double delay = double(normalized.get("delay", normalized.get("fade_delay", 0.0)));
		double pitch = double(normalized.get("pitch", 1.0));
		double pitch_random = double(normalized.get("pitch_random", 0.0));
		normalized["delay"] = delay < 0.0 ? 0.0 : delay;
		normalized["volume_db"] = double(normalized.get("volume_db", 0.0));
		normalized["pitch"] = pitch < 0.01 ? 0.01 : pitch;
		normalized["pitch_random"] = pitch_random < 0.0 ? 0.0 : pitch_random;
		double duration = double(normalized.get("duration", 0.25));
		int count = int(normalized.get("count", 8));
		normalized["duration"] = duration < 0.0 ? 0.0 : duration;
		normalized["count"] = count < 1 ? 1 : count;
		return normalized;
	}

	Dictionary LTENoteSkinServer::_normalize_hit_sounds(const Variant& value) const {
		Dictionary hit_sounds;
		if (value.get_type() != Variant::DICTIONARY) {
			return _make_default_hit_sounds();
		}
		Dictionary source = value;
		Array keys = source.keys();
		for (int index = 0; index < keys.size(); ++index) {
			String sound_id = String(keys[index]).strip_edges();
			if (sound_id.is_empty()) {
				continue;
			}
			hit_sounds[sound_id] = _normalize_hit_sound(sound_id, source[keys[index]]);
		}
		return hit_sounds;
	}

	Dictionary LTENoteSkinServer::_normalize_hit_sound(const String& sound_id, const Variant& value) const {
		Dictionary hit_sound;
		hit_sound["id"] = sound_id;
		hit_sound["name"] = sound_id.capitalize();
		hit_sound["variants"] = Array();
		hit_sound["volume_db"] = 0.0;
		hit_sound["pitch"] = 1.0;
		hit_sound["pitch_random"] = 0.0;
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary source = value;
			hit_sound["id"] = String(source.get("id", sound_id)).strip_edges();
			if (String(hit_sound["id"]).is_empty()) {
				hit_sound["id"] = sound_id;
			}
			hit_sound["name"] = String(source.get("name", String(hit_sound["id"]).capitalize()));
			Array variants;
			Variant variants_value = source.get("variants", Array());
			if (variants_value.get_type() == Variant::ARRAY) {
				Array source_variants = variants_value;
				for (int index = 0; index < source_variants.size(); ++index) {
					String sound_path = String(source_variants[index]).replace("\\", "/").strip_edges().simplify_path();
					if (!sound_path.is_empty() && _is_safe_zip_path(sound_path) && _is_sound_file(sound_path) && !variants.has(sound_path)) {
						variants.append(sound_path);
					}
				}
			} else {
				String sound_path = String(source.get("sound", String())).replace("\\", "/").strip_edges().simplify_path();
				if (!sound_path.is_empty() && _is_safe_zip_path(sound_path) && _is_sound_file(sound_path)) {
					variants.append(sound_path);
				}
			}
			hit_sound["variants"] = variants;
			hit_sound["volume_db"] = double(source.get("volume_db", 0.0));
			double pitch = double(source.get("pitch", 1.0));
			double pitch_random = double(source.get("pitch_random", 0.0));
			hit_sound["pitch"] = pitch < 0.01 ? 0.01 : pitch;
			hit_sound["pitch_random"] = pitch_random < 0.0 ? 0.0 : pitch_random;
		}
		return hit_sound;
	}

	Dictionary LTENoteSkinServer::_normalize_hit_events(const Variant& value) const {
		Dictionary events;
		if (value.get_type() != Variant::DICTIONARY) {
			return _make_default_hit_events();
		}
		Dictionary source = value;
		Array keys = source.keys();
		for (int index = 0; index < keys.size(); ++index) {
			String event_id = String(keys[index]).strip_edges();
			if (event_id.is_empty()) {
				continue;
			}
			events[event_id] = _normalize_hit_event(event_id, source[keys[index]]);
		}
		return events;
	}

	Dictionary LTENoteSkinServer::_normalize_hit_event(const String& event_id, const Variant& value) const {
		Dictionary event;
		event["id"] = event_id;
		event["name"] = event_id.capitalize();
		event["effect"] = String();
		event["sound"] = String();
		event["trigger_mode"] = "per_note";
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary source = value;
			event["id"] = String(source.get("id", event_id)).strip_edges();
			if (String(event["id"]).is_empty()) {
				event["id"] = event_id;
			}
			event["name"] = String(source.get("name", String(event["id"]).capitalize()));
			event["effect"] = String(source.get("effect", String()));
			event["sound"] = String(source.get("sound", String()));
			String trigger_mode = String(source.get("trigger_mode", "per_note")).strip_edges().to_lower();
			if (trigger_mode != "per_note" && trigger_mode != "group_center" && trigger_mode != "both") {
				trigger_mode = "per_note";
			}
			event["trigger_mode"] = trigger_mode;
		}
		return event;
	}

	Dictionary LTENoteSkinServer::_normalize_designer_config(const Dictionary& config) const {
		Dictionary normalized = config.duplicate(true);
		normalized["view_type"] = _normalize_view_type(normalized.get("view_type", "grid"));
		normalized["show_time_anchor"] = bool(normalized.get("show_time_anchor", true));
		return normalized;
	}

	Dictionary LTENoteSkinServer::_normalize_note_structures(const Variant& value, const Variant& fallback) const {
		Dictionary notes;
		Dictionary fallback_note = _normalize_note_structure(fallback);
		notes["key"] = fallback_note.duplicate(true);
		notes["lt"] = fallback_note.duplicate(true);
		if (value.get_type() != Variant::DICTIONARY) {
			return notes;
		}

		Dictionary source = value;
		if (source.has("key")) {
			notes["key"] = _normalize_note_structure(source["key"]);
		}
		if (source.has("lt")) {
			notes["lt"] = _normalize_note_structure(source["lt"]);
		}
		return notes;
	}

	Dictionary LTENoteSkinServer::_normalize_note_structure(const Variant& value) const {
		Dictionary note = _make_default_note_structure();
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary source = value;
			note["tap_parts"] = source.get("tap_parts", note["tap_parts"]);
			note["hold_parts"] = source.get("hold_parts", note["hold_parts"]);
			note["parts"] = _normalize_parts(source.get("parts", note["parts"]));
		}
		return note;
	}

	Dictionary LTENoteSkinServer::_normalize_parts(const Variant& value) const {
		Dictionary default_note = _make_default_note_structure();
		Dictionary default_parts = default_note["parts"];
		Dictionary parts;
		Array default_keys = default_parts.keys();
		for (int index = 0; index < default_keys.size(); ++index) {
			String id = String(default_keys[index]);
			parts[id] = _normalize_part(id, default_parts[id]);
		}
		if (value.get_type() != Variant::DICTIONARY) {
			return parts;
		}
		Dictionary source = value;
		Array keys = source.keys();
		for (int index = 0; index < keys.size(); ++index) {
			String id = String(keys[index]).strip_edges();
			if (id.is_empty()) {
				continue;
			}
			parts[id] = _normalize_part(id, source[keys[index]]);
		}
		return parts;
	}

	Dictionary LTENoteSkinServer::_normalize_part(const String& id, const Variant& value) const {
		Dictionary part = _make_default_part(id, id.capitalize(), id, 30.0, 0.0);
		if (value.get_type() == Variant::DICTIONARY) {
			Dictionary source = value;
			part["id"] = String(source.get("id", id)).strip_edges();
			if (String(part["id"]).is_empty()) {
				part["id"] = id;
			}
			part["name"] = String(source.get("name", String(part["id"]).capitalize()));
			part["role"] = String(source.get("role", id));
			part["height"] = double(source.get("height", part["height"]));
			part["time_anchor_y"] = double(source.get("time_anchor_y", part["time_anchor_y"]));
			part["layers"] = _normalize_layers(source.get("layers", part["layers"]));
		}
		return part;
	}

	Array LTENoteSkinServer::_normalize_layers(const Variant& value) const {
		Array layers;
		if (value.get_type() != Variant::ARRAY) {
			return layers;
		}
		Array source = value;
		for (int index = 0; index < source.size(); ++index) {
			if (source[index].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary layer = source[index];
			layers.append(_normalize_layer(layer, index));
		}
		return layers;
	}

	Dictionary LTENoteSkinServer::_normalize_layer(const Dictionary& layer, const int index) const {
		Dictionary normalized = _make_default_layer(vformat("layer_%d", index), vformat("Layer %d", index + 1), false);
		Array keys = layer.keys();
		for (int key_index = 0; key_index < keys.size(); ++key_index) {
			Variant key = keys[key_index];
			normalized[key] = layer.get(key, Variant());
		}
		normalized["id"] = String(normalized.get("id", vformat("layer_%d", index))).strip_edges();
		if (String(normalized["id"]).is_empty()) {
			normalized["id"] = vformat("layer_%d", index);
		}
		normalized["name"] = String(normalized.get("name", String(normalized["id"]).capitalize()));
		normalized["texture"] = String(normalized.get("texture", String()));
		normalized["offset_x"] = double(normalized.get("offset_x", 0.0));
		normalized["offset_y"] = double(normalized.get("offset_y", 0.0));
		normalized["width"] = double(normalized.get("width", 0.0));
		normalized["height"] = double(normalized.get("height", 0.0));
		normalized["tint"] = String(normalized.get("tint", "#ffffffff"));
		normalized["visible"] = bool(normalized.get("visible", true));
		normalized["recolor"] = bool(normalized.get("recolor", false));
		normalized["flip_h"] = bool(normalized.get("flip_h", false));
		normalized["flip_v"] = bool(normalized.get("flip_v", false));
		return normalized;
	}

	Dictionary LTENoteSkinServer::_make_default_note_structure() const {
		Dictionary note;
		Array tap_parts;
		tap_parts.append("head");
		note["tap_parts"] = tap_parts;
		Array hold_parts;
		hold_parts.append("tail");
		hold_parts.append("body");
		hold_parts.append("head");
		note["hold_parts"] = hold_parts;
		Dictionary parts;
		parts["head"] = _make_default_part("head", "Head", "head", 30.0, 30.0);
		parts["body"] = _make_default_part("body", "Body", "body", 180.0, 0.0);
		parts["tail"] = _make_default_part("tail", "Tail", "tail", 30.0, 0.0);
		note["parts"] = parts;
		return note;
	}

	Dictionary LTENoteSkinServer::_make_default_part(const String& id, const String& name, const String& role, const double height, const double time_anchor_y) const {
		Dictionary part;
		part["id"] = id;
		part["name"] = name;
		part["role"] = role;
		part["height"] = height;
		part["time_anchor_y"] = time_anchor_y;
		Array layers;
		layers.append(_make_default_layer("base", "Base", true));
		part["layers"] = layers;
		return part;
	}

	Dictionary LTENoteSkinServer::_make_default_layer(const String& id, const String& name, const bool recolor) const {
		Dictionary layer;
		layer["id"] = id;
		layer["name"] = name;
		layer["texture"] = String();
		layer["offset_x"] = 0.0;
		layer["offset_y"] = 0.0;
		layer["width"] = 0.0;
		layer["height"] = 0.0;
		layer["tint"] = "#ffffffff";
		layer["visible"] = true;
		layer["recolor"] = recolor;
		layer["flip_h"] = false;
		layer["flip_v"] = false;
		return layer;
	}

	Dictionary LTENoteSkinServer::_make_default_hit_effects() const {
		Dictionary effects;
		Dictionary tap_effect;
		tap_effect["id"] = "tap_hit";
		tap_effect["name"] = "Tap Hit";
		tap_effect["duration"] = 0.25;
		Array tap_layers;
		Dictionary flash_layer;
		flash_layer["id"] = "flash";
		flash_layer["name"] = "Flash";
		flash_layer["type"] = "sprite";
		flash_layer["texture"] = String();
		flash_layer["sound"] = String();
		flash_layer["offset_x"] = 0.0;
		flash_layer["offset_y"] = 0.0;
		flash_layer["width"] = 0.0;
		flash_layer["height"] = 0.0;
		flash_layer["tint"] = "#ffffffff";
		flash_layer["visible"] = true;
		flash_layer["recolor"] = false;
		flash_layer["delay"] = 0.0;
		flash_layer["volume_db"] = 0.0;
		flash_layer["pitch"] = 1.0;
		flash_layer["pitch_random"] = 0.0;
		flash_layer["duration"] = 0.18;
		flash_layer["count"] = 8;
		tap_layers.append(flash_layer);
		tap_effect["layers"] = tap_layers;
		effects["tap_hit"] = tap_effect;
		return effects;
	}

	Dictionary LTENoteSkinServer::_make_default_hit_sounds() const {
		Dictionary sounds;
		Dictionary tap_sound;
		tap_sound["id"] = "tap_hit";
		tap_sound["name"] = "Tap Hit";
		tap_sound["variants"] = Array();
		tap_sound["volume_db"] = 0.0;
		tap_sound["pitch"] = 1.0;
		tap_sound["pitch_random"] = 0.0;
		sounds["tap_hit"] = tap_sound;
		return sounds;
	}

	Dictionary LTENoteSkinServer::_make_default_hit_events() const {
		Dictionary events;
		PackedStringArray ids;
		ids.append("tap_hit");
		ids.append("tap_miss");
		ids.append("hold_head_hit");
		ids.append("hold_tail_hit");
		ids.append("hold_miss");
		ids.append("double_hit");
		for (int index = 0; index < ids.size(); ++index) {
			String id = ids[index];
			Dictionary event;
			event["id"] = id;
			event["name"] = id.capitalize();
			event["effect"] = id == "tap_hit" || id == "double_hit" ? String("tap_hit") : String();
			event["sound"] = id == "tap_hit" || id == "double_hit" ? String("tap_hit") : String();
			event["trigger_mode"] = id == "double_hit" ? String("group_center") : String("per_note");
			events[id] = event;
		}
		return events;
	}

	String LTENoteSkinServer::_normalize_view_type(const String& view_type) const {
		String normalized_view_type = view_type.strip_edges().to_lower();
		if (normalized_view_type == "single") {
			return "single";
		}
		return "grid";
	}

	bool LTENoteSkinServer::_is_texture_file(const String& file_path) const {
		String extension = file_path.get_extension().to_lower();
		return extension == "png" ||
				extension == "jpg" ||
				extension == "jpeg" ||
				extension == "webp" ||
				extension == "bmp" ||
				extension == "tga" ||
				extension == "svg";
	}

	bool LTENoteSkinServer::_is_sound_file(const String& file_path) const {
		String extension = file_path.get_extension().to_lower();
		return extension == "wav" ||
				extension == "ogg" ||
				extension == "mp3";
	}

	PackedByteArray LTENoteSkinServer::_read_file_bytes(const String& file_path) const {
		Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
		if (file.is_null()) {
			return PackedByteArray();
		}
		PackedByteArray buffer = file->get_buffer(file->get_length());
		file->close();
		return buffer;
	}

	PackedByteArray LTENoteSkinServer::_read_skin_archive_bytes(const String& skin_path) const {
		String normalized_path = normalize_skin_path(skin_path);
		if (normalized_path.is_empty()) {
			return PackedByteArray();
		}
		if (FileAccess::file_exists(normalized_path)) {
			return _read_file_bytes(normalized_path);
		}

		ResourceLoader* resource_loader = ResourceLoader::get_singleton();
		if (!resource_loader) {
			return PackedByteArray();
		}
		Ref<Resource> resource = resource_loader->load(normalized_path, "LTENoteSkinResource");
		if (resource.is_null()) {
			return PackedByteArray();
		}
		Ref<LTENoteSkinResource> skin_resource = resource;
		if (skin_resource.is_null()) {
			return PackedByteArray();
		}
		return skin_resource->get_data();
	}

	String LTENoteSkinServer::_get_skin_archive_reader_path(const String& skin_path) const {
		String normalized_path = normalize_skin_path(skin_path);
		if (normalized_path.is_empty()) {
			return String();
		}
		if (FileAccess::file_exists(normalized_path)) {
			return normalized_path;
		}

		PackedByteArray archive_data = _read_skin_archive_bytes(normalized_path);
		if (archive_data.is_empty()) {
			return String();
		}

		String cache_dir = "user://lightech_note_skin_import_cache";
		Ref<DirAccess> dir_access = DirAccess::open("user://");
		if (dir_access.is_null()) {
			return String();
		}
		Error error = dir_access->make_dir_recursive(cache_dir);
		if (error != OK) {
			return String();
		}

		String cache_path = cache_dir.path_join(normalized_path.sha256_text() + ".lteskin");
		Ref<FileAccess> file = FileAccess::open(cache_path, FileAccess::WRITE);
		if (file.is_null()) {
			return String();
		}
		file->store_buffer(archive_data);
		file->close();
		return cache_path;
	}

	PackedStringArray LTENoteSkinServer::_read_zip_file_list(const String& skin_path) const {
		String reader_path = _get_skin_archive_reader_path(skin_path);
		if (reader_path.is_empty()) {
			return PackedStringArray();
		}
		Ref<ZIPReader> reader;
		reader.instantiate();
		Error error = reader->open(reader_path);
		if (error != OK) {
			return PackedStringArray();
		}
		PackedStringArray files = reader->get_files();
		reader->close();
		files.sort();
		return files;
	}

	String LTENoteSkinServer::_make_unique_zip_path(const String& preferred_path, const PackedStringArray& existing_files) const {
		String normalized_path = preferred_path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_path.is_empty() || !_is_safe_zip_path(normalized_path)) {
			normalized_path = "textures/texture.png";
		}
		if (!existing_files.has(normalized_path)) {
			return normalized_path;
		}

		String base_path = normalized_path.get_basename();
		String extension = normalized_path.get_extension();
		int index = 2;
		while (true) {
			String candidate = vformat("%s_%d.%s", base_path, index, extension);
			if (!existing_files.has(candidate)) {
				return candidate;
			}
			++index;
		}
		return normalized_path;
	}

	Error LTENoteSkinServer::_rewrite_skin_archive(const String& skin_path, const Dictionary& files) const {
		String temp_path = skin_path + String(".tmp");
		String backup_path = skin_path + String(".bak");
		if (FileAccess::file_exists(temp_path)) {
			DirAccess::remove_absolute(temp_path);
		}
		if (FileAccess::file_exists(backup_path)) {
			DirAccess::remove_absolute(backup_path);
		}

		Ref<ZIPPacker> packer;
		packer.instantiate();
		Error error = packer->open(temp_path, ZIPPacker::APPEND_CREATE);
		if (error != OK) {
			return error;
		}

		Array keys = files.keys();
		keys.sort();
		for (int index = 0; index < keys.size(); ++index) {
			String file_path = String(keys[index]).replace("\\", "/").strip_edges().simplify_path();
			if (file_path.is_empty() || !_is_safe_zip_path(file_path)) {
				continue;
			}
			PackedByteArray buffer = files.get(file_path, PackedByteArray());
			error = packer->start_file(file_path);
			if (error == OK) {
				error = packer->write_file(buffer);
			}
			if (error == OK) {
				error = packer->close_file();
			}
			if (error != OK) {
				break;
			}
		}
		Error close_error = packer->close();
		if (error == OK) {
			error = close_error;
		}
		if (error != OK) {
			DirAccess::remove_absolute(temp_path);
			return error;
		}

		error = DirAccess::rename_absolute(skin_path, backup_path);
		if (error != OK) {
			DirAccess::remove_absolute(temp_path);
			return error;
		}
		error = DirAccess::rename_absolute(temp_path, skin_path);
		if (error != OK) {
			DirAccess::rename_absolute(backup_path, skin_path);
			DirAccess::remove_absolute(temp_path);
			return error;
		}
		DirAccess::remove_absolute(backup_path);
		return OK;
	}

	bool LTENoteSkinServer::_is_safe_zip_path(const String& file_path) const {
		return !file_path.begins_with("/") &&
				!file_path.begins_with("\\") &&
				file_path.find("..") < 0;
	}
}
