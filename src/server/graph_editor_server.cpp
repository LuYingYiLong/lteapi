#include "graph_editor_server.h"

#include "chart_helper.h"
#include "project_manager.h"
#include "settings_config.h"
#include "timeline_server.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace godot {
	LTEGraphEditorServer* LTEGraphEditorServer::singleton = nullptr;

	void LTEGraphEditorServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("fetch_graph_editor_config", "uuid"), &LTEGraphEditorServer::fetch_graph_editor_config);
		ClassDB::bind_method(D_METHOD("set_playhead_auto_scroll", "uuid", "enable"), &LTEGraphEditorServer::set_playhead_auto_scroll);
		ClassDB::bind_method(D_METHOD("set_split_ratio", "uuid", "ratio"), &LTEGraphEditorServer::set_split_ratio);
		ClassDB::bind_method(D_METHOD("set_curve_view", "uuid", "curve_id", "scroll_offset", "content_scale"), &LTEGraphEditorServer::set_curve_view);
		ClassDB::bind_method(D_METHOD("set_curve_tree_scroll", "uuid", "scroll"), &LTEGraphEditorServer::set_curve_tree_scroll);
		ClassDB::bind_method(D_METHOD("find_adjacent_keyframe_time", "keyframe_times", "current_time", "forward"), &LTEGraphEditorServer::find_adjacent_keyframe_time);
		ClassDB::bind_method(D_METHOD("fetch_speed_graph_model", "uuid"), &LTEGraphEditorServer::fetch_speed_graph_model);
		ClassDB::bind_method(D_METHOD("set_active_chart_path", "uuid", "chart_path"), &LTEGraphEditorServer::set_active_chart_path);
		ClassDB::bind_method(D_METHOD("submit_speed_curve", "uuid", "chart_path", "speeds"), &LTEGraphEditorServer::submit_speed_curve);

		ADD_SIGNAL(MethodInfo("speed_graph_model_changed",
			PropertyInfo(Variant::STRING, "uuid"),
			PropertyInfo(Variant::DICTIONARY, "model")));
	}

	String LTEGraphEditorServer::_get_absolute_chart_path(const String& chart_path) const {
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

	bool LTEGraphEditorServer::_is_chart_in_current_project(const String& chart_path) const {
		String absolute_chart_path = _get_absolute_chart_path(chart_path).replace("\\", "/").simplify_path();
		if (absolute_chart_path.is_empty()) {
			return false;
		}

		LTEProjectManager* project_manager = LTEProjectManager::get_singleton();
		String root_dir = project_manager ? project_manager->get_project_path().replace("\\", "/").simplify_path() : String();
		if (root_dir.is_empty()) {
			return false;
		}
		if (!root_dir.ends_with("/")) {
			root_dir += "/";
		}
		return absolute_chart_path.to_lower().begins_with(root_dir.to_lower());
	}

	Dictionary LTEGraphEditorServer::_find_opened_chart_by_path(const String& chart_path) const {
		if (chart_path.is_empty()) {
			return Dictionary();
		}

		LTETimelineServer* timeline_server = LTETimelineServer::get_singleton();
		if (!timeline_server) {
			return Dictionary();
		}

		String target_chart_path = _get_absolute_chart_path(chart_path);
		Array opened_charts = timeline_server->get_opened_chart_list();
		for (int index = 0; index < opened_charts.size(); ++index) {
			Dictionary opened_chart = opened_charts[index];
			String opened_chart_path = opened_chart.get("path", String());
			if (!_is_chart_in_current_project(opened_chart_path)) {
				continue;
			}
			if (opened_chart_path == chart_path) {
				return opened_chart;
			}
			if (!target_chart_path.is_empty() && _get_absolute_chart_path(opened_chart_path) == target_chart_path) {
				return opened_chart;
			}
		}
		return Dictionary();
	}

	Dictionary LTEGraphEditorServer::_get_latest_opened_chart() const {
		LTETimelineServer* timeline_server = LTETimelineServer::get_singleton();
		if (!timeline_server) {
			return Dictionary();
		}

		Array opened_charts = timeline_server->get_opened_chart_list();
		for (int index = opened_charts.size() - 1; index >= 0; --index) {
			Dictionary opened_chart = opened_charts[index];
			String opened_chart_path = opened_chart.get("path", String());
			if (_is_chart_in_current_project(opened_chart_path)) {
				return opened_chart;
			}
		}
		return Dictionary();
	}

	Dictionary LTEGraphEditorServer::_make_empty_model(const String& uuid) const {
		Dictionary model;
		model["uuid"] = uuid;
		model["source_uuid"] = String();
		model["chart_path"] = String();
		model["has_chart"] = false;
		model["duration"] = 0.0;
		model["current_time"] = 0.0;
		model["snap_mode"] = false;
		model["per_bar"] = 4;
		model["speeds"] = Array();
		model["bpms"] = Array();
		model["chart"] = Dictionary();
		model["chart_helper"] = Variant();
		return model;
	}

	Dictionary LTEGraphEditorServer::_make_model(const String& uuid, const Dictionary& opened_chart) const {
		if (opened_chart.is_empty()) {
			return _make_empty_model(uuid);
		}

		Dictionary chart = opened_chart.get("chart", Dictionary());
		Dictionary metadata = chart.get("metadata", Dictionary());
		String chart_path = opened_chart.get("path", String());
		String source_uuid = opened_chart.get("uuid", String());

		Ref<ChartHelper> chart_helper;
		LTETimelineServer* timeline_server = LTETimelineServer::get_singleton();
		if (timeline_server) {
			chart_helper = timeline_server->get_chart_helper(source_uuid, chart_path);
		}

		double duration = metadata.get("duration", 0.0);
		if (chart_helper.is_valid()) {
			duration = chart_helper->get_duration();
		}
		double current_time = 0.0;
		bool snap_mode = false;
		int per_bar = chart_helper.is_valid() ? chart_helper->get_per_bar() : 4;
		if (timeline_server) {
			Dictionary config = timeline_server->fetch_timeline_config(source_uuid, chart_path);
			current_time = timeline_server->get_time_from_playhead(source_uuid, chart_path);
			snap_mode = config.get("snap_mode", false);
			per_bar = config.get("chart_per_bar", per_bar);
		}

		Dictionary model;
		model["uuid"] = uuid;
		model["source_uuid"] = source_uuid;
		model["chart_path"] = chart_path;
		model["has_chart"] = true;
		model["duration"] = duration;
		model["current_time"] = current_time;
		model["snap_mode"] = snap_mode;
		model["per_bar"] = per_bar;
		model["speeds"] = _normalize_speeds(chart.get("speed", Array()));
		model["bpms"] = chart.get("bpm", Array());
		model["chart"] = chart.duplicate(true);
		model["chart_helper"] = chart_helper;
		return model;
	}

	Dictionary LTEGraphEditorServer::_normalize_speed(const Dictionary& speed) const {
		Dictionary normalized = speed.duplicate(true);
		double beat = normalized.get("beat", 0.0);
		double speed_value = normalized.get("speed", normalized.get("bpm", 1.0));
		String interpolation = normalized.get("interpolation", String("linear"));

		beat = std::max(0.0, std::round(beat * 1000000.0) / 1000000.0);
		speed_value = std::max(0.0, speed_value);
		if (interpolation.is_empty()) {
			interpolation = String("linear");
		}

		normalized["beat"] = beat;
		normalized["speed"] = speed_value;
		normalized["interpolation"] = interpolation;
		normalized.erase("bpm");
		normalized.erase("update_mode");
		if (interpolation != String("bezier")) {
			normalized.erase("in_handle");
			normalized.erase("out_handle");
		}
		return normalized;
	}

	Array LTEGraphEditorServer::_normalize_speeds(const Array& speeds) const {
		std::vector<Dictionary> normalized_speeds;
		normalized_speeds.reserve(speeds.size());

		for (int index = 0; index < speeds.size(); ++index) {
			Variant value = speeds[index];
			if (value.get_type() != Variant::DICTIONARY) {
				continue;
			}
			Dictionary speed = value;
			normalized_speeds.push_back(_normalize_speed(speed));
		}

		std::sort(normalized_speeds.begin(), normalized_speeds.end(), [](const Dictionary& a, const Dictionary& b) {
			return double(a.get("beat", 0.0)) < double(b.get("beat", 0.0));
		});

		std::vector<Dictionary> deduped_speeds;
		deduped_speeds.reserve(normalized_speeds.size());
		for (const Dictionary& speed : normalized_speeds) {
			if (!deduped_speeds.empty()) {
				double previous_beat = deduped_speeds.back().get("beat", 0.0);
				double current_beat = speed.get("beat", 0.0);
				if (std::abs(previous_beat - current_beat) <= 0.000001) {
					deduped_speeds.back() = speed;
					continue;
				}
			}
			deduped_speeds.push_back(speed);
		}

		Array result;
		for (const Dictionary& speed : deduped_speeds) {
			result.append(speed);
		}
		return result;
	}

	LTEGraphEditorServer::LTEGraphEditorServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
	}

	LTEGraphEditorServer::~LTEGraphEditorServer() {
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEGraphEditorServer* LTEGraphEditorServer::get_singleton() {
		return singleton;
	}

	void LTEGraphEditorServer::clear_project_state() {
		active_chart_paths.clear();
	}

	Dictionary LTEGraphEditorServer::fetch_graph_editor_config(const String& uuid) const {
		Dictionary config;
		config["playhead_auto_scroll"] = true;
		config["split_ratio"] = 0.25;
		config["curve_views"] = Dictionary();
		if (uuid.is_empty()) {
			return config;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return config;
		}
		config["playhead_auto_scroll"] = settings_config->graph_editor_playhead_auto_scroll.get(uuid, config["playhead_auto_scroll"]);
		Variant view_settings_value = settings_config->graph_editor_view_settings.get(uuid, Dictionary());
		if (view_settings_value.get_type() == Variant::DICTIONARY) {
			Dictionary view_settings = view_settings_value;
			config["split_ratio"] = view_settings.get("split_ratio", config["split_ratio"]);
			config["curve_views"] = view_settings.get("curve_views", config["curve_views"]);
			config["curve_tree_scroll"] = view_settings.get("curve_tree_scroll", Vector2(-1, -1));
		}
		return config;
	}

	void LTEGraphEditorServer::set_playhead_auto_scroll(const String& uuid, const bool enable) {
		if (uuid.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		settings_config->graph_editor_playhead_auto_scroll[uuid] = enable;
		settings_config->save_settings_config();
	}

	void LTEGraphEditorServer::set_split_ratio(const String& uuid, const double ratio) {
		if (uuid.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		Dictionary view_settings = settings_config->graph_editor_view_settings.get(uuid, Dictionary());
		view_settings = view_settings.duplicate(true);
		view_settings["split_ratio"] = std::clamp(ratio, 0.05, 0.95);
		settings_config->graph_editor_view_settings[uuid] = view_settings;
		settings_config->save_settings_config();
	}

	void LTEGraphEditorServer::set_curve_view(const String& uuid, const String& curve_id, const Vector2& scroll_offset, const Vector2& content_scale) {
		if (uuid.is_empty() || curve_id.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		Dictionary view_settings = settings_config->graph_editor_view_settings.get(uuid, Dictionary());
		view_settings = view_settings.duplicate(true);
		Dictionary curve_views = view_settings.get("curve_views", Dictionary());
		curve_views = curve_views.duplicate(true);
		Dictionary curve_view;
		curve_view["scroll_offset"] = scroll_offset;
		curve_view["content_scale"] = content_scale;
		curve_views[curve_id] = curve_view;
		view_settings["curve_views"] = curve_views;
		settings_config->graph_editor_view_settings[uuid] = view_settings;
		settings_config->save_settings_config();
	}

	void LTEGraphEditorServer::set_curve_tree_scroll(const String& uuid, const Vector2& scroll) {
		if (uuid.is_empty()) {
			return;
		}
		LTESettingsConfig* settings_config = LTESettingsConfig::get_singleton();
		if (!settings_config) {
			return;
		}
		Dictionary view_settings = settings_config->graph_editor_view_settings.get(uuid, Dictionary());
		view_settings = view_settings.duplicate(true);
		view_settings["curve_tree_scroll"] = scroll;
		settings_config->graph_editor_view_settings[uuid] = view_settings;
		settings_config->save_settings_config();
	}

	double LTEGraphEditorServer::find_adjacent_keyframe_time(const Array& keyframe_times, const double current_time, const bool forward) const {
		std::vector<double> times;
		times.reserve(keyframe_times.size());
		for (int index = 0; index < keyframe_times.size(); ++index) {
			Variant value = keyframe_times[index];
			if (value.get_type() != Variant::INT && value.get_type() != Variant::FLOAT) {
				continue;
			}
			times.push_back(std::max(0.0, double(value)));
		}
		if (times.empty()) {
			return -1.0;
		}

		std::sort(times.begin(), times.end());
		const double epsilon = 0.0001;
		times.erase(std::unique(times.begin(), times.end(), [epsilon](const double first_time, const double second_time) {
			return std::abs(first_time - second_time) <= epsilon;
		}), times.end());

		if (forward) {
			for (const double time : times) {
				if (time > current_time + epsilon) {
					return time;
				}
			}
			return -1.0;
		}

		for (auto iterator = times.rbegin(); iterator != times.rend(); ++iterator) {
			if (*iterator < current_time - epsilon) {
				return *iterator;
			}
		}
		return -1.0;
	}

	Dictionary LTEGraphEditorServer::fetch_speed_graph_model(const String& uuid) {
		String active_chart_path = active_chart_paths.get(uuid, String());
		Dictionary opened_chart = _find_opened_chart_by_path(active_chart_path);
		if (opened_chart.is_empty()) {
			opened_chart = _get_latest_opened_chart();
		}
		if (opened_chart.is_empty()) {
			active_chart_paths.erase(uuid);
			return _make_empty_model(uuid);
		}

		active_chart_paths[uuid] = opened_chart.get("path", String());
		return _make_model(uuid, opened_chart);
	}

	Dictionary LTEGraphEditorServer::set_active_chart_path(const String& uuid, const String& chart_path) {
		active_chart_paths[uuid] = chart_path;
		Dictionary model = fetch_speed_graph_model(uuid);
		emit_signal("speed_graph_model_changed", uuid, model);
		return model;
	}

	Dictionary LTEGraphEditorServer::submit_speed_curve(const String& uuid, const String& chart_path, const Array& speeds) {
		Dictionary opened_chart = _find_opened_chart_by_path(chart_path);
		if (opened_chart.is_empty()) {
			Dictionary model = fetch_speed_graph_model(uuid);
			emit_signal("speed_graph_model_changed", uuid, model);
			return model;
		}

		String source_uuid = opened_chart.get("uuid", String());
		String opened_chart_path = opened_chart.get("path", chart_path);
		Dictionary chart = opened_chart.get("chart", Dictionary()).duplicate(true);
		chart["speed"] = _normalize_speeds(speeds);

		LTETimelineServer* timeline_server = LTETimelineServer::get_singleton();
		if (timeline_server) {
			timeline_server->submit_chart_changes(source_uuid, opened_chart_path, chart);
		}

		active_chart_paths[uuid] = opened_chart_path;
		Dictionary model = fetch_speed_graph_model(uuid);
		emit_signal("speed_graph_model_changed", uuid, model);
		return model;
	}
}
