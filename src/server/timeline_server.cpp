#include "timeline_server.h"

#include "file_save_server.h"
#include "preferences_manager.h"
#include "project_manager.h"
#include "undo_redo_server.h"

#include <chrono>

#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
    LTETimelineServer* LTETimelineServer::singleton = nullptr;

    namespace {
        const char TIMELINE_PANEL_PER_BAR_PRESETS_PREF_KEY[] = "core.timeline_panel_per_bar_presets";

        Array get_timeline_panel_per_bar_presets(const Array& fallback) {
            LTEPreferencesManager* preferences_manager = LTEPreferencesManager::get_singleton();
            if (preferences_manager == nullptr || !preferences_manager->has_key(TIMELINE_PANEL_PER_BAR_PRESETS_PREF_KEY)) {
                return LTESettingsConfig::normalize_timeline_panel_per_bars(fallback);
            }
            Ref<LTEPreferenceConfigHandle> handle = preferences_manager->get_pref_config_handle(TIMELINE_PANEL_PER_BAR_PRESETS_PREF_KEY);
            if (handle.is_null() || !handle->is_valid()) {
                return LTESettingsConfig::normalize_timeline_panel_per_bars(fallback);
            }
            Variant value = handle->get_value();
            if (value.get_type() != Variant::ARRAY) {
                return LTESettingsConfig::normalize_timeline_panel_per_bars(fallback);
            }
            return LTESettingsConfig::normalize_timeline_panel_per_bars(value);
        }
    }

	void LTETimelineServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("_apply_chart_change", "opened_chart", "target_chart"), &LTETimelineServer::_apply_chart_change);
        ClassDB::bind_method(D_METHOD("_on_file_saved", "path", "tag", "revision"), &LTETimelineServer::_on_file_saved);
        ClassDB::bind_method(D_METHOD("_on_file_save_failed", "path", "tag", "revision", "error"), &LTETimelineServer::_on_file_save_failed);
        ClassDB::bind_method(D_METHOD("_flush_async_chart_save_results"), &LTETimelineServer::_flush_async_chart_save_results);
        ClassDB::bind_method(D_METHOD("_sort_in_out_compare", "a", "b"), &LTETimelineServer::_sort_in_out_compare);
        ClassDB::bind_method(D_METHOD("_sort_markers_compare", "a", "b"), &LTETimelineServer::_sort_markers_compare);
        ClassDB::bind_method(D_METHOD("fetch_timeline_config", "uuid", "chart_path"), &LTETimelineServer::fetch_timeline_config);
        ClassDB::bind_method(D_METHOD("open_chart", "uuid", "chart_path"), &LTETimelineServer::open_chart);
        ClassDB::bind_method(D_METHOD("get_opened_chart_list"), &LTETimelineServer::get_opened_chart_list);
        ClassDB::bind_method(D_METHOD("get_chart_helper", "uuid", "chart_path"), &LTETimelineServer::get_chart_helper);
        ClassDB::bind_method(D_METHOD("enable_playhead_auto_scroll", "uuid", "enable"), &LTETimelineServer::enable_playhead_auto_scroll);
        ClassDB::bind_method(D_METHOD("enable_show_audio_waveform", "uuid", "enable"), &LTETimelineServer::enable_show_audio_waveform);
        ClassDB::bind_method(D_METHOD("enable_smooth_audio_waveform", "uuid", "enable"), &LTETimelineServer::enable_smooth_audio_waveform);
        ClassDB::bind_method(D_METHOD("enable_highlight_4k_tracks", "uuid", "enable"), &LTETimelineServer::enable_highlight_4k_tracks);
        ClassDB::bind_method(D_METHOD("set_chart_spacing", "uuid", "spacing"), &LTETimelineServer::set_chart_spacing);
        ClassDB::bind_method(D_METHOD("set_chart_scroll_position", "uuid", "chart_path", "position"), &LTETimelineServer::set_chart_scroll_position);
        ClassDB::bind_method(D_METHOD("get_last_opened_chart", "uuid"), &LTETimelineServer::get_last_opened_chart);
        ClassDB::bind_method(D_METHOD("enable_chart_snap_mode", "uuid", "enable"), &LTETimelineServer::enable_chart_snap_mode);
        ClassDB::bind_method(D_METHOD("enable_chart_coverage_mode", "uuid", "enable"), &LTETimelineServer::enable_chart_coverage_mode);
        ClassDB::bind_method(D_METHOD("set_per_bars", "new_per_bars"), &LTETimelineServer::set_per_bars);
        ClassDB::bind_method(D_METHOD("set_chart_column_config", "uuid", "chart_path", "new_columns"), &LTETimelineServer::set_chart_column_config);
        ClassDB::bind_method(D_METHOD("get_note_group_colors", "chart_path"), &LTETimelineServer::get_note_group_colors);
        ClassDB::bind_method(D_METHOD("set_note_group_color", "chart_path", "group_name", "color", "default_color"), &LTETimelineServer::set_note_group_color);
        ClassDB::bind_method(D_METHOD("add_chart_marker", "chart_path", "name", "time", "annotation", "color"), &LTETimelineServer::add_chart_marker);
        ClassDB::bind_method(D_METHOD("update_chart_marker", "chart_path", "index", "name", "time", "annotation", "color"), &LTETimelineServer::update_chart_marker);
        ClassDB::bind_method(D_METHOD("remove_chart_marker", "chart_path", "index"), &LTETimelineServer::remove_chart_marker);
        ClassDB::bind_method(D_METHOD("add_chart_in_point", "chart_path", "current_time", "shift_pressed"), &LTETimelineServer::add_chart_in_point, DEFVAL(false));
        ClassDB::bind_method(D_METHOD("add_chart_out_point", "chart_path", "current_time"), &LTETimelineServer::add_chart_out_point);
        ClassDB::bind_method(D_METHOD("go_to_chart_in_point", "chart_path", "current_time", "preview"), &LTETimelineServer::go_to_chart_in_point, DEFVAL(true));
        ClassDB::bind_method(D_METHOD("go_to_chart_out_point", "chart_path", "current_time", "preview"), &LTETimelineServer::go_to_chart_out_point, DEFVAL(true));
        ClassDB::bind_method(D_METHOD("remove_chart_in_out_point", "chart_path", "index"), &LTETimelineServer::remove_chart_in_out_point);
        ClassDB::bind_method(D_METHOD("set_chart_per_bar", "uuid", "chart_path", "per_bar"), &LTETimelineServer::set_chart_per_bar);
        ClassDB::bind_method(D_METHOD("set_playhead", "uuid", "chart_path", "beat"), &LTETimelineServer::set_playhead);
        ClassDB::bind_method(D_METHOD("get_chart_audio_abs_path", "chart_path"), &LTETimelineServer::get_chart_audio_abs_path);
        ClassDB::bind_method(D_METHOD("get_time_from_playhead", "uuid", "chart_path"), &LTETimelineServer::get_time_from_playhead);
        ClassDB::bind_method(D_METHOD("get_chart_duration", "uuid", "chart_path"), &LTETimelineServer::get_chart_duration);
        ClassDB::bind_method(D_METHOD("submit_chart_changes", "uuid", "chart_path", "new_chart"), &LTETimelineServer::submit_chart_changes);
        ClassDB::bind_method(D_METHOD("save_chart", "uuid", "chart_path"), &LTETimelineServer::save_chart);
        ClassDB::bind_method(D_METHOD("flush_pending_chart_saves"), &LTETimelineServer::flush_pending_chart_saves);
        ClassDB::bind_method(D_METHOD("find_opened_chart_info", "uuid", "chart_path"), &LTETimelineServer::find_opened_chart_info);
        ClassDB::bind_method(D_METHOD("time_step_back", "chart_path", "time"), &LTETimelineServer::time_step_back);
        ClassDB::bind_method(D_METHOD("time_step_forward", "chart_path", "time"), &LTETimelineServer::time_step_forward);
        ClassDB::bind_method(D_METHOD("step_back", "uuid", "chart_path", "current_row", "per_bar"), &LTETimelineServer::step_back);
        ClassDB::bind_method(D_METHOD("step_forward", "uuid", "chart_path", "current_row", "per_bar"), &LTETimelineServer::step_forward);
        ADD_SIGNAL(MethodInfo("chart_opened",
            PropertyInfo(Variant::STRING, "uuid"),
            PropertyInfo(Variant::DICTIONARY, "chart_info")));
        ADD_SIGNAL(MethodInfo("chart_saved",
            PropertyInfo(Variant::STRING, "path")));
        ADD_SIGNAL(MethodInfo("chart_save_failed",
            PropertyInfo(Variant::STRING, "path"),
            PropertyInfo(Variant::INT, "error")));
        ADD_SIGNAL(MethodInfo("chart_changes_submitted",
            PropertyInfo(Variant::STRING, "uuid"),
            PropertyInfo(Variant::STRING, "chart_path")));
        ADD_SIGNAL(MethodInfo("chart_history_changed",
            PropertyInfo(Variant::STRING, "uuid"),
            PropertyInfo(Variant::STRING, "chart_path"),
            PropertyInfo(Variant::DICTIONARY, "chart_info")));
        ADD_SIGNAL(MethodInfo("chart_playhead_changed",
            PropertyInfo(Variant::STRING, "chart_path"),
            PropertyInfo(Variant::FLOAT, "time")));
    }

    Dictionary LTETimelineServer::_record_chart_data(const String& uuid, const String& chart_path) {
        Dictionary chart = Utils::load_json_file(chart_path);
        if (chart.is_empty()) return Dictionary();

        Dictionary opened_chart;
        opened_chart["uuid"] = uuid;
        opened_chart["path"] = chart_path;
        LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
        opened_chart["project_path"] = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
        opened_chart["saved"] = true;
        opened_chart["save_revision"] = 0;
        opened_chart["chart"] = chart;

        Dictionary col_cfg_dict = settings_config->timeline_panel_chart_column_configs.get(uuid, Dictionary());
        opened_chart["column_config"] = col_cfg_dict.get(chart_path, Array());

        Ref<ChartHelper> helper = ChartHelper::open(chart_path);
        settings_config->timeline_panel_last_opened[uuid] = chart_path;

        Dictionary per_bar_dict = settings_config->timeline_panel_chart_per_bar.get(uuid, Dictionary());
        if (!per_bar_dict.has(chart_path)) {
            per_bar_dict[chart_path] = 4;
            helper->set_per_bar(4);
            settings_config->timeline_panel_chart_per_bar[uuid] = per_bar_dict;
        }
        else {
            helper->set_per_bar(per_bar_dict.get(chart_path, 4));
        }

        Dictionary playhead_dict = settings_config->timeline_panel_chart_playhead.get(uuid, Dictionary());
        if (!playhead_dict.has(chart_path)) {
            playhead_dict[chart_path] = 0.0;
            settings_config->timeline_panel_chart_playhead[uuid] = playhead_dict;
        }

        opened_chart["chart_helper"] = helper;
        opened_charts.append(opened_chart);
        return opened_chart;
    }

    void LTETimelineServer::_apply_chart_change(Dictionary p_opened_chart, Dictionary p_target_chart) {
        // 注意：这里必须执行 duplicate(true)，否则 UndoRedo 历史栈里存的将是引用，
        // 后续修改会导致历史记录也跟着变，深拷贝能保证快照独立。
        String changed_chart_path = p_opened_chart.get("path", String());
        String target_chart_path = _get_absolute_chart_path(changed_chart_path);
        int64_t change_revision = ++chart_save_revision_counter;
        bool changed_any = false;
        for (int32_t index = 0; index < opened_charts.size(); index++) {
            Dictionary opened_chart = opened_charts[index];
            if (!_is_opened_chart_in_current_project(opened_chart)) {
                continue;
            }
            String opened_chart_path = opened_chart.get("path", String());
            if (opened_chart_path != changed_chart_path && _get_absolute_chart_path(opened_chart_path) != target_chart_path) {
                continue;
            }
            opened_chart["chart"] = p_target_chart.duplicate(true);
            opened_chart["saved"] = false;
            opened_chart["save_revision"] = change_revision;
            emit_signal("chart_history_changed", opened_chart.get("uuid", String()), opened_chart_path, opened_chart);
            changed_any = true;
        }
        if (!changed_any) {
            p_opened_chart["chart"] = p_target_chart.duplicate(true);
            p_opened_chart["saved"] = false;
            p_opened_chart["save_revision"] = change_revision;
            emit_signal("chart_history_changed", p_opened_chart.get("uuid", String()), changed_chart_path, p_opened_chart);
        }
        if (!submitting_chart_change && _is_chart_auto_save_enabled()) {
            _queue_chart_save(p_opened_chart, _get_auto_save_delay_msec());
        }
    }

    bool LTETimelineServer::_is_chart_auto_save_enabled() const {
        LTEPreferencesManager* preferences_manager = LTEPreferencesManager::get_singleton();
        return preferences_manager != nullptr && preferences_manager->get_bool_value("core.chart_auto_save", false);
    }

    int32_t LTETimelineServer::_get_auto_save_delay_msec() const {
        LTEPreferencesManager* preferences_manager = LTEPreferencesManager::get_singleton();
        double interval = preferences_manager != nullptr
            ? preferences_manager->get_float_value("core.auto_save_interval", DEFAULT_AUTO_SAVE_INTERVAL_SECONDS)
            : DEFAULT_AUTO_SAVE_INTERVAL_SECONDS;
        interval = UtilityFunctions::clampf(interval, 0.1, 60.0);
        return static_cast<int32_t>(interval * 1000.0);
    }

    void LTETimelineServer::_start_chart_save_worker() {
        if (chart_save_worker_running) {
            return;
        }
        chart_save_worker_stop_requested = false;
        chart_save_worker_running = true;
        chart_save_worker = std::thread(&LTETimelineServer::_chart_save_worker_loop, this);
    }

    void LTETimelineServer::_stop_chart_save_worker() {
        {
            std::lock_guard<std::mutex> lock(chart_save_mutex);
            chart_save_worker_stop_requested = true;
        }
        chart_save_condition.notify_all();
        if (chart_save_worker.joinable()) {
            chart_save_worker.join();
        }
        chart_save_worker_running = false;
    }

    void LTETimelineServer::_queue_chart_save(const Dictionary& opened_chart, const int32_t delay_msec) {
        AsyncChartSaveJob job;
        job.uuid = opened_chart.get("uuid", String());
        job.chart_path = opened_chart.get("path", String());
        job.absolute_path = _get_absolute_chart_path(job.chart_path);
        Dictionary chart = opened_chart.get("chart", Dictionary());
        job.chart = chart.duplicate(true);
        job.revision = static_cast<int64_t>(opened_chart.get("save_revision", 0));
        if (job.absolute_path.is_empty()) {
            return;
        }

        LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
        if (!file_save_server) {
            return;
        }
        file_save_server->queue_json_save(job.absolute_path, job.chart, "", _make_chart_save_tag(job.uuid, job.chart_path, job.revision), delay_msec, true);
    }

    String LTETimelineServer::_make_chart_save_tag(const String& uuid, const String& chart_path, const int64_t revision) const {
        return "chart\n" + uuid + "\n" + chart_path + "\n" + String::num_int64(revision);
    }

    bool LTETimelineServer::_parse_chart_save_tag(const String& tag, String& r_uuid, String& r_chart_path, int64_t& r_revision) const {
        PackedStringArray parts = tag.split("\n", false);
        if (parts.size() != 4 || parts[0] != "chart") {
            return false;
        }
        r_uuid = parts[1];
        r_chart_path = parts[2];
        r_revision = parts[3].to_int();
        return true;
    }

    void LTETimelineServer::_on_file_saved(const String& path, const String& tag, const int64_t revision) {
        String uuid;
        String chart_path;
        int64_t chart_revision = 0;
        if (!_parse_chart_save_tag(tag, uuid, chart_path, chart_revision)) {
            return;
        }
        String target_chart_path = _get_absolute_chart_path(chart_path);
        bool saved_any = false;
        for (int32_t index = 0; index < opened_charts.size(); index++) {
            Dictionary opened_chart = opened_charts[index];
            if (!_is_opened_chart_in_current_project(opened_chart)) {
                continue;
            }
            String opened_chart_path = opened_chart.get("path", String());
            if (opened_chart_path != chart_path && _get_absolute_chart_path(opened_chart_path) != target_chart_path) {
                continue;
            }
            int64_t current_revision = static_cast<int64_t>(opened_chart.get("save_revision", 0));
            if (current_revision != chart_revision) {
                continue;
            }
            opened_chart["saved"] = true;
            saved_any = true;
        }
        if (saved_any) {
            emit_signal("chart_saved", chart_path);
        }
    }

    void LTETimelineServer::_on_file_save_failed(const String& path, const String& tag, const int64_t revision, const int64_t error) {
        String uuid;
        String chart_path;
        int64_t chart_revision = 0;
        if (!_parse_chart_save_tag(tag, uuid, chart_path, chart_revision)) {
            return;
        }
        String target_chart_path = _get_absolute_chart_path(chart_path);
        for (int32_t index = 0; index < opened_charts.size(); index++) {
            Dictionary opened_chart = opened_charts[index];
            if (!_is_opened_chart_in_current_project(opened_chart)) {
                continue;
            }
            String opened_chart_path = opened_chart.get("path", String());
            if (opened_chart_path != chart_path && _get_absolute_chart_path(opened_chart_path) != target_chart_path) {
                continue;
            }
            int64_t current_revision = static_cast<int64_t>(opened_chart.get("save_revision", 0));
            if (current_revision == chart_revision) {
                opened_chart["saved"] = false;
            }
        }
        ERR_PRINT(vformat("Failed to save chart file: %s. Error Code: %d", path, error));
        emit_signal("chart_save_failed", chart_path, error);
    }

    void LTETimelineServer::_chart_save_worker_loop() {
        while (true) {
            AsyncChartSaveJob job;
            {
                std::unique_lock<std::mutex> lock(chart_save_mutex);
                chart_save_condition.wait(lock, [this]() {
                    return chart_save_worker_stop_requested || !pending_chart_saves.empty();
                });
                if (chart_save_worker_stop_requested && pending_chart_saves.empty()) {
                    break;
                }
                if (!chart_save_worker_stop_requested) {
                    chart_save_condition.wait_for(lock, std::chrono::milliseconds(CHART_SAVE_COALESCE_DELAY_MS), [this]() {
                        return chart_save_worker_stop_requested;
                    });
                }
                if (chart_save_worker_stop_requested && pending_chart_saves.empty()) {
                    break;
                }
                if (pending_chart_saves.empty()) {
                    continue;
                }
                job = pending_chart_saves.front();
                pending_chart_saves.erase(pending_chart_saves.begin());
                ++active_chart_save_count;
                active_chart_save_path = job.absolute_path;
                active_chart_save_revision = job.revision;
            }

            AsyncChartSaveResult result;
            result.uuid = job.uuid;
            result.chart_path = job.chart_path;
            result.absolute_path = job.absolute_path;
            result.revision = job.revision;
            result.error = _save_chart_snapshot_atomic(job.absolute_path, job.chart);

            bool should_defer_flush = false;
            {
                std::lock_guard<std::mutex> lock(chart_save_mutex);
                completed_chart_saves.push_back(result);
                --active_chart_save_count;
                active_chart_save_path = String();
                active_chart_save_revision = -1;
                should_defer_flush = !chart_save_worker_stop_requested;
            }
            if (should_defer_flush) {
                call_deferred("_flush_async_chart_save_results");
            }
        }
    }

    void LTETimelineServer::_flush_async_chart_save_results() {
        std::vector<AsyncChartSaveResult> results;
        {
            std::lock_guard<std::mutex> lock(chart_save_mutex);
            results.swap(completed_chart_saves);
        }
        for (const AsyncChartSaveResult& result : results) {
            if (result.error == OK) {
                String target_chart_path = _get_absolute_chart_path(result.chart_path);
                bool saved_any = false;
                for (int32_t index = 0; index < opened_charts.size(); index++) {
                    Dictionary opened_chart = opened_charts[index];
                    if (!_is_opened_chart_in_current_project(opened_chart)) {
                        continue;
                    }
                    String opened_chart_path = opened_chart.get("path", String());
                    if (opened_chart_path != result.chart_path && _get_absolute_chart_path(opened_chart_path) != target_chart_path) {
                        continue;
                    }
                    int64_t current_revision = static_cast<int64_t>(opened_chart.get("save_revision", 0));
                    if (current_revision != result.revision) {
                        continue;
                    }
                    opened_chart["saved"] = true;
                    saved_any = true;
                }
                if (saved_any) {
                    emit_signal("chart_saved", result.chart_path);
                }
            }
            else {
                String target_chart_path = _get_absolute_chart_path(result.chart_path);
                for (int32_t index = 0; index < opened_charts.size(); index++) {
                    Dictionary opened_chart = opened_charts[index];
                    if (!_is_opened_chart_in_current_project(opened_chart)) {
                        continue;
                    }
                    String opened_chart_path = opened_chart.get("path", String());
                    if (opened_chart_path != result.chart_path && _get_absolute_chart_path(opened_chart_path) != target_chart_path) {
                        continue;
                    }
                    int64_t current_revision = static_cast<int64_t>(opened_chart.get("save_revision", 0));
                    if (current_revision == result.revision) {
                        opened_chart["saved"] = false;
                    }
                }
                ERR_PRINT(vformat("Failed to save chart file: %s. Error Code: %d", result.absolute_path, result.error));
                emit_signal("chart_save_failed", result.chart_path, static_cast<int64_t>(result.error));
            }
        }
    }

    Error LTETimelineServer::_save_chart_snapshot_atomic(const String& path, const Dictionary& chart) const {
        if (path.is_empty()) {
            return ERR_INVALID_PARAMETER;
        }

        String dir_path = path.get_base_dir();
        Error error = DirAccess::make_dir_recursive_absolute(dir_path);
        if (error != OK) {
            return error;
        }

        String temp_path = path + String(".saving.tmp");
        String backup_path = path + String(".saving.bak");
        if (FileAccess::file_exists(temp_path)) {
            DirAccess::remove_absolute(temp_path);
        }
        if (FileAccess::file_exists(backup_path)) {
            DirAccess::remove_absolute(backup_path);
        }

        Ref<FileAccess> file = FileAccess::open(temp_path, FileAccess::WRITE);
        if (file.is_null()) {
            return FileAccess::get_open_error();
        }

        String json = JSON::stringify(chart, "");
        if (!file->store_string(json)) {
            file->close();
            DirAccess::remove_absolute(temp_path);
            return ERR_FILE_CANT_WRITE;
        }
        file->close();

        bool had_original = FileAccess::file_exists(path);
        if (had_original) {
            error = DirAccess::rename_absolute(path, backup_path);
            if (error != OK) {
                DirAccess::remove_absolute(temp_path);
                return error;
            }
        }

        error = DirAccess::rename_absolute(temp_path, path);
        if (error != OK) {
            if (had_original) {
                DirAccess::rename_absolute(backup_path, path);
            }
            DirAccess::remove_absolute(temp_path);
            return error;
        }

        if (had_original) {
            DirAccess::remove_absolute(backup_path);
        }
        return OK;
    }

    int LTETimelineServer::_find_single_in_point(const String& chart_path) {
        String point_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_in_out_points, chart_path);
        Array points = settings_config->timeline_panel_chart_in_out_points.get(point_key, Array());
        for (int i = 0; i < points.size(); ++i) {
            Array point = points[i];
            if (point.size() == 1) {
                return i;
            }
        }
        return -1;
    }

    String LTETimelineServer::_get_absolute_chart_path(const String& chart_path) const {
        String normalized_path = chart_path.replace("\\", "/");
        if (normalized_path.is_empty()) {
            return String();
        }
        LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
        String root_dir = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
        if (normalized_path.begins_with("/") && !root_dir.is_empty()) {
            return (root_dir + normalized_path).simplify_path();
        }
        if (Utils::is_absolute_path(normalized_path)) {
            return normalized_path.simplify_path();
        }
        if (root_dir.is_empty()) {
            return normalized_path.simplify_path();
        }
        return root_dir.path_join(normalized_path).simplify_path();
    }

    bool LTETimelineServer::_is_opened_chart_in_current_project(const Dictionary& opened_chart) const {
        LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
        String root_dir = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
        if (root_dir.is_empty()) {
            return false;
        }

        String opened_project_path = String(opened_chart.get("project_path", String())).replace("\\", "/").simplify_path();
        if (!opened_project_path.is_empty()) {
            return opened_project_path.to_lower() == root_dir.to_lower();
        }

        String opened_chart_path = _get_absolute_chart_path(opened_chart.get("path", String())).replace("\\", "/").simplify_path();
        if (opened_chart_path.is_empty()) {
            return false;
        }
        if (!root_dir.ends_with("/")) {
            root_dir += "/";
        }
        return opened_chart_path.to_lower().begins_with(root_dir.to_lower());
    }

    String LTETimelineServer::_resolve_chart_setting_key(const Dictionary& chart_dict, const String& chart_path) const {
        if (chart_dict.has(chart_path)) {
            return chart_path;
        }
        String target_chart_path = _get_absolute_chart_path(chart_path);
        if (target_chart_path.is_empty()) {
            return chart_path;
        }
        Array keys = chart_dict.keys();
        for (int32_t index = 0; index < keys.size(); ++index) {
            String key = keys[index];
            if (_get_absolute_chart_path(key) == target_chart_path) {
                return key;
            }
        }
        return target_chart_path;
    }

    // a[0] < b[0]
    bool LTETimelineServer::_sort_in_out_compare(const Variant& a, const Variant& b) const {
        Array aa = a;
        Array bb = b;
        if (aa.is_empty() || bb.is_empty()) return false;
        return aa[0] < bb[0];
    }

    // a.get("time") < b.get("time")
    bool LTETimelineServer::_sort_markers_compare(const Variant& a, const Variant& b) const {
        Dictionary da = a;
        Dictionary db = b;
        return da.get("time", 0.0) < db.get("time", 0.0);
    }

    LTETimelineServer::LTETimelineServer() {
        if (singleton == nullptr) {
            singleton = this;
        }
        settings_config = LTESettingsConfig::get_singleton();
        LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
        if (file_save_server) {
            file_save_server->connect("file_saved", Callable(this, "_on_file_saved"));
            file_save_server->connect("file_save_failed", Callable(this, "_on_file_save_failed"));
        }
    }

    LTETimelineServer::~LTETimelineServer() {
        flush_pending_chart_saves();
        LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
        if (file_save_server) {
            Callable saved_callable = Callable(this, "_on_file_saved");
            Callable failed_callable = Callable(this, "_on_file_save_failed");
            if (file_save_server->is_connected("file_saved", saved_callable)) {
                file_save_server->disconnect("file_saved", saved_callable);
            }
            if (file_save_server->is_connected("file_save_failed", failed_callable)) {
                file_save_server->disconnect("file_save_failed", failed_callable);
            }
        }
        if (singleton == this) {
            singleton = nullptr;
        }
        settings_config = nullptr;
    }

    LTETimelineServer* LTETimelineServer::get_singleton() {
        return singleton;
    }

    void LTETimelineServer::clear_project_state() {
        flush_pending_chart_saves();
        opened_charts.clear();
        submitting_chart_change = false;
    }

    Dictionary LTETimelineServer::fetch_timeline_config(const String& uuid, const String& chart_path) const {
        Dictionary config;
        Dictionary chart_list;
        config["playhead_auto_scroll"] = settings_config->timeline_panel_playhead_auto_scroll.get(uuid, true);
        config["show_audio_waveform"] = settings_config->timeline_panel_show_audio_waveform.get(uuid, true);
        config["smooth_audio_waveform"] = settings_config->timeline_panel_smooth_audio_waveform.get(uuid, true);
        config["highlight_4k_tracks"] = settings_config->timeline_panel_highlight_4k_tracks.get(uuid, true);
        config["spacing"] = settings_config->timeline_panel_spacing.get(uuid, 128.0);
        config["last_opened"] = settings_config->timeline_panel_last_opened.get(uuid, "");
        chart_list = settings_config->timeline_panel_chart_column_configs.get(uuid, Dictionary());
        config["chart_column_config"] = chart_list.get(chart_path, Array());
        config["snap_mode"] = settings_config->timeline_panel_snap_mode.get(uuid, true);
        config["coverage_mode"] = settings_config->timeline_panel_coverage_mode.get(uuid, false);
        config["per_bars"] = get_timeline_panel_per_bar_presets(settings_config->timeline_panel_per_bars);
        String marker_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_markers, chart_path);
        String in_out_point_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_in_out_points, chart_path);
        String note_group_color_key = _resolve_chart_setting_key(settings_config->timeline_panel_note_group_colors, chart_path);
        config["chart_markers"] = settings_config->timeline_panel_chart_markers.get(marker_key, Array());
        config["chart_in_out_points"] = settings_config->timeline_panel_chart_in_out_points.get(in_out_point_key, Array());
        config["note_group_colors"] = settings_config->timeline_panel_note_group_colors.get(note_group_color_key, Dictionary());
        chart_list = settings_config->timeline_panel_chart_per_bar.get(uuid, Dictionary());
        config["chart_per_bar"] = chart_list.get(chart_path, 4);
        chart_list = settings_config->timeline_panel_chart_playhead.get(uuid, Dictionary());
        config["chart_playhead"] = chart_list.get(chart_path, 0.0);
        chart_list = settings_config->timeline_panel_chart_scroll_position.get(uuid, Dictionary());
        config["chart_scroll_position"] = chart_list.get(chart_path, Vector2i(-1, -1));
        return config;
    }

    void LTETimelineServer::open_chart(const String& uuid, const String& chart_path) {
        Dictionary opened_chart = find_opened_chart_info(uuid, chart_path);
        if (opened_chart.is_empty()) {
            opened_chart = _record_chart_data(uuid, chart_path);
        }
        else {
            settings_config->timeline_panel_last_opened[uuid] = opened_chart.get("path", chart_path);
        }
        emit_signal("chart_opened", uuid, opened_chart);
        settings_config->save_settings_config(false);
    }

    Array LTETimelineServer::get_opened_chart_list() {
        Array current_project_charts;
        for (int index = 0; index < opened_charts.size(); ++index) {
            Dictionary opened_chart = opened_charts[index];
            if (_is_opened_chart_in_current_project(opened_chart)) {
                current_project_charts.append(opened_chart);
            }
        }
        return current_project_charts;
    }

    Ref<ChartHelper> LTETimelineServer::get_chart_helper(const String& uuid, const String& chart_path) {
        Dictionary opened_chart = find_opened_chart_info(uuid, chart_path);
        if (!opened_chart.is_empty()) {
            Variant val = opened_chart.get("chart_helper", Variant());
            Object* obj = val;
            // 安全转换，如果类型不对或 obj 为空，cast_to 会返回 nullptr
            ChartHelper* helper = Object::cast_to<ChartHelper>(obj);
            if (helper) {
                return Ref<ChartHelper>(helper);
            }
        }
        // 未找到则生成再次尝试获取
        opened_chart = _record_chart_data(uuid, chart_path);
        Variant val = opened_chart.get("chart_helper", Variant());
        Object* obj = val;
        return Object::cast_to<ChartHelper>(obj);
    }

    void LTETimelineServer::enable_playhead_auto_scroll(const String& uuid, const bool enable) {
        settings_config->timeline_panel_playhead_auto_scroll[uuid] = enable;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::enable_show_audio_waveform(const String& uuid, const bool enable) {
        settings_config->timeline_panel_show_audio_waveform[uuid] = enable;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::enable_smooth_audio_waveform(const String& uuid, const bool enable) {
        settings_config->timeline_panel_smooth_audio_waveform[uuid] = enable;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::enable_highlight_4k_tracks(const String& uuid, const bool enable) {
        settings_config->timeline_panel_highlight_4k_tracks[uuid] = enable;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::set_chart_spacing(const String& uuid, const double spacing) {
        settings_config->timeline_panel_spacing[uuid] = spacing;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::set_chart_scroll_position(const String& uuid, const String& chart_path, const Vector2i& position) {
        if (uuid.is_empty() || chart_path.is_empty()) {
            return;
        }
        Dictionary scroll_position_dict = settings_config->timeline_panel_chart_scroll_position.get(uuid, Dictionary());
        Vector2i current_position = scroll_position_dict.get(chart_path, Vector2i(-1, -1));
        if (current_position == position) {
            return;
        }
        scroll_position_dict[chart_path] = position;
        settings_config->timeline_panel_chart_scroll_position[uuid] = scroll_position_dict;
        settings_config->save_settings_config(false);
    }

    String LTETimelineServer::get_last_opened_chart(const String& uuid) const {
        Dictionary last = settings_config->timeline_panel_last_opened;
        return last.get(uuid, "");
    }

    void LTETimelineServer::enable_chart_snap_mode(const String& uuid, const bool enable) {
        settings_config->timeline_panel_snap_mode[uuid] = enable;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::enable_chart_coverage_mode(const String& uuid, const bool enable) {
        settings_config->timeline_panel_coverage_mode[uuid] = enable;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::set_per_bars(const Array& new_per_bars) {
        Array normalized_per_bars = LTESettingsConfig::normalize_timeline_panel_per_bars(new_per_bars);
        settings_config->timeline_panel_per_bars = normalized_per_bars;
        LTEPreferencesManager* preferences_manager = LTEPreferencesManager::get_singleton();
        if (preferences_manager != nullptr && preferences_manager->has_key(TIMELINE_PANEL_PER_BAR_PRESETS_PREF_KEY)) {
            Ref<LTEPreferenceConfigHandle> handle = preferences_manager->get_pref_config_handle(TIMELINE_PANEL_PER_BAR_PRESETS_PREF_KEY);
            if (handle.is_valid() && handle->is_valid()) {
                handle->set_value(normalized_per_bars);
            }
        }
        settings_config->save_settings_config();
    }

    void LTETimelineServer::set_chart_column_config(const String& uuid, const String& chart_path, const Array& new_columns) {
        Dictionary col_cfg = settings_config->timeline_panel_chart_column_configs.get(uuid, Dictionary());
        col_cfg[chart_path] = new_columns;
        settings_config->timeline_panel_chart_column_configs[uuid] = col_cfg;
        settings_config->save_settings_config(false);
    }

    Dictionary LTETimelineServer::get_note_group_colors(const String& chart_path) const {
        if (!settings_config || chart_path.is_empty()) {
            return Dictionary();
        }
        String chart_key = _resolve_chart_setting_key(settings_config->timeline_panel_note_group_colors, chart_path);
        Dictionary chart_colors = settings_config->timeline_panel_note_group_colors.get(chart_key, Dictionary());
        return chart_colors.duplicate(true);
    }

    void LTETimelineServer::set_note_group_color(const String& chart_path, const String& group_name, const Color& color, const Color& default_color) {
        if (!settings_config || chart_path.is_empty()) {
            return;
        }
        String group = group_name.strip_edges();
        if (group.is_empty()) {
            return;
        }
        String chart_key = _resolve_chart_setting_key(settings_config->timeline_panel_note_group_colors, chart_path);
        Dictionary chart_colors = settings_config->timeline_panel_note_group_colors.get(chart_key, Dictionary());
        if (color == default_color) {
            chart_colors.erase(group);
        }
        else {
            chart_colors[group] = color;
        }
        if (chart_colors.is_empty()) {
            settings_config->timeline_panel_note_group_colors.erase(chart_key);
        }
        else {
            settings_config->timeline_panel_note_group_colors[chart_key] = chart_colors;
        }
        settings_config->save_settings_config();
    }

    void LTETimelineServer::add_chart_marker(const String& chart_path, const String& name, const double time, const String& annotation, const Color& color) {
        Dictionary markers_dict = settings_config->timeline_panel_chart_markers;
        String marker_key = _resolve_chart_setting_key(markers_dict, chart_path);
        Array markers = markers_dict.get(marker_key, Array());
        Dictionary new_marker;
        new_marker["name"] = name;
        new_marker["time"] = time;
        new_marker["annotation"] = annotation;
        new_marker["color"] = color;
        markers.append(new_marker);
        markers.sort_custom(Callable(this, "_sort_markers_compare"));
        settings_config->timeline_panel_chart_markers.set(marker_key, markers); // 假设setter
        settings_config->save_settings_config();
    }

    void LTETimelineServer::update_chart_marker(const String& chart_path, const int32_t index, const String& name, const double time, const String& annotation, const Color& color) {
        String marker_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_markers, chart_path);
        Array marker_list = settings_config->timeline_panel_chart_markers.get(marker_key, Array());
        if (index < 0 || index >= marker_list.size()) return;
        Dictionary marker_dict = marker_list[index];
        marker_dict["name"] = name;
        marker_dict["time"] = time;
        marker_dict["annotation"] = annotation;
        marker_dict["color"] = color;
        marker_list.set(index, marker_dict);
        marker_list.sort_custom(Callable(this, "_sort_markers_compare"));
        settings_config->timeline_panel_chart_markers[marker_key] = marker_list;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::remove_chart_marker(const String& chart_path, const int64_t index) {
        String marker_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_markers, chart_path);
        Array marker_list = settings_config->timeline_panel_chart_markers.get(marker_key, Array());
        if (index >= 0 && index < marker_list.size()) marker_list.remove_at(index);
        settings_config->timeline_panel_chart_markers[marker_key] = marker_list;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::add_chart_in_point(const String& chart_path, double current_time, bool shift_pressed) {
        Dictionary all_points_dict = settings_config->timeline_panel_chart_in_out_points;
        String point_key = _resolve_chart_setting_key(all_points_dict, chart_path);
        Array in_out_points = all_points_dict.get(point_key, Array());
        String chart_abs_path = _get_absolute_chart_path(chart_path);
        Ref<ChartHelper> helper = ChartHelper::open(chart_abs_path);
        if (helper.is_null()) return;
        double duration = helper.is_valid() ? helper->get_duration() : 0.0;
        if (current_time < 0.0) current_time = 0.0;
        if (duration > 0.0 && current_time > duration) current_time = duration;

        double in_point;
        double out_point;

        if (in_out_points.is_empty()) {
            out_point = helper.is_valid() ? helper->get_duration() : 0.0;
            in_point = current_time;

            Array new_pair;
            new_pair.append(in_point);
            new_pair.append(out_point);
            in_out_points.append(new_pair);
        }
        else {
            int single_in_point_index = _find_single_in_point(point_key);

            for (int i = 0; i < in_out_points.size(); ++i) {
                // C++ 中 Array[i] 返回的是 Variant，需要转回 Array 才能操作下标
                Array in_out_point = in_out_points[i];

                // 逻辑条件 1
                if (in_out_point.size() == 2 && static_cast<double>(in_out_point[0]) <= current_time && static_cast<double>(in_out_point[1]) >= current_time) {
                    in_out_point[0] = current_time;
                    break;
                }
                // 逻辑条件 2 (Shift Pressed 极其复杂的逻辑)
                else if (shift_pressed) {
                    // 为了可读性，先提取前一个元素的条件
                    bool prev_is_pair = false;
                    double prev_out = 0.0;
                    if (i > 0) {
                        Array prev_arr = in_out_points[i - 1];
                        if (prev_arr.size() == 2) {
                            prev_is_pair = true;
                            prev_out = static_cast<double>(prev_arr[1]);
                        }
                    }

                    bool current_cond_1 = (i < in_out_points.size() - 1) &&
                        ((i > 0 && prev_is_pair) || i == 0) &&
                        ((in_out_point.size() == 2) || i == 0) &&
                        ((i > 0 && prev_is_pair && prev_out <= current_time) || i == 0) &&
                        (static_cast<double>(in_out_point[0]) > current_time || i == 0); // 注意：GDScript 是 current_time < in_out_point[0]

                    bool current_cond_2 = (i == in_out_points.size() - 1);
                    if (current_cond_2) {
                        Array last = in_out_points[in_out_points.size() - 1];
                        if (last.size() >= 2 && current_time > static_cast<double>(last[1])) {
                            // 满足条件
                        }
                        else {
                            current_cond_2 = false;
                        }
                    }

                    if (current_cond_1 || current_cond_2) {
                        if (single_in_point_index != -1) {
                            in_out_points.remove_at(single_in_point_index);
                        }
                        Array new_single;
                        new_single.append(current_time);
                        in_out_points.append(new_single);
                        break;
                    }
                }
                // 逻辑条件 3
                else if (in_out_point.size() == 2) {
                    bool cond = false;
                    if (i == 0) cond = true;
                    else {
                        Array prev = in_out_points[i - 1];
                        if (prev.size() == 1 || current_time > static_cast<double>(prev[1])) {
                            cond = true;
                        }
                    }

                    if (cond && current_time < static_cast<double>(in_out_point[0])) {
                        in_out_point[0] = current_time;
                        break;
                    }
                    if (i == in_out_points.size() - 1 && current_time > static_cast<double>(in_out_point[1])) {
                        Array new_pair;
                        new_pair.append(current_time);
                        new_pair.append(duration);
                        in_out_points.append(new_pair);
                        break;
                    }
                }
            }
        }
        in_out_points.sort_custom(Callable(this, "_sort_in_out_compare"));
        settings_config->timeline_panel_chart_in_out_points.set(point_key, in_out_points);
        settings_config->save_settings_config(true);
    }

    void LTETimelineServer::add_chart_out_point(const String& chart_path, double current_time) {
        Dictionary all_points_dict = settings_config->timeline_panel_chart_in_out_points;
        String point_key = _resolve_chart_setting_key(all_points_dict, chart_path);
        Array in_out_points = all_points_dict.get(point_key, Array());
        String chart_abs_path = _get_absolute_chart_path(chart_path);
        Ref<ChartHelper> helper = ChartHelper::open(chart_abs_path);
        if (helper.is_null()) return;
        double duration = helper.is_valid() ? helper->get_duration() : 0.0;
        if (current_time < 0.0) current_time = 0.0;
        if (duration > 0.0 && current_time > duration) current_time = duration;
        double in_point;
        double out_point;
        if (in_out_points.is_empty()) {
            out_point = current_time;
            in_point = 0.0;
            Array new_pair;
            new_pair.append(in_point);
            new_pair.append(out_point);
            in_out_points.append(new_pair);
        }
        else {
            int single_in_point_index = _find_single_in_point(point_key);
            bool handled = false;
            if (single_in_point_index == -1 && !in_out_points.is_empty()) {
                Array first = in_out_points[0];
                if (!first.is_empty() && current_time < static_cast<double>(first[0])) {
                    Array new_pair;
                    new_pair.append(0.0);
                    new_pair.append(current_time);
                    in_out_points.append(new_pair);
                    handled = true;
                }
            }
            for (int i = 0; !handled && i < in_out_points.size(); ++i) {
                Array in_out_point = in_out_points[i];
                if (in_out_point.size() == 2 && static_cast<double>(in_out_point[0]) <= current_time && static_cast<double>(in_out_point[1]) >= current_time) {
                    in_out_point[1] = current_time;
                    break;
                }
                else if (single_in_point_index != -1 && in_out_point.size() == 1) {
                    bool has_next_start = false;
                    double next_start = 0.0;
                    // 检查下一段开始
                    if (i < in_out_points.size() - 1) {
                        Array next = in_out_points[i + 1];
                        if (!next.is_empty()) {
                            next_start = static_cast<double>(next[0]);
                            has_next_start = true;
                        }
                    }
                    Array single_arr = in_out_points[single_in_point_index];
                    double single_time = static_cast<double>(single_arr[0]);
                    if (current_time <= single_time || (has_next_start && current_time >= next_start)) {
                        OS::get_singleton()->alert("The current time cannot exceed the next in point or be earlier than the current point.", "ERROR");
                        break;
                    }
                    in_out_point.append(current_time);
                    break;
                }
                else if (single_in_point_index == -1) {
                    double next_start = INFINITY;
                    if (i < in_out_points.size() - 1) {
                        Array next = in_out_points[i + 1];
                        next_start = static_cast<double>(next[0]);
                    }
                    if ((i == in_out_points.size() - 1) || (current_time > static_cast<double>(in_out_point[1]) && current_time < next_start)) {
                        in_out_point[1] = current_time;
                        break;
                    }
                }
            }
        }
        in_out_points.sort_custom(Callable(this, "_sort_in_out_compare"));
        settings_config->timeline_panel_chart_in_out_points.set(point_key, in_out_points);
        settings_config->save_settings_config(true);
    }

    double LTETimelineServer::go_to_chart_in_point(const String& chart_path, double current_time, bool preview) {
        String point_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_in_out_points, chart_path);
        Array points = settings_config->timeline_panel_chart_in_out_points.get(point_key, Array());
        Array in_out_points_copy = points.duplicate(true);
        in_out_points_copy.sort_custom(Callable(this, "_sort_in_out_compare"));
        double time = current_time;
        const double epsilon = 0.000001;
        bool found = false;
        bool has_previous_in = false;
        double previous_in_time = 0.0;
        for (int i = 0; i < in_out_points_copy.size(); ++i) {
            Array in_out_point = in_out_points_copy[i];
            if (in_out_point.is_empty()) continue;
            double in_time = static_cast<double>(in_out_point[0]);
            double out_time = in_out_point.size() >= 2 ? static_cast<double>(in_out_point[1]) : in_time;
            if (!found) {
                time = in_time;
                found = true;
            }
            double distance_to_in = current_time - in_time;
            if (distance_to_in < 0.0) {
                distance_to_in = -distance_to_in;
            }
            if (distance_to_in <= epsilon) {
                time = has_previous_in ? previous_in_time : in_time;
                break;
            }
            if (current_time < in_time - epsilon) {
                time = in_time;
                break;
            }
            if (current_time <= out_time + epsilon) {
                time = in_time;
                break;
            }
            time = in_time;
            previous_in_time = in_time;
            has_previous_in = true;
        }
        if (preview && found) emit_signal("chart_playhead_changed", point_key, time);
        return time;
    }

    double LTETimelineServer::go_to_chart_out_point(const String& chart_path, double current_time, bool preview) {
        String point_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_in_out_points, chart_path);
        Array points = settings_config->timeline_panel_chart_in_out_points.get(point_key, Array());
        Array in_out_points_copy = points.duplicate(true);
        in_out_points_copy.sort_custom(Callable(this, "_sort_in_out_compare"));
        double time = current_time;
        const double epsilon = 0.000001;
        bool found = false;
        for (int i = 0; i < in_out_points_copy.size(); ++i) {
            Array in_out_point = in_out_points_copy[i];
            if (in_out_point.size() == 1) continue;
            double out_time = static_cast<double>(in_out_point[1]);
            if (!found) {
                time = out_time;
                found = true;
            }
            if (out_time > current_time + epsilon) {
                time = out_time;
                break;
            }
            time = out_time;
        }
        if (preview && found) emit_signal("chart_playhead_changed", point_key, time);
        return time;
    }

    void LTETimelineServer::remove_chart_in_out_point(const String& chart_path, const int64_t index) {
        String point_key = _resolve_chart_setting_key(settings_config->timeline_panel_chart_in_out_points, chart_path);
        Array in_out_point_list = settings_config->timeline_panel_chart_in_out_points.get(point_key, Array());
        if (index >= 0 && index < in_out_point_list.size()) in_out_point_list.remove_at(index);
        settings_config->timeline_panel_chart_in_out_points[point_key] = in_out_point_list;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::set_chart_per_bar(const String& uuid, const String& chart_path, int per_bar) {
        Ref<ChartHelper> helper = get_chart_helper(uuid, chart_path);
        if (helper.is_valid()) helper->set_per_bar(per_bar);
        Dictionary chart_list = settings_config->timeline_panel_chart_per_bar.get(uuid, Dictionary());
        chart_list[chart_path] = per_bar;
        settings_config->timeline_panel_chart_per_bar[uuid] = chart_list;
        settings_config->save_settings_config();
    }

    void LTETimelineServer::set_playhead(const String& uuid, const String& chart_path, const double beat) {
        Dictionary playhead_dict = settings_config->timeline_panel_chart_playhead.get(uuid, Dictionary());
        playhead_dict[chart_path] = beat;
        settings_config->timeline_panel_chart_playhead[uuid] = playhead_dict;
        settings_config->save_settings_config(false);
        double time = get_time_from_playhead(uuid, chart_path);
        emit_signal("chart_playhead_changed", chart_path, time);
    }

    String LTETimelineServer::get_chart_audio_abs_path(const String& chart_path) const {
        LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
        String root_dir = project_manager ? project_manager->get_project_path() : String();
        if (!chart_path.begins_with(root_dir)) return "";
        Dictionary chart = Utils::load_json_file(chart_path);
        Dictionary meta = chart.get("metadata", Dictionary());
        String audio = meta.get("audio", "");
        return root_dir + audio;
    }

    double LTETimelineServer::get_time_from_playhead(const String& uuid, const String& chart_path) {
        Ref<ChartHelper> helper = get_chart_helper(uuid, chart_path);
        if (helper.is_null()) return 0.0;
        Dictionary playhead_dict = settings_config->timeline_panel_chart_playhead.get(uuid, Dictionary());
        double playhead = playhead_dict.get(chart_path, 0.0);
        return helper->beat_to_time(playhead);
    }

    double LTETimelineServer::get_chart_duration(const String& uuid, const String& chart_path) {
        Ref<ChartHelper> helper = get_chart_helper(uuid, chart_path);
        return helper.is_valid() ? helper->get_duration() : 0.0;
    }

    Dictionary LTETimelineServer::submit_chart_changes(const String& uuid, const String& chart_path, const Dictionary& new_chart) {
        Dictionary opened_chart = find_opened_chart_info(uuid, chart_path);
        if (opened_chart.is_empty()) return Dictionary();

        LTEUndoRedo* undo_redo = LTEUndoRedo::get_singleton();
        if (!undo_redo) return Dictionary();

        Dictionary old_chart = opened_chart.get("chart", Dictionary());
        old_chart = old_chart.duplicate(true);
        Dictionary committed_chart = new_chart.duplicate(true);
        undo_redo->create_action("chart_changed");
        Callable do_call = Callable(this, "_apply_chart_change").bind(opened_chart, committed_chart);
        Callable undo_call = Callable(this, "_apply_chart_change").bind(opened_chart, old_chart);
        undo_redo->add_do_method(do_call);
        undo_redo->add_undo_method(undo_call);
        submitting_chart_change = true;
        undo_redo->commit_action();
        submitting_chart_change = false;

        opened_chart["saved"] = false;
        emit_signal("chart_changes_submitted", uuid, chart_path);
        if (_is_chart_auto_save_enabled()) {
            _queue_chart_save(opened_chart, _get_auto_save_delay_msec());
            return opened_chart;
        }
        return opened_chart;
    }

    Dictionary LTETimelineServer::save_chart(const String& uuid, const String& chart_path) {
        Dictionary opened_chart = find_opened_chart_info(uuid, chart_path);
        if (opened_chart.is_empty()) return Dictionary();
        _queue_chart_save(opened_chart);
        return opened_chart;
    }

    void LTETimelineServer::flush_pending_chart_saves() {
        LTEFileSaveServer* file_save_server = LTEFileSaveServer::get_singleton();
        if (file_save_server) {
            file_save_server->flush_pending_saves();
        }
    }

    Dictionary LTETimelineServer::find_opened_chart_info(const String& uuid, const String& chart_path) const {
        String target_chart_path = _get_absolute_chart_path(chart_path);
        for (int i = 0; i < opened_charts.size(); ++i) {
            Dictionary oc = opened_charts[i];
            if (!_is_opened_chart_in_current_project(oc)) {
                continue;
            }
            if (oc.get("uuid", "") != uuid) {
                continue;
            }
            String opened_chart_path = oc.get("path", "");
            if (opened_chart_path == chart_path) {
                return oc;
            }
            if (!target_chart_path.is_empty() && _get_absolute_chart_path(opened_chart_path) == target_chart_path) {
                return oc;
            }
        }
        return Dictionary();
    }

    double LTETimelineServer::time_step_back(const String& chart_path, const double time) const {
        Ref<ChartHelper> helper = ChartHelper::open(chart_path);
        if (helper.is_null()) {
            ERR_PRINT("time_step_back: Failed to open chart path");
            return 0.0;
        }
        int per_bar = settings_config->timeline_panel_chart_per_bar.get(chart_path, 4);
        helper->set_per_bar(per_bar);
        int64_t row = UtilityFunctions::clampi(
            UtilityFunctions::snappedi(helper->time_to_row(time) - 1, 1),
            0, (double)helper->get_row_total()
        );
        return helper->row_to_time((double)row);
    }

    double LTETimelineServer::time_step_forward(const String& chart_path, const double time) const {
        Ref<ChartHelper> helper = ChartHelper::open(chart_path);
        if (helper.is_null()) {
            ERR_PRINT("time_step_back: Failed to open chart path");
            return time;
        }
        int per_bar = settings_config->timeline_panel_chart_per_bar.get(chart_path, 4);
        helper->set_per_bar(per_bar);
        int64_t row = UtilityFunctions::clampi(
            UtilityFunctions::snappedi(helper->time_to_row(time) + 1, 1),
            0, (double)helper->get_row_total()
        );
        return helper->row_to_time((double)row);
    }

    int LTETimelineServer::step_back(const String& uuid, const String& chart_path, const int current_row, const int per_bar) {
        Ref<ChartHelper> helper = get_chart_helper(uuid, chart_path);
        if (helper.is_null()) return current_row;
        helper->set_per_bar(per_bar);
        int row = Math::clamp(
            Math::snapped((double)current_row - 1, 1.0),
            0.0, (double)helper->get_row_total()
        );
        return row;
    }

    int LTETimelineServer::step_forward(const String& uuid, const String& chart_path, const int current_row, const int per_bar) {
        Ref<ChartHelper> helper = get_chart_helper(uuid, chart_path);
        if (helper.is_null()) return current_row;
        helper->set_per_bar(per_bar);
        int row = Math::clamp(
            Math::snapped((double)current_row + 1, 1.0),
            0.0, (double)helper->get_row_total()
        );
        return row;
    }
}
