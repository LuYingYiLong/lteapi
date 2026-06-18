#pragma once

#include <godot_cpp/classes/label.hpp>

using namespace godot;

class StautsLabel : public Label {
	GDCLASS(StautsLabel, Label)

public:
	enum StautsType {
		STAUTS_OK,
		STAUTS_WARNING,
		STAUTS_ERROR
	};

private:
	StautsType stauts = STAUTS_OK;

	void _update_theme_type_variation();

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	StautsLabel() = default;

	void set_stauts(StautsType p_stauts);
	StautsType get_stauts() const;

	void set_stauts_message(StautsType p_stauts, const String& p_message);
};

VARIANT_ENUM_CAST(StautsLabel::StautsType);