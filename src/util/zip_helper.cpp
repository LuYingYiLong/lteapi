#include "zip_helper.h"

namespace godot {
	void ZipHelper::_bind_methods() {
		ClassDB::bind_static_method("ZipHelper", D_METHOD("pack_folder", "src_dir", "dst_zip"), &ZipHelper::pack_folder);
		ClassDB::bind_static_method("ZipHelper", D_METHOD("read_zip_single_file", "zip_path", "file_path"), &ZipHelper::read_zip_single_file);
		ClassDB::bind_static_method("ZipHelper", D_METHOD("extract_all_from_zip", "zip_path", "dst_dir"), &ZipHelper::extract_all_from_zip);
	}

	Error ZipHelper::_pack_recurse(Ref<ZIPPacker> packer, const String& base, const String& entry) {
		Ref<DirAccess> dir = DirAccess::open(base + entry);
		if (dir.is_null()) return ERR_FILE_CANT_OPEN;
		dir->list_dir_begin();
		String file_name = dir->get_next();
		Error err = OK;
		while (!file_name.is_empty()) {
			if (dir->current_is_dir()) {
				String folder_path = entry + file_name + "/";
				err = packer->start_file(folder_path);
				if (err != OK) return err;
				err = packer->close_file();
				if (err != OK) return err;
				err = _pack_recurse(packer, base, folder_path);
				if (err != OK) return err;
			}
			else {
				String path_to_zip = entry + file_name;
				err = _add_file_to_zip(packer, base + path_to_zip, path_to_zip);
				if (err != OK) return err;
			}
			file_name = dir->get_next();
		}
		dir->list_dir_end();
		return err;
	}

	Error ZipHelper::_add_file_to_zip(Ref<ZIPPacker> packer, const String& disk_path, const String& zip_path) {
		Ref<FileAccess> file = FileAccess::open(disk_path, FileAccess::READ);
		if (file.is_null()) {
			return ERR_FILE_CANT_OPEN;
		}
		Error err = packer->start_file(zip_path);
		if (err != OK) return err;
		PackedByteArray buffer = file->get_buffer(file->get_length());
		err = packer->write_file(buffer);
		if (err != OK) return err;
		err = packer->close_file();
		file->close();
		return err;
	}

	Error ZipHelper::pack_folder(const String& src_dir, const String& dst_zip) {
		Ref<ZIPPacker> packer;
		packer.instantiate();
		Error err = packer->open(dst_zip);
		if (err != OK) return err;
		String base = src_dir;
		if (!base.ends_with("/")) {
			base += "/";
		}
		err = _pack_recurse(packer, base, "");
		if (err != OK) return err;
		err = packer->close();
		return err;
	}

	PackedByteArray ZipHelper::read_zip_single_file(const String& zip_path, const String& file_path) {
		Ref<ZIPReader> reader;
		reader.instantiate();
		Error err = reader->open(zip_path);
		if (err != OK) return PackedByteArray();
		PackedByteArray res = reader->read_file(file_path);
		reader->close();
		return res;
	}

	Error ZipHelper::extract_all_from_zip(const String& zip_path, const String& dst_dir) {
		Ref<ZIPReader> reader;
		reader.instantiate();
		Error err = reader->open(zip_path);
		if (err != OK) return err;
		Ref<DirAccess> dir = DirAccess::open(dst_dir);
		if (dir.is_null()) return ERR_FILE_CANT_OPEN;
		PackedStringArray files = reader->get_files();
		for (int32_t index = 0; index < files.size(); index++) {
			String file_path = files[index];
			// 严格路径净化
			if (file_path.begins_with("/") || file_path.find("..") != -1) return ERR_INVALID_PARAMETER;
			if (file_path.ends_with("/")) {
				err = dir->make_dir_recursive(file_path);
				if (err != OK) return err;
				continue;
			}
			String current_dir = dir->get_current_dir().path_join(file_path);
			String base_dir = current_dir.get_base_dir();
			err = dir->make_dir_recursive(base_dir);
			if (err != OK) return err;
			Ref<FileAccess> file = FileAccess::open(current_dir, FileAccess::WRITE);
			if (file.is_null()) return ERR_FILE_CANT_OPEN;
			PackedByteArray buffer = reader->read_file(file_path);
			file->store_buffer(buffer);
			file->close();
		}
		err = reader->close();
		return err;
	}
}