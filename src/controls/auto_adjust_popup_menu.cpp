#include "auto_adjust_popup_menu.h"

namespace godot {
	void AutoAdjustPopupMenu::_bind_methods() {
		ClassDB::bind_method(D_METHOD("enable_auto_adjust", "enable"), &AutoAdjustPopupMenu::enable_auto_adjust);
		ClassDB::bind_method(D_METHOD("is_auto_adjust"), &AutoAdjustPopupMenu::is_auto_adjust);

		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_adjust"), "enable_auto_adjust", "is_auto_adjust");
	}

	void AutoAdjustPopupMenu::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_READY: {
			connect("visibility_changed", callable_mp(this, &AutoAdjustPopupMenu::_adjust_to_fit_screen), CONNECT_DEFERRED);
		} break;
		}
	}

	AutoAdjustPopupMenu::AutoAdjustPopupMenu() {
		
	}

	void AutoAdjustPopupMenu::_adjust_to_fit_screen() {
		if (!auto_adjust) return;
		if (!is_embedded() && DisplayServer::get_singleton()->has_feature(DisplayServer::FEATURE_SELF_FITTING_WINDOWS)) {
			// DisplayServer 会处理这些
			return;
		}
		if (!is_inside_tree()) return;
		SceneTree* tree = get_tree();
		if (!tree) return;
		Window* window = tree->get_root();
		if (!window) return;
		Viewport* viewport = window->get_viewport();
		if (!viewport) return;
		Rect2i parent_rect = viewport->get_visible_rect();
		Rect2i current = Rect2i(get_position(), get_size());
		if (current.position.x + current.size.x > parent_rect.position.x + parent_rect.size.x) {
			current.position.x = parent_rect.position.x + parent_rect.size.x - current.size.x;
		}

		if (current.position.x < parent_rect.position.x) {
			current.position.x = parent_rect.position.x;
		}

		if (current.position.y + current.size.y > parent_rect.position.y + parent_rect.size.y) {
			current.position.y = parent_rect.position.y + parent_rect.size.y - current.size.y;
		}

		if (current.position.y < parent_rect.position.y) {
			current.position.y = parent_rect.position.y;
		}

		if (current.size.y > parent_rect.size.y) {
			current.size.y = parent_rect.size.y;
		}

		if (current.size.x > parent_rect.size.x) {
			current.size.x = parent_rect.size.x;
		}
		// 如果未设置最大尺寸，则提前退出
		Size2i popup_max_size = get_max_size();
		if (popup_max_size <= Size2()) {
			set_position(current.position);
			set_size(current.size);
			return;
		}

		if (current.size.x > popup_max_size.x) {
			current.size.x = popup_max_size.x;
		}

		if (current.size.y > popup_max_size.y) {
			current.size.y = popup_max_size.y;
		}

		set_position(current.position);
		set_size(current.size);
	}

	void AutoAdjustPopupMenu::enable_auto_adjust(const bool enable) {
		auto_adjust = enable;
	}

	bool AutoAdjustPopupMenu::is_auto_adjust() const {
		return auto_adjust;
	}
}