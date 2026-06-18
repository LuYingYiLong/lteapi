#include "scene_layers_server.h"

#include "project_manager.h"
#include "settings_config.h"
#include "utils.h"

namespace godot {
	LTESceneLayersServer* LTESceneLayersServer::singleton = nullptr;

	void LTESceneLayersServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("get_scroll", "uuid", "scene_path"), &LTESceneLayersServer::get_scroll);
		ClassDB::bind_method(D_METHOD("set_scroll", "uuid", "scene_path", "scroll"), &LTESceneLayersServer::set_scroll);
		ClassDB::bind_method(D_METHOD("get_collapsed_items", "uuid", "scene_path"), &LTESceneLayersServer::get_collapsed_items);
		ClassDB::bind_method(D_METHOD("set_collapsed_items", "uuid", "scene_path", "items"), &LTESceneLayersServer::set_collapsed_items);
	}

	LTESceneLayersServer::LTESceneLayersServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTESceneLayersServer::~LTESceneLayersServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTESceneLayersServer* LTESceneLayersServer::get_singleton() {
		return singleton;
	}

	String LTESceneLayersServer::_get_absolute_scene_path(const String& scene_path) const {
		String normalized_path = scene_path.replace("\\", "/");
		if (normalized_path.is_empty()) {
			return String();
		}
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_path = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (normalized_path.begins_with("/") && !root_path.is_empty()) {
			return (root_path + normalized_path).simplify_path();
		}
		if (Utils::is_absolute_path(normalized_path)) {
			return normalized_path.simplify_path();
		}
		if (root_path.is_empty()) {
			return normalized_path.simplify_path();
		}
		return root_path.path_join(normalized_path).simplify_path();
	}

	Vector2 LTESceneLayersServer::get_scroll(const String& uuid, const String& scene_path) const {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return Vector2();
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return Vector2();
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary panel_settings = settings_config->scene_layers_settings.get(uuid, Dictionary());
		Dictionary config = panel_settings.get(scene_key, Dictionary());
		return Vector2(float(config.get("scroll_x", 0)), float(config.get("scroll_y", 0)));
	}

	void LTESceneLayersServer::set_scroll(const String& uuid, const String& scene_path, const Vector2& scroll) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary panel_settings = settings_config->scene_layers_settings.get(uuid, Dictionary());
		Dictionary config = panel_settings.get(scene_key, Dictionary());
		config["scroll_x"] = scroll.x;
		config["scroll_y"] = scroll.y;
		panel_settings[scene_key] = config;
		settings_config->scene_layers_settings[uuid] = panel_settings;
		settings_config->save_settings_config(false);
	}

	PackedStringArray LTESceneLayersServer::get_collapsed_items(const String& uuid, const String& scene_path) const {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return PackedStringArray();
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return PackedStringArray();
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary panel_settings = settings_config->scene_layers_settings.get(uuid, Dictionary());
		Dictionary config = panel_settings.get(scene_key, Dictionary());
		Variant collapsed_items = config.get("collapsed_items", PackedStringArray());
		if (collapsed_items.get_type() == Variant::PACKED_STRING_ARRAY) {
			return collapsed_items;
		}
		PackedStringArray result;
		if (collapsed_items.get_type() == Variant::ARRAY) {
			Array source_items = collapsed_items;
			for (int index = 0; index < source_items.size(); ++index) {
				result.append(String(source_items[index]));
			}
		}
		return result;
	}

	void LTESceneLayersServer::set_collapsed_items(const String& uuid, const String& scene_path, const PackedStringArray& items) {
		if (uuid.is_empty() || scene_path.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		String scene_key = _get_absolute_scene_path(scene_path);
		Dictionary panel_settings = settings_config->scene_layers_settings.get(uuid, Dictionary());
		Dictionary config = panel_settings.get(scene_key, Dictionary());
		config["collapsed_items"] = items;
		panel_settings[scene_key] = config;
		settings_config->scene_layers_settings[uuid] = panel_settings;
		settings_config->save_settings_config(false);
	}
}
