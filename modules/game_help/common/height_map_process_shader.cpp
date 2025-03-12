
#if TOOLS_ENABLED
#include "height_map_process_shader.h"

void HeightMapTemplateShader::_bind_methods() {
    ClassDB::bind_method(D_METHOD("init", "process_file_path","priview_file_path"), &HeightMapTemplateShader::init);
    ClassDB::bind_method(D_METHOD("get_process_file_path"), &HeightMapTemplateShader::get_process_file_path);
    ClassDB::bind_method(D_METHOD("get_priview_file_path"), &HeightMapTemplateShader::get_priview_file_path);
    ClassDB::bind_method(D_METHOD("get_process_shader_code"), &HeightMapTemplateShader::get_process_shader_code);
    ClassDB::bind_method(D_METHOD("get_priview_shader_code"), &HeightMapTemplateShader::get_priview_shader_code);
    ClassDB::bind_method(D_METHOD("is_error"), &HeightMapTemplateShader::get_is_error);
}
void HeightMapTemplateShader::init(const String& p_process_file_path,const String& p_priview_file_path) {
    is_error = false;
    process_file_path = p_process_file_path;
    priview_file_path = p_priview_file_path;
    Error err;
    Ref<FileAccess> file = FileAccess::open(p_process_file_path, FileAccess::READ, &err);
    if(err != OK) {
        is_error = true;
		ERR_FAIL_COND_MSG(err != OK, p_process_file_path + ": file not exist");
    }
    if(file.is_null()) {
        is_error = true;
		ERR_FAIL_COND_MSG(file.is_null(), p_process_file_path + ": file not exist");
    }

    process_shader_code = file->get_as_utf8_string();


    file = FileAccess::open(p_priview_file_path, FileAccess::READ, &err);

    if(err != OK) {
        is_error = true;
		ERR_FAIL_COND_MSG(err != OK, p_priview_file_path + ": file not exist");
    }
    if(file.is_null()) {
        is_error = true;
		ERR_FAIL_COND_MSG(file.is_null(), p_priview_file_path + ": file not exist");
    }

    priview_shader_code = file->get_as_utf8_string();
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
}
void HeightMapProcessShader::init(const Ref<HeightMapTemplateShader>& p_template_shader,const String& p_code_file_path) {
    is_error = false;
    if(p_template_shader.is_null()) {
        is_error = true;
        ERR_FAIL_COND_MSG(p_template_shader.is_null(), "template shader not exist");
    }
    template_shader = p_template_shader;
    if(p_template_shader->get_is_error()) {
        is_error = true;
        ERR_FAIL_COND_MSG(p_template_shader->get_is_error(), "template shader error");
    }

    Error err;
    Ref<FileAccess> file = FileAccess::open(p_code_file_path, FileAccess::READ, &err);
	ERR_FAIL_COND_MSG(err != OK, p_code_file_path + ": file not exist");
	ERR_FAIL_COND_MSG(file.is_null(), p_code_file_path + ": file not exist");

    String code = file->get_as_utf8_string();
    if(code.is_empty()) {
        is_error = true;
		ERR_FAIL_COND_MSG(code.is_empty(), p_code_file_path + ": file not exist");
    }
    code = code.remove_char(' ').remove_char('\r').remove_char('\t');
    Vector<String> lines = code.split("$$",false);

    if(lines.size() != 4) {
        is_error = true;
		ERR_FAIL_COND_MSG(lines.size() != 4, p_code_file_path + L": 格式錯誤，請檢查，必須存在$$分隔符號，格式為[is_inv_blend_value:(true|false)]\n$$[params_a:(name,value,min,max);params_b;...]\n$$[function_code]\n$$[process_code]");
    }

    String _is_inv_blend_value = lines[0].remove_char('\n');
    String _params = lines[1];
    function_code = lines[2];
    process_code = lines[3];

    if(_is_inv_blend_value == "true" || _is_inv_blend_value == "1") {
        this->is_inv_blend_value = true;
    } else {
        this->is_inv_blend_value = false;
    }      

    Vector<String> params_list = _params.split(";",false);

    if(params_list.size()  > 32) {
        is_error = true;
		ERR_FAIL_COND_MSG(params_list.size()  > 32, p_code_file_path + L": params數量過多,最多32個");
    }

    for(int i=0;i<params_list.size();i++) {
        Vector<String> param = params_list[i].split(":",false);
        if(param.size() != 4) {
            is_error = true;                
			ERR_FAIL_COND_MSG(param.size() != 2, p_code_file_path + L": params格式錯誤[name,value,min,max]請檢查 " + String::num_int64(i) + ":" + params_list[i]);
        }

        Dictionary param_dict;
        param_dict["name"] = param[0];
        param_dict["value"] = param[1].to_float();
        param_dict["min"] = param[2].to_float();
        param_dict["max"] = param[3].to_float();
        params.push_back(param_dict);
    }


    process_shader_file.instantiate();

        //"uniform float time : hint_range(0.0, 10.0);"
    String params_str;
    for(int i=0;i<params.size();i++) {
		Dictionary param_dict = params[i];
        params_str += "#define " + (String)param_dict["name"] + " " + "(blendparameters.arg[" + String::num_int64(i) + "])" + "\n";
    }



    String file_txt = p_template_shader->get_process_shader_code();
    file_txt = file_txt.replace("//@BLEND_PARAMETER_RENAME",params_str);
    file_txt = file_txt.replace("//@BLEND_FUNCTION",function_code);
    file_txt = file_txt.replace("//@BLEND_CODE",process_code);
    String base_path = p_template_shader->get_process_file_path().get_base_dir();

    err = process_shader_file->parse_versions_from_text(file_txt
        , is_inv_blend_value ? "#define BLEND_INV_VALUE_DEFINE 1\n" : "#define BLEND_INV_VALUE_DEFINE 0\n" 
        , _include_function, &base_path);

    if (err != OK) {
        is_error = true;
        process_shader_file->print_errors(p_code_file_path);
    }

    file_txt = p_template_shader->get_priview_shader_code();
    params_str = "";
    for(int i=0;i<params.size();i++) {
		Dictionary param_dict = params[i];
        params_str += "uniform float " + (String)param_dict["name"] + " : hint_range(" + String::num_int64(param_dict["min"]) + ", " + String::num_int64(param_dict["max"]) + ") = " + String::num_int64(param_dict["value"])  + "\n";
    }
    file_txt = file_txt.replace("//@BLEND_PARAMETER_RENAME",params_str);
    file_txt = file_txt.replace("//@BLEND_FUNCTION",function_code);
    file_txt = file_txt.replace("//@BLEND_CODE",process_code);
    err = process_shader_file->parse_versions_from_text(file_txt
        , is_inv_blend_value ? "#define BLEND_INV_VALUE_DEFINE 1\n" : "#define BLEND_INV_VALUE_DEFINE 0\n" 
        , _include_function, &base_path);
    if (err != OK) {
        is_error = true;
        process_shader_file->print_errors(p_code_file_path);
    }
    if(priview_shader.is_null()) {
        priview_shader.instantiate();
    }
    // priview_shader->set_path(p_template_shader->get_priview_file_path());
	priview_shader->set_include_path(p_template_shader->get_priview_file_path().get_base_dir());
	priview_shader->set_code(file_txt);


}
#endif
