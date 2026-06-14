#include "preference_config_schema.h"
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void LTEPreferenceConfigSchema::_bind_methods() {
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_BOOL);
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_INT);
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_FLOAT);
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_STRING);
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_COLOR);
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_ENUM);
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_VECTOR2);
        BIND_ENUM_CONSTANT(PREF_CONFIG_TYPE_SCENE);
        ClassDB::bind_method(D_METHOD("get_key"), &LTEPreferenceConfigSchema::get_key);
        ClassDB::bind_method(D_METHOD("get_type"), &LTEPreferenceConfigSchema::get_type);
        ClassDB::bind_method(D_METHOD("get_category"), &LTEPreferenceConfigSchema::get_category);
        ClassDB::bind_method(D_METHOD("setup", "key", "type", "default_value", "display_name", "category"),
            &LTEPreferenceConfigSchema::setup);
        ClassDB::bind_method(D_METHOD("set_value", "value"), &LTEPreferenceConfigSchema::set_value);
        ClassDB::bind_method(D_METHOD("get_value"), &LTEPreferenceConfigSchema::get_value);
        ClassDB::bind_method(D_METHOD("get_default_value"), &LTEPreferenceConfigSchema::get_default_value);
        ClassDB::bind_method(D_METHOD("reset_to_default"), &LTEPreferenceConfigSchema::reset_to_default);
        ClassDB::bind_method(D_METHOD("set_editor_hint", "hint", "hint_string"), &LTEPreferenceConfigSchema::set_editor_hint, DEFVAL(String()));
        ClassDB::bind_method(D_METHOD("get_editor_hint"), &LTEPreferenceConfigSchema::get_editor_hint);
        ClassDB::bind_method(D_METHOD("get_editor_hint_string"), &LTEPreferenceConfigSchema::get_editor_hint_string);
        ClassDB::bind_method(D_METHOD("set_editor_scene_path", "path"), &LTEPreferenceConfigSchema::set_editor_scene_path);
        ClassDB::bind_method(D_METHOD("get_editor_scene_path"), &LTEPreferenceConfigSchema::get_editor_scene_path);
        ClassDB::bind_method(D_METHOD("set_editor_scene_full_rect", "enabled"), &LTEPreferenceConfigSchema::set_editor_scene_full_rect);
        ClassDB::bind_method(D_METHOD("is_editor_scene_full_rect"), &LTEPreferenceConfigSchema::is_editor_scene_full_rect);
        ClassDB::bind_method(D_METHOD("connect_changed", "callback"), &LTEPreferenceConfigSchema::connect_changed);
        ClassDB::bind_method(D_METHOD("set_range", "min", "max", "step"), &LTEPreferenceConfigSchema::set_range,
            DEFVAL(Variant()));
        ADD_SIGNAL(MethodInfo("value_changed",
            PropertyInfo(Variant::NIL, "old_value"),
            PropertyInfo(Variant::NIL, "new_value")));
    }

    LTEPreferenceConfigSchema::LTEPreferenceConfigSchema() {}
    LTEPreferenceConfigSchema::~LTEPreferenceConfigSchema() {}

    void LTEPreferenceConfigSchema::setup(const String& p_key, int p_type, const Variant& p_default,
        const String& p_display_name, const String& p_tooltip, const String& p_category) {
        key = p_key;
        type = p_type;
        default_value = p_default;
        if (type == PREF_CONFIG_TYPE_ENUM) {
            Variant::Type type = default_value.get_type();
            switch (type) {
            case Variant::PACKED_STRING_ARRAY: {
                PackedStringArray packed_string = default_value;
                current_value = packed_string.size() > 0 ? packed_string[0] : String();
                break;
            }  
            case Variant::PACKED_INT64_ARRAY: {
                PackedInt64Array packed_int64 = default_value;
                current_value = packed_int64.size() > 0 ? packed_int64[0] : 0;
                break;
            }
            }
        }
        else {
            current_value = default_value;
        }
        display_name = p_display_name;
        tooltip = p_tooltip;
        category = p_category;
    }

    void LTEPreferenceConfigSchema::set_value(const Variant& p_value, bool notify) {
        if (!is_type_compatible(p_value)) {
            ERR_PRINT(vformat("Preference Config: Type mismatch for key '%s'", key));
            return;
        }
        if (!validate(p_value)) {
            ERR_PRINT(vformat("Preference Config: Validation failed for key '%s'", key));
            return;
        }
        Variant value = p_value;
        if (has_range) {
            if (value.get_type() == Variant::FLOAT) {
                value = UtilityFunctions::snappedf(value, step);
                value = UtilityFunctions::clampf(value, min_value, max_value);
            }
            else if (value.get_type() == Variant::INT) {
                value = UtilityFunctions::snappedi(value, step);
                value = UtilityFunctions::clampi(value, min_value, max_value);
            }
        }
        Variant old_val = current_value;
        if (old_val == value) return;
        current_value = value;
        if (notify) {
            emit_changed(old_val, current_value);
        }
    }

    Variant LTEPreferenceConfigSchema::get_value() const {
        return current_value;
    }

    Variant LTEPreferenceConfigSchema::get_default_value() const {
        return default_value;
    }

    void LTEPreferenceConfigSchema::reset_to_default() {
        set_value(default_value);
    }

    void LTEPreferenceConfigSchema::set_editor_hint(const int32_t p_hint, const String &p_hint_string) {
        editor_hint = p_hint;
        editor_hint_string = p_hint_string;
    }

    int32_t LTEPreferenceConfigSchema::get_editor_hint() const {
        return editor_hint;
    }

    String LTEPreferenceConfigSchema::get_editor_hint_string() const {
        return editor_hint_string;
    }

    void LTEPreferenceConfigSchema::set_editor_scene_path(const String &p_path) {
        editor_scene_path = p_path;
    }

    String LTEPreferenceConfigSchema::get_editor_scene_path() const {
        return editor_scene_path;
    }

    void LTEPreferenceConfigSchema::set_editor_scene_full_rect(const bool p_enabled) {
        editor_scene_full_rect = p_enabled;
    }

    bool LTEPreferenceConfigSchema::is_editor_scene_full_rect() const {
        return editor_scene_full_rect;
    }

    bool LTEPreferenceConfigSchema::validate(const Variant& value) const {
        for (const auto& validator : validators) {
            if (!validator(value)) return false;
        }
        return true;
    }

    void LTEPreferenceConfigSchema::add_validator(const Callable& validator) {
        // 包装成std::function以便C++内部使用
        validators.push_back([validator](const Variant& val) -> bool {
            Variant ret = validator.call(val);
            return ret.booleanize();
            });
    }

    void LTEPreferenceConfigSchema::set_range(const double min_val, const double max_val, const double step_val) {
        min_value = min_val;
        max_value = max_val;
        step = step_val;
        has_range = true;
    }

    void LTEPreferenceConfigSchema::emit_changed(const Variant& old_val, const Variant& new_val) {
        // 发出Godot信号
        emit_signal("value_changed", old_val, new_val);

        // 调用直接绑定的回调
        for (const auto& cb : change_callbacks) {
            if (cb.is_valid()) {
                cb.call(old_val, new_val);
            }
        }
    }

    void LTEPreferenceConfigSchema::connect_changed(const Callable& callback) {
        change_callbacks.push_back(callback);
        // 同时连接到Godot信号系统
        connect("value_changed", callback);
    }

    bool LTEPreferenceConfigSchema::is_type_compatible(const Variant& value) const {
        Variant::Type vt = value.get_type();
        switch (type) {
        case PREF_CONFIG_TYPE_BOOL: return vt == Variant::BOOL;
        case PREF_CONFIG_TYPE_INT: return vt == Variant::INT;
        case PREF_CONFIG_TYPE_FLOAT: return vt == Variant::FLOAT;
        case PREF_CONFIG_TYPE_STRING: return vt == Variant::STRING;
        case PREF_CONFIG_TYPE_COLOR: return vt == Variant::COLOR;
        case PREF_CONFIG_TYPE_VECTOR2: return vt == Variant::VECTOR2;
        case PREF_CONFIG_TYPE_ENUM: return vt == Variant::INT || vt == Variant::STRING;
        case PREF_CONFIG_TYPE_SCENE:
            return vt != Variant::OBJECT && vt != Variant::RID && vt != Variant::CALLABLE && vt != Variant::SIGNAL;
        default: return false;
        }
    }

    // Getter 供GDScript使用
    String LTEPreferenceConfigSchema::get_key() const { return key; }
    int LTEPreferenceConfigSchema::get_type() const { return type; }
    String LTEPreferenceConfigSchema::get_category() const { return category; }
}
