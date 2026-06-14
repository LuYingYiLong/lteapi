#ifndef BUILD_IN_RANDOM_H
#define BUILD_IN_RANDOM_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include "crypto_random.h"

namespace godot {
	class BuildInRandom : public CryptoRandom {
		GDCLASS(BuildInRandom, CryptoRandom)

	private:
		Ref<RandomNumberGenerator> rand = nullptr;

	protected:
		static void _bind_methods();

	public:
		BuildInRandom();
		~BuildInRandom();

		PackedByteArray bytes(const int64_t size) override;
	};
}

#endif // !BUILD_IN_RANDOM_H