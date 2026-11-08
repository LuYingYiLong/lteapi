#include "lightech_zip_reader.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/templates/hash_set.hpp>

namespace godot {
	namespace {
		constexpr char PACKAGE_PREFIX[] = "ltz://";

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
	}

	void LightechZipReader::_bind_methods() {
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("is_archive", "path"), &LightechZipReader::is_archive);
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("open", "path"), &LightechZipReader::open);
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("read_entry", "archive_path", "entry"), &LightechZipReader::read_entry);
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("is_package_path", "path"), &LightechZipReader::is_package_path);
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("resolve_asset", "project_path", "path"), &LightechZipReader::resolve_asset);
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("read_asset", "path"), &LightechZipReader::read_asset);
		ClassDB::bind_method(D_METHOD("read_resource", "path"), &LightechZipReader::read_resource);
		ClassDB::bind_method(D_METHOD("get_project_roots"), &LightechZipReader::get_project_roots);
		ClassDB::bind_method(D_METHOD("get_info", "project_root"), &LightechZipReader::get_info);
		ClassDB::bind_method(D_METHOD("get_entries"), &LightechZipReader::get_entries);
		ClassDB::bind_method(D_METHOD("get_entry_name", "project_root", "relative_path"), &LightechZipReader::get_entry_name);
		ClassDB::bind_method(D_METHOD("read_project_file", "project_root", "relative_path"), &LightechZipReader::read_project_file);
	}

	bool LightechZipReader::is_archive(const String& path) {
		String extension = path.get_extension().to_lower();
		return extension == "ltz" || extension == "zip";
	}

	Ref<LightechZipReader> LightechZipReader::open(const String& path) {
		String archive_path = ProjectSettings::get_singleton()->globalize_path(path).replace("\\", "/").simplify_path();
		if (!is_archive(archive_path) || !FileAccess::file_exists(archive_path)) {
			return nullptr;
		}
		Ref<ZIPReader> reader;
		reader.instantiate();
		if (reader->open(archive_path) != OK) {
			return nullptr;
		}
		Ref<LightechZipReader> package;
		package.instantiate();
		package->reader = reader;
		package->archive_path = archive_path;
		HashSet<String> seen;
		for (const String& name : reader->get_files()) {
			String normalized = name.replace("\\", "/");
			String entry = normalized.trim_suffix("/");
			String key = path_key(entry);
			if (!is_relative_path(entry) || seen.has(key)) {
				return nullptr;
			}
			seen.insert(key);
			if (!normalized.ends_with("/")) {
				package->entries.append(entry);
				package->entry_names[key] = name;
			}
		}

		Dictionary candidates;
		for (const String& entry : package->entries) {
			for (String parent = entry.get_base_dir(); !parent.is_empty(); parent = parent.get_base_dir()) {
				if (package->entry_names.has(path_key(parent))) {
					return nullptr;
				}
			}
			if (entry.get_file() != "info.json") {
				continue;
			}
			String name = package->entry_names[path_key(entry)];
			Variant parsed = JSON::parse_string(reader->read_file(name, true).get_string_from_utf8());
			if (parsed.get_type() == Variant::DICTIONARY && !Dictionary(parsed).is_empty()) {
				candidates[entry.get_base_dir()] = parsed;
			}
		}

		PackedStringArray roots(candidates.keys());
		roots.sort();
		for (const String& root : roots) {
			bool nested = false;
			for (const Variant& parent_key : package->project_info.keys()) {
				String parent = parent_key;
				if (parent.is_empty() || path_key(root).begins_with(path_key(parent) + "/")) {
					nested = true;
					break;
				}
			}
			if (!nested) {
				package->project_info[root] = candidates[root];
			}
		}
		return package->project_info.is_empty() ? Ref<LightechZipReader>() : package;
	}

	PackedByteArray LightechZipReader::read_entry(const String& archive_path, const String& entry) {
		if (!is_archive(archive_path) || !is_relative_path(entry.replace("\\", "/"))) {
			return {};
		}
		Ref<ZIPReader> reader;
		reader.instantiate();
		if (reader->open(archive_path) != OK) {
			return {};
		}
		if (reader->file_exists(entry, true)) {
			return reader->read_file(entry, true);
		}
		String key = path_key(entry.replace("\\", "/"));
		for (const String& name : reader->get_files()) {
			if (path_key(name.replace("\\", "/")) == key) {
				return reader->read_file(name, true);
			}
		}
		return {};
	}

	bool LightechZipReader::is_package_path(const String& path) {
		return path.begins_with(PACKAGE_PREFIX);
	}

	String LightechZipReader::resolve_asset(const String& project_path, const String& path) {
		String normalized = path.replace("\\", "/").strip_edges();
		if (normalized.begins_with("res://") || normalized.begins_with("uid://")) {
			return normalized;
		}
		PackedStringArray root = project_path.get_slice("#", 0).trim_prefix(PACKAGE_PREFIX).split("/", true, 1);
		if (!is_package_path(project_path) || root.size() != 2 || root[0].is_empty()) {
			return {};
		}
		String project_root = root[1].uri_decode();
		String entry;
		if (is_package_path(normalized)) {
			PackedStringArray target = normalized.get_slice("#", 0).trim_prefix(PACKAGE_PREFIX).split("/", true, 1);
			if (target.size() != 2 || path_key(target[0].uri_decode()) != path_key(root[0].uri_decode())) {
				return {};
			}
			entry = target[1].uri_decode().replace("\\", "/").simplify_path();
			if (!project_root.is_empty() && !path_key(entry).begins_with(path_key(project_root + String("/")))) {
				return {};
			}
		} else {
			String relative = normalized.trim_prefix("/").simplify_path();
			if (!is_relative_path(relative)) {
				return {};
			}
			entry = project_root.path_join(relative);
		}
		if (!is_relative_path(entry)) {
			return {};
		}
		String identifier = String(PACKAGE_PREFIX) + root[0] + String("/") + path_key(entry).uri_encode();
		String revision = project_path.get_slice("#", 1);
		return revision.is_empty() ? identifier : identifier + String("#") + revision;
	}

	PackedByteArray LightechZipReader::read_asset(const String& path) {
		if (!is_package_path(path)) {
			return FileAccess::file_exists(path) ? FileAccess::get_file_as_bytes(path) : PackedByteArray();
		}
		PackedStringArray reference = path.get_slice("#", 0).trim_prefix(PACKAGE_PREFIX).split("/", true, 1);
		return reference.size() == 2 ? read_entry(reference[0].uri_decode(), reference[1].uri_decode()) : PackedByteArray();
	}

	PackedByteArray LightechZipReader::read_resource(const String& path) const {
		if (!is_package_path(path)) {
			return read_asset(path);
		}
		PackedStringArray reference = path.get_slice("#", 0).trim_prefix(PACKAGE_PREFIX).split("/", true, 1);
		if (reference.size() != 2 || path_key(reference[0].uri_decode()) != path_key(archive_path)) {
			return {};
		}
		String entry = entry_names.get(path_key(reference[1].uri_decode().replace("\\", "/")), String());
		if (entry.is_empty()) {
			return {};
		}
		std::lock_guard<std::mutex> lock(reader_mutex);
		return reader->read_file(entry, true);
	}

	Dictionary LightechZipReader::index_memory_zip(const PackedByteArray& bytes) {
		const int64_t size = bytes.size();
		const int64_t min_end = size > 65557 ? size - 65557 : 0;
		int64_t end = size - 22;
		for (; end >= min_end; --end) {
			if (bytes.decode_u32(end) == 0x06054b50 && end + 22 + bytes.decode_u16(end + 20) == size) {
				break;
			}
		}
		if (end < min_end || bytes.decode_u16(end + 4) != 0 || bytes.decode_u16(end + 6) != 0 ||
				bytes.decode_u16(end + 8) != bytes.decode_u16(end + 10)) {
			return {};
		}
		int64_t offset = bytes.decode_u32(end + 16);
		const int64_t directory_end = offset + bytes.decode_u32(end + 12);
		const int count = bytes.decode_u16(end + 10);
		if (directory_end > end) {
			return {};
		}
		Dictionary index;
		for (int i = 0; i < count; ++i) {
			if (offset + 46 > directory_end || bytes.decode_u32(offset) != 0x02014b50) {
				return {};
			}
			const int64_t name_end = offset + 46 + bytes.decode_u16(offset + 28);
			const int64_t next = name_end + bytes.decode_u16(offset + 30) + bytes.decode_u16(offset + 32);
			if (next > directory_end) {
				return {};
			}
			String name = bytes.slice(offset + 46, name_end).get_string_from_utf8().replace("\\", "/");
			if (!is_relative_path(name.trim_suffix("/"))) {
				return {};
			}
			if (!name.ends_with("/")) {
				if (index.has(name)) {
					return {};
				}
				index[name] = offset;
			}
			offset = next;
		}
		return index;
	}

	PackedByteArray LightechZipReader::read_memory_zip_entry(const PackedByteArray& bytes, const Dictionary& index, const String& name) {
		int64_t entry = index.get(name, -1);
		if (entry < 0 || entry + 46 > bytes.size() || (bytes.decode_u16(entry + 8) & 1) != 0) {
			return {};
		}
		const int64_t local = bytes.decode_u32(entry + 42);
		const int64_t compressed_size = bytes.decode_u32(entry + 20);
		const int64_t size = bytes.decode_u32(entry + 24);
		if (local + 30 > bytes.size() || bytes.decode_u32(local) != 0x04034b50 || size <= 0 || size > (256LL << 20)) {
			return {};
		}
		const int64_t start = local + 30 + bytes.decode_u16(local + 26) + bytes.decode_u16(local + 28);
		if (start + compressed_size > bytes.size()) {
			return {};
		}
		const int method = bytes.decode_u16(entry + 10);
		if (method == 0 && compressed_size == size) {
			return bytes.slice(start, start + size);
		}
		if (method != 8) {
			return {};
		}
		// Wrap ZIP's raw DEFLATE stream with the checksum and size required by GZIP.
		PackedByteArray gzip;
		for (uint8_t byte : { 0x1f, 0x8b, 0x08, 0, 0, 0, 0, 0, 0, 0xff }) {
			gzip.append(byte);
		}
		gzip.append_array(bytes.slice(start, start + compressed_size));
		const int64_t trailer = gzip.size();
		gzip.resize(trailer + 8);
		gzip.encode_u32(trailer, bytes.decode_u32(entry + 16));
		gzip.encode_u32(trailer + 4, size);
		return gzip.decompress(size, FileAccess::COMPRESSION_GZIP);
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

	PackedStringArray LightechZipReader::get_entries() const {
		return entries;
	}

	String LightechZipReader::get_entry_name(const String& project_root, const String& relative_path) const {
		String relative = relative_path.replace("\\", "/").strip_edges().trim_prefix("/").simplify_path();
		if (!project_info.has(project_root) || !is_relative_path(relative)) {
			return {};
		}
		return entry_names.get(path_key(project_root.path_join(relative)), String());
	}

	PackedByteArray LightechZipReader::read_project_file(const String& project_root, const String& relative_path) const {
		String entry = get_entry_name(project_root, relative_path);
		if (entry.is_empty()) {
			return {};
		}
		std::lock_guard<std::mutex> lock(reader_mutex);
		return reader->read_file(entry, true);
	}

}
