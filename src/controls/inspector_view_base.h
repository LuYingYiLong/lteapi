#ifndef INSPECTOR_VIEW_BASE_H
#define INSPECTOR_VIEW_BASE_H

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
	class LTEInspectorViewBase : public VBoxContainer {
		GDCLASS(LTEInspectorViewBase, VBoxContainer)

	private:
		Dictionary property_validation_data;

	protected:
		static void _bind_methods();

	public:
		GDVIRTUAL1(_validate_schema_property, Dictionary)

		Dictionary validate_schema_property(const Dictionary &property, const Dictionary &data_context);
		Dictionary get_property_validation_data() const;
	};
}

#endif
