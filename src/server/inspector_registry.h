#ifndef INSPECTOR_REGISTRY_H
#define INSPECTOR_REGISTRY_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	class LTEInspectorRegistry : public Object {
		GDCLASS(LTEInspectorRegistry, Object)

	private:
		static LTEInspectorRegistry *singleton;

		Dictionary layer_type_schemas;
		Dictionary custom_section_registry;
		Dictionary schema_versions;

		bool _validate_property_entry(const Dictionary &entry) const;
		bool _validate_section_entry(const Dictionary &section) const;
		bool _is_valid_property_type(const String &type) const;
		bool _evaluate_single_condition(const Dictionary &condition, const Dictionary &data_context) const;

	protected:
		static void _bind_methods();

	public:
		LTEInspectorRegistry();
		~LTEInspectorRegistry();

		static LTEInspectorRegistry *get_singleton();

		/* Layer type schema registration */
		void register_layer_type_schema(const String &layer_type, const Dictionary &schema);
		void unregister_layer_type_schema(const String &layer_type);
		Dictionary get_layer_type_schema(const String &layer_type) const;
		Array get_registered_layer_types() const;
		bool has_layer_type_schema(const String &layer_type) const;

		/* Schema validation */
		bool validate_schema(const Dictionary &schema) const;

		/* Schema merging & sorting */
		Dictionary merge_schemas(const Dictionary &base_schema, const Dictionary &override_schema) const;
		Array sort_schema_sections(const Array &sections, const String &sort_key = "id") const;

		/* Custom section registration */
		void register_custom_section(const String &section_id, const String &scene_path, const String &label, const Dictionary &options = Dictionary());
		void unregister_custom_section(const String &section_id);
		Dictionary get_custom_section_info(const String &section_id) const;
		bool has_custom_section(const String &section_id) const;
		Array get_custom_sections_for_context(const Dictionary &context) const;

		/* Visibility conditions */
		bool evaluate_visibility_condition(const Dictionary &condition, const Dictionary &data_context) const;

		/* Schema version tracking */
		void set_schema_version(const String &schema_id, const String &version);
		String get_schema_version(const String &schema_id) const;
	};
}

#endif /* INSPECTOR_REGISTRY_H */
