#ifndef SCENE_LAYERS_SERVER_H
#define SCENE_LAYERS_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace godot {
	class LTESceneLayersServer : public Object {
		GDCLASS(LTESceneLayersServer, Object)

	private:
		static LTESceneLayersServer* singleton;

		String _get_absolute_scene_path(const String& scene_path) const;

	protected:
		static void _bind_methods();

	public:
		LTESceneLayersServer();
		~LTESceneLayersServer();

		static LTESceneLayersServer* get_singleton();

		Vector2 get_scroll(const String& uuid, const String& scene_path) const;
		void set_scroll(const String& uuid, const String& scene_path, const Vector2& scroll);
		PackedStringArray get_collapsed_items(const String& uuid, const String& scene_path) const;
		void set_collapsed_items(const String& uuid, const String& scene_path, const PackedStringArray& items);
	};
}

#endif // SCENE_LAYERS_SERVER_H
