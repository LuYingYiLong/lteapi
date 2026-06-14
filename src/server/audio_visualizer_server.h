#ifndef AUDIO_VISUALIZER_SERVER_H
#define AUDIO_VISUALIZER_SERVER_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
	class LTEAudioVisualizerServer : public Object {
		GDCLASS(LTEAudioVisualizerServer, Object)

	private:
		static LTEAudioVisualizerServer* singleton;

		Dictionary _make_default_config() const;
		String _normalize_path(const String& path) const;

	protected:
		static void _bind_methods();

	public:
		LTEAudioVisualizerServer();
		~LTEAudioVisualizerServer();

		static LTEAudioVisualizerServer* get_singleton();

		Dictionary normalize_config(const Dictionary& config) const;
		Dictionary fetch_config(const String& uuid) const;
		void submit_config(const String& uuid, const Dictionary& config);
		Dictionary load_config_file(const String& path) const;
	};
}

#endif // !AUDIO_VISUALIZER_SERVER_H
