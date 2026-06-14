#include "base_random.h"

namespace godot {
	void BaseRandom::_bind_methods() {
		ClassDB::bind_method(D_METHOD("bytes", "size"), &BaseRandom::bytes);
	}
	
	PackedByteArray BaseRandom::bytes(const int64_t size) {
		PackedByteArray result;
		result.resize(size);
		return result;
	}
}