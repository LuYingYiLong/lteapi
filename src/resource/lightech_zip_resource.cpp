#include "lightech_zip_resource.h"

#include "lightech_zip_reader.h"

namespace godot {
	void LightechZipResource::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_data", "data"), &LightechZipResource::set_data);
		ClassDB::bind_method(D_METHOD("get_data"), &LightechZipResource::get_data);
		ClassDB::bind_method(D_METHOD("set_source_path", "path"), &LightechZipResource::set_source_path);
		ClassDB::bind_method(D_METHOD("get_source_path"), &LightechZipResource::get_source_path);
		ClassDB::bind_method(D_METHOD("is_empty"), &LightechZipResource::is_empty);
		ClassDB::bind_method(D_METHOD("get_size"), &LightechZipResource::get_size);
		ClassDB::bind_method(D_METHOD("open_reader"), &LightechZipResource::open_reader);
		ClassDB::bind_method(D_METHOD("get_project_path", "project_root"), &LightechZipResource::get_project_path);

		ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_data", "get_data");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "source_path", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_source_path", "get_source_path");
	}

	void LightechZipResource::set_data(const PackedByteArray& p_data) {
		data = p_data;
		reader_cache.unref();
		reader_identity = String();
	}

	PackedByteArray LightechZipResource::get_data() const {
		return data;
	}

	void LightechZipResource::set_source_path(const String& p_path) {
		String normalized_path = p_path.replace("\\", "/").simplify_path();
		if (source_path == normalized_path) {
			return;
		}
		source_path = normalized_path;
		reader_cache.unref();
		reader_identity = String();
	}

	String LightechZipResource::get_source_path() const {
		return source_path;
	}

	bool LightechZipResource::is_empty() const {
		return data.is_empty();
	}

	int64_t LightechZipResource::get_size() const {
		return data.size();
	}

	Ref<LightechZipReader> LightechZipResource::open_reader() const {
		String identity = source_path.is_empty() ? get_path() : source_path;
		if (identity.is_empty()) {
			identity = "memory://lightech_zip_" + String::num_uint64(get_instance_id());
		}
		identity = identity.replace("\\", "/").simplify_path();
		if (reader_cache.is_valid() && reader_identity == identity) {
			return reader_cache;
		}
		reader_cache.unref();
		if (data.is_empty()) {
			return {};
		}
		reader_cache = LightechZipReader::open_memory(data, identity);
		reader_identity = identity;
		return reader_cache;
	}

	String LightechZipResource::get_project_path(const String& p_project_root) const {
		Ref<LightechZipReader> reader = open_reader();
		return reader.is_valid() ? reader->get_project_path(p_project_root) : String();
	}
}
