#ifndef CONFIRM_DIALOG_H
#define CONFIRM_DIALOG_H

#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/classes/theme.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <godot_cpp/templates/hash_map.hpp>

namespace godot {
	class LTEConfirmDialog : public Window {
		GDCLASS(LTEConfirmDialog, Window)

	private:
		PanelContainer* _panel_container = nullptr;
		MarginContainer* _margin_container = nullptr;
		Label* _message_label = nullptr;
		TextureRect* _icon_rect = nullptr;
		HBoxContainer* _button_area = nullptr;
		HBoxContainer* _content_area = nullptr;
		VBoxContainer* _vbox = nullptr;

		struct CountdownData {
			Button* button = nullptr;
			String original_text;
			String text_template;
		};
		HashMap<Timer*, CountdownData> _countdowns;

		void _build_ui();
		void _apply_editor_theme();
		void _apply_dwm_style();
		void _update_countdowns(double delta);

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		LTEConfirmDialog();
		~LTEConfirmDialog();

		void set_message(const String& text);
		String get_message() const;
		void set_icon(const Ref<Texture2D>& texture);
		void set_icon_visible(bool visible);
		Button* add_button(const String& text);
		Timer* set_countdown(Button* button, double seconds, const String& text_template = "%s (%d)");
		void set_cancel_button(Button* button);
		HBoxContainer* get_button_area() const;
		HBoxContainer* get_content_area() const;
		void set_margins(int32_t left, int32_t top, int32_t right, int32_t bottom);
		void set_content_spacing(int32_t separation_px);
		void _show_and_apply_dwm();
		void _on_default_close_requested();
		void _on_countdown_timeout(Timer* timer);
	};
}

#endif
