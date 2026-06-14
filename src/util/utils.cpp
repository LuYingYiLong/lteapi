#include "utils.h"

namespace godot {
	void Utils::_bind_methods() {
		BIND_ENUM_CONSTANT(UUID_V7);

		ClassDB::bind_static_method("Utils", D_METHOD("load_json_file", "file_path"), &Utils::load_json_file);
		ClassDB::bind_static_method("Utils", D_METHOD("save_json_file", "path", "dict"), &Utils::save_json_file, DEFVAL("\t"));
		ClassDB::bind_static_method("Utils", D_METHOD("hms_to_seconds", "hms_string"), &Utils::hms_to_seconds);
		ClassDB::bind_static_method("Utils", D_METHOD("ms_to_seconds", "ms_string"), &Utils::ms_to_seconds);
		ClassDB::bind_static_method("Utils", D_METHOD("seconds_to_hms", "sec"), &Utils::seconds_to_hms);
		ClassDB::bind_static_method("Utils", D_METHOD("seconds_to_ms", "sec"), &Utils::seconds_to_ms);
		ClassDB::bind_static_method("Utils", D_METHOD("is_absolute_path", "path"), &Utils::is_absolute_path);
		ClassDB::bind_static_method("Utils", D_METHOD("check_dwm_at_runtime"), &Utils::check_dwm_at_runtime);
		ClassDB::bind_static_method("Utils", D_METHOD("uuid", "version"), &Utils::uuid);
		ClassDB::bind_static_method("Utils", D_METHOD("get_keycode_texture", "key"), &Utils::get_keycode_texture);
		ClassDB::bind_static_method("Utils", D_METHOD("get_icon_image"), &Utils::get_icon_image);
		ClassDB::bind_static_method("Utils", D_METHOD("normalize_path", "path"), &Utils::normalize_path);
		ClassDB::bind_static_method("Utils", D_METHOD("normalize_directory_path", "path"), &Utils::normalize_directory_path);
		ClassDB::bind_static_method("Utils", D_METHOD("is_windows_drive_path", "path"), &Utils::is_windows_drive_path);
		ClassDB::bind_static_method("Utils", D_METHOD("is_unc_absolute_path", "path"), &Utils::is_unc_absolute_path);
		ClassDB::bind_static_method("Utils", D_METHOD("is_project_resource_path", "path"), &Utils::is_project_resource_path);
		ClassDB::bind_static_method("Utils", D_METHOD("path_exists", "path"), &Utils::path_exists);
		ClassDB::bind_static_method("Utils", D_METHOD("is_path_in_project", "path", "root_dir"), &Utils::is_path_in_project);
		ClassDB::bind_static_method("Utils", D_METHOD("make_project_relative_path", "file_path", "root_dir"), &Utils::make_project_relative_path);
		ClassDB::bind_static_method("Utils", D_METHOD("resolve_project_path", "file_path", "root_dir"), &Utils::resolve_project_path);
		ClassDB::bind_static_method("Utils", D_METHOD("make_available_child_path", "parent_dir", "file_name"), &Utils::make_available_child_path);
		ClassDB::bind_static_method("Utils", D_METHOD("read_file_content_safe", "file_path"), &Utils::read_file_content_safe);
	}

	Dictionary Utils::load_json_file(const String& file_path) {
		Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
		if (file.is_null()) {
            Error err = FileAccess::get_open_error();
            ERR_PRINT(vformat("Failed to open file: %s. Error Code: %d", file_path, err));
			return Dictionary();
		}
		String content = file->get_as_text();
		file->close();
		Variant parse_result = JSON::parse_string(content);
		if (parse_result.get_type() != Variant::DICTIONARY) {
			ERR_PRINT(vformat("JSON content is not a dictionary in file: %s", file_path));
			return Dictionary();
		}
		return parse_result;
	}

	Error Utils::save_json_file(const String& path, const Dictionary& dict, const String& indent) {
        // 检查目录是否存在，不存在则创建
        String dir_path = path.get_base_dir();
        Error err = OK;
        Ref<DirAccess> dir = DirAccess::open(dir_path);
        if (dir.is_null()) {
            dir = DirAccess::open("user://");
            WARN_PRINT(vformat("Failed to open dir: %s", dir_path + ", defaulting to the user data dir"));
        }
        if (dir.is_valid()) {
            // 创建多级目录
            err = dir->make_dir_recursive(dir_path);
            if (err != OK) return err;
        }
		Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
        if (file.is_null()) return ERR_FILE_CANT_OPEN;
		String json = JSON::stringify(dict, indent);
		if (!file->store_string(json)) return ERR_FILE_CANT_WRITE;
		file->close();
        return err;
	}

    double Utils::hms_to_seconds(const String& p_hms_string) {
        // 移除空格
        String hms_string = p_hms_string.strip_edges();
        if (hms_string.is_empty()) return 0.0;
        String main_part = hms_string;
        double ms_seconds = 0.0;
        // 处理毫秒，兼容 "." 和 "," (SRT格式常用逗号)
        int dot_pos = hms_string.find(".");
        if (dot_pos == -1) {
            dot_pos = hms_string.find(",");
        }
        if (dot_pos != -1) {
            main_part = hms_string.substr(0, dot_pos);
            String ms_part = hms_string.substr(dot_pos + 1);

            if (!ms_part.is_empty()) {
                String float_str = "0." + ms_part;
                ms_seconds = float_str.to_float();
            }
        }
        // 解析时分秒
        PackedStringArray time_parts = main_part.split(":");
        double seconds = 0.0;
        int64_t count = time_parts.size();
        if (count >= 3) {
            seconds = time_parts[0].to_int() * 3600 + time_parts[1].to_int() * 60 + time_parts[2].to_int();
        }
        else if (count == 2) {
            seconds = time_parts[0].to_int() * 60 + time_parts[1].to_int();
        }
        else if (count == 1) {
            seconds = time_parts[0].to_int();
        }
        // 合并结果
        return seconds + ms_seconds;
    }

    double Utils::ms_to_seconds(const String& p_ms_string) {
        // 分割字符串
        PackedStringArray parts = p_ms_string.split(":");
        // 根据分割后的数量计算
        if (parts.size() == 2) {
            // MM:SS (parts[1] 如果带小数点，to_float会自动处理)
            return parts[0].to_float() * 60.0 + parts[1].to_float();
        }
        else if (parts.size() == 1) {
            // 只有秒
            return parts[0].to_float();
        }
        return 0.0;
    }

    String Utils::seconds_to_hms(const double sec) {
        int64_t total_sec = static_cast<int64_t>(sec);
        int64_t ms = static_cast<int64_t>(sec * 1000.0) % 1000;
        int64_t h = total_sec / 3600;
        int64_t m = (total_sec / 60) % 60;
        int64_t s = total_sec % 60;
        return vformat("%02d:%02d:%02d.%03d", h, m, s, ms);
    }

    String Utils::seconds_to_ms(const double sec) {
        int64_t total_sec = static_cast<int64_t>(sec);
        int64_t m = total_sec / 60;
        int64_t s = total_sec % 60;
        return vformat("%02d:%02d", m, s);
    }

    bool Utils::is_absolute_path(const String& path) {
        if (path.is_empty()) return false;
        // 检查是否以 "/" 开头（Unix风格）
        if (path.begins_with("/")) return true;
        // 检查是否为Windows风格的绝对路径（如 "C:\" 或 "D:/"）
        if (path.length() >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) return true;
		return false;
    }

    bool Utils::check_dwm_at_runtime() {
        return ClassDB::class_exists("DWM");
    }

    String Utils::uuid(UUIDVersion version) {
        switch (version) {
        case UUID_V7: {
            PackedByteArray bytes;
            bytes.resize(16);
            int64_t unix_time_ms = static_cast<uint64_t>(Time::get_singleton()->get_unix_time_from_system() * 1000.0);
            Ref<BuildInRandom> random;
			random.instantiate();
            PackedByteArray rands = random->bytes(10);
            bytes[0] = (unix_time_ms >> 40) & 0xFF;
            bytes[1] = (unix_time_ms >> 32) & 0xFF;
            bytes[2] = (unix_time_ms >> 24) & 0xFF;
            bytes[3] = (unix_time_ms >> 16) & 0xFF;

            bytes[4] = (unix_time_ms >> 8) & 0xFF;
            bytes[5] = unix_time_ms & 0xFF;
			bytes[6] = 0x70 | rands[0] & 0x0F;          // 版本号7
            bytes[7] = rands[1];

			bytes[8] = 0x80 | (rands[2] & 0x3F);        // 变体10xx
            bytes[9] = rands[3];
            bytes[10] = rands[4];
            bytes[11] = rands[5];

            bytes[12] = rands[6];
            bytes[13] = rands[7];
            bytes[14] = rands[8];
            bytes[15] = rands[9];
			return vformat("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                bytes[0], bytes[1], bytes[2], bytes[3],
                bytes[4], bytes[5],
                bytes[6], bytes[7],
                bytes[8], bytes[9],
                bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
        }
		}
        return String();
    }

    Variant Utils::load_variant(const String& p_path) {
        ResourceLoader* loader = ResourceLoader::get_singleton();
        if (loader != nullptr) {
            return loader->load(p_path);
        }
        return Variant();
    }

    Ref<Texture2D> Utils::get_keycode_texture(const Key key) {
        return get_key_texture(key);
    }

    Ref<Image> Utils::get_icon_image() {
        PackedByteArray data;
        data.resize(icon_size);
        memcpy(data.ptrw(), icon_data, icon_size);
        Ref<Image> image;
        image.instantiate();
        Error err = image->load_png_from_buffer(data);
        if (err != OK) {
            ERR_PRINT("Failed to load icon from PNG data");
            return Ref<Image>();
        }
        return image;
    }

	String Utils::normalize_path(const String& path) {
		return path.replace("\\", "/").strip_edges().simplify_path();
	}

	String Utils::normalize_directory_path(const String& path) {
		String normalized = normalize_path(path);
		if (normalized.length() > 3 && normalized[normalized.length() - 1] == '/') {
			normalized = normalized.substr(0, normalized.length() - 1);
		}
		return normalized;
	}

	bool Utils::is_windows_drive_path(const String& path) {
		if (path.length() < 3) return false;
		char32_t drive_code = path[0];
		bool has_drive_letter = (drive_code >= 'A' && drive_code <= 'Z') || (drive_code >= 'a' && drive_code <= 'z');
		return has_drive_letter && path[1] == ':' && path[2] == '/';
	}

	bool Utils::is_unc_absolute_path(const String& path) {
		return path.begins_with("//") && path.length() > 2;
	}

	bool Utils::is_project_resource_path(const String& path) {
		String normalized = path.replace("\\", "/").strip_edges();
		return normalized.begins_with("res://") || normalized.begins_with("user://");
	}

	bool Utils::path_exists(const String& path) {
		return FileAccess::file_exists(path) || DirAccess::dir_exists_absolute(path);
	}

	bool Utils::is_path_in_project(const String& path, const String& root_dir) {
		String normalized_path = normalize_path(path);
		String normalized_root_dir = normalize_directory_path(root_dir);
		if (normalized_path.is_empty()) return false;
		if (normalized_root_dir.is_empty()) return true;
		return normalized_path == normalized_root_dir || normalized_path.begins_with(normalized_root_dir + "/");
	}

	String Utils::make_project_relative_path(const String& file_path, const String& root_dir) {
		String normalized_path = normalize_path(file_path);
		String normalized_root_dir = normalize_path(root_dir);
		if (normalized_root_dir.is_empty()) return normalized_path;
		if (normalized_path == normalized_root_dir) return "/";
		if (normalized_path.begins_with(normalized_root_dir + "/")) {
			return normalized_path.substr(normalized_root_dir.length());
		}
		return normalized_path;
	}

	String Utils::resolve_project_path(const String& file_path, const String& root_dir) {
		String normalized_path = file_path.replace("\\", "/").strip_edges();
		if (normalized_path.is_empty()) return "";
		if (is_project_resource_path(normalized_path)) return normalized_path.simplify_path();
		if (is_unc_absolute_path(normalized_path) || is_windows_drive_path(normalized_path)) {
			return normalized_path.simplify_path();
		}
		if (normalized_path.begins_with("/") && !root_dir.is_empty()) {
			return (root_dir + normalized_path).replace("\\", "/").simplify_path();
		}
		if (normalized_path.is_absolute_path()) return normalized_path.simplify_path();
		if (root_dir.is_empty()) return normalized_path.simplify_path();
		return root_dir.path_join(normalized_path).replace("\\", "/").simplify_path();
	}

	String Utils::make_available_child_path(const String& parent_dir, const String& file_name) {
		String normalized_parent_dir = normalize_directory_path(parent_dir);
		String safe_file_name = file_name.get_file();
		if (safe_file_name.is_empty()) safe_file_name = "audio";
		String base_name = safe_file_name.get_basename();
		String extension = safe_file_name.get_extension();
		int count = 1;
		String candidate_name = safe_file_name;
		String candidate_path = normalized_parent_dir.path_join(candidate_name);
		while (path_exists(candidate_path)) {
			count++;
			if (extension.is_empty()) {
				candidate_name = vformat("%s%d", base_name, count);
			} else {
				candidate_name = vformat("%s%d.%s", base_name, count, extension);
			}
			candidate_path = normalized_parent_dir.path_join(candidate_name);
		}
		return candidate_path;
	}

	String Utils::read_file_content_safe(const String& file_path) {
		if (!FileAccess::file_exists(file_path)) return "";
		Ref<FileAccess> file = FileAccess::open(file_path, FileAccess::READ);
		if (file.is_null()) return "";
		PackedByteArray raw_bytes = file->get_buffer(file->get_length());
		file->close();
		return Marshalls::get_singleton()->raw_to_base64(raw_bytes);
	}
}