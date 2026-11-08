#include "lightech_zip_reader.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/templates/hash_set.hpp>

#include <filesystem>
#include <limits>

namespace godot {
	namespace {
		constexpr char PACKAGE_PREFIX[] = "ltz://";
		constexpr uint32_t ZIP64_FIELD = std::numeric_limits<uint32_t>::max();

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

		PackedByteArray read_file_range(const Ref<FileAccess>& file, int64_t offset, int64_t length) {
			file->seek(offset);
			return file->get_buffer(length);
		}

		template <typename Read>
		Dictionary index_zip(int64_t size, const Read& read) {
			const int64_t tail_offset = size > 65557 ? size - 65557 : 0;
			PackedByteArray tail = read(tail_offset, size - tail_offset);
			int64_t end = tail.size() - 22;
			for (; end >= 0; --end) {
				if (tail.decode_u32(end) == 0x06054b50 && end + 22 + tail.decode_u16(end + 20) == tail.size()) {
					break;
				}
			}
			if (end < 0 || tail.decode_u16(end + 4) != 0 || tail.decode_u16(end + 6) != 0 ||
					tail.decode_u16(end + 8) != tail.decode_u16(end + 10)) {
				return {};
			}
			uint64_t offset = tail.decode_u32(end + 16);
			uint64_t length = tail.decode_u32(end + 12);
			uint64_t count = tail.decode_u16(end + 10);
			uint64_t directory_limit = tail_offset + end;
			if (offset == ZIP64_FIELD || length == ZIP64_FIELD || count == 0xffff) {
				if (directory_limit < 20) {
					return {};
				}
				PackedByteArray locator = read(directory_limit - 20, 20);
				if (locator.size() == 20 && locator.decode_u32(0) == 0x07064b50) {
					uint64_t zip64_offset = locator.decode_u64(8);
					if (locator.decode_u32(4) != 0 || locator.decode_u32(16) != 1 ||
							directory_limit < 76 || zip64_offset > directory_limit - 76) {
						return {};
					}
					PackedByteArray zip64 = read(zip64_offset, 56);
					if (zip64.size() != 56 || zip64.decode_u32(0) != 0x06064b50 ||
							zip64.decode_u32(16) != 0 || zip64.decode_u32(20) != 0 ||
							zip64.decode_u64(24) != zip64.decode_u64(32)) {
						return {};
					}
					count = zip64.decode_u64(32);
					length = zip64.decode_u64(40);
					offset = zip64.decode_u64(48);
					directory_limit = zip64_offset;
				}
			}
			if (offset > directory_limit || length > directory_limit - offset || count > length / 46) {
				return {};
			}
			PackedByteArray directory = read(offset, length);
			if (directory.size() != int64_t(length)) {
				return {};
			}
			Dictionary index;
			int64_t position = 0;
			for (uint64_t i = 0; i < count; ++i) {
				if (position + 46 > directory.size() || directory.decode_u32(position) != 0x02014b50) {
					return {};
				}
				const int64_t name_end = position + 46 + directory.decode_u16(position + 28);
				const int64_t extra_end = name_end + directory.decode_u16(position + 30);
				const int64_t next = extra_end + directory.decode_u16(position + 32);
				if (next > directory.size() || directory.decode_u16(position + 34) != 0) {
					return {};
				}
				String name = directory.slice(position + 46, name_end).get_string_from_utf8();
				if (index.has(name)) {
					return {};
				}
				uint64_t uncompressed = directory.decode_u32(position + 24);
				uint64_t compressed = directory.decode_u32(position + 20);
				uint64_t local = directory.decode_u32(position + 42);
				if (uncompressed == ZIP64_FIELD || compressed == ZIP64_FIELD || local == ZIP64_FIELD) {
					int64_t extra = name_end;
					for (; extra + 4 <= extra_end && directory.decode_u16(extra) != 1; ) {
						extra += 4 + directory.decode_u16(extra + 2);
					}
					if (extra + 4 > extra_end) {
						return {};
					}
					int64_t field = extra + 4;
					const int64_t field_end = field + directory.decode_u16(extra + 2);
					if (field_end > extra_end) {
						return {};
					}
					for (uint64_t* value : { &uncompressed, &compressed, &local }) {
						if (*value == ZIP64_FIELD) {
							if (field + 8 > field_end) {
								return {};
							}
							*value = directory.decode_u64(field);
							field += 8;
						}
					}
				}
				if (local > offset || compressed > offset - local || uncompressed > INT64_MAX) {
					return {};
				}
				PackedInt64Array entry;
				for (int64_t value : { int64_t(local), int64_t(compressed), int64_t(uncompressed),
							int64_t(directory.decode_u32(position + 16)), int64_t(directory.decode_u16(position + 10)),
							int64_t(directory.decode_u16(position + 8)) }) {
					entry.append(value);
				}
				index[name] = entry;
				position = next;
			}
			return index;
		}

		template <typename Read>
		PackedByteArray read_zip_entry(const PackedInt64Array& entry, int64_t archive_size, int64_t size_limit, const Read& read) {
			if (entry.size() != 6 || (entry[5] & 1) != 0) {
				return {};
			}
			const int64_t local = entry[0];
			const int64_t compressed_size = entry[1];
			const int64_t size = entry[2];
			const int64_t method = entry[4];
			if (local > archive_size - 30 || size <= 0 || size > size_limit ||
					(method != 8 && (method != 0 || compressed_size != size))) {
				return {};
			}
			PackedByteArray header = read(local, 30);
			if (header.size() != 30 || header.decode_u32(0) != 0x04034b50) {
				return {};
			}
			const int64_t start = local + 30 + header.decode_u16(26) + header.decode_u16(28);
			if (start > archive_size || compressed_size > archive_size - start) {
				return {};
			}
			PackedByteArray data = read(start, compressed_size);
			if (data.size() != compressed_size || method == 0) {
				return data.size() == compressed_size ? data : PackedByteArray();
			}
			// Wrap ZIP's raw DEFLATE stream with the checksum and size required by GZIP.
			PackedByteArray gzip = String("1f8b08000000000000ff").hex_decode();
			gzip.append_array(data);
			const int64_t trailer = gzip.size();
			gzip.resize(trailer + 8);
			gzip.encode_u32(trailer, entry[3]);
			gzip.encode_u32(trailer + 4, size);
			return gzip.decompress(size, FileAccess::COMPRESSION_GZIP);
		}
	}

	void LightechZipReader::_bind_methods() {
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("is_archive", "path"), &LightechZipReader::is_archive);
		ClassDB::bind_static_method("LightechZipReader", D_METHOD("get_archive_stamp", "path"), &LightechZipReader::get_archive_stamp);
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

	PackedInt64Array LightechZipReader::get_archive_stamp(const String& path) {
		String absolute = ProjectSettings::get_singleton()->globalize_path(path);
#ifdef _WIN32
		std::filesystem::path source(absolute.utf16().get_data());
#else
		std::filesystem::path source(absolute.utf8().get_data());
#endif
		std::error_code error;
		const auto modified = std::filesystem::last_write_time(source, error);
		if (error) {
			return {};
		}
		const auto size = std::filesystem::file_size(source, error);
		if (error || size > INT64_MAX) {
			return {};
		}
		PackedInt64Array stamp;
		stamp.append(modified.time_since_epoch().count());
		stamp.append(size);
		return stamp;
	}

	Ref<LightechZipReader> LightechZipReader::open(const String& path) {
		String archive_path = ProjectSettings::get_singleton()->globalize_path(path).replace("\\", "/").simplify_path();
		if (!is_archive(archive_path)) {
			return nullptr;
		}
		PackedInt64Array stamp = get_archive_stamp(archive_path);
		if (stamp.is_empty()) {
			return nullptr;
		}
		Ref<FileAccess> file = FileAccess::open(archive_path, FileAccess::READ);
		if (file.is_null() || file->get_length() != uint64_t(stamp[1])) {
			return nullptr;
		}
		const auto read = [&file](int64_t offset, int64_t length) { return read_file_range(file, offset, length); };
		Ref<LightechZipReader> package;
		package.instantiate();
		package->archive_path = archive_path;
		package->archive_stamp = stamp;
		package->entry_index = index_zip(stamp[1], read);
		HashSet<String> seen;
		for (const Variant& name_value : package->entry_index.keys()) {
			String name = name_value;
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
			PackedByteArray bytes = read_zip_entry(package->entry_index[name], stamp[1], ZIP64_FIELD, read);
			if (bytes.is_empty()) {
				continue;
			}
			Variant parsed = JSON::parse_string(bytes.get_string_from_utf8());
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
		return package->project_info.is_empty() || get_archive_stamp(archive_path) != stamp ? Ref<LightechZipReader>() : package;
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
		return _read_indexed_file(entry);
	}

	PackedByteArray LightechZipReader::_read_indexed_file(const String& name) const {
		if (get_archive_stamp(archive_path) != archive_stamp) {
			return {};
		}
		Ref<FileAccess> file = FileAccess::open(archive_path, FileAccess::READ);
		if (file.is_null() || file->get_length() != uint64_t(archive_stamp[1])) {
			return {};
		}
		const auto read = [&file](int64_t offset, int64_t length) { return read_file_range(file, offset, length); };
		PackedByteArray data = read_zip_entry(entry_index.get(name, PackedInt64Array()), archive_stamp[1], ZIP64_FIELD, read);
		return get_archive_stamp(archive_path) == archive_stamp ? data : PackedByteArray();
	}

	Dictionary LightechZipReader::index_memory_zip(const PackedByteArray& bytes) {
		const auto read = [&bytes](int64_t offset, int64_t length) { return bytes.slice(offset, offset + length); };
		Dictionary index;
		Dictionary raw_index = index_zip(bytes.size(), read);
		for (const Variant& key : raw_index.keys()) {
			String name = String(key).replace("\\", "/");
			if (!is_relative_path(name.trim_suffix("/")) || index.has(name)) {
				return {};
			}
			if (!name.ends_with("/")) {
				index[name] = raw_index[key];
			}
		}
		return index;
	}

	PackedByteArray LightechZipReader::read_memory_zip_entry(const PackedByteArray& bytes, const Dictionary& index, const String& name) {
		const auto read = [&bytes](int64_t offset, int64_t length) { return bytes.slice(offset, offset + length); };
		return read_zip_entry(index.get(name, PackedInt64Array()), bytes.size(), 256LL << 20, read);
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
		return _read_indexed_file(entry);
	}

}
