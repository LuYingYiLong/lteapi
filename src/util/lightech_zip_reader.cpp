#include "lightech_zip_reader.h"

#include "utils.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/templates/hash_set.hpp>

#include <mutex>

namespace godot {
	namespace {
		constexpr char CACHE_ROOT[] = "user://cache/chart_packages";
		constexpr char MANIFEST_NAME[] = "package.cfg";
		constexpr int CACHE_VERSION = 1;

		std::mutex package_cache_mutex;

		Error rename_cache_directory(const String& from, const String& to) {
			Error error = DirAccess::rename_absolute(from, to);
#ifdef _WIN32
			for (int attempt = 0; attempt < 3 && (error == FAILED || error == ERR_BUSY || error == ERR_FILE_NO_PERMISSION); ++attempt) {
				OS::get_singleton()->delay_msec(25 * (attempt + 1));
				error = DirAccess::rename_absolute(from, to);
			}
#endif
			return error;
		}

		String path_key(const String& path) {
#ifdef _WIN32
			return path.to_lower();
#else
			return path;
#endif
		}

		bool is_relative_path(const String& path) {
			if (path.is_empty() || path.is_absolute_path() || path.contains(":") || path.contains("\\")) {
				return false;
			}
			for (int64_t index = 0; index < path.length(); ++index) {
				if (path[index] == 0) {
					return false;
				}
			}
			for (const String& part : path.split("/")) {
				if (part.is_empty() || part == "." || part == ".." || part.ends_with(".") || part.ends_with(" ")) {
					return false;
				}
#ifdef _WIN32
				String stem = part.get_slice(".", 0).strip_edges().to_upper();
				if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" ||
						(stem.length() == 4 && (stem.begins_with("COM") || stem.begins_with("LPT")) &&
								stem[3] >= '0' && stem[3] <= '9')) {
					return false;
				}
#endif
			}
			return true;
		}

		// Only remove staging and backup directories under the package cache.
		bool remove_cache_directory(const String& cache_root, const String& path) {
			if (!path.begins_with(cache_root + String("/"))) {
				return false;
			}
			Ref<DirAccess> parent = DirAccess::open(path.get_base_dir());
			if (parent.is_null()) {
				return false;
			}
			if (parent->is_link(path.get_file())) {
				return parent->remove(path.get_file()) == OK;
			}
			if (!DirAccess::dir_exists_absolute(path)) {
				return !FileAccess::file_exists(path);
			}
			Ref<DirAccess> directory = DirAccess::open(path);
			if (directory.is_null()) {
				return false;
			}
			directory->set_include_hidden(true);
			for (const String& child : directory->get_directories()) {
				if (!remove_cache_directory(cache_root, path.path_join(child))) {
					return false;
				}
			}
			for (const String& child : directory->get_files()) {
				if (directory->remove(child) != OK) {
					return false;
				}
			}
			return parent->remove(path.get_file()) == OK;
		}

		bool load_index(const Ref<ConfigFile>& manifest, const String& content_path, Dictionary& info, PackedStringArray& entries) {
			if (int(manifest->get_value("package", "version", 0)) != CACHE_VERSION) {
				return false;
			}
			Variant stored_info = manifest->get_value("package", "info", Variant());
			Variant stored_entries = manifest->get_value("package", "entries", Variant());
			if (stored_info.get_type() != Variant::DICTIONARY || stored_entries.get_type() != Variant::PACKED_STRING_ARRAY) {
				return false;
			}
			Dictionary candidate = stored_info;
			for (const Variant& key : candidate.keys()) {
				String root = key;
				if ((!root.is_empty() && !is_relative_path(root)) || candidate[key].get_type() != Variant::DICTIONARY ||
						!FileAccess::file_exists(content_path.path_join(root).path_join("info.json"))) {
					return false;
				}
			}
			info = candidate;
			entries = stored_entries;
			return !info.is_empty();
		}

		bool extract_package(const String& archive_path, const String& destination, Dictionary& info, PackedStringArray& entries) {
			Ref<ZIPReader> reader;
			reader.instantiate();
			if (reader->open(archive_path) != OK) {
				return false;
			}
			PackedStringArray names = reader->get_files();
			PackedStringArray paths;
			HashSet<String> seen;
			for (const String& name : names) {
				String path = name.replace("\\", "/");
				String key = path_key(path.trim_suffix("/"));
				if (!is_relative_path(path.trim_suffix("/")) || seen.has(key)) {
					return false;
				}
				seen.insert(key);
				paths.append(path);
			}

			Dictionary candidates;
			for (int64_t index = 0; index < paths.size(); ++index) {
				const String& path = paths[index];
				String target = destination.path_join(path);
				String directory = path.ends_with("/") ? target : target.get_base_dir();
				if (DirAccess::make_dir_recursive_absolute(directory) != OK) {
					return false;
				}
				if (path.ends_with("/")) {
					continue;
				}
				Ref<FileAccess> output = FileAccess::open(target, FileAccess::WRITE);
				if (output.is_null()) {
					return false;
				}
				PackedByteArray bytes = reader->read_file(names[index]);
				if (!output->store_buffer(bytes)) {
					return false;
				}
				output->close();
				entries.append(path);
				if (path.get_file() == "info.json") {
					Variant parsed = JSON::parse_string(bytes.get_string_from_utf8());
					if (parsed.get_type() == Variant::DICTIONARY && !Dictionary(parsed).is_empty()) {
						candidates[path.get_base_dir()] = parsed;
					}
				}
			}
			if (reader->close() != OK) {
				return false;
			}

			PackedStringArray roots(candidates.keys());
			roots.sort();
			for (const String& root : roots) {
				bool nested = false;
				for (const Variant& parent_key : info.keys()) {
					String parent = parent_key;
					if (parent.is_empty() || path_key(root).begins_with(path_key(parent) + "/")) {
						nested = true;
						break;
					}
				}
				if (!nested) {
					info[root] = candidates[root];
				}
			}
			return !info.is_empty();
		}
	}

	void LightechZipReader::_bind_methods() {
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("is_archive", "path"), &LightechZipReader::is_archive);
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("open", "path", "verify_content"), &LightechZipReader::open, DEFVAL(false));
		ClassDB::bind_method(D_METHOD("get_project_roots"), &LightechZipReader::get_project_roots);
		ClassDB::bind_method(D_METHOD("get_info", "project_root"), &LightechZipReader::get_info);
		ClassDB::bind_method(D_METHOD("get_root_path", "project_root"), &LightechZipReader::get_root_path);
		ClassDB::bind_method(D_METHOD("get_entries"), &LightechZipReader::get_entries);
	}

	bool LightechZipReader::is_archive(const String& path) {
		String extension = path.get_extension().to_lower();
		return extension == "ltz" || extension == "zip";
	}

	Ref<LightechZipReader> LightechZipReader::open(const String& path, bool verify_content) {
		ProjectSettings* settings = ProjectSettings::get_singleton();
		String archive_path = settings->globalize_path(path).replace("\\", "/").simplify_path();
		if (!is_archive(archive_path) || !FileAccess::file_exists(archive_path)) {
			return nullptr;
		}
		std::lock_guard<std::mutex> lock(package_cache_mutex);
		int64_t size = FileAccess::get_size(archive_path);
		uint64_t modified = FileAccess::get_modified_time(archive_path);
		String cache_root = settings->globalize_path(CACHE_ROOT).replace("\\", "/").simplify_path();
		String cache_dir = cache_root.path_join(path_key(archive_path).sha256_text());
		String manifest_path = cache_dir.path_join(MANIFEST_NAME);
		Ref<ConfigFile> manifest;
		manifest.instantiate();
		Ref<LightechZipReader> package;
		package.instantiate();
		package->content_path = cache_dir.path_join("content");

		bool indexed = manifest->load(manifest_path) == OK && load_index(manifest, package->content_path, package->project_info, package->entries);
		bool same_stamp = indexed && size >= 0 && modified != 0 &&
				size == int64_t(manifest->get_value("package", "size", -1)) &&
				modified == uint64_t(manifest->get_value("package", "modified", 0));
		if (same_stamp && !verify_content) {
			return package;
		}
		String digest = FileAccess::get_sha256(archive_path);
		if (digest.is_empty() || size != FileAccess::get_size(archive_path) || modified != FileAccess::get_modified_time(archive_path)) {
			return nullptr;
		}
		manifest->set_value("package", "size", size);
		manifest->set_value("package", "modified", modified);
		if (indexed && String(manifest->get_value("package", "digest", "")) == digest) {
			if (!same_stamp) {
				manifest->save(manifest_path);
			}
			return package;
		}

		String suffix = Utils::uuid(Utils::UUID_V7);
		String pending = cache_root.path_join(".pending-" + suffix);
		String previous = cache_root.path_join(".previous-" + suffix);
		package->project_info.clear();
		package->entries.clear();
		if (!extract_package(archive_path, pending.path_join("content"), package->project_info, package->entries)) {
			if (DirAccess::dir_exists_absolute(pending)) {
				remove_cache_directory(cache_root, pending);
			}
			WARN_PRINT(vformat("Invalid or unreadable chart package: %s", archive_path));
			return nullptr;
		}
		manifest->set_value("package", "version", CACHE_VERSION);
		manifest->set_value("package", "digest", digest);
		manifest->set_value("package", "info", package->project_info);
		manifest->set_value("package", "entries", package->entries);
		if (manifest->save(pending.path_join(MANIFEST_NAME)) != OK ||
				size != FileAccess::get_size(archive_path) || modified != FileAccess::get_modified_time(archive_path)) {
			remove_cache_directory(cache_root, pending);
			return nullptr;
		}
		bool had_cache = DirAccess::dir_exists_absolute(cache_dir);
		if (had_cache && rename_cache_directory(cache_dir, previous) != OK) {
			remove_cache_directory(cache_root, pending);
			WARN_PRINT(vformat("Cannot move the existing chart package cache: %s", cache_dir));
			return nullptr;
		}
		if (rename_cache_directory(pending, cache_dir) != OK) {
			if (had_cache) {
				rename_cache_directory(previous, cache_dir);
			}
			remove_cache_directory(cache_root, pending);
			WARN_PRINT(vformat("Cannot publish the chart package cache: %s", cache_dir));
			return nullptr;
		}
		if (had_cache) {
			remove_cache_directory(cache_root, previous);
		}
		return package;
	}

	PackedStringArray LightechZipReader::get_project_roots() const {
		PackedStringArray roots(project_info.keys());
		roots.sort();
		return roots;
	}

	Dictionary LightechZipReader::get_info(const String& project_root) const {
		Dictionary info = project_info.get(project_root, Dictionary());
		return info.duplicate(true);
	}

	String LightechZipReader::get_root_path(const String& project_root) const {
		return project_info.has(project_root) ? content_path.path_join(project_root).simplify_path() : String();
	}

	PackedStringArray LightechZipReader::get_entries() const {
		return entries;
	}
}
