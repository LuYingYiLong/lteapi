#ifndef LIGHTECH_ZIP_READER_H
#define LIGHTECH_ZIP_READER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
	class LightechZipReader : public RefCounted {
		GDCLASS(LightechZipReader, RefCounted)

	private:
		String archive_path;
		PackedInt64Array archive_stamp;
		Dictionary entry_index;
		Dictionary project_info;
		PackedStringArray entries;
		Dictionary entry_names;
		PackedByteArray _read_indexed_file(const String& name) const;

	protected:
		static void _bind_methods();

	public:
		static bool is_archive(const String& path);
		static PackedInt64Array get_archive_stamp(const String& path);
		static Ref<LightechZipReader> open(const String& path);
		static PackedByteArray read_entry(const String& archive_path, const String& entry);
		static bool is_package_path(const String& path);
		static String resolve_asset(const String& project_path, const String& path);
		static PackedByteArray read_asset(const String& path);
		static Dictionary index_memory_zip(const PackedByteArray& bytes);
		static PackedByteArray read_memory_zip_entry(const PackedByteArray& bytes, const Dictionary& index, const String& name);
		PackedByteArray read_resource(const String& path) const;
		PackedStringArray get_project_roots() const;
		Dictionary get_info(const String& project_root) const;
		PackedStringArray get_entries() const;
		String get_entry_name(const String& project_root, const String& relative_path) const;
		PackedByteArray read_project_file(const String& project_root, const String& relative_path) const;
	};
}

#endif // LIGHTECH_ZIP_READER_H
