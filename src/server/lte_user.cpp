#include "lte_user.h"

#include <godot_cpp/classes/crypto.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	const char* LTEUser::USER_CONFIG_PATH = "user://user_config.cfg";

	LTEUser* LTEUser::singleton = nullptr;

	void LTEUser::_bind_methods() {
		ClassDB::bind_method(D_METHOD("get_user_uid"), &LTEUser::get_user_uid);
		ClassDB::bind_method(D_METHOD("get_user_name"), &LTEUser::get_user_name);
		ClassDB::bind_method(D_METHOD("set_user_name", "name"), &LTEUser::set_user_name);
		ClassDB::bind_method(D_METHOD("ensure_user_identity"), &LTEUser::ensure_user_identity);
		ClassDB::bind_method(D_METHOD("save_user_config"), &LTEUser::save_user_config);
		ClassDB::bind_method(D_METHOD("load_settings_config"), &LTEUser::load_settings_config);
		ClassDB::bind_method(D_METHOD("reset_onboarding"), &LTEUser::reset_onboarding);
		ClassDB::bind_method(D_METHOD("get_document_course_progress", "id"), &LTEUser::get_document_course_progress);
		ClassDB::bind_method(D_METHOD("set_document_course_progress", "id", "progress"), &LTEUser::set_document_course_progress);

		ClassDB::bind_method(D_METHOD("get_editor_show_home_page"), &LTEUser::get_editor_show_home_page);
		ClassDB::bind_method(D_METHOD("set_editor_show_home_page", "v"), &LTEUser::set_editor_show_home_page);
		ClassDB::bind_method(D_METHOD("get_editor_home_page_requested"), &LTEUser::get_editor_home_page_requested);
		ClassDB::bind_method(D_METHOD("set_editor_home_page_requested", "v"), &LTEUser::set_editor_home_page_requested);
		ClassDB::bind_method(D_METHOD("get_editor_language_initialized"), &LTEUser::get_editor_language_initialized);
		ClassDB::bind_method(D_METHOD("set_editor_language_initialized", "v"), &LTEUser::set_editor_language_initialized);
		ClassDB::bind_method(D_METHOD("get_editor_show_stauts_bar"), &LTEUser::get_editor_show_stauts_bar);
		ClassDB::bind_method(D_METHOD("set_editor_show_stauts_bar", "v"), &LTEUser::set_editor_show_stauts_bar);
		ClassDB::bind_method(D_METHOD("get_editor_fullscreen_mode"), &LTEUser::get_editor_fullscreen_mode);
		ClassDB::bind_method(D_METHOD("set_editor_fullscreen_mode", "v"), &LTEUser::set_editor_fullscreen_mode);
		ClassDB::bind_method(D_METHOD("get_editor_window_has_state"), &LTEUser::get_editor_window_has_state);
		ClassDB::bind_method(D_METHOD("set_editor_window_has_state", "v"), &LTEUser::set_editor_window_has_state);
		ClassDB::bind_method(D_METHOD("get_editor_window_screen"), &LTEUser::get_editor_window_screen);
		ClassDB::bind_method(D_METHOD("set_editor_window_screen", "v"), &LTEUser::set_editor_window_screen);
		ClassDB::bind_method(D_METHOD("get_editor_window_position"), &LTEUser::get_editor_window_position);
		ClassDB::bind_method(D_METHOD("set_editor_window_position", "v"), &LTEUser::set_editor_window_position);
		ClassDB::bind_method(D_METHOD("get_editor_window_size"), &LTEUser::get_editor_window_size);
		ClassDB::bind_method(D_METHOD("set_editor_window_size", "v"), &LTEUser::set_editor_window_size);
		ClassDB::bind_method(D_METHOD("get_editor_window_mode"), &LTEUser::get_editor_window_mode);
		ClassDB::bind_method(D_METHOD("set_editor_window_mode", "v"), &LTEUser::set_editor_window_mode);
		ClassDB::bind_method(D_METHOD("get_onboarding_completed"), &LTEUser::get_onboarding_completed);
		ClassDB::bind_method(D_METHOD("set_onboarding_completed", "v"), &LTEUser::set_onboarding_completed);
		ClassDB::bind_method(D_METHOD("get_onboarding_experience"), &LTEUser::get_onboarding_experience);
		ClassDB::bind_method(D_METHOD("set_onboarding_experience", "v"), &LTEUser::set_onboarding_experience);
		ClassDB::bind_method(D_METHOD("get_onboarding_goal"), &LTEUser::get_onboarding_goal);
		ClassDB::bind_method(D_METHOD("set_onboarding_goal", "v"), &LTEUser::set_onboarding_goal);
		ClassDB::bind_method(D_METHOD("get_onboarding_task_progress"), &LTEUser::get_onboarding_task_progress);
		ClassDB::bind_method(D_METHOD("set_onboarding_task_progress", "v"), &LTEUser::set_onboarding_task_progress);
		ClassDB::bind_method(D_METHOD("get_home_show_getting_started"), &LTEUser::get_home_show_getting_started);
		ClassDB::bind_method(D_METHOD("set_home_show_getting_started", "v"), &LTEUser::set_home_show_getting_started);
		ClassDB::bind_method(D_METHOD("get_plugin_enabled_states"), &LTEUser::get_plugin_enabled_states);
		ClassDB::bind_method(D_METHOD("set_plugin_enabled_states", "v"), &LTEUser::set_plugin_enabled_states);

		ADD_PROPERTY(PropertyInfo(Variant::STRING, "user_uid"), "", "get_user_uid");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "user_name"), "set_user_name", "get_user_name");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editor_show_home_page"), "set_editor_show_home_page", "get_editor_show_home_page");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editor_home_page_requested"), "set_editor_home_page_requested", "get_editor_home_page_requested");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editor_language_initialized"), "set_editor_language_initialized", "get_editor_language_initialized");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editor_show_stauts_bar"), "set_editor_show_stauts_bar", "get_editor_show_stauts_bar");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editor_fullscreen_mode"), "set_editor_fullscreen_mode", "get_editor_fullscreen_mode");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editor_window_has_state"), "set_editor_window_has_state", "get_editor_window_has_state");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "editor_window_screen"), "set_editor_window_screen", "get_editor_window_screen");
		ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "editor_window_position"), "set_editor_window_position", "get_editor_window_position");
		ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "editor_window_size"), "set_editor_window_size", "get_editor_window_size");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "editor_window_mode"), "set_editor_window_mode", "get_editor_window_mode");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "onboarding_completed"), "set_onboarding_completed", "get_onboarding_completed");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "onboarding_experience"), "set_onboarding_experience", "get_onboarding_experience");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "onboarding_goal"), "set_onboarding_goal", "get_onboarding_goal");
		ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "onboarding_task_progress"), "set_onboarding_task_progress", "get_onboarding_task_progress");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "home_show_getting_started"), "set_home_show_getting_started", "get_home_show_getting_started");
		ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "plugin_enabled_states"), "set_plugin_enabled_states", "get_plugin_enabled_states");

		ADD_SIGNAL(MethodInfo("user_config_update"));
	}

	LTEUser::LTEUser() {
		if (singleton == nullptr) {
			singleton = this;
		}
		user_config_path = _resolve_user_config_path();
		if (!FileAccess::file_exists(ProjectSettings::get_singleton()->globalize_path(user_config_path))) {
			save_user_config();
		}
		load_settings_config();
		ensure_user_identity();
		if (_is_session_test_isolated()) {
			onboarding_completed = true;
		}
	}

	LTEUser::~LTEUser() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEUser* LTEUser::get_singleton() {
		return singleton;
	}

	void LTEUser::ensure_user_identity() {
		bool changed = false;
		if (user_uid.strip_edges().is_empty()) {
			user_uid = _make_user_uid();
			changed = true;
		}
		if (user_name.strip_edges().is_empty()) {
			user_name = _make_default_user_name();
			changed = true;
		}
		if (changed) {
			save_user_config();
		}
	}

	void LTEUser::set_user_name(const String& new_user_name) {
		String clean_user_name = new_user_name.strip_edges();
		if (clean_user_name.is_empty()) {
			clean_user_name = _make_default_user_name();
		}
		if (user_name == clean_user_name) {
			return;
		}
		user_name = clean_user_name;
		save_user_config();
	}

	void LTEUser::save_user_config() {
		Ref<ConfigFile> file;
		file.instantiate();
		file->set_value("user", "uid", user_uid);
		file->set_value("user", "name", user_name);
		file->set_value("editor", "show_home_page", editor_show_home_page);
		file->set_value("editor", "home_page_requested", editor_home_page_requested);
		file->set_value("editor", "language_initialized", editor_language_initialized);
		file->set_value("editor", "show_stauts_bar", editor_show_stauts_bar);
		file->set_value("editor", "fullscreen_mode", editor_fullscreen_mode);
		file->set_value("editor", "window_has_state", editor_window_has_state);
		file->set_value("editor", "window_screen", editor_window_screen);
		file->set_value("editor", "window_position", editor_window_position);
		file->set_value("editor", "window_size", editor_window_size);
		file->set_value("editor", "window_mode", editor_window_mode);
		file->set_value("documents", "course_progress", documents_course_progress);
		file->set_value("onboarding", "completed", onboarding_completed);
		file->set_value("onboarding", "experience", onboarding_experience);
		file->set_value("onboarding", "goal", onboarding_goal);
		file->set_value("onboarding", "task_progress", onboarding_task_progress);
		file->set_value("onboarding", "home_show_getting_started", home_show_getting_started);
		file->set_value("plugins", "enabled_states", plugin_enabled_states);
		Error err = file->save(user_config_path);
		if (err != OK) {
			ERR_PRINT(vformat("Failed to save user config: %d", err));
		}
		emit_signal("user_config_update");
	}

	void LTEUser::load_settings_config() {
		Ref<ConfigFile> file;
		file.instantiate();
		Error err = file->load(user_config_path);
		if (err == OK) {
			user_uid = file->get_value("user", "uid", "");
			user_name = file->get_value("user", "name", "");
			editor_show_home_page = file->get_value("editor", "show_home_page", true);
			editor_home_page_requested = file->get_value("editor", "home_page_requested", false);
			editor_language_initialized = file->get_value("editor", "language_initialized", false);
			editor_show_stauts_bar = file->get_value("editor", "show_stauts_bar", true);
			editor_fullscreen_mode = file->get_value("editor", "fullscreen_mode", false);
			editor_window_has_state = file->get_value("editor", "window_has_state", false);
			editor_window_screen = file->get_value("editor", "window_screen", 0);
			editor_window_position = file->get_value("editor", "window_position", Vector2i());
			editor_window_size = file->get_value("editor", "window_size", Vector2i(1152, 648));
			editor_window_mode = file->get_value("editor", "window_mode", 3);
			documents_course_progress = file->get_value("documents", "course_progress", Dictionary());
			onboarding_completed = file->get_value("onboarding", "completed", false);
			onboarding_experience = file->get_value("onboarding", "experience", "");
			onboarding_goal = file->get_value("onboarding", "goal", "");
			onboarding_task_progress = file->get_value("onboarding", "task_progress", Dictionary());
			home_show_getting_started = file->get_value("onboarding", "home_show_getting_started", true);
			plugin_enabled_states = file->get_value("plugins", "enabled_states", Dictionary());
		}
		else {
			WARN_PRINT(vformat("Failed to load user config: %d", err));
		}
		emit_signal("user_config_update");
	}

	void LTEUser::reset_onboarding() {
		onboarding_completed = false;
		onboarding_experience = "";
		onboarding_goal = "";
		onboarding_task_progress.clear();
		home_show_getting_started = true;
		save_user_config();
	}

	float LTEUser::get_document_course_progress(const StringName& document_course_id) const {
		Variant val = documents_course_progress.get(String(document_course_id), 0.0f);
		return CLAMP(float(val), 0.0f, 100.0f);
	}

	void LTEUser::set_document_course_progress(const StringName& document_course_id, float document_progress) {
		float progress_value = CLAMP(document_progress, 0.0f, 100.0f);
		String progress_key = document_course_id;
		float current = documents_course_progress.get(progress_key, 0.0f);
		if (Math::is_equal_approx(current, progress_value)) {
			return;
		}
		documents_course_progress[progress_key] = progress_value;
		save_user_config();
	}

	String LTEUser::_make_user_uid() const {
		Ref<Crypto> crypto;
		crypto.instantiate();
		PackedByteArray random_bytes = crypto->generate_random_bytes(16);
		String uid;
		for (int64_t i = 0; i < random_bytes.size(); i++) {
			uid += vformat("%02x", random_bytes[i]);
		}
		return uid;
	}

	String LTEUser::_make_default_user_name() const {
		String os_user_name = OS::get_singleton()->get_environment("USERNAME").strip_edges();
		if (os_user_name.is_empty()) {
			os_user_name = OS::get_singleton()->get_environment("USER").strip_edges();
		}
		if (os_user_name.is_empty()) {
			return "User";
		}
		return os_user_name;
	}

	String LTEUser::_resolve_user_config_path() const {
		PackedStringArray args = OS::get_singleton()->get_cmdline_user_args();
		for (int64_t i = 0; i < args.size(); i++) {
			String argument = args[i];
			if (argument == "--session-test-isolated-user") {
				return vformat("user://user_config_session_test_%d.cfg", OS::get_singleton()->get_process_id());
			}
			if (argument.begins_with("--session-test-profile=")) {
				String profile_name = argument.trim_prefix("--session-test-profile=").strip_edges();
				profile_name = _sanitize_user_config_profile_name(profile_name);
				if (!profile_name.is_empty()) {
					return vformat("user://user_config_%s.cfg", profile_name);
				}
			}
		}
		return USER_CONFIG_PATH;
	}

	bool LTEUser::_is_session_test_isolated() const {
		return user_config_path.contains("session_test");
	}

	String LTEUser::_sanitize_user_config_profile_name(const String& profile_name) const {
		String clean_name;
		for (int64_t i = 0; i < profile_name.length(); i++) {
			String character = profile_name.substr(i, 1);
			if (character.is_valid_identifier() || character.is_valid_int() || character == "-") {
				clean_name += character;
			}
		}
		return clean_name;
	}

	String LTEUser::get_user_uid() const {
		return user_uid;
	}

	String LTEUser::get_user_name() const {
		return user_name;
	}

	bool LTEUser::get_editor_show_home_page() const {
		return editor_show_home_page;
	}
	void LTEUser::set_editor_show_home_page(bool v) {
		editor_show_home_page = v;
	}

	bool LTEUser::get_editor_home_page_requested() const {
		return editor_home_page_requested;
	}

	void LTEUser::set_editor_home_page_requested(bool v) {
		editor_home_page_requested = v;
	}

	bool LTEUser::get_editor_language_initialized() const {
		return editor_language_initialized;
	}

	void LTEUser::set_editor_language_initialized(bool v) {
		editor_language_initialized = v;
	}

	bool LTEUser::get_editor_show_stauts_bar() const {
		return editor_show_stauts_bar;
	}
	void LTEUser::set_editor_show_stauts_bar(bool v) {
		editor_show_stauts_bar = v;
	}

	bool LTEUser::get_editor_fullscreen_mode() const {
		return editor_fullscreen_mode;
	}

	void LTEUser::set_editor_fullscreen_mode(bool v) {
		editor_fullscreen_mode = v;
	}

	bool LTEUser::get_editor_window_has_state() const {
		return editor_window_has_state;
	}

	void LTEUser::set_editor_window_has_state(bool v) {
		editor_window_has_state = v;
	}

	int32_t LTEUser::get_editor_window_screen() const {
		return editor_window_screen;
	}

	void LTEUser::set_editor_window_screen(int32_t v) {
		editor_window_screen = v;
	}

	Vector2i LTEUser::get_editor_window_position() const {
		return editor_window_position;
	}

	void LTEUser::set_editor_window_position(const Vector2i& v) {
		editor_window_position = v;
	}

	Vector2i LTEUser::get_editor_window_size() const {
		return editor_window_size;
	}

	void LTEUser::set_editor_window_size(const Vector2i& v) {
		editor_window_size = v;
	}

	int32_t LTEUser::get_editor_window_mode() const {
		return editor_window_mode;
	}

	void LTEUser::set_editor_window_mode(int32_t v) {
		editor_window_mode = v;
	}

	bool LTEUser::get_onboarding_completed() const {
		return onboarding_completed;
	}

	void LTEUser::set_onboarding_completed(bool v) {
		onboarding_completed = v;
	}

	String LTEUser::get_onboarding_experience() const {
		return onboarding_experience;
	}

	void LTEUser::set_onboarding_experience(const String& v) {
		onboarding_experience = v;
	}

	Dictionary LTEUser::get_plugin_enabled_states() const {
		return plugin_enabled_states;
	}

	void LTEUser::set_plugin_enabled_states(const Dictionary& v) {
		plugin_enabled_states = v;
	}

	String LTEUser::get_onboarding_goal() const {
		return onboarding_goal;
	}

	void LTEUser::set_onboarding_goal(const String& v) {
		onboarding_goal = v;
	}

	Dictionary LTEUser::get_onboarding_task_progress() const {
		return onboarding_task_progress;
	}

	void LTEUser::set_onboarding_task_progress(const Dictionary& v) {
		onboarding_task_progress = v;
	}

	bool LTEUser::get_home_show_getting_started() const {
		return home_show_getting_started;
	}

	void LTEUser::set_home_show_getting_started(bool v) {
		home_show_getting_started = v;
	}
}
