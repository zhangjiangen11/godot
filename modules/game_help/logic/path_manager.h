#pragma once
#include "../csv/CSV_EditorImportPlugin.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"

class PathManager : public Object {
	GDCLASS(PathManager, Object);
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("reload"), &PathManager::reload);
		ClassDB::bind_method(D_METHOD("add_path", "group", "path", "enable"), &PathManager::add_path);
		ClassDB::bind_method(D_METHOD("load_file_read", "group", "sub_path"), &PathManager::load_file_read);
		ClassDB::bind_method(D_METHOD("load_file_write", "group", "sub_path"), &PathManager::load_file_write);
	}

public:
	static PathManager *get_singleton();
	// 初始化路径
	void init();

	void reload() {
		groups.clear();
		is_init = false;
	}

	void add_path(StringName group, String path, bool enable) {
		if (!DirAccess::exists(path)) {
			return;
		}
		groups[group].dirs.push_back({ DirAccess::create_for_path(path), enable });
	}

	Ref<FileAccess> load_file_read(String group, String sub_path) {
		init();
		if (groups.has(group)) {
			return groups[group].load_file_read(sub_path);
		}
		return nullptr;
	}

	Ref<FileAccess> load_file_write(String group, String sub_path) {
		init();
		if (groups.has(group)) {
			return groups[group].load_file_write(sub_path);
		}
		return nullptr;
	}
	PathManager();
	~PathManager();

public:
	struct DirItem {
		Ref<DirAccess> da;
		bool is_exist(const String &sub_path) const {
			return da->dir_exists(sub_path);
		}
		Ref<FileAccess> load_file_read(String sub_path) const {
			if (!is_exist(sub_path)) {
				return nullptr;
			}
			String full_path = da->get_current_dir().path_join(sub_path);
			Error err;
			return FileAccess::open(full_path, FileAccess::READ, &err);
		}
		Ref<FileAccess> load_file_write(String sub_path) const {
			if (!is_exist(sub_path)) {
				return nullptr;
			}
			String full_path = da->get_current_dir().path_join(sub_path);
			Error err;
			return FileAccess::open(full_path, FileAccess::WRITE, &err);
		}
		bool enable;
	};

	struct PathGroup {
		Vector<DirItem> dirs;
		Ref<FileAccess> load_file_read(String sub_path) const {
			for (int i = 0; i < dirs.size(); i++) {
				if (!dirs[i].enable) {
					continue;
				}
				if (dirs[i].is_exist(sub_path)) {
					return dirs[i].load_file_read(sub_path);
				}
			}
			return nullptr;
		}
		Ref<FileAccess> load_file_write(String sub_path) const {
			for (int i = 0; i < dirs.size(); i++) {
				if (!dirs[i].enable) {
					continue;
				}
				return dirs[i].load_file_write(sub_path);
			}
			return nullptr;
		}
	};
	bool is_init = false;
	int data_version = 0;
	HashMap<StringName, PathGroup> groups;
};