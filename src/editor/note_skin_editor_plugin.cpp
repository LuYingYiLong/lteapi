#include "note_skin_editor_plugin.h"

#include "lightech_zip_importer.h"
#include "note_skin_importer.h"

namespace godot {
	void LTENoteSkinEditorPlugin::_bind_methods() {
	}

	void LTENoteSkinEditorPlugin::_notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_ENTER_TREE: {
				if (!importer.is_valid()) {
					importer.instantiate();
					add_import_plugin(importer);
				}
				if (!zip_importer.is_valid()) {
					zip_importer.instantiate();
					add_import_plugin(zip_importer);
				}
			} break;

			case NOTIFICATION_EXIT_TREE: {
				if (importer.is_valid()) {
					remove_import_plugin(importer);
					importer.unref();
				}
				if (zip_importer.is_valid()) {
					remove_import_plugin(zip_importer);
					zip_importer.unref();
				}
			} break;
		}
	}

	String LTENoteSkinEditorPlugin::_get_plugin_name() const {
		return "Lightech Resource Importers";
	}

	bool LTENoteSkinEditorPlugin::_has_main_screen() const {
		return false;
	}
}
