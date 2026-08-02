#include "undo_redo_server.h"

#include <algorithm>
#include <limits>

namespace godot {
	LTEUndoRedo* LTEUndoRedo::singleton = nullptr;

	void LTEUndoRedo::_bind_methods() {
		ClassDB::bind_method(D_METHOD("create_action", "name", "merge_mode", "backward_undo_ops"), &LTEUndoRedo::create_action, DEFVAL(UndoRedo::MERGE_DISABLE), DEFVAL(false));
		ClassDB::bind_method(D_METHOD("commit_action", "execute"), &LTEUndoRedo::commit_action, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("is_committing_action"), &LTEUndoRedo::is_committing_action);
		ClassDB::bind_method(D_METHOD("add_do_method", "callable"), &LTEUndoRedo::add_do_method);
		ClassDB::bind_method(D_METHOD("add_undo_method", "callable"), &LTEUndoRedo::add_undo_method);
		ClassDB::bind_method(D_METHOD("add_do_property", "object", "property", "value"), &LTEUndoRedo::add_do_property);
		ClassDB::bind_method(D_METHOD("add_undo_property", "object", "property", "value"), &LTEUndoRedo::add_undo_property);
		ClassDB::bind_method(D_METHOD("add_do_reference", "object"), &LTEUndoRedo::add_do_reference);
		ClassDB::bind_method(D_METHOD("add_undo_reference", "object"), &LTEUndoRedo::add_undo_reference);
		ClassDB::bind_method(D_METHOD("add_action_memory_cost", "bytes"), &LTEUndoRedo::add_action_memory_cost);
		ClassDB::bind_method(D_METHOD("clear_history", "increase_version"), &LTEUndoRedo::clear_history, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("get_history_count"), &LTEUndoRedo::get_history_count);
		ClassDB::bind_method(D_METHOD("get_current_action"), &LTEUndoRedo::get_current_action);
		ClassDB::bind_method(D_METHOD("get_action_name", "id"), &LTEUndoRedo::get_action_name);
		ClassDB::bind_method(D_METHOD("get_current_action_name"), &LTEUndoRedo::get_current_action_name);
		ClassDB::bind_method(D_METHOD("has_undo"), &LTEUndoRedo::has_undo);
		ClassDB::bind_method(D_METHOD("has_redo"), &LTEUndoRedo::has_redo);
		ClassDB::bind_method(D_METHOD("undo"), &LTEUndoRedo::undo);
		ClassDB::bind_method(D_METHOD("redo"), &LTEUndoRedo::redo);
		ClassDB::bind_method(D_METHOD("set_max_steps", "max_steps"), &LTEUndoRedo::set_max_steps);
		ClassDB::bind_method(D_METHOD("get_max_steps"), &LTEUndoRedo::get_max_steps);
		ClassDB::bind_method(D_METHOD("set_memory_budget_bytes", "bytes"), &LTEUndoRedo::set_memory_budget_bytes);
		ClassDB::bind_method(D_METHOD("get_memory_budget_bytes"), &LTEUndoRedo::get_memory_budget_bytes);
		ClassDB::bind_method(D_METHOD("get_version"), &LTEUndoRedo::get_version);

		ADD_SIGNAL(MethodInfo("history_changed"));
	}

	LTEUndoRedo::LTEUndoRedo() {
		if (singleton == nullptr) {
			singleton = this;
		}
		undo_redo = memnew(UndoRedo);
	}

	LTEUndoRedo::~LTEUndoRedo() {
		if (singleton == this) {
			singleton = nullptr;
		}
		if (undo_redo) {
			memdelete(undo_redo);
			undo_redo = nullptr;
		}
	}

	LTEUndoRedo* LTEUndoRedo::get_singleton() {
		return singleton;
	}

	void LTEUndoRedo::create_action(const String& name, const int32_t merge_mode, const bool backward_undo_ops) {
		if (!undo_redo) {
			return;
		}
		_synchronize_memory_costs();
		const int32_t current_action = undo_redo->get_current_action();
		const size_t applied_count = current_action < 0 ? 0 : static_cast<size_t>(current_action + 1);
		if (applied_count < action_memory_costs.size()) {
			action_memory_costs.resize(applied_count);
		}
		pending_action_memory_cost = 0;
		undo_redo->create_action(name, static_cast<UndoRedo::MergeMode>(merge_mode), backward_undo_ops);
	}

	bool LTEUndoRedo::commit_action(const bool execute) {
		if (!undo_redo) {
			return false;
		}
		size_t remove_count = 0;
		const int32_t configured_max_steps = undo_redo->get_max_steps();
		if (memory_budget_bytes > 0 && !action_memory_costs.empty()) {
			uint64_t projected_total = pending_action_memory_cost;
			for (const uint64_t cost : action_memory_costs) {
				projected_total = cost > std::numeric_limits<uint64_t>::max() - projected_total
						? std::numeric_limits<uint64_t>::max()
						: projected_total + cost;
			}
			while (projected_total > memory_budget_bytes && remove_count < action_memory_costs.size()) {
				projected_total -= action_memory_costs[remove_count];
				++remove_count;
			}
			if (remove_count > 0) {
				const int32_t keep_after_commit = static_cast<int32_t>(action_memory_costs.size() - remove_count + 1);
				undo_redo->set_max_steps(std::max(keep_after_commit, 1));
			}
		}
		undo_redo->commit_action(execute);
		if (remove_count > 0) {
			undo_redo->set_max_steps(configured_max_steps);
			action_memory_costs.erase(action_memory_costs.begin(), action_memory_costs.begin() + remove_count);
		}
		const int32_t history_count_after = undo_redo->get_history_count();
		if (static_cast<size_t>(history_count_after) > action_memory_costs.size()) {
			while (action_memory_costs.size() + 1 < static_cast<size_t>(history_count_after)) {
				action_memory_costs.push_back(0);
			}
			action_memory_costs.push_back(pending_action_memory_cost);
		}
		else if (history_count_after > 0 && !action_memory_costs.empty()) {
			const uint64_t previous_cost = action_memory_costs.back();
			action_memory_costs.back() = pending_action_memory_cost > std::numeric_limits<uint64_t>::max() - previous_cost
					? std::numeric_limits<uint64_t>::max()
					: previous_cost + pending_action_memory_cost;
		}
		pending_action_memory_cost = 0;
		_synchronize_memory_costs();
		emit_signal("history_changed");
		return true;
	}

	bool LTEUndoRedo::is_committing_action() const {
		return undo_redo ? undo_redo->is_committing_action() : false;
	}

	void LTEUndoRedo::add_do_method(const Callable& callable) {
		if (!undo_redo) {
			return;
		}
		undo_redo->add_do_method(callable);
	}

	void LTEUndoRedo::add_undo_method(const Callable& callable) {
		if (!undo_redo) {
			return;
		}
		undo_redo->add_undo_method(callable);
	}

	void LTEUndoRedo::add_do_property(Object* object, const StringName& property, const Variant& value) {
		if (!undo_redo || !object) {
			return;
		}
		undo_redo->add_do_property(object, property, value);
	}

	void LTEUndoRedo::add_undo_property(Object* object, const StringName& property, const Variant& value) {
		if (!undo_redo || !object) {
			return;
		}
		undo_redo->add_undo_property(object, property, value);
	}

	void LTEUndoRedo::add_do_reference(Object* object) {
		if (!undo_redo || !object) {
			return;
		}
		undo_redo->add_do_reference(object);
	}

	void LTEUndoRedo::add_undo_reference(Object* object) {
		if (!undo_redo || !object) {
			return;
		}
		undo_redo->add_undo_reference(object);
	}

	void LTEUndoRedo::add_action_memory_cost(const uint64_t bytes) {
		pending_action_memory_cost = bytes > std::numeric_limits<uint64_t>::max() - pending_action_memory_cost
				? std::numeric_limits<uint64_t>::max()
				: pending_action_memory_cost + bytes;
	}

	void LTEUndoRedo::clear_history(const bool increase_version) {
		if (!undo_redo) {
			return;
		}
		undo_redo->clear_history(increase_version);
		action_memory_costs.clear();
		pending_action_memory_cost = 0;
		emit_signal("history_changed");
	}

	int32_t LTEUndoRedo::get_history_count() const {
		return undo_redo ? undo_redo->get_history_count() : 0;
	}

	int32_t LTEUndoRedo::get_current_action() const {
		return undo_redo ? undo_redo->get_current_action() : -1;
	}

	String LTEUndoRedo::get_action_name(const int32_t id) const {
		return undo_redo ? undo_redo->get_action_name(id) : String();
	}

	String LTEUndoRedo::get_current_action_name() const {
		return undo_redo ? undo_redo->get_current_action_name() : String();
	}

	bool LTEUndoRedo::has_undo() const {
		return undo_redo ? undo_redo->has_undo() : false;
	}

	bool LTEUndoRedo::has_redo() const {
		return undo_redo ? undo_redo->has_redo() : false;
	}

	bool LTEUndoRedo::undo() {
		if (!undo_redo || !undo_redo->has_undo()) {
			return false;
		}
		bool success = undo_redo->undo();
		if (success) {
			emit_signal("history_changed");
		}
		return success;
	}

	bool LTEUndoRedo::redo() {
		if (!undo_redo || !undo_redo->has_redo()) {
			return false;
		}
		bool success = undo_redo->redo();
		if (success) {
			emit_signal("history_changed");
		}
		return success;
	}

	void LTEUndoRedo::set_max_steps(const int32_t max_steps) {
		if (!undo_redo) {
			return;
		}
		undo_redo->set_max_steps(max_steps);
		_synchronize_memory_costs();
	}

	int32_t LTEUndoRedo::get_max_steps() const {
		return undo_redo ? undo_redo->get_max_steps() : 0;
	}

	void LTEUndoRedo::set_memory_budget_bytes(const uint64_t bytes) {
		memory_budget_bytes = bytes;
		_synchronize_memory_costs();
	}

	uint64_t LTEUndoRedo::get_memory_budget_bytes() const {
		return memory_budget_bytes;
	}

	void LTEUndoRedo::_synchronize_memory_costs() {
		if (!undo_redo) {
			action_memory_costs.clear();
			return;
		}
		const int32_t history_count = std::max(undo_redo->get_history_count(), 0);
		if (action_memory_costs.size() > static_cast<size_t>(history_count)) {
			action_memory_costs.erase(
					action_memory_costs.begin(),
					action_memory_costs.begin() + (action_memory_costs.size() - static_cast<size_t>(history_count))
			);
		}
		while (action_memory_costs.size() < static_cast<size_t>(history_count)) {
			action_memory_costs.insert(action_memory_costs.begin(), 0);
		}
	}

	uint64_t LTEUndoRedo::get_version() const {
		return undo_redo ? undo_redo->get_version() : 0;
	}
}
