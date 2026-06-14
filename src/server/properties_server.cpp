#include "properties_server.h"

namespace godot {
	LTEPropertiesServer* LTEPropertiesServer::singleton = nullptr;

	void LTEPropertiesServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_selection", "context_id", "items", "metadata"), &LTEPropertiesServer::set_selection, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("set_selection_silent", "context_id", "items", "metadata"), &LTEPropertiesServer::set_selection_silent, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("clear_selection", "context_id", "metadata_filter"), &LTEPropertiesServer::clear_selection, DEFVAL(String()), DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("get_selection", "context_id"), &LTEPropertiesServer::get_selection, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("get_selection_context_id"), &LTEPropertiesServer::get_selection_context_id);
		ClassDB::bind_method(D_METHOD("get_selection_items", "context_id"), &LTEPropertiesServer::get_selection_items, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("get_selection_metadata", "context_id"), &LTEPropertiesServer::get_selection_metadata, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("get_all_selections"), &LTEPropertiesServer::get_all_selections);
		ClassDB::bind_method(D_METHOD("has_selection", "context_id"), &LTEPropertiesServer::has_selection, DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("submit_items", "context_id", "old_items", "new_items", "metadata"), &LTEPropertiesServer::submit_items, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("submit_property_patch", "context_id", "patch", "metadata"), &LTEPropertiesServer::submit_property_patch, DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("set_property_schema", "context_id", "schema"), &LTEPropertiesServer::set_property_schema);
		ClassDB::bind_method(D_METHOD("get_property_schema", "context_id"), &LTEPropertiesServer::get_property_schema);
		ClassDB::bind_method(D_METHOD("get_all_property_schemas"), &LTEPropertiesServer::get_all_property_schemas);
		ClassDB::bind_method(D_METHOD("clear_property_schema", "context_id"), &LTEPropertiesServer::clear_property_schema);
		ClassDB::bind_method(D_METHOD("clear_all_property_schemas"), &LTEPropertiesServer::clear_all_property_schemas);

		ADD_SIGNAL(MethodInfo("selection_changed", PropertyInfo(Variant::DICTIONARY, "selection")));
		ADD_SIGNAL(MethodInfo("selection_cleared", PropertyInfo(Variant::DICTIONARY, "previous_selection")));
		ADD_SIGNAL(MethodInfo("items_submitted",
			PropertyInfo(Variant::STRING, "context_id"),
			PropertyInfo(Variant::ARRAY, "old_items"),
			PropertyInfo(Variant::ARRAY, "new_items"),
			PropertyInfo(Variant::DICTIONARY, "metadata")
		));
		ADD_SIGNAL(MethodInfo("property_patch_submitted",
			PropertyInfo(Variant::STRING, "context_id"),
			PropertyInfo(Variant::DICTIONARY, "patch"),
			PropertyInfo(Variant::DICTIONARY, "metadata")
		));
		ADD_SIGNAL(MethodInfo("property_schema_changed",
			PropertyInfo(Variant::STRING, "context_id"),
			PropertyInfo(Variant::DICTIONARY, "schema")
		));
		ADD_SIGNAL(MethodInfo("property_schema_removed", PropertyInfo(Variant::STRING, "context_id")));
	}

	LTEPropertiesServer::LTEPropertiesServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTEPropertiesServer::~LTEPropertiesServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEPropertiesServer* LTEPropertiesServer::get_singleton() {
		return singleton;
	}

	void LTEPropertiesServer::clear_project_state() {
		clear_selection();
		clear_all_property_schemas();
	}

	Dictionary LTEPropertiesServer::make_selection(const String& context_id) const {
		String target_context_id = context_id.is_empty() ? selection_context_id : context_id;
		if (target_context_id.is_empty() || !selections.has(target_context_id)) {
			return Dictionary();
		}
		Dictionary selection = selections[target_context_id];
		return selection.duplicate(true);
	}

	bool LTEPropertiesServer::metadata_matches_filter(const Dictionary& selection, const Dictionary& metadata_filter) const {
		Dictionary metadata = selection.get("metadata", Dictionary());
		Array keys = metadata_filter.keys();
		for (int64_t i = 0; i < keys.size(); i++) {
			Variant key = keys[i];
			if (!metadata.has(key) || metadata.get(key, Variant()) != metadata_filter.get(key, Variant())) {
				return false;
			}
		}
		return true;
	}

	void LTEPropertiesServer::update_active_selection_after_clear() {
		if (!selection_context_id.is_empty() && selections.has(selection_context_id)) {
			return;
		}
		Array context_ids = selections.keys();
		if (context_ids.is_empty()) {
			selection_context_id = String();
			return;
		}
		selection_context_id = String(context_ids[0]);
	}

	void LTEPropertiesServer::set_selection(const String& context_id, const Array& items, const Dictionary& metadata) {
		if (context_id.is_empty()) {
			return;
		}
		Dictionary selection;
		selection["context_id"] = context_id;
		selection["items"] = items.duplicate(true);
		selection["metadata"] = metadata.duplicate(true);
		selections[context_id] = selection;
		selection_context_id = context_id;
		emit_signal("selection_changed", make_selection());
	}

	void LTEPropertiesServer::set_selection_silent(const String& context_id, const Array& items, const Dictionary& metadata) {
		if (context_id.is_empty()) {
			return;
		}
		Dictionary selection;
		selection["context_id"] = context_id;
		selection["items"] = items.duplicate(true);
		selection["metadata"] = metadata.duplicate(true);
		selections[context_id] = selection;
		selection_context_id = context_id;
	}

	void LTEPropertiesServer::clear_selection(const String& context_id, const Dictionary& metadata_filter) {
		if (selections.is_empty()) {
			return;
		}
		if (!context_id.is_empty() && !selections.has(context_id)) {
			return;
		}

		Array context_ids;
		if (context_id.is_empty()) {
			context_ids = selections.keys();
		} else {
			context_ids.append(context_id);
		}

		Array previous_selections;
		for (int64_t i = 0; i < context_ids.size(); i++) {
			String target_context_id = String(context_ids[i]);
			if (!selections.has(target_context_id)) {
				continue;
			}
			Dictionary selection = selections[target_context_id];
			if (!metadata_matches_filter(selection, metadata_filter)) {
				continue;
			}
			previous_selections.append(selection.duplicate(true));
			selections.erase(target_context_id);
		}
		if (previous_selections.is_empty()) {
			return;
		}
		update_active_selection_after_clear();
		emit_signal("selection_changed", make_selection());
		for (int64_t i = 0; i < previous_selections.size(); i++) {
			emit_signal("selection_cleared", previous_selections[i]);
		}
	}

	Dictionary LTEPropertiesServer::get_selection(const String& context_id) const {
		return make_selection(context_id);
	}

	String LTEPropertiesServer::get_selection_context_id() const {
		return selection_context_id;
	}

	Array LTEPropertiesServer::get_selection_items(const String& context_id) const {
		Dictionary selection = make_selection(context_id);
		if (selection.is_empty()) {
			return Array();
		}
		Array items = selection.get("items", Array());
		return items.duplicate(true);
	}

	Dictionary LTEPropertiesServer::get_selection_metadata(const String& context_id) const {
		Dictionary selection = make_selection(context_id);
		if (selection.is_empty()) {
			return Dictionary();
		}
		Dictionary metadata = selection.get("metadata", Dictionary());
		return metadata.duplicate(true);
	}

	Dictionary LTEPropertiesServer::get_all_selections() const {
		return selections.duplicate(true);
	}

	bool LTEPropertiesServer::has_selection(const String& context_id) const {
		return context_id.is_empty() ? !selections.is_empty() : selections.has(context_id);
	}

	void LTEPropertiesServer::submit_items(const String& context_id, const Array& old_items, const Array& new_items, const Dictionary& metadata) {
		if (context_id.is_empty()) {
			return;
		}
		emit_signal("items_submitted", context_id, old_items.duplicate(true), new_items.duplicate(true), metadata.duplicate(true));
	}

	void LTEPropertiesServer::submit_property_patch(const String& context_id, const Dictionary& patch, const Dictionary& metadata) {
		if (context_id.is_empty()) {
			return;
		}
		emit_signal("property_patch_submitted", context_id, patch.duplicate(false), metadata.duplicate(false));
	}

	void LTEPropertiesServer::set_property_schema(const String& context_id, const Dictionary& schema) {
		if (context_id.is_empty()) {
			return;
		}
		Dictionary schema_copy = schema.duplicate(true);
		property_schemas[context_id] = schema_copy;
		emit_signal("property_schema_changed", context_id, schema_copy);
	}

	Dictionary LTEPropertiesServer::get_property_schema(const String& context_id) const {
		if (!property_schemas.has(context_id)) {
			return Dictionary();
		}
		Dictionary schema = property_schemas[context_id];
		return schema.duplicate(true);
	}

	Dictionary LTEPropertiesServer::get_all_property_schemas() const {
		return property_schemas.duplicate(true);
	}

	void LTEPropertiesServer::clear_property_schema(const String& context_id) {
		if (!property_schemas.has(context_id)) {
			return;
		}
		property_schemas.erase(context_id);
		emit_signal("property_schema_removed", context_id);
	}

	void LTEPropertiesServer::clear_all_property_schemas() {
		Array context_ids = property_schemas.keys();
		property_schemas.clear();
		for (int64_t i = 0; i < context_ids.size(); i++) {
			emit_signal("property_schema_removed", context_ids[i]);
		}
	}

} // namespace godot
