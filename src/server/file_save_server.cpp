#include "file_save_server.h"

#include <chrono>

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
	LTEFileSaveServer* LTEFileSaveServer::singleton = nullptr;

	void LTEFileSaveServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_flush_completed_saves"), &LTEFileSaveServer::_flush_completed_saves);
		ClassDB::bind_method(D_METHOD("queue_text_save", "path", "content", "tag", "delay_msec", "atomic"), &LTEFileSaveServer::queue_text_save, DEFVAL(String()), DEFVAL(250), DEFVAL(true));
		ClassDB::bind_method(D_METHOD("queue_json_save", "path", "data", "json_indent", "tag", "delay_msec", "atomic"), &LTEFileSaveServer::queue_json_save, DEFVAL(String()), DEFVAL(String()), DEFVAL(250), DEFVAL(true));
		ClassDB::bind_method(D_METHOD("queue_json_save_no_copy", "path", "data", "json_indent", "tag", "delay_msec", "atomic"), &LTEFileSaveServer::queue_json_save_no_copy);
		ClassDB::bind_method(D_METHOD("save_text_now", "path", "content", "atomic"), &LTEFileSaveServer::save_text_now, DEFVAL(true));
		ClassDB::bind_method(D_METHOD("save_json_now", "path", "data", "json_indent", "atomic"), &LTEFileSaveServer::save_json_now, DEFVAL(String()), DEFVAL(true));
		ClassDB::bind_method(D_METHOD("flush_pending_saves"), &LTEFileSaveServer::flush_pending_saves);

		ADD_SIGNAL(MethodInfo("file_saved",
				PropertyInfo(Variant::STRING, "path"),
				PropertyInfo(Variant::STRING, "tag"),
				PropertyInfo(Variant::INT, "revision")));
		ADD_SIGNAL(MethodInfo("file_save_failed",
				PropertyInfo(Variant::STRING, "path"),
				PropertyInfo(Variant::STRING, "tag"),
				PropertyInfo(Variant::INT, "revision"),
				PropertyInfo(Variant::INT, "error")));
	}

	LTEFileSaveServer::LTEFileSaveServer() {
		if (singleton == nullptr) {
			singleton = this;
		}
		_start_worker();
	}

	LTEFileSaveServer::~LTEFileSaveServer() {
		_stop_worker();
		_flush_completed_saves();
		if (singleton == this) {
			singleton = nullptr;
		}
	}

	LTEFileSaveServer* LTEFileSaveServer::get_singleton() {
		return singleton;
	}

	void LTEFileSaveServer::_start_worker() {
		if (worker_running) {
			return;
		}
		stop_requested = false;
		worker_running = true;
		save_worker = std::thread(&LTEFileSaveServer::_worker_loop, this);
	}

	void LTEFileSaveServer::_stop_worker() {
		{
			std::lock_guard<std::mutex> lock(save_mutex);
			stop_requested = true;
		}
		save_condition.notify_all();
		if (save_worker.joinable()) {
			save_worker.join();
		}
		worker_running = false;
	}

	void LTEFileSaveServer::_queue_save_job(const SaveJob& job) {
		if (job.path.is_empty()) {
			return;
		}
		{
			std::lock_guard<std::mutex> lock(save_mutex);
			for (SaveJob& pending_job : pending_saves) {
				if (pending_job.path == job.path) {
					pending_job = job;
					save_condition.notify_one();
					return;
				}
			}
			pending_saves.push_back(job);
		}
		save_condition.notify_one();
	}

	void LTEFileSaveServer::_worker_loop() {
		while (true) {
			SaveJob job;
			{
				std::unique_lock<std::mutex> lock(save_mutex);
				save_condition.wait(lock, [this]() {
					return stop_requested || !pending_saves.empty();
				});
				if (stop_requested && pending_saves.empty()) {
					break;
				}
				if (!stop_requested && !pending_saves.empty()) {
					int32_t delay_msec = pending_saves.front().delay_msec;
					save_condition.wait_for(lock, std::chrono::milliseconds(delay_msec), [this]() {
						return stop_requested;
					});
				}
				if (stop_requested && pending_saves.empty()) {
					break;
				}
				if (pending_saves.empty()) {
					continue;
				}
				job = pending_saves.front();
				pending_saves.erase(pending_saves.begin());
				++active_save_count;
			}

			SaveResult result;
			result.path = job.path;
			result.tag = job.tag;
			result.revision = job.revision;
			result.error = _save_job(job);

			bool should_defer_flush = false;
			{
				std::lock_guard<std::mutex> lock(save_mutex);
				completed_saves.push_back(result);
				--active_save_count;
				should_defer_flush = !stop_requested;
			}
			if (should_defer_flush) {
				call_deferred("_flush_completed_saves");
			}
		}
	}

	Error LTEFileSaveServer::_save_job(const SaveJob& job) const {
		if (job.mode == SaveMode::JSON) {
			Dictionary data_to_save;
			if (!job.owns_data) {
				data_to_save = job.json_data.duplicate(true);
			}
			else {
				data_to_save = job.json_data;
			}
			String content = JSON::stringify(data_to_save, job.json_indent);
			return save_text_now(job.path, content, job.atomic);
		}
		return save_text_now(job.path, job.content, job.atomic);
	}

	Error LTEFileSaveServer::_save_text_atomic(const String& path, const String& content) const {
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
		if (!file->store_string(content)) {
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

	Error LTEFileSaveServer::_save_text_direct(const String& path, const String& content) const {
		String dir_path = path.get_base_dir();
		Error error = DirAccess::make_dir_recursive_absolute(dir_path);
		if (error != OK) {
			return error;
		}
		Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
		if (file.is_null()) {
			return FileAccess::get_open_error();
		}
		if (!file->store_string(content)) {
			file->close();
			return ERR_FILE_CANT_WRITE;
		}
		file->close();
		return OK;
	}

	void LTEFileSaveServer::_flush_completed_saves() {
		std::vector<SaveResult> results;
		{
			std::lock_guard<std::mutex> lock(save_mutex);
			results.swap(completed_saves);
		}
		for (const SaveResult& result : results) {
			if (result.error == OK) {
				emit_signal("file_saved", result.path, result.tag, result.revision);
			}
			else {
				ERR_PRINT(vformat("Failed to save file: %s. Error Code: %d", result.path, result.error));
				emit_signal("file_save_failed", result.path, result.tag, result.revision, static_cast<int64_t>(result.error));
			}
		}
	}

	int64_t LTEFileSaveServer::queue_text_save(const String& path, const String& content, const String& tag, const int32_t delay_msec, const bool atomic) {
		SaveJob job;
		job.path = path.replace("\\", "/").simplify_path();
		job.tag = tag;
		job.content = content;
		job.revision = ++revision_counter;
		job.delay_msec = delay_msec < 0 ? 0 : delay_msec;
		job.atomic = atomic;
		job.mode = SaveMode::TEXT;
		_queue_save_job(job);
		return job.revision;
	}

	int64_t LTEFileSaveServer::queue_json_save(const String& path, const Dictionary& data, const String& json_indent, const String& tag, const int32_t delay_msec, const bool atomic) {
		SaveJob job;
		job.path = path.replace("\\", "/").simplify_path();
		job.tag = tag;
		job.json_data = data.duplicate(true);
		job.json_indent = json_indent;
		job.revision = ++revision_counter;
		job.delay_msec = delay_msec < 0 ? 0 : delay_msec;
		job.atomic = atomic;
		job.mode = SaveMode::JSON;
		_queue_save_job(job);
		return job.revision;
	}

	int64_t LTEFileSaveServer::queue_json_save_no_copy(const String& path, const Dictionary& data, const String& json_indent, const String& tag, const int32_t delay_msec, const bool atomic) {
		SaveJob job;
		job.path = path.replace("\\", "/").simplify_path();
		job.tag = tag;
		job.json_data = data;
		job.owns_data = false;
		job.json_indent = json_indent;
		job.revision = ++revision_counter;
		job.delay_msec = delay_msec < 0 ? 0 : delay_msec;
		job.atomic = atomic;
		job.mode = SaveMode::JSON;
		_queue_save_job(job);
		return job.revision;
	}

	Error LTEFileSaveServer::save_text_now(const String& path, const String& content, const bool atomic) const {
		String normalized_path = path.replace("\\", "/").simplify_path();
		if (normalized_path.is_empty()) {
			return ERR_INVALID_PARAMETER;
		}
		if (atomic) {
			return _save_text_atomic(normalized_path, content);
		}
		return _save_text_direct(normalized_path, content);
	}

	Error LTEFileSaveServer::save_json_now(const String& path, const Dictionary& data, const String& json_indent, const bool atomic) const {
		return save_text_now(path, JSON::stringify(data, json_indent), atomic);
	}

	void LTEFileSaveServer::flush_pending_saves() {
		while (true) {
			bool done = false;
			{
				std::lock_guard<std::mutex> lock(save_mutex);
				done = pending_saves.empty() && active_save_count == 0;
			}
			_flush_completed_saves();
			if (done) {
				break;
			}
			save_condition.notify_one();
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		_flush_completed_saves();
	}
}
