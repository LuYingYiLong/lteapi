#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/marshalls.hpp>

#include "classes/base_random.h"
#include "classes/build_in_random.h"
#include "icon_data.hpp"
#include "keycode_textures.hpp"

#define SAFE_DOUBLE(v, default) (std::isfinite(v) ? (v) : (default))

namespace godot
{
	class Utils : public RefCounted {
		GDCLASS(Utils, RefCounted)

	protected:
		static void _bind_methods();

	public:
		enum UUIDVersion {
			UUID_V7
		};

		static Dictionary load_json_file(const String& file_path);
		static Error save_json_file(const String& path, const Dictionary& dict, const String& indent = "\t");
		static double hms_to_seconds(const String& p_hms_string);
		static double ms_to_seconds(const String& p_ms_string);
		static String seconds_to_hms(const double sec);
		static String seconds_to_ms(const double sec);
		static bool is_absolute_path(const String& path);
		static bool check_dwm_at_runtime();
		static String uuid(UUIDVersion version = UUIDVersion::UUID_V7);
		static Variant load_variant(const String& p_path);
		static Ref<Texture2D> get_keycode_texture(const Key key);
		static Ref<Image> get_icon_image();

		static String normalize_path(const String& path);
		static String normalize_directory_path(const String& path);
		static bool is_windows_drive_path(const String& path);
		static bool is_unc_absolute_path(const String& path);
		static bool is_project_resource_path(const String& path);
		static bool path_exists(const String& path);
		static bool is_path_in_project(const String& path, const String& root_dir);
		static String make_project_relative_path(const String& file_path, const String& root_dir);
		static String resolve_project_path(const String& file_path, const String& root_dir);
		static String make_available_child_path(const String& parent_dir, const String& file_name);
		static String read_file_content_safe(const String& file_path);

		template <typename T>
		static Ref<T> load(const String& p_path) {
			ResourceLoader* loader = ResourceLoader::get_singleton();
			if (loader == nullptr) return Ref<T>();
			Ref<Resource> res = loader->load(p_path);
			if (res.is_null()) return Ref<T>();
			Ref<T> result = Object::cast_to<T>(res.ptr());
			return result;
		}
	};
}

VARIANT_ENUM_CAST(Utils::UUIDVersion);

#endif // !UTILS_H
