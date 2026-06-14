#include "audio_visualizer_server.h"

#include "project_manager.h"
#include "settings_config.h"
#include "utils.h"

#include <algorithm>

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTEAudioVisualizerServer* LTEAudioVisualizerServer::singleton = nullptr;

	void LTEAudioVisualizerServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("normalize_config", "config"), &LTEAudioVisualizerServer::normalize_config);
		ClassDB::bind_method(D_METHOD("fetch_config", "uuid"), &LTEAudioVisualizerServer::fetch_config);
		ClassDB::bind_method(D_METHOD("submit_config", "uuid", "config"), &LTEAudioVisualizerServer::submit_config);
		ClassDB::bind_method(D_METHOD("load_config_file", "path"), &LTEAudioVisualizerServer::load_config_file);

		ADD_SIGNAL(MethodInfo("config_changed",
			PropertyInfo(Variant::STRING, "uuid"),
			PropertyInfo(Variant::DICTIONARY, "config")));
	}

	LTEAudioVisualizerServer::LTEAudioVisualizerServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTEAudioVisualizerServer::~LTEAudioVisualizerServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEAudioVisualizerServer* LTEAudioVisualizerServer::get_singleton() {
		return singleton;
	}

	Dictionary LTEAudioVisualizerServer::_make_default_config() const {
		Dictionary config;
		config["type"] = 0;
		config["channel_mode"] = 0;
		config["display_mode"] = 0;
		config["draw_mode"] = 0;
		return config;
	}

	Dictionary LTEAudioVisualizerServer::normalize_config(const Dictionary& config) const {
		Dictionary normalized = _make_default_config();
		normalized["type"] = std::clamp(int(config.get("type", normalized["type"])), 0, 2);
		normalized["channel_mode"] = std::clamp(int(config.get("channel_mode", normalized["channel_mode"])), 0, 3);
		normalized["display_mode"] = std::clamp(int(config.get("display_mode", normalized["display_mode"])), 0, 2);
		normalized["draw_mode"] = std::clamp(int(config.get("draw_mode", normalized["draw_mode"])), 0, 2);
		return normalized;
	}

	Dictionary LTEAudioVisualizerServer::fetch_config(const String& uuid) const {
		if (uuid.is_empty()) {
			return _make_default_config();
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return _make_default_config();
		}
		Dictionary config = settings_config->audio_visualizer_configs.get(uuid, Dictionary());
		return normalize_config(config);
	}

	void LTEAudioVisualizerServer::submit_config(const String& uuid, const Dictionary& config) {
		if (uuid.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		Dictionary normalized = normalize_config(config);
		settings_config->audio_visualizer_configs[uuid] = normalized;
		settings_config->save_settings_config(false);
		emit_signal("config_changed", uuid, normalized);
	}

	Dictionary LTEAudioVisualizerServer::load_config_file(const String& path) const {
		String normalized_path = _normalize_path(path);
		if (normalized_path.is_empty() || !FileAccess::file_exists(normalized_path)) {
			return _make_default_config();
		}
		if (normalized_path.get_extension().to_lower() == String("cfg")) {
			Ref<ConfigFile> config_file;
			config_file.instantiate();
			Error error = config_file->load(normalized_path);
			if (error != OK) {
				ERR_PRINT(vformat("load_config_file: Failed to load config file: %s. Error Code: %d", normalized_path, error));
				return _make_default_config();
			}
			Dictionary config = _make_default_config();
			config["type"] = config_file->get_value("audio_visualizer", "type", config["type"]);
			config["channel_mode"] = config_file->get_value("audio_visualizer", "channel_mode", config["channel_mode"]);
			config["display_mode"] = config_file->get_value("audio_visualizer", "display_mode", config["display_mode"]);
			config["draw_mode"] = config_file->get_value("audio_visualizer", "draw_mode", config["draw_mode"]);
			return normalize_config(config);
		}
		Ref<FileAccess> file = FileAccess::open(normalized_path, FileAccess::READ);
		if (file.is_null()) {
			Error error = FileAccess::get_open_error();
			ERR_PRINT(vformat("load_config_file: Failed to open file: %s. Error Code: %d", normalized_path, error));
			return _make_default_config();
		}
		String content = file->get_as_text();
		file->close();
		Variant value = JSON::parse_string(content);
		if (value.get_type() != Variant::DICTIONARY) {
			ERR_PRINT(vformat("load_config_file: Config file must contain a JSON object: %s", normalized_path));
			return _make_default_config();
		}
		Dictionary config = value;
		return normalize_config(config);
	}

	String LTEAudioVisualizerServer::_normalize_path(const String& path) const {
		String normalized_path = path.replace("\\", "/").strip_edges().simplify_path();
		if (normalized_path.is_empty()) {
			return String();
		}
		if (Utils::is_absolute_path(normalized_path)) {
			return normalized_path;
		}
		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_dir = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (normalized_path.begins_with("/") && !root_dir.is_empty()) {
			return (root_dir + normalized_path).simplify_path();
		}
		if (root_dir.is_empty()) {
			return normalized_path;
		}
		return root_dir.path_join(normalized_path).replace("\\", "/").simplify_path();
	}
}
