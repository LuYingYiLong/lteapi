#include "plugin_context.h"
#include "component_registry.h"
#include "helpers/lte_chart_note_helper.h"
#include "server/timeline_server.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	void LTEPluginContext::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_plugin_id", "plugin_id"), &LTEPluginContext::set_plugin_id);
		ClassDB::bind_method(D_METHOD("get_plugin_id"), &LTEPluginContext::get_plugin_id);
		ClassDB::bind_method(D_METHOD("set_project_root", "root"), &LTEPluginContext::set_project_root);
		ClassDB::bind_method(D_METHOD("get_project_root"), &LTEPluginContext::get_project_root);
		ClassDB::bind_method(D_METHOD("register_panel", "panel_uuid", "panel_name", "scene_path", "icon_path", "space_type"), &LTEPluginContext::register_panel, DEFVAL(String()), DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("register_menu", "menu_id", "menu_name", "icon_path", "tooltip"), &LTEPluginContext::register_menu, DEFVAL(String()), DEFVAL(String()));
		ClassDB::bind_method(D_METHOD("register_menu_item", "menu_id", "item_id", "item_name", "icon_path", "metadata"), &LTEPluginContext::register_menu_item, DEFVAL(String()), DEFVAL(Dictionary()));
		ClassDB::bind_method(D_METHOD("register_command", "command_name", "callable"), &LTEPluginContext::register_command);
		ClassDB::bind_method(D_METHOD("save_user_config", "config"), &LTEPluginContext::save_user_config);
		ClassDB::bind_method(D_METHOD("load_user_config"), &LTEPluginContext::load_user_config);
		ClassDB::bind_method(D_METHOD("save_project_config", "config"), &LTEPluginContext::save_project_config);
		ClassDB::bind_method(D_METHOD("load_project_config"), &LTEPluginContext::load_project_config);
		ClassDB::bind_method(D_METHOD("get_cache_dir"), &LTEPluginContext::get_cache_dir);
		ClassDB::bind_method(D_METHOD("clear_cache"), &LTEPluginContext::clear_cache);
		ClassDB::bind_method(D_METHOD("unregister_all"), &LTEPluginContext::unregister_all);
	ClassDB::bind_method(D_METHOD("get_chart_notes", "uuid", "chart_path"), &LTEPluginContext::get_chart_notes);
	ClassDB::bind_method(D_METHOD("validate_notes", "notes"), &LTEPluginContext::validate_notes);
	ClassDB::bind_method(D_METHOD("make_note_patch", "old_notes", "new_notes"), &LTEPluginContext::make_note_patch);
	}

	LTEPluginContext::LTEPluginContext() {
	}

	LTEPluginContext::~LTEPluginContext() {
		unregister_all();
	}

	void LTEPluginContext::set_plugin_id(const String& pid) {
		plugin_id = pid;
	}

	String LTEPluginContext::get_plugin_id() const {
		return plugin_id;
	}

	void LTEPluginContext::set_project_root(const String& root) {
		project_root = root;
	}

	String LTEPluginContext::get_project_root() const {
		return project_root;
	}

	void LTEPluginContext::register_panel(const String& panel_uuid, const String& panel_name, const String& scene_path, const String& icon_path, const String& space_type) {
		Dictionary reg;
		reg["type"] = "panel";
		reg["uuid"] = panel_uuid;
		registrations.append(reg);

		LTEComponentRegistry* registry = LTEComponentRegistry::get_singleton();
		if (registry) {
			registry->register_component_panel(panel_uuid, icon_path, panel_name, scene_path, space_type, plugin_id);
		}
	}

	void LTEPluginContext::register_menu(const String& menu_id, const String& menu_name, const String& icon_path, const String& tooltip) {
		Dictionary reg;
		reg["type"] = "menu";
		reg["menu_id"] = menu_id;
		registrations.append(reg);

		Dictionary options;
		if (!icon_path.is_empty()) {
			options["icon"] = icon_path;
		}
		if (!tooltip.is_empty()) {
			options["tooltip"] = tooltip;
		}

		LTEComponentRegistry* registry = LTEComponentRegistry::get_singleton();
		if (registry) {
			registry->register_editor_menu(menu_id, menu_name, options);
		}
	}

	void LTEPluginContext::register_menu_item(const String& menu_id, int32_t item_id, const String& item_name, const String& icon_path, const Dictionary& metadata) {
		Dictionary reg;
		reg["type"] = "menu_item";
		reg["menu_id"] = menu_id;
		reg["item_id"] = item_id;
		registrations.append(reg);

		LTEComponentRegistry* registry = LTEComponentRegistry::get_singleton();
		if (registry) {
			registry->register_editor_menu_item(menu_id, item_id, item_name, icon_path, metadata);
		}
	}

	void LTEPluginContext::register_command(const String& command_name, const Callable& callable) {
		Dictionary reg;
		reg["type"] = "command";
		reg["name"] = command_name;
		reg["callable"] = callable;
		registrations.append(reg);
	}

	String LTEPluginContext::_get_user_config_dir() const {
		return String("user://plugin_data").path_join(plugin_id);
	}

	String LTEPluginContext::_get_user_config_path() const {
		return _get_user_config_dir().path_join("config.cfg");
	}

	String LTEPluginContext::_get_project_config_dir() const {
		if (project_root.is_empty()) {
			return String();
		}
		return project_root.path_join(".editor").path_join("plugin_data").path_join(plugin_id);
	}

	String LTEPluginContext::_get_project_config_path() const {
		String dir = _get_project_config_dir();
		if (dir.is_empty()) {
			return String();
		}
		return dir.path_join("config.cfg");
	}

	void LTEPluginContext::_save_config_file(const String& path, const Dictionary& config) {
		Ref<ConfigFile> cfg;
		cfg.instantiate();
		Array keys = config.keys();
		for (int64_t i = 0; i < keys.size(); i++) {
			String key = String(keys[i]);
			Variant value = config[key];
			String section = "config";
			int64_t slash = key.find("/");
			if (slash >= 0) {
				section = key.substr(0, slash);
				key = key.substr(slash + 1);
			}
			cfg->set_value(section, key, value);
		}
		cfg->save(path);
	}

	Dictionary LTEPluginContext::_load_config_file(const String& path) const {
		Dictionary result;
		if (!FileAccess::file_exists(path)) {
			return result;
		}
		Ref<ConfigFile> cfg;
		cfg.instantiate();
		cfg->load(path);
		PackedStringArray sections = cfg->get_sections();
		for (int64_t i = 0; i < sections.size(); i++) {
			String section = sections[i];
			PackedStringArray keys = cfg->get_section_keys(section);
			for (int64_t j = 0; j < keys.size(); j++) {
				String key = keys[j];
				String full_key = section == "config" ? key : section + "/" + key;
				result[full_key] = cfg->get_value(section, key);
			}
		}
		return result;
	}

	void LTEPluginContext::save_user_config(const Dictionary& config) {
		String dir = _get_user_config_dir();
		DirAccess::make_dir_recursive_absolute(dir);
		_save_config_file(_get_user_config_path(), config);
	}

	Dictionary LTEPluginContext::load_user_config() const {
		return _load_config_file(_get_user_config_path());
	}

	void LTEPluginContext::save_project_config(const Dictionary& config) {
		String dir = _get_project_config_dir();
		if (dir.is_empty()) {
			WARN_PRINT(vformat("Cannot save project config for plugin '%s': no project open.", plugin_id));
			return;
		}
		DirAccess::make_dir_recursive_absolute(dir);
		_save_config_file(_get_project_config_path(), config);
	}

	Dictionary LTEPluginContext::load_project_config() const {
		String path = _get_project_config_path();
		if (path.is_empty()) {
			return Dictionary();
		}
		return _load_config_file(path);
	}

	String LTEPluginContext::get_cache_dir() const {
		String dir = String("user://plugin_cache").path_join(plugin_id);
		DirAccess::make_dir_recursive_absolute(dir);
		return dir;
	}

	void LTEPluginContext::clear_cache() {
		String dir = String("user://plugin_cache").path_join(plugin_id);
		if (!DirAccess::dir_exists_absolute(dir)) {
			return;
		}
		Ref<DirAccess> d = DirAccess::open(dir);
		if (d.is_null()) {
			return;
		}
		d->list_dir_begin();
		String item = d->get_next();
		while (!item.is_empty()) {
			if (item == "." || item == "..") {
				item = d->get_next();
				continue;
			}
			String full_path = dir.path_join(item);
			if (d->current_is_dir()) {
				Ref<DirAccess> sub = DirAccess::open(full_path);
				if (sub.is_valid()) {
					sub->list_dir_begin();
					String sub_item = sub->get_next();
					while (!sub_item.is_empty()) {
						if (sub_item != "." && sub_item != "..") {
							String sub_full = full_path.path_join(sub_item);
							if (sub->current_is_dir()) {
								DirAccess::remove_absolute(sub_full);
							}
							else {
								DirAccess::remove_absolute(sub_full);
							}
						}
						sub_item = sub->get_next();
					}
					sub->list_dir_end();
				}
				DirAccess::remove_absolute(full_path);
			}
			else {
				DirAccess::remove_absolute(full_path);
			}
			item = d->get_next();
		}
		d->list_dir_end();
	}

	void LTEPluginContext::unregister_all() {
		LTEComponentRegistry* registry = LTEComponentRegistry::get_singleton();
		for (int64_t i = 0; i < registrations.size(); i++) {
			Dictionary reg = registrations[i];
			String type = reg.get("type", "");
			if (type == "panel") {
				String uuid = reg.get("uuid", "");
				if (registry) {
					registry->unregister_component_panel(uuid);
				}
			}
			else if (type == "menu") {
				String menu_id = reg.get("menu_id", "");
				if (registry) {
					registry->unregister_editor_menu(menu_id);
				}
			}
			else if (type == "menu_item") {
				String menu_id = reg.get("menu_id", "");
				int32_t item_id = reg.get("item_id", 0);
				if (registry) {
					registry->unregister_editor_menu_item(menu_id, item_id);
				}
			}
		}
		registrations.clear();
	}

	// ——— Chart data API ———

	Array LTEPluginContext::get_chart_notes(const String& uuid, const String& chart_path) {
		LTETimelineServer* ts = LTETimelineServer::get_singleton();
		if (!ts) {
			return Array();
		}
		Dictionary opened = ts->find_opened_chart_info(uuid, chart_path);
		Dictionary chart = opened.get("chart", Dictionary());
		return chart.get("note", Array());
	}

	Array LTEPluginContext::validate_notes(const Array& notes) {
		Array diagnostics;
		LTEChartNoteHelper* helper = LTEChartNoteHelper::get_singleton();
		if (!helper) {
			return diagnostics;
		}

		// 重叠检测
		Array overlaps = helper->find_overlaps(notes);
		for (int32_t i = 0; i < overlaps.size(); ++i) {
			Dictionary ov = overlaps[i];
			Dictionary diag;
			diag["level"] = "warning";
			diag["code"] = "note_overlap";
			diag["message"] = "Same track has overlapping notes.";
			diag["index_a"] = ov.get("index_a", -1);
			diag["index_b"] = ov.get("index_b", -1);
			diagnostics.append(diag);
		}

		// 基本合法性检查
		for (int32_t i = 0; i < notes.size(); ++i) {
			if (notes[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary note = notes[i];
			String type = note.get("type", "tap");
			int32_t key = static_cast<int32_t>(note.get("key", -1));
			if (key < 0) {
				Dictionary diag;
				diag["level"] = "error";
				diag["code"] = "note_key_missing";
				diag["message"] = "Note key is missing or negative.";
				diag["index"] = i;
				diagnostics.append(diag);
			}
			if (type == String("hold")) {
				double start_beat = static_cast<double>(note.get("start_beat", 0.0));
				double end_beat = static_cast<double>(note.get("end_beat", 0.0));
				if (end_beat <= start_beat) {
					Dictionary diag;
					diag["level"] = "error";
					diag["code"] = "hold_time_invalid";
					diag["message"] = "Hold end_beat must be greater than start_beat.";
					diag["index"] = i;
					diag["key"] = key;
					diagnostics.append(diag);
				}
			}
		}
		return diagnostics;
	}

	Dictionary LTEPluginContext::make_note_patch(const Array& old_notes, const Array& new_notes) {
		LTEChartNoteHelper* helper = LTEChartNoteHelper::get_singleton();
		Dictionary result;
		result["type"] = "note_patch";
		if (helper) {
			Dictionary delta = helper->make_note_delta(old_notes, new_notes);
			result["added"] = delta.get("added", Array());
			result["removed"] = delta.get("removed", Array());
		} else {
			result["added"] = Array();
			result["removed"] = Array();
		}
		result["diagnostics"] = validate_notes(new_notes);
		return result;
	}
}
