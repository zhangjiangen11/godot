#pragma once

#if TOOLS_ENABLED
#include "scene/resources/shader.h"
#include "servers/rendering/rendering_device_binds.h"

class HeightMapTemplateShader : public RefCounted {
	GDCLASS(HeightMapTemplateShader, RefCounted);

	static void _bind_methods();

private:
public:
	void init(const String &p_process_file_path, const String &p_priview_file_path);

	const String &get_process_file_path() {
		return process_file_path;
	}
	const String &get_preview_file_path() {
		return preview_file_path;
	}

	const String &get_process_shader_code() {
		return process_shader_code;
	}
	const String &get_preview_shader_code() {
		return preview_shader_code;
	}

	bool get_is_error() {
		return is_error;
	}
	void auto_reload();

	void add_using_process_shader(ObjectID id) {
		link_process_shaders.insert(id);
	}
	void remove_using_process_shader(ObjectID id) {
		link_process_shaders.erase(id);
	}
	uint64_t get_template_version() { return version; }

protected:
	void load();

protected:
	String process_file_path;
	String preview_file_path;

	String process_shader_code;
	String preview_shader_code;

	uint64_t process_file_path_time = -1;
	uint64_t priview_file_path_time = -1;
	HashSet<ObjectID> link_process_shaders;

	LocalVector<ObjectID> remove;
	uint64_t version = 0;
	bool is_error = true;
};

class HeightMapProcessShader : public RefCounted {
	GDCLASS(HeightMapProcessShader, RefCounted);
	static void _bind_methods();

public:
	void init(const Ref<HeightMapTemplateShader> &p_template_shader, const String &p_code_file_path);

	Array get_params() {
		return params;
	}
	Dictionary get_params_dict() {
		Dictionary dict;
		for (int i = 0; i < params.size(); i++) {
			Dictionary p = params[i];
			dict[p["name"]] = p["value"];
		}
		return dict;
	}
	Dictionary get_params_display_name_dict() {
		Dictionary dict;
		for (int i = 0; i < params.size(); i++) {
			Dictionary p = params[i];
			dict[p["name"]] = p["show_name"];
		}
		return dict;
	}

	// 更新处理shader的参数
	void update_process_shader_params(Dictionary p_params, Vector<uint8_t> p_code, int start_index) {
		p_code.resize((params.size() + (int64_t)start_index) * 4L);

		float *p = (float *)p_code.ptrw();
		p += start_index;
		for (int i = 0; i < params.size(); i++) {
			float v = 0;
			Dictionary dict = params[i];
			if (p_params.has(dict["name"])) {
				v = p_params[dict["name"]];
			}
			p[i] = v;
		}
	}

	Ref<RDShaderFile> get_process_shader() {
		return process_shader_file;
	}

	Ref<Shader> get_preview_shader() {
		return preview_mask_shader;
	}

	Ref<Shader> get_preview_height_shader() {
		return preview_height_shader;
	}

	Ref<Shader> get_preview_finish_shader() {
		return preview_finish_shader;
	}

	bool get_is_error() {
		return is_error;
	}

	void on_template_changed() {
		load();
	}
	const String &get_preview_name() {
		return preview_name;
	}

	void auto_reload();

protected:
	void load();
	~HeightMapProcessShader();

private:
	Ref<HeightMapTemplateShader> template_shader;
	Ref<RDShaderFile> process_shader_file;

	Ref<Shader> preview_mask_shader;
	Ref<Shader> preview_height_shader;
	Ref<Shader> preview_finish_shader;
	// 台阶化地形
	Ref<Shader> preview_stairs_shader;
	Array params;
	String code_file_path;
	String function_code;
	String process_code;
	uint64_t code_file_path_time = -1;

	String preview_name;
	uint64_t template_version = 0;
	bool is_error = false;
};

#endif
