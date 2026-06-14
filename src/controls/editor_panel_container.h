#ifndef EDITOR_PANEL_CONTAINER_H
#define EDITOR_PANEL_CONTAINER_H

#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
	class EditorPanelContainer : public PanelContainer {
		GDCLASS(EditorPanelContainer, PanelContainer)

	private:
		Dictionary hotkey_tips;
		String presence_panel_name;

		bool _can_process_panel_input() const;
		bool _is_editor_focus_mode_active() const;
		bool _is_inside_editor_focused_panel() const;
		void _deploy_hotkey_tips() const;

	protected:
		static void _bind_methods();

	public:
		EditorPanelContainer();

		void _input(const Ref<InputEvent>& p_event) override;
		void process_panel_input(const Ref<InputEvent>& p_event);

		void set_hotkey_tips(const Dictionary& p_hotkey_tips);
		Dictionary get_hotkey_tips() const;
		void clear_all_hotkey_tips();

		void set_presence_panel_name(const String& p_name);
		String get_presence_panel_name() const;
	};
}

#endif // !EDITOR_PANEL_CONTAINER_H
