# LTEAPI

LTEAPI is the native GDExtension runtime used by Lightech and LTE SDK plugins.

This repository owns the C++ source, godot-cpp binding, SCons build setup, and release packaging for `addons/lteapi`.

## Build

```powershell
scons --directory=D:\GodotProjects\LTEAPI
```

The default Windows build writes runtime files to:

```text
addons/lteapi/bin/
```

## Release Artifact

Consumers such as Lightech and LTE-SDK should use the release artifact form of `addons/lteapi`, not this source tree directly.

A minimal runtime artifact contains:

```text
addons/lteapi/bin/lteapi.gdextension
addons/lteapi/bin/lteapi.windows.template_debug.x86_64.dll
addons/lteapi/bin/lteapi.windows.template_release.x86_64.dll
addons/lteapi/plugin.cfg
```

## Importing Built-in LTZ Packages

The editor importer recognizes `.ltz` files and stores a validated package as a `LightechZipResource`. The imported resource contains the archive bytes, so exported projects can read package contents without opening the source file from the filesystem.

```gdscript
var package_resource: LightechZipResource = preload("res://data/builtin_levels.ltz")
var package: LightechZipReader = package_resource.open_reader()
var project_root: String = package.get_project_roots()[0]
var project_path: String = package_resource.get_project_path(project_root)
var chart_path: String = LightechZipReader.resolve_asset(project_path, "charts/master.json")
var chart_bytes: PackedByteArray = package.read_resource(chart_path)
```

## Repository Boundary

- `LTEAPI` maintains native source and compiled GDExtension artifacts.
- `Lightech` consumes compiled artifacts only.
- `LTE-SDK` embeds compiled artifacts for plugin template projects.
