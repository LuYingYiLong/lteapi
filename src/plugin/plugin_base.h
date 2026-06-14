#ifndef PLUGIN_BASE_H
#define PLUGIN_BASE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
	class LTEPluginBase : public RefCounted {
		GDCLASS(LTEPluginBase, RefCounted)

	protected:
		static void _bind_methods();

	public:
		LTEPluginBase();
		~LTEPluginBase();

		GDVIRTUAL1(_initialize, Variant)
		GDVIRTUAL0(_enable)
		GDVIRTUAL0(_disable)
		GDVIRTUAL0(_shutdown)
		GDVIRTUAL1(_on_project_opened, Dictionary)
		GDVIRTUAL0(_on_project_closed)
		GDVIRTUAL0(_on_editor_ready)

		void _initialize(const Variant& context);
		void _enable();
		void _disable();
		void _shutdown();
		void _on_project_opened(const Dictionary& project);
		void _on_project_closed();
		void _on_editor_ready();
	};
}

#endif // !PLUGIN_BASE_H
