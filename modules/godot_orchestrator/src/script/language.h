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
#ifndef ORCHESTRATOR_SCRIPT_LANGUAGE_H
#define ORCHESTRATOR_SCRIPT_LANGUAGE_H

#include "common/logger.h"
#include "common/version.h"
#include "core/object/script_language.h"
#include "core/object/script_language_extension.h"
#include "core/os/mutex.h"
#include "core/string/string_name.h"
#include "core/templates/hash_map.h"
#include "core/templates/self_list.h"
#include "script/serialization/format_defs.h"

//

/// Forward declarations
class Orchestration;
class OScript;
class OScriptExecutionContext;
class OScriptInstance;
class OScriptNode;
class OScriptVirtualMachine;

/// Defines an extension for Godot where we define the language for Orchestrations.
class OScriptLanguage : public ScriptLanguageExtension
{
    GDCLASS(OScriptLanguage, ScriptLanguageExtension);

protected:
    static void _bind_methods() { }

private:
#if GODOT_VERSION >= 0x040300
    struct CallStack
    {
        Variant* stack{ nullptr };
        Variant** working_memory{ nullptr };
        const StringName* current_function{ nullptr };
        OScriptInstance* instance{ nullptr };
        int* id{ nullptr };
    };
#endif

    static OScriptLanguage* _singleton;                       //! The one and only instance
    SelfList<OScript>::List _scripts;                         //! all loaded scripts
    HashMap<StringName, Variant> _global_constants;           //! Stores global constants
    HashMap<StringName, Variant> _named_global_constants;     //! Stores named global constants
    String _extension{ ORCHESTRATOR_SCRIPT_TEXT_EXTENSION };  //! The language's extension

#if GODOT_VERSION >= 0x040300
    int _debug_parse_err_line{ -1 };    //! The line number of the parse error
    String _debug_parse_err_file;       //! The script file name of the parse error
    String _debug_error;                //! The error message
    int _debug_call_stack_pos{ 0 };     //! The current call stack position
    int _debug_max_call_stack{ 0 };     //! The maximum call stack size
    CallStack* _call_stack{ nullptr };  //! The call stack
#endif

public:
    /// Public lock used for specific synchronizing use cases.
    Ref<Mutex> lock;

    /// The language's type
    static inline const char* TYPE = "Orchestrator";

    /// The language's default icon
    static inline const char* ICON = "res://addons/orchestrator/icons/Orchestrator_16x16.png";

    /// Get the singleton instance for the language.
    /// @return the language instance
    static OScriptLanguage* get_singleton();

    /// Constructs the OScriptLanguage instance, assigning the singleton.
    OScriptLanguage();

    /// Destroys the OScriptLanguage instance, clearing the singleton reference.
    ~OScriptLanguage() override;

    //~ Begin ScriptLanguageExtension Interface
    void init() override;
    String get_name() const override;
    String get_type() const override;
    String get_extension() const override;
    void get_recognized_extensions(List<String>* p_extensions) const override;
    bool can_inherit_from_file() const override;
    bool supports_builtin_mode() const override;
    bool supports_documentation() const override;
    bool is_using_templates() override;
    virtual Vector<ScriptTemplate> get_built_in_templates(const StringName& p_object) override;
    Ref<Script> make_template(const String& p_template, const String& p_class_name,
                              const String& p_base_class_name) const override;
    bool overrides_external_editor() override;
    Error open_in_external_editor(const Ref<Script>& p_script, int32_t p_line, int32_t p_column) override;
    String validate_path(const String& p_path) const override;
    virtual bool validate(const String& p_script, const String& p_path = "", List<String>* r_functions = nullptr,
                          List<ScriptLanguage::ScriptError>* r_errors = nullptr,
                          List<ScriptLanguage::Warning>* r_warnings = nullptr,
                          HashSet<int>* r_safe_lines = nullptr) const override;

    Script* create_script() const override;
    Vector<String> get_comment_delimiters() const override;
    Vector<String> get_string_delimiters() const override;
    Vector<String> get_reserved_words() const override;
    bool has_named_classes() const override;
    bool is_control_flow_keyword(const String& p_keyword) const override;
    void add_global_constant(const StringName& p_name, const Variant& p_value) override;
    void add_named_global_constant(const StringName& p_name, const Variant& p_value) override;
    void remove_named_global_constant(const StringName& p_name) override;
    int32_t find_function(const String& p_function_name, const String& p_code) const override;
    String make_function(const String& p_class_name, const String& p_function_name,
                         const PackedStringArray& p_function_args) const override;
    bool can_make_function() const override;
    virtual void get_public_functions(List<MethodInfo>* p_functions) const override;
    virtual void get_public_constants(List<Pair<String, Variant>>* p_constants) const override;
    virtual void get_public_annotations(List<MethodInfo>* p_annotations) const override;
    virtual void auto_indent_code(String& p_code, int p_from_line, int p_to_line) const override;
#ifdef TOOLS_ENABLED
    virtual Error lookup_code(const String& p_code, const String& p_symbol, const String& p_path, Object* p_owner,
                              LookupResult& r_result) override;
#endif
    virtual Error complete_code(const String& p_code, const String& p_path, Object* p_owner,
                                List<ScriptLanguage::CodeCompletionOption>* r_options, bool& r_forced,
                                String& r_call_hint) override;
    void reload_all_scripts() override;
    void reload_tool_script(const Ref<Script>& p_script, bool p_soft_reload) override;
    void thread_enter() override;
    void thread_exit() override;
    void profiling_start() override;
    void profiling_stop() override;
    void frame() override;
    void finish() override;
    String debug_get_stack_level_source(int32_t p_level) const override;
    int32_t debug_get_stack_level_line(int32_t p_level) const override;
    String debug_get_stack_level_function(int32_t p_level) const override;
    ScriptInstance* debug_get_stack_level_instance(int32_t p_level) override;
    virtual void debug_get_stack_level_members(int p_level, List<String>* p_members, List<Variant>* p_values,
                                               int p_max_subitems = -1, int p_max_depth = -1) override;
    virtual void debug_get_globals(List<String>* p_globals, List<Variant>* p_values, int p_max_subitems = -1,
                                   int p_max_depth = -1) override;
    String debug_get_error() const override;
    int32_t debug_get_stack_level_count() const override;
    Vector<StackInfo> debug_get_current_stack_info() override;
    bool handles_global_class_type(const String& p_type) const override;
    virtual String get_global_class_name(const String& p_path, String* r_base_type = nullptr,
                                         String* r_icon_path = nullptr, bool* r_is_abstract = nullptr,
                                         bool* r_is_tool = nullptr) const override;
    //~ End ScriptLanguageExtension Interface

    bool has_any_global_constant(const StringName& p_name) const;
    Variant get_any_global_constant(const StringName& p_name);
    List<String> get_global_constant_names() const;

    // Debugging
    bool debug_break(const String& p_error, bool p_allow_continue);
    bool debug_break_parse(const String& p_file, int p_node, const String& p_error);
#if GODOT_VERSION >= 0x040300
    void function_entry(const StringName* p_method, const OScriptExecutionContext* p_context);
    void function_exit(const StringName* p_method, const OScriptExecutionContext* p_context);
#endif

    String get_script_extension_filter() const;

#ifdef TOOLS_ENABLED
    /// Get a list of all orchestration scripts
    /// @return list of references
    List<Ref<OScript>> get_scripts() const;
#endif
};

#endif  // ORCHESTRATOR_SCRIPT_LANGUAGE_H
