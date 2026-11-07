#ifndef LIGHTECH_ZIP_READER_H
#define LIGHTECH_ZIP_READER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
	class LightechZipReader : public RefCounted {
		GDCLASS(LightechZipReader, RefCounted)

	private:
		String content_path;
		Dictionary project_info;
		PackedStringArray entries;

	protected:
		static void _bind_methods();

	public:
		static bool is_archive(const String& path);
		static Ref<LightechZipReader> open(const String& path, bool verify_content = false);
		PackedStringArray get_project_roots() const;
		Dictionary get_info(const String& project_root) const;
		String get_root_path(const String& project_root) const;
		PackedStringArray get_entries() const;
	};
}

#endif // LIGHTECH_ZIP_READER_H
