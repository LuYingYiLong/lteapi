#include "note_skin_editor_plugin.h"

#include "note_skin_importer.h"

namespace godot {
	void LTENoteSkinEditorPlugin::_bind_methods() {
	}

	void LTENoteSkinEditorPlugin::_notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_ENTER_TREE: {
				if (importer.is_valid()) {
					return;
				}
				importer.instantiate();
				add_import_plugin(importer);
			} break;

			case NOTIFICATION_EXIT_TREE: {
				if (importer.is_valid()) {
					remove_import_plugin(importer);
					importer.unref();
				}
			} break;
		}
	}

	String LTENoteSkinEditorPlugin::_get_plugin_name() const {
		return "Lightech Note Skin Importer";
	}

	bool LTENoteSkinEditorPlugin::_has_main_screen() const {
		return false;
	}
}
