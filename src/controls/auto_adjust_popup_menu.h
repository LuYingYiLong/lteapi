#ifndef AUTO_ADJUST_POPUP_MENU_H
#define AUTO_ADJUST_POPUP_MENU_H

#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

namespace godot {
	class AutoAdjustPopupMenu : public PopupMenu {
		GDCLASS(AutoAdjustPopupMenu, PopupMenu)

	private:
		bool auto_adjust = true;

		void _adjust_to_fit_screen();

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		AutoAdjustPopupMenu();

		void enable_auto_adjust(const bool enable);
		bool is_auto_adjust() const;
	};
}

#endif // !AUTO_ADJUST_POPUP_MENU_H