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

## Repository Boundary

- `LTEAPI` maintains native source and compiled GDExtension artifacts.
- `Lightech` consumes compiled artifacts only.
- `LTE-SDK` embeds compiled artifacts for plugin template projects.
