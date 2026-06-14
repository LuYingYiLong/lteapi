#ifndef PREFERENCE_CONFIG_HANDLE_H
#define PREFERENCE_CONFIG_HANDLE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/callable.hpp>

#include "preference_config_schema.h"

namespace godot {
	class LTEPreferenceConfigHandle : public RefCounted {
		GDCLASS(LTEPreferenceConfigHandle, RefCounted)

	private:
		Ref<LTEPreferenceConfigSchema> schema;

	protected:
		static void _bind_methods();

	public:
		LTEPreferenceConfigHandle();
		LTEPreferenceConfigHandle(const Ref<LTEPreferenceConfigSchema>& p_schema);
		~LTEPreferenceConfigHandle();

		void set_schema(const Ref<LTEPreferenceConfigSchema>& p_schema);
		Ref<LTEPreferenceConfigSchema> get_schema() const;

		// 核心操作
		void set_value(const Variant& p_value);
		Variant get_value() const;
		Variant get_default_value() const;
		void set_value_no_signal(const Variant& value);
		bool has_range() const;
		void set_range(const double min_val, const double max_val, const double step_val) const;
		double get_min_value() const;
		double get_max_value() const;
		double get_step() const;
		void set_editor_hint(const int32_t hint, const String &hint_string = String()) const;
		int32_t get_editor_hint() const;
		String get_editor_hint_string() const;
		void set_editor_scene_path(const String &path) const;
		String get_editor_scene_path() const;
		void set_editor_scene_full_rect(const bool enabled) const;
		bool is_editor_scene_full_rect() const;

		// 元数据访问
		String get_key() const;
		String get_display_name() const;
		String get_tooltip() const;
		int get_config_type() const;
		bool is_valid() const;

		// 监听（委托给schema）
		void on_changed(const Callable& callback);
		//void disconnect_changed(const Callable& callback);

		// 重置
		void reset_to_default();
		bool is_default() const;

		// 序列化支持
		Dictionary to_dict() const;
		void from_dict(const Dictionary& dict);
	};
}

#endif
