#include "build_in_random.h"

namespace godot {
	void BuildInRandom::_bind_methods() {
	}

	BuildInRandom::BuildInRandom() {
		uint64_t seed = 0;
		PackedByteArray seed_bytes = crypto->generate_random_bytes(4);
		seed |= (static_cast<uint64_t>(seed_bytes[0]) << 24);
		seed |= (static_cast<uint64_t>(seed_bytes[1]) << 16);
		seed |= (static_cast<uint64_t>(seed_bytes[2]) << 8);
		seed |= (seed_bytes[3]);
		rand.instantiate();
		rand->set_seed(seed);
	}

	BuildInRandom::~BuildInRandom() {

	}

	PackedByteArray BuildInRandom::bytes(const int64_t size) {
		uint64_t len_ints = (size + 3) / 4;
		PackedInt32Array ints;
		ints.resize(len_ints);
		for (int index = 0; index < len_ints; ++index) {
			ints.set(index, static_cast<int32_t>(rand->randi()));
		}
		return ints.to_byte_array();
	}
}