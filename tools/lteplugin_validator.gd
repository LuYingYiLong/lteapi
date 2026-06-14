extends RefCounted

const MANIFEST_FILE_NAME: String = "manifest.json"
const PACKAGE_PCK_FILE_NAME: String = "plugin.pck"
const KNOWN_PERMISSIONS: PackedStringArray = [
	"editor_panels",
	"project_files",
	"external_files",
	"network",
	"process",
	"settings",
	"session",
	"workspace_layout",
	"menus",
	"export",
	"background_service",
]
const KNOWN_PLUGIN_TYPES: PackedStringArray = [
	"panel",
	"tool",
	"service",
	"theme",
	"library",
]
const REQUIRED_FIELDS: PackedStringArray = [
	"id",
	"name",
	"version",
	"api_version",
	"entry",
]

func validate_source_dir(source_dir: String) -> Dictionary:
	var result: Dictionary = {
		"ok": true,
		"source_dir": source_dir,
		"errors": [],
		"warnings": [],
		"manifest": {},
	}

	var normalized_dir: String = source_dir.replace("\\", "/").simplify_path()
	if !DirAccess.dir_exists_absolute(normalized_dir):
		result["ok"] = false
		result["errors"].append("Source directory does not exist: %s" % normalized_dir)
		return result

	var manifest_path: String = normalized_dir.path_join(MANIFEST_FILE_NAME)
	if !FileAccess.file_exists(manifest_path):
		result["ok"] = false
		result["errors"].append("Missing %s" % MANIFEST_FILE_NAME)
		return result

	var manifest_text: String = FileAccess.get_file_as_string(manifest_path)
	var manifest_data: Variant = JSON.parse_string(manifest_text)
	if !(manifest_data is Dictionary):
		result["ok"] = false
		result["errors"].append("manifest.json must be a valid JSON object")
		return result

	var manifest: Dictionary = manifest_data
	result["manifest"] = manifest

	for required_field: String in REQUIRED_FIELDS:
		if !manifest.has(required_field) or str(manifest[required_field]).strip_edges().is_empty():
			result["ok"] = false
			result["errors"].append("Missing required field: %s" % required_field)

	var plugin_id: String = str(manifest.get("id", "")).strip_edges()
	if !plugin_id.is_empty() and !_is_valid_plugin_id(plugin_id):
		result["ok"] = false
		result["errors"].append("Invalid plugin id: '%s'. Only lowercase letters, digits, hyphens, underscores, dots allowed." % plugin_id)

	var entry: String = str(manifest.get("entry", "")).strip_edges()
	if !entry.is_empty() and !entry.begins_with("res://plugins/"):
		result["ok"] = false
		result["errors"].append("entry must start with 'res://plugins/': '%s'" % entry)

	if !plugin_id.is_empty() and !entry.is_empty():
		var expected_prefix: String = "res://plugins/" + plugin_id + "/"
		if !entry.begins_with(expected_prefix):
			result["warnings"].append("entry '%s' does not start with expected prefix '%s'" % [entry, expected_prefix])

	if manifest.has("permissions"):
		var permissions = manifest["permissions"]
		if permissions is Array:
			for perm: String in permissions:
				var stripped: String = perm.strip_edges()
				if stripped.is_empty():
					continue
				if !KNOWN_PERMISSIONS.has(stripped):
					result["warnings"].append("Unknown permission: '%s'" % stripped)

	if manifest.has("plugin_type"):
		var plugin_type: String = str(manifest["plugin_type"]).strip_edges()
		if !plugin_type.is_empty() and !KNOWN_PLUGIN_TYPES.has(plugin_type):
			result["warnings"].append("Unknown plugin_type: '%s'. Known types: %s" % [plugin_type, ", ".join(KNOWN_PLUGIN_TYPES)])

	if manifest.has("version"):
		var version: String = str(manifest["version"]).strip_edges()
		if !_is_semver(version):
			result["warnings"].append("Version '%s' is not strict SemVer" % version)

	if manifest.has("dependencies"):
		var deps = manifest["dependencies"]
		if deps is Dictionary:
			for dep_id: String in deps.keys():
				if !_is_valid_plugin_id(dep_id):
					result["warnings"].append("Invalid dependency id: '%s'" % dep_id)
				var dep_version: String = str(deps[dep_id])
				if !_is_valid_version_constraint(dep_version):
					result["warnings"].append("Invalid version constraint for '%s': '%s'" % [dep_id, dep_version])

	if manifest.has("optional_dependencies"):
		var opt_deps = manifest["optional_dependencies"]
		if opt_deps is Dictionary:
			for dep_id: String in opt_deps.keys():
				if !_is_valid_plugin_id(dep_id):
					result["warnings"].append("Invalid optional dependency id: '%s'" % dep_id)
				var dep_version: String = str(opt_deps[dep_id])
				if !_is_valid_version_constraint(dep_version):
					result["warnings"].append("Invalid version constraint for optional '%s': '%s'" % [dep_id, dep_version])

	if result["ok"]:
		result["errors"].is_empty()

	return result

func validate_package(package_path: String) -> Dictionary:
	var result: Dictionary = {
		"ok": true,
		"package_path": package_path,
		"errors": [],
		"warnings": [],
		"manifest": {},
		"files": [],
	}

	var normalized_path: String = package_path.replace("\\", "/").simplify_path()
	if !FileAccess.file_exists(normalized_path):
		result["ok"] = false
		result["errors"].append("Package file does not exist: %s" % normalized_path)
		return result

	if !normalized_path.ends_with(".lteplugin"):
		result["warnings"].append("Package file does not have .lteplugin extension: %s" % normalized_path)

	var reader: ZIPReader = ZIPReader.new()
	var open_error: Error = reader.open(normalized_path)
	if open_error != OK:
		result["ok"] = false
		result["errors"].append("Failed to open package as ZIP: %s (error=%d)" % [normalized_path, open_error])
		return result

	var files: PackedStringArray = reader.get_files()
	result["files"] = []
	for file_path: String in files:
		result["files"].append(file_path)

	var has_manifest: bool = false
	var has_pck: bool = false
	for file_path: String in files:
		if file_path == MANIFEST_FILE_NAME:
			has_manifest = true
		if file_path == PACKAGE_PCK_FILE_NAME:
			has_pck = true

	if !has_manifest:
		result["ok"] = false
		result["errors"].append("Package is missing %s" % MANIFEST_FILE_NAME)
	else:
		var manifest_bytes: PackedByteArray = reader.read_file(MANIFEST_FILE_NAME)
		if manifest_bytes.is_empty():
			result["ok"] = false
			result["errors"].append("manifest.json is empty")
		else:
			var manifest_text: String = manifest_bytes.get_string_from_utf8()
			var manifest_data: Variant = JSON.parse_string(manifest_text)
			if !(manifest_data is Dictionary):
				result["ok"] = false
				result["errors"].append("manifest.json must be a valid JSON object")
			else:
				result["manifest"] = manifest_data
				for required_field: String in REQUIRED_FIELDS:
					if !manifest_data.has(required_field) or str(manifest_data[required_field]).strip_edges().is_empty():
						result["ok"] = false
						result["errors"].append("Missing required field in manifest: %s" % required_field)

	if !has_pck:
		result["ok"] = false
		result["errors"].append("Package is missing %s" % PACKAGE_PCK_FILE_NAME)

	reader.close()
	return result

func inspect_package(package_path: String) -> Dictionary:
	var result: Dictionary = {
		"package_path": package_path,
		"format": "lteplugin",
		"manifest": {},
		"file_count": 0,
		"total_size_bytes": 0,
		"files": [],
	}

	var normalized_path: String = package_path.replace("\\", "/").simplify_path()
	if !FileAccess.file_exists(normalized_path):
		result["error"] = "Package file does not exist: %s" % normalized_path
		return result

	result["file_size_bytes"] = FileAccess.open(normalized_path, FileAccess.READ).get_length() if _file_exists(normalized_path) else 0

	var reader: ZIPReader = ZIPReader.new()
	var open_error: Error = reader.open(normalized_path)
	if open_error != OK:
		result["error"] = "Failed to open package as ZIP: error=%d" % open_error
		return result

	var files: PackedStringArray = reader.get_files()
	result["file_count"] = files.size()

	for file_path: String in files:
		var file_bytes: PackedByteArray = reader.read_file(file_path)
		var file_info: Dictionary = {
			"path": file_path,
			"size_bytes": file_bytes.size(),
		}

		if file_path == MANIFEST_FILE_NAME:
			var manifest_text: String = file_bytes.get_string_from_utf8()
			var manifest_data: Variant = JSON.parse_string(manifest_text)
			if manifest_data is Dictionary:
				file_info["parsed"] = true
				result["manifest"] = manifest_data
			else:
				file_info["parsed"] = false
				file_info["parse_error"] = "Not a valid JSON object"

		if file_path == PACKAGE_PCK_FILE_NAME:
			file_info["type"] = "pck"

		result["files"].append(file_info)
		result["total_size_bytes"] = int(result["total_size_bytes"]) + file_bytes.size()

	reader.close()
	return result

func _is_valid_plugin_id(plugin_id: String) -> bool:
	if plugin_id.is_empty():
		return false
	for char: String in plugin_id:
		var code: int = char.unicode_at(0)
		var valid: bool = (code >= 48 and code <= 57) or (code >= 65 and code <= 90) or (code >= 97 and code <= 122) or code == 95 or code == 45 or code == 46
		if !valid:
			return false
	return true

func _is_semver(version: String) -> bool:
	var semver_regex: RegEx = RegEx.new()
	semver_regex.compile("^[0-9]+\\.[0-9]+\\.[0-9]+(-[0-9A-Za-z.-]+)?(\\+[0-9A-Za-z.-]+)?$")
	return semver_regex.search(version) != null

func _is_valid_version_constraint(constraint: String) -> bool:
	if constraint.is_empty():
		return false
	var cleaned: String = constraint.strip_edges()
	if cleaned.begins_with(">=") or cleaned.begins_with("<=") or cleaned.begins_with("^") or cleaned.begins_with("~"):
		cleaned = cleaned.substr(1).strip_edges()
	elif cleaned.begins_with(">") or cleaned.begins_with("<"):
		cleaned = cleaned.substr(1).strip_edges()
	if cleaned.begins_with("="):
		cleaned = cleaned.substr(1).strip_edges()
	return _is_semver(cleaned)

func _file_exists(path: String) -> bool:
	return FileAccess.file_exists(path)
