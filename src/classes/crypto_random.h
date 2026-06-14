#ifndef CRYPTO_RANDOM_H
#define CRYPTO_RANDOM_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/crypto.hpp>
#include "base_random.h"

namespace godot {
	class CryptoRandom : public BaseRandom {
		GDCLASS(CryptoRandom, BaseRandom)

	protected:
		static void _bind_methods();

	public:
		CryptoRandom();
		~CryptoRandom();

		Ref<Crypto> crypto;

		virtual PackedByteArray bytes(const int64_t size) override;
	};
}

#endif // !CRYPTO_RANDOM_H