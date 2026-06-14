#ifndef LTE_USER_H
#define LTE_USER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
	class LTEUser : public Object {
		GDCLASS(LTEUser, Object)

	private:
		static LTEUser* singleton;

		static const char* USER_CONFIG_PATH;

		String user_uid;
		String user_name;
		String user_config_path;

		bool editor_show_home_page;
		bool editor_home_page_requested;
		bool editor_language_initialized;
		bool editor_show_stauts_bar;
		bool editor_fullscreen_mode;
		bool editor_window_has_state;
		int32_t editor_window_screen;
		Vector2i editor_window_position;
		Vector2i editor_window_size;
		int32_t editor_window_mode;

		Dictionary documents_course_progress;
		bool onboarding_completed;
		String onboarding_experience;
		String onboarding_goal;
		Dictionary onboarding_task_progress;
		bool home_show_getting_started;

		Dictionary plugin_enabled_states;

		String _make_user_uid() const;
		String _make_default_user_name() const;
		String _resolve_user_config_path() const;
		bool _is_session_test_isolated() const;
		String _sanitize_user_config_profile_name(const String& profile_name) const;

	protected:
		static void _bind_methods();

	public:
		LTEUser();
		~LTEUser();

		static LTEUser* get_singleton();

		void ensure_user_identity();
		void save_user_config();
		void load_settings_config();
		void set_user_name(const String& new_user_name);
		void reset_onboarding();

		float get_document_course_progress(const StringName& document_course_id) const;
		void set_document_course_progress(const StringName& document_course_id, float document_progress);

		String get_user_uid() const;
		String get_user_name() const;

		bool get_editor_show_home_page() const;
		void set_editor_show_home_page(bool v);

		bool get_editor_home_page_requested() const;
		void set_editor_home_page_requested(bool v);

		bool get_editor_language_initialized() const;
		void set_editor_language_initialized(bool v);

		bool get_editor_show_stauts_bar() const;
		void set_editor_show_stauts_bar(bool v);

		bool get_editor_fullscreen_mode() const;
		void set_editor_fullscreen_mode(bool v);

		bool get_editor_window_has_state() const;
		void set_editor_window_has_state(bool v);

		int32_t get_editor_window_screen() const;
		void set_editor_window_screen(int32_t v);

		Vector2i get_editor_window_position() const;
		void set_editor_window_position(const Vector2i& v);

		Vector2i get_editor_window_size() const;
		void set_editor_window_size(const Vector2i& v);

		int32_t get_editor_window_mode() const;
		void set_editor_window_mode(int32_t v);

		bool get_onboarding_completed() const;
		void set_onboarding_completed(bool v);

		String get_onboarding_experience() const;
		void set_onboarding_experience(const String& v);

		String get_onboarding_goal() const;
		void set_onboarding_goal(const String& v);

		Dictionary get_onboarding_task_progress() const;
		void set_onboarding_task_progress(const Dictionary& v);

		bool get_home_show_getting_started() const;
		void set_home_show_getting_started(bool v);

		Dictionary get_plugin_enabled_states() const;
		void set_plugin_enabled_states(const Dictionary& v);
	};
}

#endif // !LTE_USER_H
