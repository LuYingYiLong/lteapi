# LTEAPI Server Boundaries

`LTEAPI` is the extension entry point and editor bridge. Runtime/editor systems should call the concrete singleton that owns the behavior instead of routing through `LTEAPI`.

## LTEAPI

Owns only extension lifetime and editor-facing bridge helpers.

- Registers and owns singleton instances.
- Stores the editor root instance.
- Exposes editor shell helpers: workspace root, focused panel root, basic workspace files, hotkey overlay calls.
- Does not forward project, file system, timeline, source monitor, properties, workspace, plugin, or preference APIs.
- Does not relay server signals.

## LTESettingsConfig

Owns per-project editor settings persistence.

- Loads and saves `.editor/project_settings.cfg`.
- Stores editor settings used by workspace, timeline, source monitor, audio preview, and file system panels.
- Emits settings load/update signals.
- Does not open projects, scan files, or mutate chart data directly.

## LTEProjectManager

Owns project list and currently opened project state.

- Opens projects and prepares project folders.
- Creates, imports, scans, renames, and removes projects.
- Emits project lifecycle signals.
- Coordinates project-open side effects by emitting signals, not by exposing those subsystems through `LTEAPI`.

## LTEFileSystemServer

Owns project file indexing and file selections.

- Scans the opened project directory.
- Caches file/chart/scene/shader lists and `info.json`.
- Stores file-system-panel selection state through `LTESettingsConfig`.
- Does not decide which charts are open or how editor panels are arranged.

## LTEWorkspaceManager

Owns editor layout and panel/window orchestration.

- Scans, opens, saves, copies, exports, renames, and removes workspace layouts.
- Adds, changes, closes, focuses, and duplicates editor panels.
- Creates editor/runtime windows.
- Uses `LTEAPI` only for editor shell access.
- Does not own project selection, chart editing, or source monitor playback state.

## LTETimelineServer

Owns chart editing session state.

- Opens chart files and tracks unsaved chart snapshots.
- Owns timeline config, playhead, columns, per-bar settings, markers, in/out points, undo, and redo.
- Emits chart lifecycle/playhead signals.
- Does not play audio/game previews or scan project files.

## LTESourceMonitorServer

Owns source monitor preview/runtime playback.

- Opens game preview scenes for a difficulty.
- Controls preview play, pause, scrub, hot reload, and external runtime windows.
- Owns source-monitor-specific config such as ratio, judgement line, auto play, debug info, screen, and window mode.
- Does not edit chart data or persist timeline playhead state.

## LTEPropertiesServer

Owns the editor property selection bus and property edit submissions.

- Stores one active selection with a context id, item array, and metadata.
- Emits generic selection, item-submit, patch-submit, and schema signals.
- Does not apply chart edits directly; the panel that owns the selection context decides how to consume submitted properties.

## LTEComponentRegistry

Owns editor component/panel registration.

- Registers and unregisters panel component metadata.
- Resolves panel UUIDs to scenes.
- Does not instantiate or arrange panels.

## LTEPreferencesManager

Owns global editor preferences.

- Registers preference schemas and handles.
- Loads, caches, updates, resets, and saves preference values.
- Does not store per-project workspace/timeline/source-monitor config.

## LTEPluginManager

Owns plugin discovery and enablement.

- Scans `.editor/plugins`.
- Loads plugin metadata and enables/disables plugin instances.
- Reacts to project-open signals from `LTEProjectManager`.
- Does not manage project files or editor layouts directly.
