#include "stauts_label.h"

using namespace godot;

void StautsLabel::_bind_methods() {
	BIND_ENUM_CONSTANT(STAUTS_OK);
	BIND_ENUM_CONSTANT(STAUTS_WARNING);
	BIND_ENUM_CONSTANT(STAUTS_ERROR);

	ClassDB::bind_method(D_METHOD("set_stauts", "new_stauts"), &StautsLabel::set_stauts);
	ClassDB::bind_method(D_METHOD("get_stauts"), &StautsLabel::get_stauts);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "stauts", PROPERTY_HINT_ENUM, "OK,Warning,Error"), "set_stauts", "get_stauts");

	ClassDB::bind_method(D_METHOD("set_stauts_message", "stauts", "message"), &StautsLabel::set_stauts_message);
}

void StautsLabel::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_READY: {
		_update_theme_type_variation();
	} break;
	}
}

void StautsLabel::_update_theme_type_variation() {
	switch (stauts) {
	case STAUTS_OK: {
		set_theme_type_variation("OKLabel");
	} break;
	case STAUTS_WARNING: {
		set_theme_type_variation("WarningLabel");
	} break;
	case STAUTS_ERROR: {
		set_theme_type_variation("ErrorLabel");
	} break;
	}
}

void StautsLabel::set_stauts(StautsType p_stauts) {
	stauts = p_stauts;
	_update_theme_type_variation();
}

StautsLabel::StautsType StautsLabel::get_stauts() const {
	return stauts;
}

void StautsLabel::set_stauts_message(StautsType p_stauts, const String& p_message) {
	set_stauts(p_stauts);
	set_text(p_message);
}