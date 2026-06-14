#include "time_scrubber.h"

namespace godot {

	void TimeScrubber::_bind_methods() {
		BIND_ENUM_CONSTANT(TIME_FORMAT_HHMMSS_MSS);
		BIND_ENUM_CONSTANT(TIME_FORMAT_MMSS);

		ClassDB::bind_method(D_METHOD("get_line_edit"), &TimeScrubber::get_line_edit);

		ClassDB::bind_method(D_METHOD("set_scrub_speed", "speed"), &TimeScrubber::set_scrub_speed);
		ClassDB::bind_method(D_METHOD("get_scrub_speed"), &TimeScrubber::get_scrub_speed);

		ClassDB::bind_method(D_METHOD("set_time_format", "time_format"), &TimeScrubber::set_time_format);
		ClassDB::bind_method(D_METHOD("get_time_format"), &TimeScrubber::get_time_format);

		ClassDB::bind_method(D_METHOD("set_editable", "editable"), &TimeScrubber::set_editable);
		ClassDB::bind_method(D_METHOD("get_editable"), &TimeScrubber::get_editable);

		ClassDB::bind_method(D_METHOD("set_overrun_behavior", "overrun_behavior"), &TimeScrubber::set_overrun_behavior);
		ClassDB::bind_method(D_METHOD("get_overrun_behavior"), &TimeScrubber::get_overrun_behavior);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scrub_speed"), "set_scrub_speed", "get_scrub_speed");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "time_format", PROPERTY_HINT_ENUM, "HHMMSS.mss,MMSS"), "set_time_format", "get_time_format");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "editable"), "set_editable", "get_editable");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "text_overrun_behavior", PROPERTY_HINT_ENUM, "No Trimming,Trim Char,Trim Word,Ellipsis"), "set_overrun_behavior", "get_overrun_behavior");
	}

	Ref<TextLine> TimeScrubber::_configure_text_line(const String& p_text) const {
		Ref<TextLine> text_line;
		text_line.instantiate();

		Ref<Font> font = get_theme_font("font", "LineEdit");
		if (font.is_null()) font = get_theme_font("font");
		int font_size = get_theme_constant("font_size", "LineEdit");
		if (font_size <= 0) font_size = 16;

		text_line->set_direction(TextServer::DIRECTION_LTR);
		text_line->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);

		float width = (overrun_behavior == TextServer::OVERRUN_NO_TRIMMING) ? -1 : get_size().x;
		text_line->set_width(width);

		text_line->add_string(p_text, font, font_size);
		return text_line;
	}

	void TimeScrubber::_on_line_edit_editing_toggled(const bool toggled_on) {
		if (toggled_on) return;
		String text = line_edit->get_text();
		if (time_format == TIME_FORMAT_HHMMSS_MSS) {
			_set_value(Utils::hms_to_seconds(text));
		}
		else {
			_set_value(Utils::ms_to_seconds(text));
		}
		line_edit->set_visible(false);
	}

	void TimeScrubber::_set_value(const double value) {
		set_value(value);
		if (overrun_behavior == TextServer::OVERRUN_NO_TRIMMING) {
			update_minimum_size();
		}
		queue_redraw();
	}

	TimeScrubber::TimeScrubber() {
		set_clip_contents(true);
		set_focus_mode(FOCUS_ALL);
		set_mouse_filter(MOUSE_FILTER_STOP);
		set_step(0.001);
		set_page(0.001);

		line_edit = memnew(LineEdit);
		line_edit->set_visible(false);
		line_edit->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		line_edit->set_h_size_flags(SIZE_EXPAND_FILL);
		line_edit->set_v_size_flags(SIZE_EXPAND_FILL);
		line_edit->set_anchors_preset(Control::PRESET_FULL_RECT);
		line_edit->set_offsets_preset(Control::PRESET_FULL_RECT);
		add_child(line_edit, false, INTERNAL_MODE_FRONT);

		line_edit->connect("editing_toggled", callable_mp(this, &TimeScrubber::_on_line_edit_editing_toggled), CONNECT_DEFERRED);
	}

	void TimeScrubber::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_DRAW: {
			if (!this) return;
			if (!line_edit) return;
			if (is_queued_for_deletion()) return;
			if (line_edit && line_edit->is_inside_tree() && line_edit->is_visible()) break;
			RID canvas_rid = get_canvas_item();
			Size2 current_size = get_size();
			String txt = (time_format == TIME_FORMAT_HHMMSS_MSS) ? Utils::seconds_to_hms(get_value()) : Utils::seconds_to_ms(get_value());
			Ref<TextLine> text_line = _configure_text_line(txt);
			if (text_line.is_null()) return;
			float text_h = text_line->get_size().y;
			Vector2 draw_pos(0, (current_size.y - text_h) / 2.0f);
			if (overrun_behavior == TextServer::OVERRUN_NO_TRIMMING) {
				float text_w = text_line->get_size().x;
				draw_pos.x = (current_size.x - text_w) / 2.0f;
			}
			text_line->draw(canvas_rid, draw_pos, editable ? get_theme_color("font_color", "LineEdit") : get_theme_color("font_uneditable_color", "LineEdit"));
		} break;
		}
	}

	void TimeScrubber::_gui_input(const Ref<InputEvent>& p_event) {
		if (editable) {
			Ref<InputEventMouseButton> mb = p_event;
			if (mb.is_valid() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
				if (mb->is_pressed()) {
					// Open LineEdit
					if (mb->is_double_click() || mb->is_command_or_control_pressed()) {
						line_edit->set_text(time_format == TIME_FORMAT_HHMMSS_MSS ? 
							Utils::seconds_to_hms(get_value()) : 
							Utils::seconds_to_ms(get_value()));
						line_edit->set_visible(true);
						line_edit->grab_focus();
						line_edit->select_all();
						queue_redraw();
					}
					else {
						// Start drag
						dragging = true;
						value_at_start = get_value();
						mouse_start_pos = mb->get_position();
						Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
						queue_redraw();
					}
				}
				else {
					// Stop drag
					if (dragging) {
						dragging = false;
						Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
						Input::get_singleton()->warp_mouse(get_global_position() + mouse_start_pos);
						queue_redraw();
					}
				}
			}
		}
		Ref<InputEventMouseMotion> mm = p_event;
		if (mm.is_valid() && dragging) {
			float diff = mm->get_relative().x;
			if (mm->is_shift_pressed() && mm->is_command_or_control_pressed()) {
				diff *= 0.001;
			}
			else if (mm->is_command_or_control_pressed()) {
				diff *= 0.01;
			}
			else if (mm->is_shift_pressed()) {
				diff *= 0.1;
			}
			double new_val = get_value() + diff * scrub_speed * (time_format == TIME_FORMAT_HHMMSS_MSS ? 1000.0 : 1.0) * get_step();
			_set_value(new_val);
		}
	}

	Size2 TimeScrubber::_get_minimum_size() const {
		String placeholder = (time_format == TIME_FORMAT_HHMMSS_MSS) ? "00:00:00.000" : "00:00";
		Ref<TextLine> tyl = _configure_text_line(placeholder);
		return tyl->get_size() + Size2(20, 10);
	}

	LineEdit* TimeScrubber::get_line_edit() {
		return line_edit;
	}

	void TimeScrubber::set_scrub_speed(float p_speed) {
		scrub_speed = p_speed;
	}

	float TimeScrubber::get_scrub_speed() const {
		return scrub_speed;
	}

	void TimeScrubber::set_time_format(TimeFormat p_time_format) {
		time_format = p_time_format;
		if (time_format == TIME_FORMAT_HHMMSS_MSS) {
			set_step(0.001);
			set_page(0.001);
		}
		else {
			set_step(1.0);
			set_page(1.0);
		}
		queue_redraw();
	}

	int TimeScrubber::get_time_format() const {
		return time_format;
	}

	void TimeScrubber::set_editable(bool p_editable) {
		editable = p_editable;
		queue_redraw();
	}

	bool TimeScrubber::get_editable() const {
		return editable;
	}

	void TimeScrubber::set_overrun_behavior(TextServer::OverrunBehavior p_behavior) {
		if (overrun_behavior == p_behavior) return;
		overrun_behavior = p_behavior;
		update_minimum_size();
		queue_redraw();
	}

	TextServer::OverrunBehavior TimeScrubber::get_overrun_behavior() const {
		return overrun_behavior;
	}
}
