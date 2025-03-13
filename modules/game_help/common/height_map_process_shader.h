#pragma once

#if TOOLS_ENABLED
#include "servers/rendering/rendering_device_binds.h"
#include "scene/resources/shader.h"


class HeightMapTemplateShader : public RefCounted {
	GDCLASS(HeightMapTemplateShader, RefCounted);

	static void _bind_methods();
private:

public:
	void init(const String& p_process_file_path, const String& p_priview_file_path);

	const String& get_process_file_path() {
		return process_file_path;
	}
	const String& get_priview_file_path() {
		return priview_file_path;
	}

	const String& get_process_shader_code() {
		return process_shader_code;
	}
	const String& get_priview_shader_code() {
		return priview_shader_code;
	}

	bool get_is_error() {
		return is_error;
	}
protected:
    void load();
protected:
	String process_file_path;
	String priview_file_path;

	String process_shader_code;
	String priview_shader_code;

    int64_t process_file_path_time = -1;
    int64_t priview_file_path_time = -1;

	bool is_error = true;
};

class HeightMapProcessShader : public RefCounted {
    GDCLASS(HeightMapProcessShader,RefCounted);
    static void _bind_methods();
public:
    void init(const Ref<HeightMapTemplateShader>& p_template_shader,const String& p_code_file_path) ;

   Array get_params() {
        return params;
   }

    Ref<RDShaderFile> get_process_shader() {

        return process_shader_file;
    }

    Ref<Shader> get_priview_shader() {
        return priview_shader;
    }

    bool get_is_error() {
        return is_error;
    }

    void on_template_changed() {
        load();
    }

protected:
    void load();
    ~HeightMapProcessShader();
private:
    Ref<HeightMapTemplateShader> template_shader;
    Ref<RDShaderFile> process_shader_file;

    Ref<Shader> priview_shader;
    Array params;
    String code_file_path;
    String function_code;
    String process_code;
    int64_t code_file_path_time = -1;

    bool is_error = false;
};

#endif
