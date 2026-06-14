#ifndef PROPERTIES_SERVER_H
#define PROPERTIES_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
	class LTEPropertiesServer : public Object {
		GDCLASS(LTEPropertiesServer, Object)

	private:
		static LTEPropertiesServer* singleton;

		String selection_context_id;
		Dictionary selections;
		Dictionary property_schemas;

		Dictionary make_selection(const String& context_id = String()) const;
		bool metadata_matches_filter(const Dictionary& selection, const Dictionary& metadata_filter) const;
		void update_active_selection_after_clear();

	protected:
		static void _bind_methods();

	public:
		LTEPropertiesServer();
		~LTEPropertiesServer();

		static LTEPropertiesServer* get_singleton();

		void clear_project_state();
		void set_selection(const String& context_id, const Array& items, const Dictionary& metadata = Dictionary());
		void set_selection_silent(const String& context_id, const Array& items, const Dictionary& metadata = Dictionary());
		void clear_selection(const String& context_id = String(), const Dictionary& metadata_filter = Dictionary());
		Dictionary get_selection(const String& context_id = String()) const;
		String get_selection_context_id() const;
		Array get_selection_items(const String& context_id = String()) const;
		Dictionary get_selection_metadata(const String& context_id = String()) const;
		Dictionary get_all_selections() const;
		bool has_selection(const String& context_id = String()) const;

		void submit_items(const String& context_id, const Array& old_items, const Array& new_items, const Dictionary& metadata = Dictionary());
		void submit_property_patch(const String& context_id, const Dictionary& patch, const Dictionary& metadata = Dictionary());

		void set_property_schema(const String& context_id, const Dictionary& schema);
		Dictionary get_property_schema(const String& context_id) const;
		Dictionary get_all_property_schemas() const;
		void clear_property_schema(const String& context_id);
		void clear_all_property_schemas();
	};
}


#endif // PROPERTIES_SERVER_H
