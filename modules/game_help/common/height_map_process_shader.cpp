
#if TOOLS_ENABLED
#include "height_map_process_shader.h"

void HeightMapTemplateShader::_bind_methods() {
    ClassDB::bind_method(D_METHOD("init", "process_file_path","priview_file_path"), &HeightMapTemplateShader::init);
    ClassDB::bind_method(D_METHOD("get_process_file_path"), &HeightMapTemplateShader::get_process_file_path);
    ClassDB::bind_method(D_METHOD("get_priview_file_path"), &HeightMapTemplateShader::get_priview_file_path);
    ClassDB::bind_method(D_METHOD("get_process_shader_code"), &HeightMapTemplateShader::get_process_shader_code);
    ClassDB::bind_method(D_METHOD("get_priview_shader_code"), &HeightMapTemplateShader::get_priview_shader_code);
    ClassDB::bind_method(D_METHOD("is_error"), &HeightMapTemplateShader::get_is_error);
    
    ADD_SIGNAL(MethodInfo("changed"));
}
void HeightMapTemplateShader::init(const String& p_process_file_path,const String& p_priview_file_path) {
    process_file_path = p_process_file_path;
    priview_file_path = p_priview_file_path;
    load();

}
void HeightMapTemplateShader::load() {
	is_error = false;
    process_file_path_time = FileAccess::get_modified_time(process_file_path);
    priview_file_path_time = FileAccess::get_modified_time(priview_file_path);
    Error err;
    Ref<FileAccess> file = FileAccess::open(process_file_path, FileAccess::READ, &err);
    if(err != OK) {
        is_error = true;
		ERR_FAIL_COND_MSG(err != OK, process_file_path + ": file not exist");
    }
    if(file.is_null()) {
        is_error = true;
		ERR_FAIL_COND_MSG(file.is_null(), process_file_path + ": file not exist");
    }

    process_shader_code = file->get_as_utf8_string();

    file = FileAccess::open(priview_file_path, FileAccess::READ, &err);

    if(err != OK) {
        is_error = true;
		ERR_FAIL_COND_MSG(err != OK, priview_file_path + ": file not exist");
    }
    if(file.is_null()) {
        is_error = true;
		ERR_FAIL_COND_MSG(file.is_null(), priview_file_path + ": file not exist");
    }
    priview_shader_code = file->get_as_utf8_string();
	emit_signal(CoreStringName(changed));
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////


static String _include_function(const String &p_path, void *userpointer) {
	Error err;

	String *base_path = (String *)userpointer;

	String include = p_path;
	if (include.is_relative_path()) {
		include = base_path->path_join(include);
	}

	Ref<FileAccess> file_inc = FileAccess::open(include, FileAccess::READ, &err);
	if (err != OK) {
		return String();
	}
	return file_inc->get_as_utf8_string();
}
void HeightMapProcessShader::_bind_methods() {
    ClassDB::bind_method(D_METHOD("init", "template_shader","code_file_path"), &HeightMapProcessShader::init);
    ClassDB::bind_method(D_METHOD("get_params"), &HeightMapProcessShader::get_params);
    ClassDB::bind_method(D_METHOD("get_is_error"), &HeightMapProcessShader::get_is_error);
    ClassDB::bind_method(D_METHOD("get_process_shader"), &HeightMapProcessShader::get_process_shader);
    ClassDB::bind_method(D_METHOD("get_priview_shader"), &HeightMapProcessShader::get_priview_shader);

    ADD_SIGNAL(MethodInfo("changed"));
}
void HeightMapProcessShader::init(const Ref<HeightMapTemplateShader>& p_template_shader,const String& p_code_file_path) {
    code_file_path = p_code_file_path;
    if(template_shader.is_valid()) {
        template_shader->disconnect(CoreStringName(changed),callable_mp(this, &HeightMapProcessShader::on_template_changed));
    }
	template_shader = p_template_shader;
    if(template_shader.is_null()) {
        is_error = true;
        ERR_FAIL_COND_MSG(p_template_shader.is_null(), "template shader not exist");
    }
    if(template_shader.is_valid()) {
        template_shader->connect(CoreStringName(changed),callable_mp(this, &HeightMapProcessShader::on_template_changed));
    }
	load();
}


void HeightMapProcessShader::load() {

	is_error = false;
	if (template_shader.is_null()) {
		is_error = true;
		return;
	}
	if (template_shader->get_is_error()) {
		is_error = true;
		ERR_FAIL_COND_MSG(template_shader->get_is_error(), "template shader error");
	}
    code_file_path_time = FileAccess::get_modified_time(code_file_path);
    Error err;
    Ref<FileAccess> file = FileAccess::open(code_file_path, FileAccess::READ, &err);
	ERR_FAIL_COND_MSG(err != OK, code_file_path + ": file not exist");
	ERR_FAIL_COND_MSG(file.is_null(), code_file_path + ": file not exist");

    String code = file->get_as_utf8_string();
    if(code.is_empty()) {
        is_error = true;
		ERR_FAIL_COND_MSG(code.is_empty(), code_file_path + ": file not exist");
    }
    code = code.remove_annotate().remove_char(' ').remove_char('\r').remove_char('\t');
    Vector<String> lines = code.split("$$",false);

    if(lines.size() != 3) {
        is_error = true;
		ERR_FAIL_COND_MSG(lines.size() != 3, code_file_path + L": 格式錯誤，請檢查，必須存在$$分隔符號，格式為[params_a:(name,value,min,max);params_b;...]\n$$[function_code]\n$$[process_code]");
    }

    String _params = lines[0];
    function_code = lines[1];
    process_code = lines[2];
  

    Vector<String> params_list = _params.split("\n", false);


    for(int i=0;i<params_list.size();i++) {
        Vector<String> param = params_list[i].remove_char(';').split(",", false);
		if (param.size() == 0) {
			continue;
		}
        if(param.size() != 4) {
            is_error = true;                
			ERR_FAIL_COND_MSG(param.size() != 2, code_file_path + L": params格式錯誤[name,value,min,max]請檢查 " + String::num_int64(i) + ":" + params_list[i]);
        }

        Dictionary param_dict;
        param_dict["arg_name"] = param[0];
        param_dict["value"] = param[1].to_float();
        param_dict["min"] = param[2].to_float();
        param_dict["max"] = param[3].to_float();

        param_dict["name"] = String("shader_param/") + param[0];
        param_dict["type"] = Variant::FLOAT;
        param_dict["hint"] = PROPERTY_HINT_RANGE;
        param_dict["usage"] = PROPERTY_USAGE_DEFAULT;
        param_dict["hint_string"] = param[2] + "," + param[3] + ",0.01";
        params.push_back(param_dict);
    }
	if (params.size() > 32) {
		is_error = true;
		ERR_FAIL_COND_MSG(params.size() > 32, code_file_path + L": params數量過多,最多32個");
	}

    if(process_shader_file.is_null()) {
        process_shader_file.instantiate();
    }

    //"uniform float time : hint_range(0.0, 10.0);"
    String params_str;
    for(int i=0;i<params.size();i++) {
		Dictionary param_dict = params[i];
        params_str += "#define " + (String)param_dict["arg_name"] + " " + "(blendparameters.arg[" + String::num_int64(i) + "])" + "\n";
    }



    String file_txt = template_shader->get_process_shader_code();
    file_txt = file_txt.replace("//@BLEND_PARAMETER_RENAME",params_str);
    file_txt = file_txt.replace("//@BLEND_FUNCTION",function_code);
    file_txt = file_txt.replace("//@BLEND_CODE",process_code);
    String base_path = template_shader->get_process_file_path().get_base_dir();

    err = process_shader_file->parse_versions_from_text(file_txt
        , "#define PROCESS_SHADER"
        , _include_function, &base_path);

    if (err != OK) {
        is_error = true;
        process_shader_file->print_errors(code_file_path);
    }

    file_txt = template_shader->get_priview_shader_code();
    params_str = "";
    for(int i=0;i<params.size();i++) {
		Dictionary param_dict = params[i];
        params_str += "uniform float " + (String)param_dict["arg_name"] + " : hint_range(" + String::num_int64(param_dict["min"]) + ", " + String::num_int64(param_dict["max"]) + ") = " + String::num_int64(param_dict["value"])  + "\n";
    }
    file_txt = file_txt.replace("//@BLEND_PARAMETER_RENAME",params_str);
    file_txt = file_txt.replace("//@BLEND_FUNCTION",function_code);
    file_txt = file_txt.replace("//@BLEND_CODE",process_code);
    err = process_shader_file->parse_versions_from_text(file_txt
        , "#define PRIVATE_SHADER"
        , _include_function, &base_path);
    if (err != OK) {
        is_error = true;
        process_shader_file->print_errors(code_file_path);
    }
    if(priview_shader.is_null()) {
        priview_shader.instantiate();
    }
    // priview_shader->set_path(p_template_shader->get_priview_file_path());
	priview_shader->set_include_path(template_shader->get_priview_file_path().get_base_dir());
	priview_shader->set_code(file_txt);

	emit_signal(CoreStringName(changed));

}
HeightMapProcessShader::~HeightMapProcessShader() {
    if(template_shader.is_valid()) {
        template_shader->disconnect(CoreStringName(changed),callable_mp(this, &HeightMapProcessShader::on_template_changed));
    }
    template_shader = nullptr;
}
#endif
