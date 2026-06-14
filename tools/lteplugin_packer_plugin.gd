@tool
extends EditorPlugin

const TOOL_MENU_NAME: String = "Package LTE Plugin"
const PACK_BUILDER_SCRIPT: GDScript = preload("res://addons/lteapi/tools/lteplugin_pack_builder.gd")

var _dialog: AcceptDialog
var _source_edit: LineEdit
var _output_edit: LineEdit
var _status_label: Label
var _source_file_dialog: EditorFileDialog
var _output_file_dialog: EditorFileDialog

func _enter_tree() -> void:
	add_tool_menu_item(TOOL_MENU_NAME, _open_packer_dialog)

func _exit_tree() -> void:
	remove_tool_menu_item(TOOL_MENU_NAME)
	_free_dialogs()

func _open_packer_dialog() -> void:
	_ensure_dialog()
	_status_label.text = ""
	_dialog.popup_centered(Vector2i(640, 220))

func _ensure_dialog() -> void:
	if _dialog != null and is_instance_valid(_dialog):
		return

	_dialog = AcceptDialog.new()
	_dialog.title = "Package LTE Plugin"
	_dialog.ok_button_text = "Pack"
	_dialog.confirmed.connect(_on_pack_confirmed)

	var root: VBoxContainer = VBoxContainer.new()
	root.add_theme_constant_override("separation", 8)
	_dialog.add_child(root)

	var source_row: HBoxContainer = HBoxContainer.new()
	root.add_child(source_row)
	var source_label: Label = Label.new()
	source_label.text = "Source"
	source_label.custom_minimum_size.x = 72.0
	source_row.add_child(source_label)
	_source_edit = LineEdit.new()
	_source_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	source_row.add_child(_source_edit)
	var browse_source_button: Button = Button.new()
	browse_source_button.text = "..."
	browse_source_button.pressed.connect(_on_browse_source_pressed)
	source_row.add_child(browse_source_button)

	var output_row: HBoxContainer = HBoxContainer.new()
	root.add_child(output_row)
	var output_label: Label = Label.new()
	output_label.text = "Output"
	output_label.custom_minimum_size.x = 72.0
	output_row.add_child(output_label)
	_output_edit = LineEdit.new()
	_output_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	output_row.add_child(_output_edit)
	var browse_output_button: Button = Button.new()
	browse_output_button.text = "..."
	browse_output_button.pressed.connect(_on_browse_output_pressed)
	output_row.add_child(browse_output_button)

	_status_label = Label.new()
	_status_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	root.add_child(_status_label)

	var editor_base_control: Control = get_editor_interface().get_base_control()
	editor_base_control.add_child(_dialog)
	_create_file_dialogs(editor_base_control)

func _create_file_dialogs(editor_base_control: Control) -> void:
	_source_file_dialog = EditorFileDialog.new()
	_source_file_dialog.file_mode = EditorFileDialog.FILE_MODE_OPEN_DIR
	_source_file_dialog.access = EditorFileDialog.ACCESS_FILESYSTEM
	_source_file_dialog.dir_selected.connect(_on_source_dir_selected)
	editor_base_control.add_child(_source_file_dialog)

	_output_file_dialog = EditorFileDialog.new()
	_output_file_dialog.file_mode = EditorFileDialog.FILE_MODE_SAVE_FILE
	_output_file_dialog.access = EditorFileDialog.ACCESS_FILESYSTEM
	_output_file_dialog.filters = PackedStringArray(["*.lteplugin ; Lightech Plugin Package"])
	_output_file_dialog.file_selected.connect(_on_output_file_selected)
	editor_base_control.add_child(_output_file_dialog)

func _on_browse_source_pressed() -> void:
	_ensure_dialog()
	_source_file_dialog.popup_file_dialog()

func _on_browse_output_pressed() -> void:
	_ensure_dialog()
	var source_dir: String = _source_edit.text.strip_edges()
	if !_output_edit.text.strip_edges().is_empty():
		_output_file_dialog.current_path = _output_edit.text.strip_edges()
	elif !source_dir.is_empty():
		_output_file_dialog.current_path = source_dir.get_base_dir().path_join(source_dir.get_file() + ".lteplugin")
	_output_file_dialog.popup_file_dialog()

func _on_source_dir_selected(path: String) -> void:
	_source_edit.text = path.replace("\\", "/").simplify_path()
	if _output_edit.text.strip_edges().is_empty():
		_output_edit.text = _source_edit.text.get_base_dir().path_join(_source_edit.text.get_file() + ".lteplugin")

func _on_output_file_selected(path: String) -> void:
	_output_edit.text = path.replace("\\", "/").simplify_path()

func _on_pack_confirmed() -> void:
	var source_dir: String = _source_edit.text.strip_edges()
	var output_path: String = _output_edit.text.strip_edges()
	if source_dir.is_empty() or output_path.is_empty():
		_status_label.text = "Source and output are required."
		_dialog.popup_centered(Vector2i(640, 220))
		return

	var builder: RefCounted = PACK_BUILDER_SCRIPT.new() as RefCounted
	var exit_code: int = int(builder.call("pack_plugin", source_dir, output_path))
	if exit_code == 0:
		_status_label.text = "Packed: %s" % output_path
	else:
		_status_label.text = "Package failed. See Output panel for details."
	_dialog.popup_centered(Vector2i(640, 220))

func _free_dialogs() -> void:
	if _dialog != null and is_instance_valid(_dialog):
		_dialog.queue_free()
	if _source_file_dialog != null and is_instance_valid(_source_file_dialog):
		_source_file_dialog.queue_free()
	if _output_file_dialog != null and is_instance_valid(_output_file_dialog):
		_output_file_dialog.queue_free()
	_dialog = null
	_source_file_dialog = null
	_output_file_dialog = null
