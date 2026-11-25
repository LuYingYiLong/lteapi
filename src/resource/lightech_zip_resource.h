#ifndef LIGHTECH_ZIP_RESOURCE_H
#define LIGHTECH_ZIP_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include "lightech_zip_reader.h"

namespace godot {
	class LightechZipResource : public Resource {
		GDCLASS(LightechZipResource, Resource)

	private:
		PackedByteArray data;
		String source_path;
		mutable Ref<LightechZipReader> reader_cache;
		mutable String reader_identity;

	protected:
		static void _bind_methods();

	public:
		void set_data(const PackedByteArray& p_data);
		PackedByteArray get_data() const;
		void set_source_path(const String& p_path);
		String get_source_path() const;
		bool is_empty() const;
		int64_t get_size() const;
		Ref<LightechZipReader> open_reader() const;
		String get_project_path(const String& p_project_root) const;
	};
}

#endif
