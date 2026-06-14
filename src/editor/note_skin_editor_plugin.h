#ifndef NOTE_SKIN_EDITOR_PLUGIN_H
#define NOTE_SKIN_EDITOR_PLUGIN_H

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/ref.hpp>

namespace godot {
	class LTENoteSkinImporter;

	class LTENoteSkinEditorPlugin : public EditorPlugin {
		GDCLASS(LTENoteSkinEditorPlugin, EditorPlugin)

	private:
		Ref<LTENoteSkinImporter> importer;

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		virtual String _get_plugin_name() const override;
		virtual bool _has_main_screen() const override;
	};
}

#endif
