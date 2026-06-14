#include "crypto_random.h"

namespace godot {
	void CryptoRandom::_bind_methods() {
		ClassDB::bind_method(D_METHOD("bytes", "size"), &CryptoRandom::bytes);
	}

	CryptoRandom::CryptoRandom() {
		crypto.instantiate();
	}

	CryptoRandom::~CryptoRandom() {
	}

	PackedByteArray CryptoRandom::bytes(const int64_t size) {
		crypto.instantiate();
		return crypto->generate_random_bytes(size);
	}
}