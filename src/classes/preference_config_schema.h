#ifndef PREFERENCE_CONFIG_SCHEMA_H
#define PREFERENCE_CONFIG_SCHEMA_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <vector>
#include <functional>

namespace godot {
	class LTEPreferenceConfigSchema : public RefCounted {
		GDCLASS(LTEPreferenceConfigSchema, RefCounted)  

	private:
		std::vector<Callable> change_callbacks;
		std::vector<std::function<bool(const Variant&)>> validators;

	protected:
		static void _bind_methods();

	public:
		LTEPreferenceConfigSchema();
		~LTEPreferenceConfigSchema();

		enum PrefConfigType {
			PREF_CONFIG_TYPE_BOOL,
			PREF_CONFIG_TYPE_INT,
			PREF_CONFIG_TYPE_FLOAT,
			PREF_CONFIG_TYPE_STRING,
			PREF_CONFIG_TYPE_COLOR,
			PREF_CONFIG_TYPE_ENUM,      // 新增：下拉选择
			PREF_CONFIG_TYPE_VECTOR2,   // 新增：向量
			PREF_CONFIG_TYPE_SCENE,     // 自定义偏好编辑器场景
			PREF_CONFIG_TYPE_MAX
		};

		// 元数据
		String key;
		int32_t type = PREF_CONFIG_TYPE_STRING;
		Variant default_value;
		Variant current_value;
		String display_name;
		String tooltip;
		String category;
		int32_t editor_hint = 0;
		String editor_hint_string;
		String editor_scene_path;
		bool editor_scene_full_rect = false;

		// 约束
		double min_value;               // 用于数值类型
		double max_value;
		double step;                    // 步进值

		// 高级：条件显示（某个字段满足条件时才显示）
		String visible_when_key;        // 依赖的key
		Variant visible_when_value;     // 依赖的值

		bool has_range = false;

		String get_key() const;
		int get_type() const;
		String get_category() const;

		// 初始化方法（GDScript 友好）
		void setup(const String& p_key, int p_type, const Variant& p_default,
			const String& p_display_name, const String& p_tooltip, const String& p_category);

		// 值操作
		void set_value(const Variant& p_value, bool notify = true);
		Variant get_value() const;
		Variant get_default_value() const;
		void reset_to_default();
		void set_editor_hint(const int32_t p_hint, const String &p_hint_string = String());
		int32_t get_editor_hint() const;
		String get_editor_hint_string() const;
		void set_editor_scene_path(const String &p_path);
		String get_editor_scene_path() const;
		void set_editor_scene_full_rect(const bool p_enabled);
		bool is_editor_scene_full_rect() const;

		// 验证
		bool validate(const Variant& value) const;
		void add_validator(const Callable& validator);
		void set_range(const double min_val, const double max_val, const double step_val);

		// 内部回调
		void emit_changed(const Variant& old_val, const Variant& new_val);
		void connect_changed(const Callable& callback);

		// 类型检查工具
		bool is_type_compatible(const Variant& value) const;
		//static bool can_convert_type(int pref_type, Variant::Type variant_type);
	};
}

VARIANT_ENUM_CAST(LTEPreferenceConfigSchema::PrefConfigType);

#endif // !PREFERENCE_CONFIG_SCHEMA_H
