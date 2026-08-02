#ifndef UNDO_REDO_SERVER_H
#define UNDO_REDO_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/undo_redo.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstdint>
#include <vector>

namespace godot {
	class LTEUndoRedo : public Object {
		GDCLASS(LTEUndoRedo, Object)

	private:
		static LTEUndoRedo* singleton;

		UndoRedo* undo_redo = nullptr;
		uint64_t memory_budget_bytes = 0;
		uint64_t pending_action_memory_cost = 0;
		std::vector<uint64_t> action_memory_costs;

		void _synchronize_memory_costs();

	protected:
		static void _bind_methods();

	public:
		LTEUndoRedo();
		~LTEUndoRedo();

		static LTEUndoRedo* get_singleton();

		void create_action(const String& name, const int32_t merge_mode = UndoRedo::MERGE_DISABLE, const bool backward_undo_ops = false);
		bool commit_action(const bool execute = true);
		bool is_committing_action() const;
		void add_do_method(const Callable& callable);
		void add_undo_method(const Callable& callable);
		void add_do_property(Object* object, const StringName& property, const Variant& value);
		void add_undo_property(Object* object, const StringName& property, const Variant& value);
		void add_do_reference(Object* object);
		void add_undo_reference(Object* object);
		void add_action_memory_cost(const uint64_t bytes);
		void clear_history(const bool increase_version = true);
		int32_t get_history_count() const;
		int32_t get_current_action() const;
		String get_action_name(const int32_t id) const;
		String get_current_action_name() const;
		bool has_undo() const;
		bool has_redo() const;
		bool undo();
		bool redo();
		void set_max_steps(const int32_t max_steps);
		int32_t get_max_steps() const;
		void set_memory_budget_bytes(const uint64_t bytes);
		uint64_t get_memory_budget_bytes() const;
		uint64_t get_version() const;
	};
}

#endif // UNDO_REDO_SERVER_H
