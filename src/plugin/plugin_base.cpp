#include "plugin_base.h"

namespace godot {
	void LTEPluginBase::_bind_methods() {
		GDVIRTUAL_BIND(_initialize, "context");
		GDVIRTUAL_BIND(_enable);
		GDVIRTUAL_BIND(_disable);
		GDVIRTUAL_BIND(_shutdown);
		GDVIRTUAL_BIND(_on_project_opened, "project");
		GDVIRTUAL_BIND(_on_project_closed);
		GDVIRTUAL_BIND(_on_editor_ready);
	}

	LTEPluginBase::LTEPluginBase() {
	}

	LTEPluginBase::~LTEPluginBase() {
	}

	void LTEPluginBase::_initialize(const Variant& context) {
		GDVIRTUAL_CALL(_initialize, context);
	}

	void LTEPluginBase::_enable() {
		GDVIRTUAL_CALL(_enable);
	}

	void LTEPluginBase::_disable() {
		GDVIRTUAL_CALL(_disable);
	}

	void LTEPluginBase::_shutdown() {
		GDVIRTUAL_CALL(_shutdown);
	}

	void LTEPluginBase::_on_project_opened(const Dictionary& project) {
		GDVIRTUAL_CALL(_on_project_opened, project);
	}

	void LTEPluginBase::_on_project_closed() {
		GDVIRTUAL_CALL(_on_project_closed);
	}

	void LTEPluginBase::_on_editor_ready() {
		GDVIRTUAL_CALL(_on_editor_ready);
	}
}
