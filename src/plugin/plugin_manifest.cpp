#include "plugin_manifest.h"

#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	void LTEPluginManifest::_bind_methods() {
		ClassDB::bind_method(D_METHOD("parse_from_json", "json_string"), &LTEPluginManifest::parse_from_json);
		ClassDB::bind_method(D_METHOD("get_parse_result"), &LTEPluginManifest::get_parse_result);
		ClassDB::bind_static_method("LTEPluginManifest", D_METHOD("is_valid_plugin_id", "plugin_id"), &LTEPluginManifest::is_valid_plugin_id);
		ClassDB::bind_static_method("LTEPluginManifest", D_METHOD("compare_versions", "a", "b"), &LTEPluginManifest::compare_versions);
		ClassDB::bind_method(D_METHOD("to_dict"), &LTEPluginManifest::to_dict);
		ClassDB::bind_method(D_METHOD("has_dependency", "dep_id"), &LTEPluginManifest::has_dependency);
		ClassDB::bind_method(D_METHOD("get_dependency_version", "dep_id"), &LTEPluginManifest::get_dependency_version);
		ClassDB::bind_method(D_METHOD("has_optional_dependency", "dep_id"), &LTEPluginManifest::has_optional_dependency);
		ClassDB::bind_method(D_METHOD("get_optional_dependency_version", "dep_id"), &LTEPluginManifest::get_optional_dependency_version);

		ClassDB::bind_method(D_METHOD("get_id"), &LTEPluginManifest::get_id);
		ClassDB::bind_method(D_METHOD("get_name"), &LTEPluginManifest::get_name);
		ClassDB::bind_method(D_METHOD("get_version"), &LTEPluginManifest::get_version);
		ClassDB::bind_method(D_METHOD("get_api_version"), &LTEPluginManifest::get_api_version);
		ClassDB::bind_method(D_METHOD("get_entry"), &LTEPluginManifest::get_entry);
		ClassDB::bind_method(D_METHOD("get_permissions"), &LTEPluginManifest::get_permissions);
		ClassDB::bind_method(D_METHOD("get_min_editor_version"), &LTEPluginManifest::get_min_editor_version);
		ClassDB::bind_method(D_METHOD("get_author"), &LTEPluginManifest::get_author);
		ClassDB::bind_method(D_METHOD("get_description"), &LTEPluginManifest::get_description);
		ClassDB::bind_method(D_METHOD("get_plugin_type"), &LTEPluginManifest::get_plugin_type);
		ClassDB::bind_method(D_METHOD("get_homepage"), &LTEPluginManifest::get_homepage);
		ClassDB::bind_method(D_METHOD("get_repository"), &LTEPluginManifest::get_repository);
		ClassDB::bind_method(D_METHOD("get_license"), &LTEPluginManifest::get_license);
		ClassDB::bind_method(D_METHOD("get_tags"), &LTEPluginManifest::get_tags);
		ClassDB::bind_method(D_METHOD("get_dependencies"), &LTEPluginManifest::get_dependencies);
		ClassDB::bind_method(D_METHOD("get_optional_dependencies"), &LTEPluginManifest::get_optional_dependencies);
		ClassDB::bind_method(D_METHOD("get_details"), &LTEPluginManifest::get_details);
	}

	LTEPluginManifest::LTEPluginManifest() {
	}

	LTEPluginManifest::~LTEPluginManifest() {
	}

	PackedStringArray LTEPluginManifest::get_required_fields() {
		PackedStringArray fields;
		fields.append("id");
		fields.append("name");
		fields.append("version");
		fields.append("api_version");
		fields.append("entry");
		return fields;
	}

	PackedStringArray LTEPluginManifest::get_known_permissions() {
		PackedStringArray perms;
		perms.append("editor_panels");
		perms.append("project_files");
		perms.append("external_files");
		perms.append("network");
		perms.append("process");
		perms.append("settings");
		perms.append("session");
		perms.append("workspace_layout");
		perms.append("menus");
		perms.append("export");
		perms.append("background_service");
		return perms;
	}

	PackedStringArray LTEPluginManifest::get_known_plugin_types() {
		PackedStringArray types;
		types.append("panel");
		types.append("tool");
		types.append("service");
		types.append("theme");
		types.append("library");
		return types;
	}

	void LTEPluginManifest::_parse_permissions(const Variant& raw_permissions) {
		permissions.clear();
		if (raw_permissions.get_type() != Variant::ARRAY) {
			return;
		}
		Array perm_array = raw_permissions;
		PackedStringArray known = get_known_permissions();
		Array unknown_perms;
		for (int64_t i = 0; i < perm_array.size(); i++) {
			String perm_str = String(perm_array[i]).strip_edges();
			if (perm_str.is_empty()) {
				continue;
			}
			if (known.has(perm_str)) {
				permissions.append(perm_str);
			}
			else {
				unknown_perms.append(perm_str);
			}
		}
		if (!unknown_perms.is_empty()) {
			details["unknown_permissions"] = unknown_perms;
		}
	}

	void LTEPluginManifest::_parse_dict_field(const Dictionary& dict, const String& key, Dictionary& target) {
		target.clear();
		Variant raw = dict.get(key, Variant());
		if (raw.get_type() == Variant::DICTIONARY) {
			Dictionary raw_dict = raw;
			Array keys = raw_dict.keys();
			for (int64_t i = 0; i < keys.size(); i++) {
				String k = String(keys[i]);
				target[k] = String(raw_dict[k]);
			}
		}
	}

	void LTEPluginManifest::_parse_string_array_field(const Dictionary& dict, const String& key, PackedStringArray& target) {
		target.clear();
		Variant raw = dict.get(key, Variant());
		if (raw.get_type() == Variant::ARRAY) {
			Array raw_array = raw;
			for (int64_t i = 0; i < raw_array.size(); i++) {
				String val = String(raw_array[i]).strip_edges();
				if (!val.is_empty()) {
					target.append(val);
				}
			}
		}
	}

	bool LTEPluginManifest::parse_from_json(const String& json_string) {
		last_parse_errors.clear();
		details.clear();

		Ref<JSON> json;
		json.instantiate();
		Error err = json->parse(json_string);
		if (err != OK) {
			last_parse_errors.append(String("JSON 解析失败: ") + json->get_error_message());
			return false;
		}

		Variant data = json->get_data();
		if (data.get_type() != Variant::DICTIONARY) {
			last_parse_errors.append(String("manifest 根元素必须是 JSON 对象"));
			return false;
		}

		Dictionary dict = data;

		PackedStringArray required = get_required_fields();
		for (int64_t i = 0; i < required.size(); i++) {
			String field = required[i];
			if (!dict.has(field) || String(dict[field]).is_empty()) {
				last_parse_errors.append(String("缺少必要字段: ") + field);
				return false;
			}
		}

		id = String(dict["id"]).strip_edges();
		name = String(dict["name"]).strip_edges();
		version = String(dict["version"]).strip_edges();
		api_version = String(dict["api_version"]).strip_edges();
		entry = String(dict["entry"]).strip_edges();
		author = String(dict.get("author", "")).strip_edges();
		description = String(dict.get("description", "")).strip_edges();
		min_editor_version = String(dict.get("min_editor_version", "")).strip_edges();
		plugin_type = String(dict.get("plugin_type", "")).strip_edges();
		homepage = String(dict.get("homepage", "")).strip_edges();
		repository = String(dict.get("repository", "")).strip_edges();
		license = String(dict.get("license", "")).strip_edges();

		_parse_permissions(dict.get("permissions", Array()));
		_parse_dict_field(dict, "dependencies", dependencies);
		_parse_dict_field(dict, "optional_dependencies", optional_dependencies);
		_parse_string_array_field(dict, "tags", tags);

		if (id.is_empty() || id.contains(" ")) {
			last_parse_errors.append(String("插件 ID 无效: '") + id + "'");
			return false;
		}

		if (!entry.begins_with("res://plugins/")) {
			last_parse_errors.append(String("entry 必须以 'res://plugins/' 开头: '") + entry + "'");
			return false;
		}

		PackedStringArray known_types = get_known_plugin_types();
		if (!plugin_type.is_empty() && !known_types.has(plugin_type)) {
			details["unknown_plugin_type"] = plugin_type;
		}

		Array known_keys;
		known_keys.append("id");
		known_keys.append("name");
		known_keys.append("version");
		known_keys.append("api_version");
		known_keys.append("entry");
		known_keys.append("author");
		known_keys.append("description");
		known_keys.append("min_editor_version");
		known_keys.append("plugin_type");
		known_keys.append("homepage");
		known_keys.append("repository");
		known_keys.append("license");
		known_keys.append("permissions");
		known_keys.append("dependencies");
		known_keys.append("optional_dependencies");
		known_keys.append("tags");

		Array dict_keys = dict.keys();
		for (int64_t i = 0; i < dict_keys.size(); i++) {
			String key = String(dict_keys[i]);
			if (!known_keys.has(key)) {
				details[key] = dict[key];
			}
		}

		return true;
	}

	Dictionary LTEPluginManifest::get_parse_result() const {
		Dictionary result;
		result["ok"] = last_parse_errors.is_empty();
		result["errors"] = last_parse_errors;
		return result;
	}

	bool LTEPluginManifest::is_valid_plugin_id(const String& plugin_id) {
		if (plugin_id.is_empty()) {
			return false;
		}
		for (int64_t i = 0; i < plugin_id.length(); i++) {
			char32_t code = plugin_id[i];
			bool valid = (code >= '0' && code <= '9') || (code >= 'A' && code <= 'Z') || (code >= 'a' && code <= 'z') || code == '_' || code == '-' || code == '.';
			if (!valid) {
				return false;
			}
		}
		return true;
	}

	int32_t LTEPluginManifest::compare_versions(const String& a, const String& b) {
		if (a.is_empty() && b.is_empty()) {
			return 0;
		}
		if (a.is_empty()) {
			return -1;
		}
		if (b.is_empty()) {
			return 1;
		}

		PackedStringArray a_parts = a.split(".");
		PackedStringArray b_parts = b.split(".");
		int64_t max_len = a_parts.size() > b_parts.size() ? a_parts.size() : b_parts.size();

		for (int64_t i = 0; i < max_len; i++) {
			String a_part = i < a_parts.size() ? a_parts[i] : "0";
			String b_part = i < b_parts.size() ? b_parts[i] : "0";

			int64_t a_num = 0;
			int64_t b_num = 0;
			String a_pre;
			String b_pre;

			int64_t dash_a = a_part.find("-");
			int64_t dash_b = b_part.find("-");

			if (dash_a >= 0) {
				a_num = a_part.substr(0, dash_a).to_int();
				a_pre = a_part.substr(dash_a + 1);
			}
			else {
				a_num = a_part.to_int();
			}

			if (dash_b >= 0) {
				b_num = b_part.substr(0, dash_b).to_int();
				b_pre = b_part.substr(dash_b + 1);
			}
			else {
				b_num = b_part.to_int();
			}

			if (a_num > b_num) return 1;
			if (a_num < b_num) return -1;

			bool a_has_pre = !a_pre.is_empty();
			bool b_has_pre = !b_pre.is_empty();

			if (!a_has_pre && b_has_pre) return 1;
			if (a_has_pre && !b_has_pre) return -1;

			if (a_has_pre && b_has_pre) {
				int64_t cmp = a_pre.naturalnocasecmp_to(b_pre);
				if (cmp != 0) return cmp > 0 ? 1 : -1;
			}
		}
		return 0;
	}

	Dictionary LTEPluginManifest::to_dict() const {
		Dictionary d;
		d["id"] = id;
		d["name"] = name;
		d["version"] = version;
		d["api_version"] = api_version;
		d["entry"] = entry;
		d["permissions"] = permissions;
		d["min_editor_version"] = min_editor_version;
		d["author"] = author;
		d["description"] = description;

		if (!plugin_type.is_empty()) {
			d["plugin_type"] = plugin_type;
		}
		if (!homepage.is_empty()) {
			d["homepage"] = homepage;
		}
		if (!repository.is_empty()) {
			d["repository"] = repository;
		}
		if (!license.is_empty()) {
			d["license"] = license;
		}
		if (!tags.is_empty()) {
			d["tags"] = tags;
		}
		if (!dependencies.is_empty()) {
			d["dependencies"] = dependencies;
		}
		if (!optional_dependencies.is_empty()) {
			d["optional_dependencies"] = optional_dependencies;
		}

		Array details_keys = details.keys();
		for (int64_t i = 0; i < details_keys.size(); i++) {
			String key = String(details_keys[i]);
			d[key] = details[key];
		}

		return d;
	}

	bool LTEPluginManifest::has_dependency(const String& dep_id) const {
		return dependencies.has(dep_id);
	}

	String LTEPluginManifest::get_dependency_version(const String& dep_id) const {
		return dependencies.get(dep_id, String());
	}

	bool LTEPluginManifest::has_optional_dependency(const String& dep_id) const {
		return optional_dependencies.has(dep_id);
	}

	String LTEPluginManifest::get_optional_dependency_version(const String& dep_id) const {
		return optional_dependencies.get(dep_id, String());
	}
}
