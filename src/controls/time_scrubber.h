#ifndef TIME_SCRUBBER_H
#define TIME_SCRUBBER_H

#include "utils.h"
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/range.hpp>
#include <godot_cpp/classes/style_box.hpp>
#include <godot_cpp/classes/text_line.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    class TimeScrubber : public Range {
        GDCLASS(TimeScrubber, Range)

    private:
        LineEdit* line_edit = nullptr;

        bool dragging = false;
        double value_at_start = 0.0;
        Vector2 mouse_start_pos;

        Ref<TextLine> _configure_text_line(const String& p_text) const;
        void _on_line_edit_editing_toggled(const bool toggled_on);
        void _set_value(const double value);

    protected:
        static void _bind_methods();
        void _notification(int p_what);

    public:
        TimeScrubber();

        float scrub_speed = 1.0;
        enum TimeFormat {
            TIME_FORMAT_HHMMSS_MSS,
            TIME_FORMAT_MMSS,
        };
        TimeFormat time_format = TIME_FORMAT_HHMMSS_MSS;
        bool editable = true;
        TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;

        virtual void _gui_input(const Ref<InputEvent>& p_event) override;
        virtual Size2 _get_minimum_size() const override;
        LineEdit* get_line_edit();

        void set_scrub_speed(float p_speed);
        float get_scrub_speed() const;

        void set_time_format(TimeFormat p_time_format);
        int get_time_format() const;
        
        void set_editable(bool p_editable);
        bool get_editable() const;

        void set_overrun_behavior(TextServer::OverrunBehavior p_behavior);
        TextServer::OverrunBehavior get_overrun_behavior() const;
    };
}

VARIANT_ENUM_CAST(TimeScrubber::TimeFormat);

#endif // !TIME_SCRUBBER_H