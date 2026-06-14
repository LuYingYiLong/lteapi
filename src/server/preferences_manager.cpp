#include "preferences_manager.h"

#include <cstdint>

#include <godot_cpp/variant/array.hpp>

namespace godot {
    LTEPreferencesManager* LTEPreferencesManager::singleton = nullptr;

    void LTEPreferencesManager::_bind_methods() {
        ClassDB::bind_method(D_METHOD("create_pref_config_handle", "key", "type", "default_value", "display_name", "tooltip", "category"),
            &LTEPreferencesManager::create_pref_config_handle, DEFVAL(""), DEFVAL("General"));
        ClassDB::bind_method(D_METHOD("get_pref_config_handle", "key"), &LTEPreferencesManager::get_pref_config_handle);
        ClassDB::bind_method(D_METHOD("get_all_pref_config_handles"), &LTEPreferencesManager::get_all_pref_config_handles);
        ClassDB::bind_method(D_METHOD("get_pref_config_category", "category"), &LTEPreferencesManager::get_pref_config_category);
        ClassDB::bind_method(D_METHOD("get_pref_config_handles_by_category"), &LTEPreferencesManager::get_pref_config_handles_by_category);
        ClassDB::bind_method(D_METHOD("get_bool_value", "key", "fallback"), &LTEPreferencesManager::get_bool_value, DEFVAL(false));
        ClassDB::bind_method(D_METHOD("get_float_value", "key", "fallback"), &LTEPreferencesManager::get_float_value, DEFVAL(0.0));
        ClassDB::bind_method(D_METHOD("has_key", "key"), &LTEPreferencesManager::has_key);
        ClassDB::bind_method(D_METHOD("reset_pref_config_category", "category"), &LTEPreferencesManager::reset_pref_config_category);
        ClassDB::bind_method(D_METHOD("reset_all_pref_config"), &LTEPreferencesManager::reset_all_pref_config);
        ClassDB::bind_method(D_METHOD("pref_queue_save"), &LTEPreferencesManager::pref_queue_save);
        ADD_SIGNAL(MethodInfo("pref_config_changed",
            PropertyInfo(Variant::STRING, "key"),
            PropertyInfo(Variant::NIL, "new_value")));
    }

    LTEPreferencesManager::LTEPreferencesManager() {
        if (singleton == nullptr) {
            singleton = this;
        }
        if (LTEAPI::get_singleton()) {
            api = LTEAPI::get_singleton();
        }
        _register_internal_configs();
    }

    LTEPreferencesManager::~LTEPreferencesManager() {
        if (singleton == this) {
            singleton = nullptr;
        }
        _flush_save();
    }

    LTEPreferencesManager* LTEPreferencesManager::get_singleton() {
        return singleton;
    }

    void LTEPreferencesManager::_register_internal_configs() {
        Ref<LTEPreferenceConfigHandle> content_scale_factor = create_pref_config_handle(
            "core.content_scale_factor",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_FLOAT,
            1.0,
            "CONTENT_SCALE_FACTOR_NAME",
            "CONTENT_SCALE_FACTOR_TOOLTIP",
            "INTERFACE_NAME"
        );
        content_scale_factor->set_range(0.5, 3.0, 0.1);

        PackedStringArray language_enum;
        language_enum.append("en");
        language_enum.append("ch");
        language_enum.append("cht");
        Ref<LTEPreferenceConfigHandle> editor_language = create_pref_config_handle(
            "core.editor_language",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_ENUM,
            language_enum,
            "EDITOR_LANGUAGE_NAME",
            "EDITOR_LANGUAGE_TOOLTIP",
            "INTERFACE_NAME"
        );

        Ref<LTEPreferenceConfigHandle> editor_keymap = create_pref_config_handle(
            "core.editor_keymap",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_SCENE,
            Dictionary(),
            "EDITOR_KEYMAP_NAME",
            "EDITOR_KEYMAP_TOOLTIP",
            "KEYMAP"
        );
        editor_keymap->set_editor_scene_path("uid://cb4bpql1hryh");
        editor_keymap->set_editor_scene_full_rect(true);

        Array timeline_panel_per_bar_presets_default;
        const int32_t default_timeline_panel_per_bars[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 32 };
        for (const int32_t per_bar : default_timeline_panel_per_bars) {
            timeline_panel_per_bar_presets_default.append(per_bar);
        }
        Ref<LTEPreferenceConfigHandle> timeline_panel_per_bar_presets = create_pref_config_handle(
            "core.timeline_panel_per_bar_presets",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_SCENE,
            timeline_panel_per_bar_presets_default,
            "TIMELINE_PANEL_PER_BAR_PRESETS_NAME",
            "TIMELINE_PANEL_PER_BAR_PRESETS_TOOLTIP",
            "TIMELINE_PANEL"
        );
        timeline_panel_per_bar_presets->set_editor_scene_path("uid://c3vf0rdl5bwj2");
        timeline_panel_per_bar_presets->set_editor_scene_full_rect(true);

        Ref<LTEPreferenceConfigHandle> editor_audio_analyzer_path = create_pref_config_handle(
            "core.editor_audio_analyzer_path",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_STRING,
            String(),
            "EDITOR_AUDIO_ANALYZER_PATH_NAME",
            "EDITOR_AUDIO_ANALYZER_PATH_TOOLTIP",
            "THIRDPARTY_NAME"
        );
        editor_audio_analyzer_path->set_editor_hint(PROPERTY_HINT_GLOBAL_FILE, "*.exe");

        Ref<LTEPreferenceConfigHandle> editor_background_image_path = create_pref_config_handle(
            "core.editor_background_image_path",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_STRING,
            String(),
            "EDITOR_BACKGROUND_IMAGE_PATH_NAME",
            "EDITOR_BACKGROUND_IMAGE_PATH_TOOLTIP",
            "PERSONALIZATION_NAME"
        );
        editor_background_image_path->set_editor_hint(PROPERTY_HINT_GLOBAL_FILE, "*.png,*.jpg,*.jpeg,*.webp,*.bmp,*.svg");

        Ref<LTEPreferenceConfigHandle> editor_background_image_opacity = create_pref_config_handle(
            "core.editor_background_image_opacity",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_FLOAT,
            0.5,
            "EDITOR_BACKGROUND_IMAGE_OPACITY_NAME",
            "EDITOR_BACKGROUND_IMAGE_OPACITY_TOOLTIP",
            "PERSONALIZATION_NAME"
        );
        editor_background_image_opacity->set_range(0.0, 1.0, 0.01);

        Ref<LTEPreferenceConfigHandle> chart_auto_save = create_pref_config_handle(
            "core.chart_auto_save",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_BOOL,
            false,
            "CHART_AUTO_SAVE_NAME",
            "CHART_AUTO_SAVE_TOOLTIP",
            "FILES"
        );

        Ref<LTEPreferenceConfigHandle> scene_auto_save = create_pref_config_handle(
            "core.scene_auto_save",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_BOOL,
            false,
            "SCENE_AUTO_SAVE_NAME",
            "SCENE_AUTO_SAVE_TOOLTIP",
            "FILES"
        );

        Ref<LTEPreferenceConfigHandle> shader_auto_save = create_pref_config_handle(
            "core.shader_auto_save",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_BOOL,
            false,
            "SHADER_AUTO_SAVE_NAME",
            "SHADER_AUTO_SAVE_TOOLTIP",
            "FILES"
        );

        Ref<LTEPreferenceConfigHandle> skin_auto_save = create_pref_config_handle(
            "core.skin_auto_save",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_BOOL,
            false,
            "SKIN_AUTO_SAVE_NAME",
            "SKIN_AUTO_SAVE_TOOLTIP",
            "FILES"
        );

        Ref<LTEPreferenceConfigHandle> auto_save_interval = create_pref_config_handle(
            "core.auto_save_interval",
            LTEPreferenceConfigSchema::PREF_CONFIG_TYPE_FLOAT,
            0.6,
            "AUTO_SAVE_INTERVAL_NAME",
            "AUTO_SAVE_INTERVAL_TOOLTIP",
            "FILES"
        );
        auto_save_interval->set_range(0.1, 60.0, 0.1);
    }

    void LTEPreferencesManager::_ensure_config_loaded() {
        if (config_loaded) return;
        Ref<ConfigFile> file;
        file.instantiate();
        if (file->load(config_file_path) == OK) {
            if (file->has_section("preferences")) {
                PackedStringArray keys = file->get_section_keys("preferences");
                for (const String& key : keys) {
                    cached_config_data[key] = file->get_value("preferences", key);
                }
            }
        }
        config_loaded = true;
    }

    void LTEPreferencesManager::_on_schema_changed(const Variant& old_val, const Variant& new_val, const String& key) {
        if (old_val == new_val) {
            return;
        }
        cached_config_data[key] = new_val;
        pending_config_data[key] = new_val;
        pref_queue_save();
        emit_signal("pref_config_changed", key, new_val);
    }

    void LTEPreferencesManager::_flush_save() {
        if (!save_pending || pending_config_data.is_empty()) {
            return;
        }
        Ref<ConfigFile> file;
        file.instantiate();
        file->load(config_file_path);
        // 只写入这次真正改动过的 key，避免加载失败或未注册 schema 时把默认值覆盖回配置文件。
        Array pending_keys = pending_config_data.keys();
        for (int i = 0; i < pending_keys.size(); ++i) {
            String key = pending_keys[i];
            Variant value = pending_config_data[key];
            file->set_value("preferences", key, value);
            cached_config_data[key] = value;
        }
        Error err = file->save(config_file_path);
        if (err != OK) {
            ERR_PRINT(vformat("Failed to save preferences: %d", err));
            return;
        }
        pending_config_data.clear();
        save_pending = false;
    }

    Ref<LTEPreferenceConfigHandle> LTEPreferencesManager::create_pref_config_handle(
        const String& key, int type, const Variant& default_value,
        const String& display_name, const String& tooltip, const String& category) {
        // 防止覆盖内部配置（安全检查）
        if (key.begins_with("core.") && schemas.has(key)) {
            // 已经初始化完成后，不允许外部创建 core. 开头的配置
            ERR_FAIL_COND_V_MSG(!Engine::get_singleton()->is_editor_hint(),
                Ref<LTEPreferenceConfigHandle>(),
                "Cannot override internal config: " + key);
        }
        if (schemas.has(key)) {
            WARN_PRINT(vformat("Preference key '%s' already exists, returning existing handle.", key));
            return get_pref_config_handle(key);
        }
        Ref<LTEPreferenceConfigSchema> schema;
        schema.instantiate();
        schema->setup(key, type, default_value, display_name, tooltip, category);
        _ensure_config_loaded();                        // 只执行一次文件IO
        if (cached_config_data.has(key)) {
            Variant saved_value = cached_config_data[key];
            // 类型检查，避免配置文件损坏导致崩溃
            if (schema->is_type_compatible(saved_value)) {
                schema->current_value = saved_value;    // 直接赋值，不触发信号
            }
        }
        // 内部存储
        schemas[key] = schema;
        // 创建Handle返回
        Ref<LTEPreferenceConfigHandle> handle;
        handle.instantiate();
        handle->set_schema(schema);
        // 连接变更信号到Manager，方便统一监听
        schema->connect("value_changed", callable_mp(this, &LTEPreferencesManager::_on_schema_changed).bind(key));
        return handle;
    }

    Ref<LTEPreferenceConfigHandle> LTEPreferencesManager::get_pref_config_handle(const String& key) const {
        Ref<LTEPreferenceConfigHandle> handle;
        handle.instantiate();
        if (schemas.has(key)) {
            handle->set_schema(schemas[key]);
        }
        return handle;
    }

    TypedArray<LTEPreferenceConfigHandle> LTEPreferencesManager::get_all_pref_config_handles() const {
        TypedArray<LTEPreferenceConfigHandle> result;
        for (const auto& pair : schemas) {
            Ref<LTEPreferenceConfigHandle> h;
            h.instantiate();
            h->set_schema(pair.value);
            result.push_back(h);
        }
        return result;
    }

    TypedArray<LTEPreferenceConfigHandle> LTEPreferencesManager::get_pref_config_category(const String& category) const {
        TypedArray<LTEPreferenceConfigHandle> result;
        for (const auto& pair : schemas) {
            if (pair.value->category == category) {
                Ref<LTEPreferenceConfigHandle> h;
                h.instantiate();
                h->set_schema(pair.value);
                result.push_back(h);
            }
        }
        return result;
    }

    Dictionary LTEPreferencesManager::get_pref_config_handles_by_category() const {
        Dictionary result;
        // 遍历所有 schema，按 category 分组
        for (const auto& pair : schemas) {
            String category = pair.value->category;
            // 获取或创建该 category 的数组
            TypedArray<LTEPreferenceConfigHandle> handles;
            if (result.has(category)) {
                handles = result[category];
            }
            // 创建 handle 并添加到数组
            Ref<LTEPreferenceConfigHandle> h;
            h.instantiate();
            h->set_schema(pair.value);
            handles.push_back(h);
            // 写回 Dictionary
            result[category] = handles;
        }
        return result;
    }

    bool LTEPreferencesManager::get_bool_value(const String& key, const bool fallback) const {
        if (!schemas.has(key)) {
            return fallback;
        }
        Ref<LTEPreferenceConfigSchema> schema = schemas[key];
        if (schema.is_null()) {
            return fallback;
        }
        Variant value = schema->get_value();
        if (value.get_type() != Variant::BOOL) {
            return fallback;
        }
        return static_cast<bool>(value);
    }

    double LTEPreferencesManager::get_float_value(const String& key, const double fallback) const {
        if (!schemas.has(key)) {
            return fallback;
        }
        Ref<LTEPreferenceConfigSchema> schema = schemas[key];
        if (schema.is_null()) {
            return fallback;
        }
        Variant value = schema->get_value();
        if (value.get_type() != Variant::FLOAT && value.get_type() != Variant::INT) {
            return fallback;
        }
        return static_cast<double>(value);
    }

    bool LTEPreferencesManager::has_key(const String& key) const {
        return schemas.has(key);
    }

    void LTEPreferencesManager::reset_pref_config_category(const String& category) {
        for (const auto& pair : schemas) {
            if (pair.value->category == category) {
                pair.value->reset_to_default();
            }
        }
    }

    void LTEPreferencesManager::reset_all_pref_config() {
        for (const auto& pair : schemas) {
            pair.value->reset_to_default();
        }
    }

    void LTEPreferencesManager::pref_queue_save() {
        save_pending = true;
        _flush_save();
    }
}
