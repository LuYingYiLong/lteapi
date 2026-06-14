#include "confirm_dialog.h"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "util/utils.h"

namespace godot {
	void LTEConfirmDialog::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_message", "text"), &LTEConfirmDialog::set_message);
		ClassDB::bind_method(D_METHOD("get_message"), &LTEConfirmDialog::get_message);
		ClassDB::bind_method(D_METHOD("set_icon", "texture"), &LTEConfirmDialog::set_icon);
		ClassDB::bind_method(D_METHOD("set_icon_visible", "visible"), &LTEConfirmDialog::set_icon_visible);
		ClassDB::bind_method(D_METHOD("add_button", "text"), &LTEConfirmDialog::add_button);
		ClassDB::bind_method(D_METHOD("set_countdown", "button", "seconds", "text_template"), &LTEConfirmDialog::set_countdown, DEFVAL("%s (%d)"));
		ClassDB::bind_method(D_METHOD("set_cancel_button", "button"), &LTEConfirmDialog::set_cancel_button);
		ClassDB::bind_method(D_METHOD("get_button_area"), &LTEConfirmDialog::get_button_area);
		ClassDB::bind_method(D_METHOD("get_content_area"), &LTEConfirmDialog::get_content_area);
		ClassDB::bind_method(D_METHOD("set_margins", "left", "top", "right", "bottom"), &LTEConfirmDialog::set_margins);
		ClassDB::bind_method(D_METHOD("set_content_spacing", "separation_px"), &LTEConfirmDialog::set_content_spacing);
		ClassDB::bind_method(D_METHOD("_show_and_apply_dwm"), &LTEConfirmDialog::_show_and_apply_dwm);
		ClassDB::bind_method(D_METHOD("_on_default_close_requested"), &LTEConfirmDialog::_on_default_close_requested);
	}

	LTEConfirmDialog::LTEConfirmDialog() {
		set_oversampling_override(1.0);
		set_initial_position(Window::WINDOW_INITIAL_POSITION_CENTER_MAIN_WINDOW_SCREEN);
		set_size(Vector2i(512, 100));
		hide();
		set_transient(true);
		set_transient_to_focused(true);
		set_exclusive(true);
		set_force_native(true);
	}

	LTEConfirmDialog::~LTEConfirmDialog() {}

	void LTEConfirmDialog::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_apply_editor_theme();
			_build_ui();
			break;

		case NOTIFICATION_READY:
			set_wrap_controls(true);
			call_deferred("_show_and_apply_dwm");
			connect("close_requested", Callable(this, "_on_default_close_requested"));
			break;

		case NOTIFICATION_PROCESS:
			_update_countdowns(get_process_delta_time());
			break;
		}
	}

	void LTEConfirmDialog::_build_ui() {
		_panel_container = memnew(PanelContainer);
		_panel_container->set_anchors_preset(Control::PRESET_FULL_RECT);
		_panel_container->set_theme_type_variation("WindowPanel");
		add_child(_panel_container);

		_margin_container = memnew(MarginContainer);
		_margin_container->add_theme_constant_override("margin_left", 8);
		_margin_container->add_theme_constant_override("margin_top", 8);
		_margin_container->add_theme_constant_override("margin_right", 8);
		_margin_container->add_theme_constant_override("margin_bottom", 8);
		_panel_container->add_child(_margin_container);

		_vbox = memnew(VBoxContainer);
		_margin_container->add_child(_vbox);

		_content_area = memnew(HBoxContainer);
		_vbox->add_child(_content_area);

		_icon_rect = memnew(TextureRect);
		_icon_rect->set_custom_minimum_size(Vector2(64, 64));
		Ref<Texture2D> default_icon = ResourceLoader::get_singleton()->load("uid://clhb4qtaicn4q");
		if (default_icon.is_valid()) {
			_icon_rect->set_texture(default_icon);
		}
		_icon_rect->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
		_icon_rect->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		_icon_rect->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);
		_icon_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
		_content_area->add_child(_icon_rect);

		_message_label = memnew(Label);
		_message_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		_content_area->add_child(_message_label);

		_button_area = memnew(HBoxContainer);
		_vbox->add_child(_button_area);
	}

	void LTEConfirmDialog::_apply_editor_theme() {
		if (get_theme().is_valid()) {
			return;
		}
		Ref<Theme> editor_theme = ResourceLoader::get_singleton()->load("uid://70ddv4b61mnu");
		if (editor_theme.is_valid()) {
			set_theme(editor_theme);
		}
	}

	void LTEConfirmDialog::_apply_dwm_style() {
		if (!Utils::check_dwm_at_runtime()) {
			return;
		}
		ClassDBSingleton* class_db = ClassDBSingleton::get_singleton();
		if (class_db == nullptr) {
			return;
		}
		Color title_bar_color("#282828");
		Color border_color("#5a5a5a");
		class_db->class_call_static("DWM", "set_title_bar_color", this, title_bar_color, true);
		class_db->class_call_static("DWM", "set_border_color", this, border_color);
	}

	void LTEConfirmDialog::_show_and_apply_dwm() {
		show();
		if (DisplayServer::get_singleton()->get_name() != "headless") {
			_apply_dwm_style();
		}
	}

	void LTEConfirmDialog::_on_default_close_requested() {
		queue_free();
	}

	void LTEConfirmDialog::set_message(const String& text) {
		_message_label->set_text(text);
	}

	String LTEConfirmDialog::get_message() const {
		return _message_label->get_text();
	}

	void LTEConfirmDialog::set_icon(const Ref<Texture2D>& texture) {
		_icon_rect->set_texture(texture);
	}

	void LTEConfirmDialog::set_icon_visible(bool visible) {
		_icon_rect->set_visible(visible);
	}

	Button* LTEConfirmDialog::add_button(const String& text) {
		Button* button = memnew(Button);
		button->set_text(text);
		button->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		button->set_default_cursor_shape(Control::CURSOR_POINTING_HAND);
		_button_area->add_child(button);
		return button;
	}

	Timer* LTEConfirmDialog::set_countdown(Button* button, double seconds, const String& text_template) {
		button->set_disabled(true);
		String original_text = button->get_text();

		Timer* timer = memnew(Timer);
		timer->set_one_shot(true);
		timer->set_wait_time(seconds);
		timer->set_autostart(true);
		add_child(timer);

		CountdownData data;
		data.button = button;
		data.original_text = original_text;
		data.text_template = text_template;
		_countdowns[timer] = data;

		timer->connect("timeout", callable_mp(this, &LTEConfirmDialog::_on_countdown_timeout).bind(timer));
		set_process(true);
		return timer;
	}

	void LTEConfirmDialog::set_cancel_button(Button* button) {
		button->connect("pressed", Callable(this, "_on_default_close_requested"));
	}

	HBoxContainer* LTEConfirmDialog::get_button_area() const {
		return _button_area;
	}

	HBoxContainer* LTEConfirmDialog::get_content_area() const {
		return _content_area;
	}

	void LTEConfirmDialog::set_margins(int32_t left, int32_t top, int32_t right, int32_t bottom) {
		if (_margin_container == nullptr) {
			return;
		}
		_margin_container->add_theme_constant_override("margin_left", left);
		_margin_container->add_theme_constant_override("margin_top", top);
		_margin_container->add_theme_constant_override("margin_right", right);
		_margin_container->add_theme_constant_override("margin_bottom", bottom);
	}

	void LTEConfirmDialog::set_content_spacing(int32_t separation_px) {
		if (_vbox == nullptr) {
			return;
		}
		_vbox->add_theme_constant_override("separation", separation_px);
	}

	void LTEConfirmDialog::_on_countdown_timeout(Timer* timer) {
		if (!_countdowns.has(timer)) {
			return;
		}
		CountdownData& data = _countdowns[timer];
		Button* button = data.button;
		if (button != nullptr) {
			button->set_disabled(false);
			button->set_text(data.original_text);
		}
		_countdowns.erase(timer);
		timer->queue_free();

		if (_countdowns.is_empty()) {
			set_process(false);
		}
	}

	void LTEConfirmDialog::_update_countdowns(double delta) {
		(void)delta;
		bool has_active = false;
		for (KeyValue<Timer*, CountdownData>& kv : _countdowns) {
			Timer* timer = kv.key;
			if (timer->is_stopped()) {
				continue;
			}
			has_active = true;
			CountdownData& data = kv.value;
			Button* button = data.button;
			if (button == nullptr) {
				continue;
			}
			int64_t remaining = (int64_t)Math::ceil(timer->get_time_left());
			Array format_args;
			format_args.append(data.original_text);
			format_args.append(remaining);
			button->set_text(data.text_template % format_args);
		}
		if (!has_active) {
			set_process(false);
		}
	}
}
