#include "path_editor_server.h"

#include "settings_config.h"

#include <godot_cpp/core/object.hpp>

namespace godot {
	LTEPathEditorServer* LTEPathEditorServer::singleton = nullptr;

	void LTEPathEditorServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("request_open_path", "context"), &LTEPathEditorServer::request_open_path);
		ClassDB::bind_method(D_METHOD("register_editor", "editor"), &LTEPathEditorServer::register_editor);
		ClassDB::bind_method(D_METHOD("unregister_editor", "editor"), &LTEPathEditorServer::unregister_editor);
		ClassDB::bind_method(D_METHOD("set_active_editor", "editor"), &LTEPathEditorServer::set_active_editor);
		ClassDB::bind_method(D_METHOD("get_active_editor"), &LTEPathEditorServer::get_active_editor);
		ClassDB::bind_method(D_METHOD("get_view_config", "uuid"), &LTEPathEditorServer::get_view_config);
		ClassDB::bind_method(D_METHOD("set_view_config", "uuid", "config"), &LTEPathEditorServer::set_view_config);
		ClassDB::bind_method(D_METHOD("clear_project_state"), &LTEPathEditorServer::clear_project_state);
		ADD_SIGNAL(MethodInfo("path_open_requested", PropertyInfo(Variant::DICTIONARY, "context")));
	}

	LTEPathEditorServer::LTEPathEditorServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
		settings_config = LTESettingsConfig::get_singleton();
	}

	LTEPathEditorServer::~LTEPathEditorServer() {
		active_editor_id = ObjectID();
		settings_config = nullptr;
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEPathEditorServer* LTEPathEditorServer::get_singleton() {
		return singleton;
	}

	void LTEPathEditorServer::request_open_path(const Dictionary& context) {
		if (context.is_empty()) {
			return;
		}
		emit_signal("path_open_requested", context.duplicate(true));
	}

	void LTEPathEditorServer::register_editor(Object* editor) {
		set_active_editor(editor);
	}

	void LTEPathEditorServer::unregister_editor(Object* editor) {
		if (editor != nullptr && active_editor_id == ObjectID(editor->get_instance_id())) {
			active_editor_id = ObjectID();
		}
	}

	void LTEPathEditorServer::set_active_editor(Object* editor) {
		active_editor_id = editor != nullptr ? editor->get_instance_id() : ObjectID();
	}

	Object* LTEPathEditorServer::get_active_editor() const {
		return active_editor_id.is_valid() ? ObjectDB::get_instance(active_editor_id) : nullptr;
	}

	Dictionary LTEPathEditorServer::get_view_config(const String& uuid) const {
		if (settings_config == nullptr || uuid.is_empty()) {
			return Dictionary();
		}
		Dictionary view_config = settings_config->path_editor_view_configs.get(uuid, Dictionary());
		return view_config.duplicate(true);
	}

	void LTEPathEditorServer::set_view_config(const String& uuid, const Dictionary& config) {
		if (settings_config == nullptr || uuid.is_empty()) {
			return;
		}
		settings_config->path_editor_view_configs[uuid] = config.duplicate(true);
		settings_config->save_settings_config(false);
	}

	void LTEPathEditorServer::clear_project_state() {
		// Path editor view configs are stored in LTESettingsConfig::path_editor_view_configs,
		// which is garbage-collected via settings_config_garbage_collection().
		// This method exists for interface consistency with other servers.
	}
}
