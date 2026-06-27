#include "inspector_registry.h"

namespace godot {
	LTEInspectorRegistry *LTEInspectorRegistry::singleton = nullptr;

	/* Supported property types */
	static const char *VALID_PROPERTY_TYPES[] = {
		"bool", "int", "float", "string", "enum", "color", "color_hex",
		"vector2", "vector3", "vector4", "file", "resource", "action", "custom", "path_editor", nullptr
	};

	/* Required top-level schema keys */
	static const char *REQUIRED_SCHEMA_KEYS[] = {
		"schema_version", "layer_type", "sections", nullptr
	};

	/* Required property entry keys */
	static const char *REQUIRED_PROPERTY_KEYS[] = {
		"id", "type", "label", nullptr
	};

	void LTEInspectorRegistry::_bind_methods() {
		/* Layer type schema */
		ClassDB::bind_method(D_METHOD("register_layer_type_schema", "layer_type", "schema"),
			&LTEInspectorRegistry::register_layer_type_schema);
		ClassDB::bind_method(D_METHOD("unregister_layer_type_schema", "layer_type"),
			&LTEInspectorRegistry::unregister_layer_type_schema);
		ClassDB::bind_method(D_METHOD("get_layer_type_schema", "layer_type"),
			&LTEInspectorRegistry::get_layer_type_schema);
		ClassDB::bind_method(D_METHOD("get_registered_layer_types"),
			&LTEInspectorRegistry::get_registered_layer_types);
		ClassDB::bind_method(D_METHOD("has_layer_type_schema", "layer_type"),
			&LTEInspectorRegistry::has_layer_type_schema);

		/* Schema validation */
		ClassDB::bind_method(D_METHOD("validate_schema", "schema"),
			&LTEInspectorRegistry::validate_schema);

		/* Schema merging & sorting */
		ClassDB::bind_method(D_METHOD("merge_schemas", "base_schema", "override_schema"),
			&LTEInspectorRegistry::merge_schemas);
		ClassDB::bind_method(D_METHOD("sort_schema_sections", "sections", "sort_key"),
			&LTEInspectorRegistry::sort_schema_sections, DEFVAL("id"));

		/* Custom section */
		ClassDB::bind_method(D_METHOD("register_custom_section", "section_id", "scene_path", "label", "options"),
			&LTEInspectorRegistry::register_custom_section, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("unregister_custom_section", "section_id"),
			&LTEInspectorRegistry::unregister_custom_section);
		ClassDB::bind_method(D_METHOD("get_custom_section_info", "section_id"),
			&LTEInspectorRegistry::get_custom_section_info);
		ClassDB::bind_method(D_METHOD("has_custom_section", "section_id"),
			&LTEInspectorRegistry::has_custom_section);
		ClassDB::bind_method(D_METHOD("get_custom_sections_for_context", "context"),
			&LTEInspectorRegistry::get_custom_sections_for_context);

		/* Visibility */
		ClassDB::bind_method(D_METHOD("evaluate_visibility_condition", "condition", "data_context"),
			&LTEInspectorRegistry::evaluate_visibility_condition);

		/* Schema version */
		ClassDB::bind_method(D_METHOD("set_schema_version", "schema_id", "version"),
			&LTEInspectorRegistry::set_schema_version);
		ClassDB::bind_method(D_METHOD("get_schema_version", "schema_id"),
			&LTEInspectorRegistry::get_schema_version);

		ADD_SIGNAL(MethodInfo("inspector_registry_changed",
			PropertyInfo(Variant::STRING, "change_type"),
			PropertyInfo(Variant::STRING, "entry_id")));
	}

	LTEInspectorRegistry::LTEInspectorRegistry() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTEInspectorRegistry::~LTEInspectorRegistry() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEInspectorRegistry *LTEInspectorRegistry::get_singleton() {
		return singleton;
	}

	/* ---- Layer type schema registration ---- */

	void LTEInspectorRegistry::register_layer_type_schema(const String &layer_type, const Dictionary &schema) {
		ERR_FAIL_COND(layer_type.is_empty());
		Dictionary schema_copy = schema.duplicate(true);
		layer_type_schemas[layer_type] = schema_copy;
		emit_signal("inspector_registry_changed", "layer_type_schema_added", layer_type);
	}

	void LTEInspectorRegistry::unregister_layer_type_schema(const String &layer_type) {
		if (!layer_type_schemas.has(layer_type)) {
			return;
		}
		layer_type_schemas.erase(layer_type);
		emit_signal("inspector_registry_changed", "layer_type_schema_removed", layer_type);
	}

	Dictionary LTEInspectorRegistry::get_layer_type_schema(const String &layer_type) const {
		if (!layer_type_schemas.has(layer_type)) {
			return Dictionary();
		}
		Dictionary schema = layer_type_schemas[layer_type];
		return schema.duplicate(true);
	}

	Array LTEInspectorRegistry::get_registered_layer_types() const {
		return layer_type_schemas.keys();
	}

	bool LTEInspectorRegistry::has_layer_type_schema(const String &layer_type) const {
		return layer_type_schemas.has(layer_type);
	}

	/* ---- Schema validation ---- */

	bool LTEInspectorRegistry::_is_valid_property_type(const String &type) const {
		for (int i = 0; VALID_PROPERTY_TYPES[i] != nullptr; i++) {
			if (type == VALID_PROPERTY_TYPES[i]) {
				return true;
			}
		}
		return false;
	}

	bool LTEInspectorRegistry::_validate_property_entry(const Dictionary &entry) const {
		/* Check required keys */
		for (int i = 0; REQUIRED_PROPERTY_KEYS[i] != nullptr; i++) {
			if (!entry.has(REQUIRED_PROPERTY_KEYS[i])) {
				return false;
			}
		}
		/* Validate type */
		String type = entry["type"];
		if (!_is_valid_property_type(type)) {
			return false;
		}
		/* Enum type must have 'values' array or 'hint' string */
		if (type == "enum") {
			bool has_values = entry.has("values") && entry["values"].get_type() == Variant::ARRAY;
			bool has_hint = entry.has("hint") && entry["hint"].get_type() == Variant::STRING;
			if (!has_values && !has_hint) {
				return false;
			}
		}
		return true;
	}

	bool LTEInspectorRegistry::_validate_section_entry(const Dictionary &section) const {
		if (!section.has("id") || !section.has("properties")) {
			return false;
		}
		Variant properties_var = section["properties"];
		if (properties_var.get_type() != Variant::ARRAY) {
			return false;
		}
		Array properties = properties_var;
		for (int64_t i = 0; i < properties.size(); i++) {
			Variant prop_var = properties[i];
			if (prop_var.get_type() != Variant::DICTIONARY) {
				return false;
			}
			if (!_validate_property_entry(Dictionary(prop_var))) {
				return false;
			}
		}
		return true;
	}

	bool LTEInspectorRegistry::validate_schema(const Dictionary &schema) const {
		if (schema.is_empty()) {
			return false;
		}
		/* Check required top-level keys */
		for (int i = 0; REQUIRED_SCHEMA_KEYS[i] != nullptr; i++) {
			if (!schema.has(REQUIRED_SCHEMA_KEYS[i])) {
				return false;
			}
		}
		/* Validate sections array */
		Variant sections_var = schema["sections"];
		if (sections_var.get_type() != Variant::ARRAY) {
			return false;
		}
		Array sections = sections_var;
		for (int64_t i = 0; i < sections.size(); i++) {
			Variant sec_var = sections[i];
			if (sec_var.get_type() != Variant::DICTIONARY) {
				return false;
			}
			if (!_validate_section_entry(Dictionary(sec_var))) {
				return false;
			}
		}
		return true;
	}

	/* ---- Schema merging & sorting ---- */

	Dictionary LTEInspectorRegistry::merge_schemas(const Dictionary &base_schema, const Dictionary &override_schema) const {
		if (base_schema.is_empty()) {
			return override_schema.duplicate(true);
		}
		if (override_schema.is_empty()) {
			return base_schema.duplicate(true);
		}

		Dictionary result = base_schema.duplicate(true);
		Array override_sections = override_schema.get("sections", Array());

		if (!result.has("sections")) {
			result["sections"] = override_sections.duplicate(true);
			return result;
		}

		Array base_sections = result["sections"];
		/* Build a lookup by section id from base */
		Dictionary section_lookup;
		for (int64_t i = 0; i < base_sections.size(); i++) {
			Dictionary sec = base_sections[i];
			String sec_id = sec.get("id", "");
			if (!sec_id.is_empty()) {
				section_lookup[sec_id] = i;
			}
		}

		/* Apply overrides */
		for (int64_t i = 0; i < override_sections.size(); i++) {
			Dictionary ov_sec = override_sections[i];
			String ov_sec_id = ov_sec.get("id", "");
			if (ov_sec_id.is_empty()) {
				continue;
			}

			if (section_lookup.has(ov_sec_id)) {
				/* Merge properties within existing section */
				int64_t base_idx = section_lookup[ov_sec_id];
				Dictionary base_sec = base_sections[base_idx];
				Array base_props = base_sec.get("properties", Array());
				Array ov_props = ov_sec.get("properties", Array());

				/* Build property lookup from base */
				Dictionary prop_lookup;
				for (int64_t j = 0; j < base_props.size(); j++) {
					Dictionary prop = base_props[j];
					String prop_id = prop.get("id", "");
					if (!prop_id.is_empty()) {
						prop_lookup[prop_id] = j;
					}
				}

				/* Override or append properties */
				for (int64_t j = 0; j < ov_props.size(); j++) {
					Dictionary ov_prop = ov_props[j];
					String ov_prop_id = ov_prop.get("id", "");
					if (ov_prop_id.is_empty()) {
						continue;
					}
					if (prop_lookup.has(ov_prop_id)) {
						base_props[prop_lookup[ov_prop_id]] = ov_prop.duplicate(true);
					} else {
						base_props.append(ov_prop.duplicate(true));
					}
				}
				base_sec["properties"] = base_props;
				base_sections[base_idx] = base_sec;
			} else {
				/* Append new section */
				base_sections.append(ov_sec.duplicate(true));
				section_lookup[ov_sec_id] = base_sections.size() - 1;
			}
		}

		result["sections"] = base_sections;

		/* Merge custom_sections */
		Array base_custom = result.get("custom_sections", Array());
		Array ov_custom = override_schema.get("custom_sections", Array());

		Dictionary custom_lookup;
		for (int64_t i = 0; i < base_custom.size(); i++) {
			Dictionary cs = base_custom[i];
			String slot = cs.get("slot", "");
			if (!slot.is_empty()) {
				custom_lookup[slot] = i;
			}
		}

		for (int64_t i = 0; i < ov_custom.size(); i++) {
			Dictionary ov_cs = ov_custom[i];
			String ov_slot = ov_cs.get("slot", "");
			if (ov_slot.is_empty()) {
				continue;
			}
			if (custom_lookup.has(ov_slot)) {
				base_custom[custom_lookup[ov_slot]] = ov_cs.duplicate(true);
			} else {
				base_custom.append(ov_cs.duplicate(true));
			}
		}
		result["custom_sections"] = base_custom;

		return result;
	}

	Array LTEInspectorRegistry::sort_schema_sections(const Array &sections, const String &sort_key) const {
		if (sections.size() <= 1) {
			return sections.duplicate(true);
		}

		Array sorted = sections.duplicate(true);
		/* Simple bubble sort by section id */
		for (int64_t i = 0; i < sorted.size() - 1; i++) {
			for (int64_t j = 0; j < sorted.size() - i - 1; j++) {
				Dictionary a = sorted[j];
				Dictionary b = sorted[j + 1];
				String key_a = a.get(sort_key, "");
				String key_b = b.get(sort_key, "");
				if (key_a > key_b) {
					Variant temp = sorted[j];
					sorted[j] = sorted[j + 1];
					sorted[j + 1] = temp;
				}
			}
		}
		return sorted;
	}

	/* ---- Custom section registration ---- */

	void LTEInspectorRegistry::register_custom_section(const String &section_id, const String &scene_path,
			const String &label, const Dictionary &options) {
		ERR_FAIL_COND(section_id.is_empty());
		ERR_FAIL_COND(scene_path.is_empty());

		Dictionary entry;
		entry["section_id"] = section_id;
		entry["scene_path"] = scene_path;
		entry["label"] = label;
		entry["options"] = options.duplicate(true);

		custom_section_registry[section_id] = entry;
		emit_signal("inspector_registry_changed", "custom_section_added", section_id);
	}

	void LTEInspectorRegistry::unregister_custom_section(const String &section_id) {
		if (!custom_section_registry.has(section_id)) {
			return;
		}
		custom_section_registry.erase(section_id);
		emit_signal("inspector_registry_changed", "custom_section_removed", section_id);
	}

	Dictionary LTEInspectorRegistry::get_custom_section_info(const String &section_id) const {
		if (!custom_section_registry.has(section_id)) {
			return Dictionary();
		}
		Dictionary entry = custom_section_registry[section_id];
		return entry.duplicate(true);
	}

	bool LTEInspectorRegistry::has_custom_section(const String &section_id) const {
		return custom_section_registry.has(section_id);
	}

	Array LTEInspectorRegistry::get_custom_sections_for_context(const Dictionary &context) const {
		Array result;
		Array keys = custom_section_registry.keys();
		for (int64_t i = 0; i < keys.size(); i++) {
			Dictionary entry = custom_section_registry[keys[i]];
			Dictionary options = entry.get("options", Dictionary());
			/* Check visibility condition if present */
			if (options.has("visible_when")) {
				Dictionary condition = options["visible_when"];
				if (!evaluate_visibility_condition(condition, context)) {
					continue;
				}
			}
			result.append(entry.duplicate(true));
		}
		return result;
	}

	/* ---- Visibility conditions ---- */

	bool LTEInspectorRegistry::_evaluate_single_condition(const Dictionary &condition, const Dictionary &data_context) const {
		String op = condition.get("operator", "eq");
		String property = condition.get("property", "");

		if (property.is_empty() && op != "and" && op != "or" && op != "not") {
			return false;
		}

		if (op == "eq") {
			Variant expected = condition.get("value", Variant());
			Variant actual = data_context.get(property, Variant());
			return actual == expected;
		}

		if (op == "neq") {
			Variant expected = condition.get("value", Variant());
			Variant actual = data_context.get(property, Variant());
			return actual != expected;
		}

		if (op == "has") {
			return data_context.has(property);
		}

		if (op == "not") {
			Dictionary inner = condition.get("condition", Dictionary());
			if (inner.is_empty()) {
				return false;
			}
			return !_evaluate_single_condition(inner, data_context);
		}

		if (op == "and") {
			Array conditions = condition.get("conditions", Array());
			if (conditions.is_empty()) {
				return false;
			}
			for (int64_t i = 0; i < conditions.size(); i++) {
				Dictionary cond = conditions[i];
				if (!_evaluate_single_condition(cond, data_context)) {
					return false;
				}
			}
			return true;
		}

		if (op == "or") {
			Array conditions = condition.get("conditions", Array());
			if (conditions.is_empty()) {
				return false;
			}
			for (int64_t i = 0; i < conditions.size(); i++) {
				Dictionary cond = conditions[i];
				if (_evaluate_single_condition(cond, data_context)) {
					return true;
				}
			}
			return false;
		}

		return false;
	}

	bool LTEInspectorRegistry::evaluate_visibility_condition(const Dictionary &condition, const Dictionary &data_context) const {
		if (condition.is_empty()) {
			return true; /* No condition = always visible */
		}
		return _evaluate_single_condition(condition, data_context);
	}

	/* ---- Schema version tracking ---- */

	void LTEInspectorRegistry::set_schema_version(const String &schema_id, const String &version) {
		ERR_FAIL_COND(schema_id.is_empty());
		schema_versions[schema_id] = version;
	}

	String LTEInspectorRegistry::get_schema_version(const String &schema_id) const {
		if (!schema_versions.has(schema_id)) {
			return "";
		}
		return schema_versions[schema_id];
	}

} /* namespace godot */
