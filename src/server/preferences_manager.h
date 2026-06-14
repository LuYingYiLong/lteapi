#ifndef PREFERENCES_MANAGER_H
#define PREFERENCES_MANAGER_H

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/timer.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "classes/preference_config_handle.h"
#include "classes/preference_config_schema.h"
#include "lte_api.h"

namespace godot {
    class LTEPreferencesManager : public Object {
        GDCLASS(LTEPreferencesManager, Object)

    private:
        static LTEPreferencesManager* singleton;

        LTEAPI* api = nullptr;
        HashMap<String, Ref<LTEPreferenceConfigSchema>> schemas;
        String config_file_path = "user://preferences.cfg";
        Dictionary cached_config_data;  // 缓存加载的数据
        Dictionary pending_config_data;
        bool config_loaded = false;
        bool save_pending = false;
        Timer* save_timer = nullptr;

        void _register_internal_configs();
        void _ensure_config_loaded();
        void _on_schema_changed(const Variant& old_val, const Variant& new_val, const String& key);
        void _flush_save();

    protected:
        static void _bind_methods();

    public:
        LTEPreferencesManager();
        ~LTEPreferencesManager();

        static LTEPreferencesManager* get_singleton();

        // 核心：创建Schema，返回Handle
        Ref<LTEPreferenceConfigHandle> create_pref_config_handle(
            const String& key,
            int type,
            const Variant& default_value,
            const String& display_name,
            const String& tooltip = "",
            const String& category = "General"
        );
        // 通过key获取Handle
        Ref<LTEPreferenceConfigHandle> get_pref_config_handle(const String& key) const;
        // 批量操作
        TypedArray<LTEPreferenceConfigHandle> get_all_pref_config_handles() const;
        TypedArray<LTEPreferenceConfigHandle> get_pref_config_category(const String& category) const;
        Dictionary get_pref_config_handles_by_category() const;
        bool get_bool_value(const String& key, const bool fallback = false) const;
        double get_float_value(const String& key, const double fallback = 0.0) const;
        // 检查是否存在
        bool has_key(const String& key) const;
        // 重置
        void reset_pref_config_category(const String& category);
        void reset_all_pref_config();
        // 标记需要保存
        void pref_queue_save();
    };
}

#endif
