extends SceneTree

const PACK_BUILDER_SCRIPT: GDScript = preload("res://addons/lteapi/tools/lteplugin_pack_builder.gd")
const VALIDATOR_SCRIPT: GDScript = preload("res://addons/lteapi/tools/lteplugin_validator.gd")

func _initialize() -> void:
	var args: PackedStringArray = OS.get_cmdline_user_args()
	if args.is_empty():
		_print_usage()
		_quit_with_code.call_deferred(1)
		return

	var command: String = args[0].to_lower()
	match command:
		"pack":
			_cmd_pack(args)
		"validate":
			_cmd_validate(args)
		"inspect":
			_cmd_inspect(args)
		"help", "--help", "-h":
			_print_usage()
			_quit_with_code.call_deferred(0)
		_:
			print("Unknown command: %s" % command)
			_print_usage()
			_quit_with_code.call_deferred(1)

func _cmd_pack(args: PackedStringArray) -> void:
	var source_dir: String = _get_option_value(args, "--source")
	var output_path: String = _get_option_value(args, "--output")
	if source_dir.is_empty() or output_path.is_empty():
		print("Error: --source and --output are required for 'pack' command.")
		print("Usage: ... -- pack --source <plugin_source_dir> --output <plugin.lteplugin>")
		_quit_with_code.call_deferred(1)
		return

	var builder: RefCounted = PACK_BUILDER_SCRIPT.new() as RefCounted
	var exit_code: int = int(builder.call("pack_plugin", source_dir, output_path))
	_quit_with_code.call_deferred(exit_code)

func _cmd_validate(args: PackedStringArray) -> void:
	var validator: RefCounted = VALIDATOR_SCRIPT.new() as RefCounted

	var source_dir: String = _get_option_value(args, "--source")
	var package_path: String = _get_option_value(args, "--package")

	if source_dir.is_empty() and package_path.is_empty():
		# 如果只提供了一个位置参数，尝试判断
		var target: String = _get_positional_target(args)
		if target.is_empty():
			print("Error: --source or --package is required for 'validate' command.")
			print("Usage: ... -- validate --source <plugin_source_dir>")
			print("Usage: ... -- validate --package <plugin.lteplugin>")
			_quit_with_code.call_deferred(1)
			return
		if target.ends_with(".lteplugin"):
			package_path = target
		else:
			source_dir = target

	var result: Dictionary
	if !package_path.is_empty():
		result = validator.call("validate_package", package_path)
	else:
		result = validator.call("validate_source_dir", source_dir)

	_print_validation_result(result)
	var exit_code: int = 0 if result.get("ok", false) else 1
	_quit_with_code.call_deferred(exit_code)

func _cmd_inspect(args: PackedStringArray) -> void:
	var package_path: String = _get_option_value(args, "--package")
	if package_path.is_empty():
		package_path = _get_positional_target(args)

	if package_path.is_empty():
		print("Error: --package is required for 'inspect' command.")
		print("Usage: ... -- inspect --package <plugin.lteplugin>")
		_quit_with_code.call_deferred(1)
		return

	var validator: RefCounted = VALIDATOR_SCRIPT.new() as RefCounted
	var result: Dictionary = validator.call("inspect_package", package_path)
	_print_inspect_result(result)
	_quit_with_code.call_deferred(0)

func _get_option_value(args: PackedStringArray, option_name: String) -> String:
	for index: int in range(args.size()):
		if args[index] == option_name and index + 1 < args.size():
			return args[index + 1].replace("\\", "/").simplify_path()
	return ""

func _get_positional_target(args: PackedStringArray) -> String:
	for arg: String in args:
		if arg.begins_with("--") or arg.begins_with("-"):
			continue
		if arg == "validate" or arg == "inspect" or arg == "pack":
			continue
		return arg.replace("\\", "/").simplify_path()
	return ""

func _quit_with_code(exit_code: int) -> void:
	quit(exit_code)

func _print_usage() -> void:
	print("Lightech Plugin Packer — CLI")
	print("")
	print("Commands:")
	print("  pack      --source <dir> --output <plugin.lteplugin>")
	print("  validate  --source <dir> | --package <plugin.lteplugin>")
	print("  inspect   --package <plugin.lteplugin>")
	print("  help")
	print("")
	print("Examples:")
	print("  godot --headless --path <project> --script res://addons/lteapi/tools/lteplugin_packer_cli.gd -- pack --source res://plugins/my-plugin --output my-plugin.lteplugin")
	print("  godot --headless --path <project> --script res://addons/lteapi/tools/lteplugin_packer_cli.gd -- validate --source res://plugins/my-plugin")
	print("  godot --headless --path <project> --script res://addons/lteapi/tools/lteplugin_packer_cli.gd -- validate --package my-plugin.lteplugin")
	print("  godot --headless --path <project> --script res://addons/lteapi/tools/lteplugin_packer_cli.gd -- inspect --package my-plugin.lteplugin")

func _print_validation_result(result: Dictionary) -> void:
	print("=== Validation Result ===")
	print("Target: %s" % (result.get("package_path", result.get("source_dir", "unknown"))))
	print("Status: %s" % ("PASSED" if result.get("ok", false) else "FAILED"))

	var errors: Array = result.get("errors", [])
	if !errors.is_empty():
		print("")
		print("Errors (%d):" % errors.size())
		for error: String in errors:
			print("  ✗ %s" % error)

	var warnings: Array = result.get("warnings", [])
	if !warnings.is_empty():
		print("")
		print("Warnings (%d):" % warnings.size())
		for warning: String in warnings:
			print("  ⚠ %s" % warning)

	var manifest: Dictionary = result.get("manifest", {})
	if !manifest.is_empty():
		print("")
		print("Manifest:")
		print("  id: %s" % manifest.get("id", "--"))
		print("  name: %s" % manifest.get("name", "--"))
		print("  version: %s" % manifest.get("version", "--"))
		print("  api_version: %s" % manifest.get("api_version", "--"))
		print("  entry: %s" % manifest.get("entry", "--"))
		if manifest.has("plugin_type"):
			print("  plugin_type: %s" % manifest["plugin_type"])
		var permissions = manifest.get("permissions", [])
		if permissions is Array and !permissions.is_empty():
			print("  permissions: %s" % ", ".join(permissions))
		var deps = manifest.get("dependencies", {})
		if deps is Dictionary and !deps.is_empty():
			print("  dependencies: %s" % var_to_str(deps))
		var opt_deps = manifest.get("optional_dependencies", {})
		if opt_deps is Dictionary and !opt_deps.is_empty():
			print("  optional_dependencies: %s" % var_to_str(opt_deps))

func _print_inspect_result(result: Dictionary) -> void:
	print("=== Package Inspection ===")
	print("Package: %s" % result.get("package_path", "unknown"))
	print("Format: %s" % result.get("format", "unknown"))

	if result.has("error"):
		print("Error: %s" % result["error"])
		return

	var file_size: int = int(result.get("file_size_bytes", 0))
	print("File size: %d bytes (%.1f KB)" % [file_size, float(file_size) / 1024.0])
	print("Entry count: %d" % int(result.get("file_count", 0)))
	print("Total content size: %d bytes" % int(result.get("total_size_bytes", 0)))

	var manifest: Dictionary = result.get("manifest", {})
	if !manifest.is_empty():
		print("")
		print("Manifest:")
		print("  id: %s" % manifest.get("id", "--"))
		print("  name: %s" % manifest.get("name", "--"))
		print("  version: %s" % manifest.get("version", "--"))
		print("  api_version: %s" % manifest.get("api_version", "--"))
		print("  entry: %s" % manifest.get("entry", "--"))
		if manifest.has("author"):
			print("  author: %s" % manifest["author"])
		if manifest.has("description"):
			print("  description: %s" % manifest["description"])
		if manifest.has("plugin_type"):
			print("  plugin_type: %s" % manifest["plugin_type"])
		if manifest.has("dependencies"):
			print("  dependencies: %s" % var_to_str(manifest["dependencies"]))
		if manifest.has("optional_dependencies"):
			print("  optional_dependencies: %s" % var_to_str(manifest["optional_dependencies"]))
		var permissions = manifest.get("permissions", [])
		if permissions is Array and !permissions.is_empty():
			print("  permissions: %s" % ", ".join(permissions))
		var tags = manifest.get("tags", [])
		if tags is Array and !tags.is_empty():
			print("  tags: %s" % ", ".join(tags))

	print("")
	print("Files:")
	var files: Array = result.get("files", [])
	for file_info: Dictionary in files:
		var path: String = file_info.get("path", "--")
		var size: int = int(file_info.get("size_bytes", 0))
		print("  %s (%d bytes)" % [path, size])
