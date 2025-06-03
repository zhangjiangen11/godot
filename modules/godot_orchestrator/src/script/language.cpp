// This file is part of the Godot Orchestrator project.
//
// Copyright (c) 2023-present Crater Crash Studios LLC and its contributors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "script/language.h"

#include "common/dictionary_utils.h"
#include "common/logger.h"
#include "common/settings.h"
#include "common/string_utils.h"
#include "core/config/engine.h"
#include "core/debugger/engine_debugger.h"
#include "script/script.h"
#include "script/vm/script_vm.h"
#include "utility_functions.h"
#if GODOT_VERSION >= 0x040300
  #include "core/os/os.h"
#endif
#ifdef TOOLS_ENABLED
  #include "core/os/mutex.h"
#endif

OScriptLanguage* OScriptLanguage::_singleton = nullptr;

OScriptLanguage::OScriptLanguage()
{
    _singleton = this;
    lock.instantiate();
}

OScriptLanguage::~OScriptLanguage()
{
    _singleton = nullptr;

#if GODOT_VERSION >= 0x040300
    if (_call_stack)
    {
        memdelete_arr(_call_stack);
        _call_stack = nullptr;
    }
#endif
}

OScriptLanguage* OScriptLanguage::get_singleton()
{
    return _singleton;
}

void OScriptLanguage::init()
{
    // Logger::info("Initializing OrchestratorScript");

    OrchestratorSettings* settings = OrchestratorSettings::get_singleton();
    if (settings)
    {
        const String format = settings->get_setting("settings/storage_format", "Text");
        if (format.match("Binary"))
            _extension = ORCHESTRATOR_SCRIPT_EXTENSION;
    }

#if GODOT_VERSION >= 0x040300
    if (EngineDebugger::get_singleton()->is_active())
    {
        OrchestratorSettings* os = OrchestratorSettings::get_singleton();
        int max_call_stack = os->get_setting("settings/runtime/max_call_stack", 1024);
        _debug_max_call_stack = max_call_stack;
        _call_stack = memnew_arr(CallStack, _debug_max_call_stack + 1);
    }
    else
    {
        _debug_max_call_stack = 0;
        _call_stack = nullptr;
    }
#endif
}

String OScriptLanguage::get_name() const
{
    return TYPE;
}

String OScriptLanguage::get_type() const
{
    return TYPE;
}

String OScriptLanguage::get_extension() const
{
    return _extension;
}

void OScriptLanguage::get_recognized_extensions(List<String>* p_extensions) const
{
    p_extensions->push_back(ORCHESTRATOR_SCRIPT_EXTENSION);
    p_extensions->push_back(ORCHESTRATOR_SCRIPT_TEXT_EXTENSION);
}

bool OScriptLanguage::can_inherit_from_file() const
{
    return true;
}

bool OScriptLanguage::supports_builtin_mode() const
{
    return true;
}

bool OScriptLanguage::supports_documentation() const
{
    return false;
}

bool OScriptLanguage::is_using_templates()
{
    return true;
}

Vector<ScriptTemplate> OScriptLanguage::get_built_in_templates(const StringName& p_object) override;
{
    ScriptTemplate.inherit = p_object;
    ScriptTemplate.name = "Orchestration";
    ScriptTemplate.description = "Basic Orchestration";
    ScriptTemplate.content = "";
    ScriptTemplate.id = 0;
    ScriptTemplate.origin = 0;  // built-in
    Vector<ScriptTemplate> data;
    data.push_back(ScriptTemplate);
    return data;
}

Ref<Script> OScriptLanguage::_make_template(const String& p_template, const String& p_class_name,
                                            const String& p_base_class_name) const
{
    // NOTE:
    // The p_template argument is the content of the template, set in _get_built_in_templates.
    // Even if the user deselects the template option in the script dialog, this method is called.
    //
    // The p_class_name is derived from the file name.
    // The p_base_class_name is the actor/class type the script inherits from.
    //
    Ref<OScript> script;
    script.instantiate();

    // Set the script's base actor/class type
    script->set_base_type(p_base_class_name);

    // All orchestrator scripts start with an "EventGraph" graph definition.
    script->create_graph("EventGraph", OScriptGraph::GF_EVENT);

    return script;
}

bool OScriptLanguage::overrides_external_editor()
{
    return true;
}

Error OScriptLanguage::open_in_external_editor(const Ref<Script>& p_script, int32_t p_line, int32_t p_column)
{
    // We don't currently support this but return OK to avoid editor errors.
    return OK;
}

String OScriptLanguage::validate_path(const String& p_path) const
{
    // This is primarily used by the CScriptScript module so that the base filename of a C#
    // file, aka the class name, does not clash with any reserved words as that is not a
    // valid combination. For GDScript and for us, returning "" means that things are okay.
    return "";
}

bool OScriptLanguage::validate(const String& p_script, const String& p_path, List<String>* r_functions,
                               List<ScriptLanguage::ScriptError>* r_errors, List<ScriptLanguage::Warning>* r_warnings,
                               HashSet<int>* r_safe_lines) const
{
    return true;
}

Script* OScriptLanguage::create_script() const
{
    // todo: this does not appear to be called in Godot.

    OScript* script = memnew(OScript);
    script->set_base_type(OrchestratorSettings::get_singleton()->get_setting("settings/default_type", "Node"));
    // All orchestrator scripts start with an "EventGraph" graph definition.
    script->create_graph("EventGraph", OScriptGraph::GF_EVENT);
    return script;
}

Vector<String> OScriptLanguage::get_comment_delimiters() const
{
    // We don't support any comments
    return {};
}

Vector<String> OScriptLanguage::get_string_delimiters() const
{
    // We don't support any string/line delimiters
    return {};
}

Vector<String> OScriptLanguage::get_reserved_words() const
{
    // We don't support reserved keywords
    return {};
}

bool OScriptLanguage::_has_named_classes() const
{
    return false;
}

bool OScriptLanguage::is_control_flow_keyword(const String& p_keyword) const
{
    return false;
}

void OScriptLanguage::add_global_constant(const StringName& p_name, const Variant& p_value)
{
    _global_constants[p_name] = p_value;
}

void OScriptLanguage::add_named_global_constant(const StringName& p_name, const Variant& p_value)
{
    _named_global_constants[p_name] = p_value;
}

void OScriptLanguage::remove_named_global_constant(const StringName& p_name)
{
    _named_global_constants.erase(p_name);
}

int32_t OScriptLanguage::find_function(const String& p_function_name, const String& p_code) const
{
    // Locates the function name in the specified code.
    // For visual scripts, we can't use this.
    return -1;
}

String OScriptLanguage::make_function(const String& p_class_name, const String& p_function_name,
                                      const PackedStringArray& p_function_args) const
{
    // Creates a function stub for the given name.
    // This is called by the ScriptTextEditor::add_callback
    // Since we don't use the ScriptTextEditor, this doesn't apply.
    return {};
}

bool OScriptLanguage::can_make_function() const
{
    return true;
}

void OScriptLanguage::get_public_functions(List<MethodInfo>* p_functions) const
{
    TypedArray<Dictionary> results;
    for (const StringName& function : OScriptUtilityFunctions::get_function_list())
    {
        const MethodInfo method = OScriptUtilityFunctions::get_function_info(function);
        results.push_back(DictionaryUtils::from_method(method));
    }
}

void OScriptLanguage::get_public_constants(List<Pair<String, Variant>>* p_constants) const
{
    // This includes things like PI, TAU, INF, and NAN.
    // Orchestrator does not have anything beyond standard Godot.
}

void OScriptLanguage::get_public_annotations(List<MethodInfo>* p_annotations) const
{
    // Returns list of annotation MethodInfo values.
    // Orchestrator does not have any.
}

void OScriptLanguage::auto_indent_code(String& p_code, int p_from_line, int p_to_line) const override
{
    // Called by the Script -> Edit -> Indentation -> Auto Indent option
}

#ifdef TOOLS_ENABLED
Error OScriptLanguage::lookup_code(const String& p_code, const String& p_symbol, const String& p_path, Object* p_owner,
                                   LookupResult& r_result) override
#endif
{
    return OK;
}

Error OScriptLanguage::complete_code(const String& p_code, const String& p_path, Object* p_owner,
                                     List<ScriptLanguage::CodeCompletionOption>* r_options, bool& r_forced,
                                     String& r_call_hint) override
{
    return OK;
}

void OScriptLanguage::reload_all_scripts()
{
#ifdef TOOLS_ENABLED
    List<Ref<OScript>> scripts = get_scripts();
    for (Ref<OScript>& script : scripts)
        script->reload(false);
#endif
}

void OScriptLanguage::reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload)
{
#ifdef TOOLS_ENABLED
    ERR_PRINT("Tool script reloading is not yet implemented");
#endif
}

void OScriptLanguage::thread_enter()
{
    // Notifies when thread is created
}

void OScriptLanguage::thread_exit()
{
    // Notifies when thread ends
}

void OScriptLanguage::profiling_start()
{
}

void OScriptLanguage::profiling_stop()
{
}

void OScriptLanguage::frame()
{
}

void OScriptLanguage::finish()
{
}

String OScriptLanguage::debug_get_stack_level_source(int32_t p_level) const
{
    if (_debug_parse_err_line >= 0)
        return _debug_parse_err_file;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});
    int l = _debug_call_stack_pos - p_level - 1;
    return _call_stack[l].instance->get_script()->get_path();
}

int32_t OScriptLanguage::debug_get_stack_level_line(int32_t p_level) const
{
    if (_debug_parse_err_line >= 0)
        return _debug_parse_err_line;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, -1);
    int l = _debug_call_stack_pos - p_level - 1;
    return *_call_stack[l].id;
}

String OScriptLanguage::debug_get_stack_level_function(int32_t p_level) const
{
    if (_debug_parse_err_line >= 0)
        return {};

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});
    int l = _debug_call_stack_pos - p_level - 1;
    return *(_call_stack[l].current_function);
}

/**
 * @brief Returns the instance of the script at the given level.
 * @param p_level The level in the call stack.
 * @return The instance of the script at the given level.
 */
ScriptInstance* OScriptLanguage::debug_get_stack_level_instance(int32_t p_level)
{
    if (_debug_parse_err_line >= 0)
        return nullptr;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, nullptr);
    int l = _debug_call_stack_pos - p_level - 1;
    return _call_stack[l].instance->_script_instance;
}

void OScriptLanguage::debug_get_stack_level_membersdebug_get_stack_level_members(
    int p_level, List<String>* p_members, List<Variant>* p_values, int p_max_subitems, int p_max_depth)
{
    if (_debug_parse_err_line >= 0)
        return;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});
    int l = _debug_call_stack_pos - p_level - 1;

    Ref<OScript> script = _call_stack[l].instance->get_script();
    if (!script.is_valid())
        return;

    List<String>& member_names = *p_members;
    List<Variant>& member_values = *p_values;

    for (const String& variable_name : script->get_variable_names())
    {
        Variant value;
        if (_call_stack[l].instance->get_variable(variable_name, value))
        {
            member_names.push_back("Variables/" + variable_name);
            member_values.push_back(value);
        }
    }
}

void OScriptLanguage::debug_get_stack_level_locals(int p_level, List<String>* p_locals, List<Variant>* p_values,
                                                   int p_max_subitems, int p_max_depth)
{
    if (_debug_parse_err_line >= 0)
        return;

    ERR_FAIL_INDEX_V(p_level, _debug_call_stack_pos, {});

    int l = _debug_call_stack_pos - p_level - 1;
    const StringName* function_name = _call_stack[l].current_function;
    ERR_FAIL_COND_V(!_call_stack[l].instance->_vm._functions.has(*function_name), {});

    OScriptNodeInstance* node = _call_stack[l].instance->_vm._nodes[*_call_stack[l].id];
    ERR_FAIL_COND_V(!node, {});

    List<String>& local_names = *p_locals;
    List<Variant>& local_values = *p_values;

    local_names.push_back("Script Node Name");
    local_values.push_back(node->get_base_node()->get_node_title());
    local_names.push_back("Script Node ID");
    local_values.push_back(node->get_base_node()->get_id());
    local_names.push_back("Script Node Type");
    local_values.push_back(node->get_base_node()->get_class());

    int offset = 0;
    for (int i = 0; i < node->input_pin_count; i++)
    {
        Ref<OScriptNodePin> pin = node->get_base_node()->find_pin(i, PD_Input);
        if (pin->is_execution())
        {
            offset++;
            continue;
        }

        if (!pin->get_label().is_empty())
            local_names.push_back("Inputs/" + pin->get_label());
        else
            local_names.push_back("Inputs/" + pin->get_pin_name());

        int in_from = node->input_pins[i - offset];
        int in_value = in_from & OScriptNodeInstance::INPUT_MASK;
        if (in_from & OScriptNodeInstance::INPUT_DEFAULT_VALUE_BIT)
            local_values.push_back(_call_stack[l].instance->_vm._default_values[in_value]);
        else
            local_values.push_back(_call_stack[l].stack[in_value]);
    }

    offset = 0;
    for (int i = 0; i < node->output_pin_count; i++)
    {
        Ref<OScriptNodePin> pin = node->get_base_node()->find_pin(i, PD_Output);
        if (pin->is_execution())
        {
            offset++;
            continue;
        }

        if (!pin->get_label().is_empty())
            local_names.push_back("Outputs/" + pin->get_label());
        else
            local_names.push_back("Outputs/" + pin->get_pin_name());

        int out = node->output_pins[i - offset];
        local_values.push_back(_call_stack[l].stack[out]);
    }
}

void OScriptLanguage::debug_get_globals(List<String>* p_globals, List<Variant>* p_values, int p_max_subitems,
                                        int p_max_depth)
{
    *p_globals = get_global_constant_names();

    for (const String& name : get_global_constant_names())
        p_values->push_back(get_any_global_constant(name));
}

String OScriptLanguage::debug_get_error() const
{
    return _debug_error;
}

int32_t OScriptLanguage::debug_get_stack_level_count() const
{
    if (_debug_parse_err_line >= 0)
        return 1;

    return _debug_call_stack_pos;
}

Vector<StackInfo> OScriptLanguage::debug_get_current_stack_info()
{
    Vector<StackInfo> array;
    for (int i = 0; i < _debug_call_stack_pos; i++)
    {
        array.write[i].file = _call_stack[i].instance->get_script()->get_path();
        array.write[i].func = *_call_stack[i].current_function;
        array.write[i].line = *_call_stack[i].id;
    }
    return array;
}

bool OScriptLanguage::handles_global_class_type(const String& p_type) const
{
    return p_type == _get_type();
}

virtual String OScriptLanguage::get_global_class_name(const String& p_path, String* r_base_type, String* r_icon_path,
                                                      bool* r_is_abstract, bool* r_is_tool) const
{
    // OrchestratorScripts do not have global class names
    String class_name;
    return class_name;
}

#ifdef TOOLS_ENABLED
List<Ref<OScript>> OScriptLanguage::get_scripts() const
{
    List<Ref<OScript>> scripts;
    {
        const PackedStringArray extensions = _get_recognized_extensions();

        MutexLock mutex_lock(*this->lock.ptr());
        const SelfList<OScript>* iterator = _scripts.first();
        while (iterator)
        {
            String path = iterator->self()->get_path();
            if (extensions.has(path.get_extension().to_lower()))
                scripts.push_back(Ref<OScript>(iterator->self()));

            iterator = iterator->next();
        }
    }
    return scripts;
}
#endif

bool OScriptLanguage::has_any_global_constant(const StringName& p_name) const
{
    return _named_global_constants.has(p_name) || _global_constants.has(p_name);
}

Variant OScriptLanguage::get_any_global_constant(const StringName& p_name)
{
    if (_named_global_constants.has(p_name))
        return _named_global_constants[p_name];

    if (_global_constants.has(p_name))
        return _global_constants[p_name];

    return Variant();
}

List<String> OScriptLanguage::get_global_constant_names() const
{
    List<String> keys;
    for (const KeyValue<StringName, Variant>& E : _named_global_constants)
        if (!keys.has(E.key))
            keys.push_back(E.key);

    for (const KeyValue<StringName, Variant>& E : _global_constants)
        if (!keys.has(E.key))
            keys.push_back(E.key);

    return keys;
}

bool OScriptLanguage::debug_break(const String& p_error, bool p_allow_continue)
{
    if (EngineDebugger::get_singleton()->is_active())
    {
        if (Thread::get_caller_id() == ::Thread::get_main_id())
        {
            _debug_parse_err_line = -1;
            _debug_parse_err_file = "";
            _debug_error = p_error;

            EngineDebugger::get_singleton()->script_debug(this, p_allow_continue, true);
            return true;
        }
    }
    return false;
}

bool OScriptLanguage::debug_break_parse(const String& p_file, int p_node, const String& p_error)
{
    if (EngineDebugger::get_singleton()->is_active())
    {
        if (Thread::get_caller_id() == ::Thread::get_main_id())
        {
            _debug_parse_err_line = p_node;
            _debug_parse_err_file = p_file;
            _debug_error = p_error;

            EngineDebugger::get_singleton()->script_debug(this, false, true);
            return true;
        }
    }
    return false;
}

void OScriptLanguage::function_entry(const StringName* p_method, const OScriptExecutionContext* p_context)
{
    // Debugging can only happen within main thread
    if (Thread::get_caller_id() != ::Thread::get_main_id())
        return;

    EngineDebugger* debugger = EngineDebugger::get_singleton();
    if (!debugger || !debugger->is_active())
        return;

    if (debugger->get_lines_left() > 0 && debugger->get_depth() >= 0)
        debugger->set_depth(debugger->get_depth() + 1);

    if (_debug_call_stack_pos >= _debug_max_call_stack)
    {
        // Stack overflow
        _debug_error = vformat("Stack overflow detected (stack size: %s)", _debug_max_call_stack);
        debugger->script_debug(this, false, false);
        return;
    }

    Variant* ptr = p_context->_working_memory;
    _call_stack[_debug_call_stack_pos].stack = reinterpret_cast<Variant*>(p_context->_stack);
    _call_stack[_debug_call_stack_pos].instance = p_context->_script_instance;
    _call_stack[_debug_call_stack_pos].current_function = p_method;
    _call_stack[_debug_call_stack_pos].working_memory = &ptr;
    _call_stack[_debug_call_stack_pos].id = const_cast<int*>(p_context->get_current_node_ref());
    _debug_call_stack_pos++;
}

void OScriptLanguage::function_exit(const StringName* p_method, const OScriptExecutionContext* p_context)
{
    // Debugging can only happen within main thread
    if (Thread::get_caller_id() != ::Thread::get_main_id())
        return;

    EngineDebugger* debugger = EngineDebugger::get_singleton();
    if (!debugger || !debugger->is_active())
        return;

    if (debugger->get_lines_left() > 0 && debugger->get_depth() >= 0)
        debugger->set_depth(debugger->get_depth() - 1);

    if (_debug_call_stack_pos == 0)
    {
        // Stack underflow
        _debug_error = "Stack underflow detected";
        debugger->script_debug(this, false, false);
        return;
    }

    if (_call_stack[_debug_call_stack_pos - 1].instance != p_context->_script_instance
        || *_call_stack[_debug_call_stack_pos - 1].current_function != *p_method)
    {
        // Function mismatch
        _debug_error = "Function mismatch detected";
        debugger->script_debug(this, false, false);
        return;
    }

    _debug_call_stack_pos--;
}

String OScriptLanguage::get_script_extension_filter() const
{
    PackedStringArray results;
    for (const String& extension : _get_recognized_extensions())
        results.push_back(vformat("*.%s", extension));

    return StringUtils::join(",", results);
}
