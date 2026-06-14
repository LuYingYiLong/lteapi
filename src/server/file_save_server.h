#ifndef FILE_SAVE_SERVER_H
#define FILE_SAVE_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace godot {
	class LTEFileSaveServer : public Object {
		GDCLASS(LTEFileSaveServer, Object)

	private:
		enum class SaveMode {
			TEXT,
			JSON,
		};

		struct SaveJob {
			String path;
			String tag;
			String content;
			String json_indent;
			Dictionary json_data;
			int64_t revision = 0;
			int32_t delay_msec = 250;
			bool atomic = true;
			bool owns_data = true;
			SaveMode mode = SaveMode::TEXT;
		};

		struct SaveResult {
			String path;
			String tag;
			int64_t revision = 0;
			Error error = OK;
		};

		static LTEFileSaveServer* singleton;

		bool worker_running = false;
		bool stop_requested = false;
		int32_t active_save_count = 0;
		int64_t revision_counter = 0;
		std::mutex save_mutex;
		std::condition_variable save_condition;
		std::thread save_worker;
		std::vector<SaveJob> pending_saves;
		std::vector<SaveResult> completed_saves;

		void _start_worker();
		void _stop_worker();
		void _worker_loop();
		void _queue_save_job(const SaveJob& job);
		Error _save_job(const SaveJob& job) const;
		Error _save_text_atomic(const String& path, const String& content) const;
		Error _save_text_direct(const String& path, const String& content) const;
		void _flush_completed_saves();

	protected:
		static void _bind_methods();

	public:
		LTEFileSaveServer();
		~LTEFileSaveServer();

		static LTEFileSaveServer* get_singleton();

		int64_t queue_text_save(const String& path, const String& content, const String& tag = String(), const int32_t delay_msec = 250, const bool atomic = true);
		int64_t queue_json_save(const String& path, const Dictionary& data, const String& json_indent = String(), const String& tag = String(), const int32_t delay_msec = 250, const bool atomic = true);
		int64_t queue_json_save_no_copy(const String& path, const Dictionary& data, const String& json_indent, const String& tag, const int32_t delay_msec, const bool atomic);
		Error save_text_now(const String& path, const String& content, const bool atomic = true) const;
		Error save_json_now(const String& path, const Dictionary& data, const String& json_indent = String(), const bool atomic = true) const;
		void flush_pending_saves();
	};
}

#endif // FILE_SAVE_SERVER_H
