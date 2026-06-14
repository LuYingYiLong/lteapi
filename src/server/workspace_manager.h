#ifndef WARKSPACE_MANAGER_H
#define WARKSPACE_MANAGER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/classes/split_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>
#include <coroutine>

#include "lte_api.h"
#include "utils.h"
#include "settings_config.h"
#include "component_registry.h"
#include "controls/panel_option_button.h"
#include "../plugin/plugin_manager.h"

namespace godot
{
	class LTEWorkspaceManager : public Object {
		GDCLASS(LTEWorkspaceManager, Object)

	private:
		static LTEWorkspaceManager* singleton;

		struct StackItem {
			uint64_t parent_id;
			Array kids;
		};

		struct FARUStackItem {
			Dictionary layout_dict;
			Node* node;
		};

		struct SignalAwaiter {
			Object* target;
			String signal;
			LTEWorkspaceManager* manager; // 需要用 manager 来作为回调中转
			uint64_t wait_token = 0;

			bool await_ready() { return false; } // 总是挂起
			void await_suspend(std::coroutine_handle<> h) {
				// 将协程句柄地址转为 uint64_t 传递给回调
				// 连接信号，触发时调用 _resume_coroutine
				target->connect(signal, callable_mp(manager, &LTEWorkspaceManager::_resume_coroutine).bind((uint64_t)h.address(), wait_token), Object::CONNECT_ONE_SHOT);
			}
			void await_resume() {}
		};

		// 定义协程的返回类型 (Fire-and-forget)
		// 使得函数返回 void 但可以使用 co_await
		struct CoVoid {
			struct promise_type {
				CoVoid get_return_object() { return {}; }
				std::suspend_never initial_suspend() { return {}; }
				std::suspend_never final_suspend() noexcept { return {}; }
				void return_void() {}
				void unhandled_exception() {}
			};
		};

		LTEAPI* api = nullptr;
		LTESettingsConfig* settings_config = nullptr;
		LTEComponentRegistry* component_registry = nullptr;
		bool is_opened_layout = false;
		PackedStringArray layout_files;
		Array current_layout_panels;
		Dictionary floating_workspace_window_ids;
		NodePath focused_panel_parent_path;
		int32_t focused_panel_index = -1;
		int32_t layout_load_generation = 0;
		uint64_t coroutine_wait_serial = 0;
		String current_opened_layout;

		void _on_project_opened(const String& path);
		Dictionary _serialize_node(Node* node);
		void _find_panel_option_button(Node* node, String current_path, Dictionary& layout_dict);
		void _resume_coroutine(uint64_t handle_address, uint64_t wait_token);
		CoVoid _load_layout_children(const Array& children, Control* parent_node, const String file_path, int32_t generation, bool main_layout, uint64_t save_root_id);
		bool _load_layout_children_immediate(const Array& children, Control* parent_node, const String& file_path, Node* save_root_node);
		void _clear_workspace_root_children(Node* root_node);
		bool _normalize_layout_root(Dictionary& layout_dict, const String& file_path) const;
		bool _layout_branch_has_panel(const Variant& branch, const String& panel_uuid) const;
		String _get_preferred_workspace_panel_uuid(const String& file_path) const;
		void _on_split_drag_ended(const String& file_path, Node* root_node);
		void _save_layout_from_root(const String& file_path, Node* root_node);
		void _fit_to_window(const Dictionary layout_dict, Node* root_node);
		void _erase_uuid(const Dictionary& layout_dict);
		String _find_and_reset_panel(Dictionary& layout_dict, Node* root_node, const String& new_panel_uuid, const String& old_runtime_uuid);
		void _on_window_instance_close_requested(Window* window);
		void _on_window_instance_visibility_changed(Window* window);
		void _find_layout_uuid(PackedStringArray& uuids, const Array& children);
		void _apply_workspace_tabs_order();
		void _save_workspace_tabs_order();
		bool _is_same_workspace_tab(const String& order_entry, const String& file_path) const;
		PackedStringArray _get_default_workspace_templates(bool include_all) const;
		Error _copy_default_workspace_templates(const String& project_path, bool include_all, PackedStringArray& copied_layout_files) const;
		String _get_first_available_layout() const;
		String _resolve_workspace_file_path(const String& file_path) const;
		String _get_first_dockable_layout(const String& excluded_file_path = String()) const;
		String _get_workspace_state_key(const String& file_path) const;
		bool _has_floating_workspace_marker(const String& file_path) const;
		void _set_floating_workspace_marker(const String& file_path, bool floating);
		void _restore_floating_workspaces();
		void _close_floating_workspace_windows();
		Window* _get_floating_workspace_window(const String& file_path) const;
		Window* _open_floating_workspace_window(const String& file_path, const Vector2i& fallback_size, bool restore_state);
		void _resize_floating_workspace_root(Window* window);
		void _save_floating_workspace_window_state(const String& file_path, Window* window);
		void _apply_floating_workspace_window_state(const String& file_path, Window* window, const Vector2i& fallback_size);
		Dictionary _sanitize_window_state(const Dictionary& state, const Vector2i& fallback_size) const;
		int32_t _get_screen_for_window_rect(const Vector2i& position, const Vector2i& size, int32_t fallback_screen) const;
		void _fit_floating_workspace_on_frame(const String& file_path, uint64_t root_id, int32_t remaining_frames);
		void _on_floating_workspace_size_changed(const String& file_path, uint64_t window_id, uint64_t root_id);
		void _on_floating_workspace_state_changed(const String& file_path, uint64_t window_id);
		void _on_floating_workspace_close_requested(const String& file_path, Window* window);

		// 插件面板热替换
		HashMap<String, String> _panel_owner_cache;      // runtime_uuid -> plugin_id
		HashMap<String, String> _known_panel_owners;     // panel_uuid -> plugin_id (persists)
		HashMap<String, Dictionary> _hot_replace_map;    // runtime_uuid -> {panel_uuid, plugin_id, panel_name, ...}

		void _connect_plugin_signals();
		void _on_plugin_disabled(const String& plugin_id);
		void _on_plugin_enabled(const String& plugin_id);
		void _on_plugin_uninstalled(const String& plugin_id);
		void _swap_panels_to_unknown_in_tree(Node* root_node, const String& plugin_id);
		void _swap_single_panel_to_unknown(Node* panel_node, const String& runtime_uuid, const String& panel_uuid, const String& plugin_id, const String& panel_name);
		void _restore_panels_in_tree(Node* root_node, const String& plugin_id);

	protected:
		static void _bind_methods();

	public:
		LTEWorkspaceManager();
		~LTEWorkspaceManager();

		static LTEWorkspaceManager* get_singleton();

		auto wait_for_signal(Object* target, String signal) {
			coroutine_wait_serial++;
			return SignalAwaiter{ target, signal, this, coroutine_wait_serial };
		}

		enum SplitDirection {
			SPLIT_HORZ_LEFT,
			SPLIT_HORZ_RIGHT,
			SPLIT_VERT_TOP,
			SPLIT_VERT_BOTTOM
		};

		void scan_workspace_directory();
		void clear_project_state();
		Error ensure_default_workspaces(const String& project_path, bool include_all);
		PackedStringArray get_layout_files() const;
		void set_layout_files_order(const PackedStringArray& ordered_layout_files);
		int32_t get_dockable_layout_count() const;
		Array get_current_layout_panels() const;
		Dictionary serialize_layout();
		void open_layout(const String& file_path, bool force = false);
		void float_layout(const String& file_path, const Vector2i& window_size);
		bool is_layout_floating(const String& file_path) const;
		String get_last_opened_layout() const;
		void save_layout(const String& file_path);
		void layout_fit_to_window(const String& file_path);
		void rename_layout(const String& file_path, const String& new_name);
		void remove_layout(const String& file_path);
		void export_layout(const String& file_path, const String& export_path);
		void copy_layout(const String& file_path);
		String reset_panel(const String& file_path, const String& new_panel_uuid, const String& old_runtime_uuid);
		void change_panel(const String& file_path, Node* panel, const String& panel_uuid, const String& runtime_uuid);
		void add_panel(const String& file_path, Node* panel, const String& panel_uuid, const SplitDirection split_direction);
		CoVoid _on_add_panel(const String file_path, Node* panel, const String panel_uuid, const SplitDirection split_direction);
		void close_panel(const String& file_path, Node* panel);
		CoVoid _on_close_panel(const String file_path, Node* panel);
		void enter_panel_focus_mode(Node* panel);
		void exit_panel_focus_mode();
		bool is_panel_focus_mode_active() const;
		Window* create_empty_window(const Vector2i& window_size = Vector2i(1152, 648), const String& title = "New Window", bool show_now = true, bool auto_release = true);
		void open_window_instance(const Vector2i& window_size, const String& panel_uuid, const String& runtime_uuid = "", const Dictionary& parameters = Dictionary());
		void duplicate_panel_into_new_window(Node* panel, const String& panel_uuid, const String& runtime_uuid = "", const String& title = "");
		void open_window_instance_from_scene(const Vector2i& window_size, const String& title, Ref<PackedScene> packed_scene);
		void add_scene_to_editor(Ref<PackedScene> packed_scene, const Dictionary& parameters = Dictionary());
		PackedStringArray get_all_layout_runtime_uuids();
	};
}

VARIANT_ENUM_CAST(LTEWorkspaceManager::SplitDirection);

#endif // !WARKSPACE_MANAGER_H
