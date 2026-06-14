extends RefCounted

const MANIFEST_FILE_NAME: String = "manifest.json"
const PACKAGE_PCK_FILE_NAME: String = "plugin.pck"
const SOURCE_BASE_PREFIX: String = "res://plugins/"

func pack_plugin(source_dir: String, output_path: String) -> int:
	var normalized_source_dir: String = source_dir.replace("\\", "/").simplify_path()
	var normalized_output_path: String = output_path.replace("\\", "/").simplify_path()
	if !DirAccess.dir_exists_absolute(normalized_source_dir):
		push_error("Plugin source directory does not exist: %s" % normalized_source_dir)
		return 1

	var manifest_path: String = normalized_source_dir.path_join(MANIFEST_FILE_NAME)
	if !FileAccess.file_exists(manifest_path):
		push_error("Plugin source directory must contain manifest.json: %s" % manifest_path)
		return 1

	var manifest_text: String = FileAccess.get_file_as_string(manifest_path)
	var manifest_data: Variant = JSON.parse_string(manifest_text)
	if !(manifest_data is Dictionary):
		push_error("manifest.json must be a JSON object.")
		return 1

	var manifest: Dictionary = manifest_data
	var plugin_id: String = str(manifest.get("id", "")).strip_edges()
	var entry_path: String = str(manifest.get("entry", "")).strip_edges()
	if plugin_id.is_empty() or entry_path.is_empty():
		push_error("manifest.json must include id and entry.")
		return 1

	var plugin_base_path: String = SOURCE_BASE_PREFIX + plugin_id + "/"
	if !entry_path.begins_with(plugin_base_path):
		push_error("Plugin entry must be inside %s" % plugin_base_path)
		return 1

	var output_dir: String = normalized_output_path.get_base_dir()
	if !output_dir.is_empty():
		var make_dir_error: Error = DirAccess.make_dir_recursive_absolute(output_dir)
		if make_dir_error != OK:
			push_error("Failed to create output directory: %s (%s)" % [output_dir, error_string(make_dir_error)])
			return 1

	var temp_pck_path: String = normalized_output_path + ".pck.tmp"
	var pck_error: Error = _write_plugin_pck(normalized_source_dir, plugin_base_path, temp_pck_path)
	if pck_error != OK:
		_remove_file_if_exists(temp_pck_path)
		return 1

	var zip_error: Error = _write_lteplugin_archive(manifest_path, temp_pck_path, normalized_output_path)
	_remove_file_if_exists(temp_pck_path)
	if zip_error != OK:
		return 1

	print("Packed plugin: %s" % normalized_output_path)
	return 0

func _write_plugin_pck(source_dir: String, plugin_base_path: String, temp_pck_path: String) -> Error:
	var source_files: PackedStringArray = PackedStringArray()
	_collect_source_files(source_dir, "", source_files)
	if source_files.is_empty():
		push_error("Plugin source directory has no files to pack.")
		return ERR_FILE_NOT_FOUND

	var packer: PCKPacker = PCKPacker.new()
	var start_error: Error = packer.pck_start(temp_pck_path, 32)
	if start_error != OK:
		push_error("Failed to start plugin PCK: %s (%s)" % [temp_pck_path, error_string(start_error)])
		return start_error

	for relative_path: String in source_files:
		var source_path: String = source_dir.path_join(relative_path)
		var pck_path: String = plugin_base_path + relative_path
		var add_error: Error = packer.add_file(pck_path, source_path)
		if add_error != OK:
			push_error("Failed to add plugin file: %s (%s)" % [relative_path, error_string(add_error)])
			return add_error

	var flush_error: Error = packer.flush()
	if flush_error != OK:
		push_error("Failed to flush plugin PCK: %s" % error_string(flush_error))
	return flush_error

func _collect_source_files(root_dir: String, relative_dir: String, files: PackedStringArray) -> void:
	var scan_dir: String = root_dir
	if !relative_dir.is_empty():
		scan_dir = root_dir.path_join(relative_dir)

	var dir: DirAccess = DirAccess.open(scan_dir)
	if dir == null:
		return

	dir.list_dir_begin()
	while true:
		var file_name: String = dir.get_next()
		if file_name.is_empty():
			break
		if file_name == "." or file_name == "..":
			continue

		var relative_path: String = file_name
		if !relative_dir.is_empty():
			relative_path = relative_dir.path_join(file_name)

		if dir.current_is_dir():
			if _should_skip_directory(file_name):
				continue
			_collect_source_files(root_dir, relative_path, files)
		else:
			if _should_skip_file(file_name):
				continue
			files.append(relative_path)
	dir.list_dir_end()

func _should_skip_directory(directory_name: String) -> bool:
	return directory_name == ".git" or directory_name == ".godot" or directory_name == ".editor"

func _should_skip_file(file_name: String) -> bool:
	return file_name.ends_with(".lteplugin") or file_name.ends_with(".pck.tmp")

func _write_lteplugin_archive(manifest_path: String, pck_path: String, output_path: String) -> Error:
	var zip_packer: ZIPPacker = ZIPPacker.new()
	var open_error: Error = zip_packer.open(output_path)
	if open_error != OK:
		push_error("Failed to open output package: %s (%s)" % [output_path, error_string(open_error)])
		return open_error

	var manifest_bytes: PackedByteArray = FileAccess.get_file_as_bytes(manifest_path)
	var manifest_error: Error = _write_zip_file(zip_packer, MANIFEST_FILE_NAME, manifest_bytes)
	if manifest_error != OK:
		zip_packer.close()
		return manifest_error

	var pck_bytes: PackedByteArray = FileAccess.get_file_as_bytes(pck_path)
	var pck_error: Error = _write_zip_file(zip_packer, PACKAGE_PCK_FILE_NAME, pck_bytes)
	if pck_error != OK:
		zip_packer.close()
		return pck_error

	return zip_packer.close()

func _write_zip_file(zip_packer: ZIPPacker, archive_path: String, bytes: PackedByteArray) -> Error:
	var start_error: Error = zip_packer.start_file(archive_path)
	if start_error != OK:
		push_error("Failed to start zip file entry: %s (%s)" % [archive_path, error_string(start_error)])
		return start_error
	zip_packer.write_file(bytes)
	var close_error: Error = zip_packer.close_file()
	if close_error != OK:
		push_error("Failed to close zip file entry: %s (%s)" % [archive_path, error_string(close_error)])
	return close_error

func _remove_file_if_exists(path: String) -> void:
	if FileAccess.file_exists(path):
		DirAccess.remove_absolute(path)
