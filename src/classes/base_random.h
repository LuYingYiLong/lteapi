#ifndef BASE_RANDOM_H
#define BASE_RANDOM_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot {
	class BaseRandom : public RefCounted {
		GDCLASS(BaseRandom, RefCounted)

	protected:
		static void _bind_methods();

	public:
		virtual PackedByteArray bytes(const int64_t size);
	};
}

#endif // !BASE_RANDOM_H