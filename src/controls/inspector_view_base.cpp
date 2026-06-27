#include "inspector_view_base.h"

namespace godot {
	void LTEInspectorViewBase::_bind_methods() {
		ClassDB::bind_method(
			D_METHOD("validate_schema_property", "property", "data_context"),
			&LTEInspectorViewBase::validate_schema_property
		);
		ClassDB::bind_method(
			D_METHOD("get_property_validation_data"),
			&LTEInspectorViewBase::get_property_validation_data
		);
		GDVIRTUAL_BIND(_validate_schema_property, "property");
	}

	Dictionary LTEInspectorViewBase::validate_schema_property(
			const Dictionary &property,
			const Dictionary &data_context) {
		Dictionary validated_property = property.duplicate(true);
		property_validation_data = data_context.duplicate(true);
		GDVIRTUAL_CALL(_validate_schema_property, validated_property);
		property_validation_data.clear();
		return validated_property;
	}

	Dictionary LTEInspectorViewBase::get_property_validation_data() const {
		return property_validation_data.duplicate(true);
	}
}
