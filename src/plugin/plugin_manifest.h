#ifndef PLUGIN_MANIFEST_H
#define PLUGIN_MANIFEST_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {
	class LTEPluginManifest : public RefCounted {
		GDCLASS(LTEPluginManifest, RefCounted)

	private:
		String id;
		String name;
		String version;
		String api_version;
		String entry;
		PackedStringArray permissions;
		String min_editor_version;
		String author;
		String description;
		String plugin_type;
		String homepage;
		String repository;
		String license;
		PackedStringArray tags;
		Dictionary dependencies;
		Dictionary optional_dependencies;
		Dictionary details;
		Array last_parse_errors;

		static PackedStringArray get_required_fields();
		static PackedStringArray get_known_permissions();
		static PackedStringArray get_known_plugin_types();
		void _parse_permissions(const Variant& raw_permissions);
		void _parse_dict_field(const Dictionary& dict, const String& key, Dictionary& target);
		void _parse_string_array_field(const Dictionary& dict, const String& key, PackedStringArray& target);

	protected:
		static void _bind_methods();

	public:
		LTEPluginManifest();
		~LTEPluginManifest();

		bool parse_from_json(const String& json_string);
		Dictionary get_parse_result() const;
		static bool is_valid_plugin_id(const String& plugin_id);
		static int32_t compare_versions(const String& a, const String& b);
		Dictionary to_dict() const;
		bool has_dependency(const String& dep_id) const;
		String get_dependency_version(const String& dep_id) const;
		bool has_optional_dependency(const String& dep_id) const;
		String get_optional_dependency_version(const String& dep_id) const;

		String get_id() const { return id; }
		String get_name() const { return name; }
		String get_version() const { return version; }
		String get_api_version() const { return api_version; }
		String get_entry() const { return entry; }
		PackedStringArray get_permissions() const { return permissions; }
		String get_min_editor_version() const { return min_editor_version; }
		String get_author() const { return author; }
		String get_description() const { return description; }
		String get_plugin_type() const { return plugin_type; }
		String get_homepage() const { return homepage; }
		String get_repository() const { return repository; }
		String get_license() const { return license; }
		PackedStringArray get_tags() const { return tags; }
		Dictionary get_dependencies() const { return dependencies; }
		Dictionary get_optional_dependencies() const { return optional_dependencies; }
		Dictionary get_details() const { return details; }
	};
}

#endif // !PLUGIN_MANIFEST_H
