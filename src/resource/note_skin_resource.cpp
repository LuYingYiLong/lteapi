#include "note_skin_resource.h"

namespace godot {
	void LTENoteSkinResource::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_data", "data"), &LTENoteSkinResource::set_data);
		ClassDB::bind_method(D_METHOD("get_data"), &LTENoteSkinResource::get_data);
		ClassDB::bind_method(D_METHOD("is_empty"), &LTENoteSkinResource::is_empty);
		ClassDB::bind_method(D_METHOD("get_size"), &LTENoteSkinResource::get_size);

		ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_data", "get_data");
	}

	void LTENoteSkinResource::set_data(const PackedByteArray& p_data) {
		data = p_data;
	}

	PackedByteArray LTENoteSkinResource::get_data() const {
		return data;
	}

	bool LTENoteSkinResource::is_empty() const {
		return data.is_empty();
	}

	int64_t LTENoteSkinResource::get_size() const {
		return data.size();
	}
}
