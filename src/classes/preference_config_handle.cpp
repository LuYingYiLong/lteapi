#include "preference_config_handle.h"

namespace godot {
    void LTEPreferenceConfigHandle::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_schema", "schema"), &LTEPreferenceConfigHandle::set_schema);
        ClassDB::bind_method(D_METHOD("get_schema"), &LTEPreferenceConfigHandle::get_schema);
        ClassDB::bind_method(D_METHOD("set_value", "value"), &LTEPreferenceConfigHandle::set_value);
        ClassDB::bind_method(D_METHOD("get_value"), &LTEPreferenceConfigHandle::get_value);
        ClassDB::bind_method(D_METHOD("get_default_value"), &LTEPreferenceConfigHandle::get_default_value);
        ClassDB::bind_method(D_METHOD("set_value_no_signal", "value"), &LTEPreferenceConfigHandle::set_value_no_signal);
        ClassDB::bind_method(D_METHOD("has_range"), &LTEPreferenceConfigHandle::has_range);
        ClassDB::bind_method(D_METHOD("set_range", "max_val", "min_val", "setp"), &LTEPreferenceConfigHandle::set_range);
        ClassDB::bind_method(D_METHOD("get_max_value"), &LTEPreferenceConfigHandle::get_max_value);
        ClassDB::bind_method(D_METHOD("get_min_value"), &LTEPreferenceConfigHandle::get_min_value);
        ClassDB::bind_method(D_METHOD("get_step"), &LTEPreferenceConfigHandle::get_step);
        ClassDB::bind_method(D_METHOD("set_editor_hint", "hint", "hint_string"), &LTEPreferenceConfigHandle::set_editor_hint, DEFVAL(String()));
        ClassDB::bind_method(D_METHOD("get_editor_hint"), &LTEPreferenceConfigHandle::get_editor_hint);
        ClassDB::bind_method(D_METHOD("get_editor_hint_string"), &LTEPreferenceConfigHandle::get_editor_hint_string);
        ClassDB::bind_method(D_METHOD("set_editor_scene_path", "path"), &LTEPreferenceConfigHandle::set_editor_scene_path);
        ClassDB::bind_method(D_METHOD("get_editor_scene_path"), &LTEPreferenceConfigHandle::get_editor_scene_path);
        ClassDB::bind_method(D_METHOD("set_editor_scene_full_rect", "enabled"), &LTEPreferenceConfigHandle::set_editor_scene_full_rect);
        ClassDB::bind_method(D_METHOD("is_editor_scene_full_rect"), &LTEPreferenceConfigHandle::is_editor_scene_full_rect);

        ClassDB::bind_method(D_METHOD("get_key"), &LTEPreferenceConfigHandle::get_key);
        ClassDB::bind_method(D_METHOD("get_display_name"), &LTEPreferenceConfigHandle::get_display_name);
        ClassDB::bind_method(D_METHOD("get_tooltip"), &LTEPreferenceConfigHandle::get_tooltip);
        ClassDB::bind_method(D_METHOD("get_config_type"), &LTEPreferenceConfigHandle::get_config_type);
        ClassDB::bind_method(D_METHOD("is_valid"), &LTEPreferenceConfigHandle::is_valid);
        ClassDB::bind_method(D_METHOD("on_changed", "callback"), &LTEPreferenceConfigHandle::on_changed);
        ClassDB::bind_method(D_METHOD("reset_to_default"), &LTEPreferenceConfigHandle::reset_to_default);
        ClassDB::bind_method(D_METHOD("to_dict"), &LTEPreferenceConfigHandle::to_dict);
        ClassDB::bind_method(D_METHOD("from_dict", "dict"), &LTEPreferenceConfigHandle::from_dict);
    }

    LTEPreferenceConfigHandle::LTEPreferenceConfigHandle() {}
    LTEPreferenceConfigHandle::LTEPreferenceConfigHandle(const Ref<LTEPreferenceConfigSchema>& p_schema)
        : schema(p_schema) {
    }
    LTEPreferenceConfigHandle::~LTEPreferenceConfigHandle() {}

    void LTEPreferenceConfigHandle::set_schema(const Ref<LTEPreferenceConfigSchema>& p_schema) {
        schema = p_schema;
    }

    Ref<LTEPreferenceConfigSchema> LTEPreferenceConfigHandle::get_schema() const {
        return schema;
    }

    void LTEPreferenceConfigHandle::set_value(const Variant& p_value) {
        if (schema.is_valid()) {
            schema->set_value(p_value, true);
        }
    }

    void LTEPreferenceConfigHandle::set_value_no_signal(const Variant& value) {
        if (schema.is_valid()) {
            schema->set_value(value, false);
        }
    }

    Variant LTEPreferenceConfigHandle::get_value() const {
        return schema.is_valid() ? schema->get_value() : Variant();
    }

    Variant LTEPreferenceConfigHandle::get_default_value() const {
        return schema.is_valid() ? schema->get_default_value() : Variant();
    }

    bool LTEPreferenceConfigHandle::has_range() const {
        return schema.is_valid() ? schema->has_range : false;
    }

    void LTEPreferenceConfigHandle::set_range(const double min_val, const double max_val, const double step_val) const {
        if (schema.is_valid()) schema->set_range(min_val, max_val, step_val);
    }

    double LTEPreferenceConfigHandle::get_min_value() const {
        return schema.is_valid() ? schema->min_value : 0.0;
    }

    double LTEPreferenceConfigHandle::get_max_value() const {
        return schema.is_valid() ? schema->max_value : 0.0;
    }

    double LTEPreferenceConfigHandle::get_step() const {
        return schema.is_valid() ? schema->step : 0.0;
    }

    void LTEPreferenceConfigHandle::set_editor_hint(const int32_t hint, const String &hint_string) const {
        if (schema.is_valid()) {
            schema->set_editor_hint(hint, hint_string);
        }
    }

    int32_t LTEPreferenceConfigHandle::get_editor_hint() const {
        return schema.is_valid() ? schema->get_editor_hint() : 0;
    }

    String LTEPreferenceConfigHandle::get_editor_hint_string() const {
        return schema.is_valid() ? schema->get_editor_hint_string() : "";
    }

    void LTEPreferenceConfigHandle::set_editor_scene_path(const String &path) const {
        if (schema.is_valid()) {
            schema->set_editor_scene_path(path);
        }
    }

    String LTEPreferenceConfigHandle::get_editor_scene_path() const {
        return schema.is_valid() ? schema->get_editor_scene_path() : "";
    }

    void LTEPreferenceConfigHandle::set_editor_scene_full_rect(const bool enabled) const {
        if (schema.is_valid()) {
            schema->set_editor_scene_full_rect(enabled);
        }
    }

    bool LTEPreferenceConfigHandle::is_editor_scene_full_rect() const {
        return schema.is_valid() ? schema->is_editor_scene_full_rect() : false;
    }

    String LTEPreferenceConfigHandle::get_key() const {
        return schema.is_valid() ? schema->key : "";
    }

    String LTEPreferenceConfigHandle::get_display_name() const {
        return schema.is_valid() ? schema->display_name : "";
    }

    String LTEPreferenceConfigHandle::get_tooltip() const {
        return schema.is_valid() ? schema->tooltip : "";
    }

    int LTEPreferenceConfigHandle::get_config_type() const {
        return schema.is_valid() ? schema->type : -1;
    }

    bool LTEPreferenceConfigHandle::is_valid() const {
        return schema.is_valid();
    }

    void LTEPreferenceConfigHandle::on_changed(const Callable& callback) {
        if (schema.is_valid()) {
            schema->connect_changed(callback);
        }
    }

    void LTEPreferenceConfigHandle::reset_to_default() {
        if (schema.is_valid()) {
            schema->reset_to_default();
        }
    }

    bool LTEPreferenceConfigHandle::is_default() const {
        if (schema.is_valid()) {
            return schema->get_value() == schema->get_default_value();
        }
        return false;
    }

    Dictionary LTEPreferenceConfigHandle::to_dict() const {
        Dictionary dict;
        if (schema.is_valid()) {
            dict["key"] = schema->key;
            dict["type"] = schema->type;
            dict["value"] = schema->get_value();
            dict["default"] = schema->get_default_value();
        }
        return dict;
    }

    void LTEPreferenceConfigHandle::from_dict(const Dictionary& dict) {
        if (dict.has("value")) {
            set_value(dict["value"]);
        }
    }
}
