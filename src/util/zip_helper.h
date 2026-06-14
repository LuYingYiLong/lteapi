#ifndef ZIP_HELPER_H
#define ZIP_HELPER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/zip_packer.hpp>
#include <godot_cpp/classes/zip_reader.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot {
	class ZipHelper : public RefCounted {
		GDCLASS(ZipHelper, RefCounted)

	private:
		static Error _pack_recurse(Ref<ZIPPacker> packer, const String& base, const String& entry);
		static Error _add_file_to_zip(Ref<ZIPPacker> packer, const String& disk_path, const String& zip_path);

	protected:
		static void _bind_methods();

	public:
		static Error pack_folder(const String& src_dir, const String& dst_zip);
		static PackedByteArray read_zip_single_file(const String& zip_path, const String& file_path);
		static Error extract_all_from_zip(const String& zip_path, const String& dst_dir);

	};
}

#endif // !ZIP_HELPER_H
