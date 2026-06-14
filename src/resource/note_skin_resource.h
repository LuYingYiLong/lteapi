#ifndef NOTE_SKIN_RESOURCE_H
#define NOTE_SKIN_RESOURCE_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

namespace godot {
	class LTENoteSkinResource : public Resource {
		GDCLASS(LTENoteSkinResource, Resource)

	private:
		PackedByteArray data;

	protected:
		static void _bind_methods();

	public:
		void set_data(const PackedByteArray& p_data);
		PackedByteArray get_data() const;
		bool is_empty() const;
		int64_t get_size() const;
	};
}

#endif
