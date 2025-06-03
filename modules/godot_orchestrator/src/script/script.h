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
#ifndef ORCHESTRATOR_SCRIPT_H
#define ORCHESTRATOR_SCRIPT_H

#include "core/extension/gdextension_interface.h"
#include "core/object/script_language_extension.h"
#include "core/templates/hash_map.h"
#include "orchestration/orchestration.h"
#include "script/instances/instance_base.h"

/// Forward declarations
class OScriptInstance;
class OScriptPlaceHolderInstance;

/// Defines the script extension for Orchestrations.
///
/// An orchestration is a visual-script like graph of nodes that allows to build code visually.
/// These graphs are stored as a script that can be attached to any scene tree node and this is
/// the base class that offers that functionality.
///
class OScript : public ScriptExtension,
                public Orchestration
{
    friend class OScriptInstance;
    GDCLASS(OScript, ScriptExtension);

private:
    bool _tool{ false };                          //! Is this script marked as a tool script
    bool _valid{ false };                         //! Determines whether the script is currently valid
    bool _placeholder_fallback_enabled{ false };  //! Deals with placeholders
    OScriptLanguage* _language{ nullptr };        //! The script language

    // these are mutable because they're modified within const function callbacks
    mutable HashMap<Object*, OScriptInstance*> _instances;
    mutable HashMap<uint64_t, OScriptPlaceHolderInstance*> _placeholders;

protected:
    // Godot bindings
    static void _bind_methods();

    //~ Begin Serialization API
    StringName _get_base_type() const { return _base_type; }
    void _set_base_type(const StringName& p_base_type) { set_base_type(p_base_type); }
    TypedArray<OScriptNode> _get_nodes() const { return _get_nodes_internal(); }
    void _set_nodes(const TypedArray<OScriptNode>& p_nodes) { _set_nodes_internal(p_nodes); }
    TypedArray<int> _get_connections() const { return _get_connections_internal(); }
    void _set_connections(const TypedArray<int>& p_connections) { _set_connections_internal(p_connections); }
    TypedArray<OScriptGraph> _get_graphs() const { return _get_graphs_internal(); }
    void _set_graphs(const TypedArray<OScriptGraph>& p_graphs) { _set_graphs_internal(p_graphs); }
    TypedArray<OScriptFunction> _get_functions() const { return _get_functions_internal(); }
    void _set_functions(const TypedArray<OScriptFunction>& p_functions) { _set_functions_internal(p_functions); }
    TypedArray<OScriptVariable> _get_variables() const { return _get_variables_internal(); }
    void _set_variables(const TypedArray<OScriptVariable>& p_variables) { _set_variables_internal(p_variables); }
    TypedArray<OScriptSignal> _get_signals() const { return _get_signals_internal(); }
    void _set_signals(const TypedArray<OScriptSignal>& p_signals) { _set_signals_internal(p_signals); }
    //~ End Serialization API

    /// Updates the exported values
    /// @param r_values the exported variable values
    /// @param r_properties the exported variable property details
    void _update_export_values(HashMap<StringName, Variant>& r_values, List<PropertyInfo>& r_properties) const;

    /// Update export placeholders
    /// @param r_err the output error
    /// @param p_recursive whether called recursively
    /// @param p_instance the script instance, should never be null
    /// @param p_base_exports_changed whether base exports changed
    bool _update_exports_placeholder(bool* r_err = nullptr, bool p_recursive = false,
                                     OScriptPlaceHolderInstance* p_instance = nullptr,
                                     bool p_base_exports_changed = false) const;

    /// Updates the exports
    /// @param p_base_exports_changed whether the base class exports changed
    void _update_exports_down(bool p_base_exports_changed);

public:
    OScript();

    //~ ScriptExtension overrides
    bool editor_can_reload_from_file() override;
    PlaceHolderScriptInstance* placeholder_instance_create(Object* p_object) override;
    void _placeholder_erased(PlaceHolderScriptInstance* p_placeholder) override;
    bool is_placeholder_fallback_enabled() const override;
    bool placeholder_has(Object* p_object) const;
    ScriptInstance* instance_create(Object* p_object) override;
    bool instance_has(const Object* p_object) const override;
    bool can_instantiate() const override;
    Ref<Script> get_base_script() const override;
    bool inherits_script(const Ref<Script>& p_script) const override;
    StringName get_global_name() const override;
    StringName get_instance_base_type() const override;
    bool has_source_code() const override;
    String get_source_code() const override;
    void set_source_code(const String& p_code) override;
    Error reload(bool p_keep_state) override;
    Vector<DocData::ClassDoc> get_documentation() const override;
    bool has_static_method(const StringName& p_method) const override;
    bool has_method(const StringName& p_method) const override;
    MethodInfo get_method_info(const StringName& p_method) const override;
    void get_script_method_list(List<MethodInfo>* p_list) const override;
    void get_script_property_list(List<PropertyInfo>* r_list) const override;
    bool is_tool() const override;
    bool is_valid() const override;
    ScriptLanguage* get_language() const override;
    bool has_script_signal(const StringName& p_signal) const override;
    void get_script_signal_list(List<MethodInfo>* r_signals) const override;
    bool has_property_default_value(const StringName& p_property) const;
    bool get_property_default_value(const StringName& p_property, Variant& r_value) const override;
    void update_exports() override;
    int32_t get_member_line(const StringName& p_member) const override;
    void get_constants(HashMap<StringName, Variant>* p_constants) override;
    void get_members(HashSet<StringName>* p_members) override;
    const Variant get_rpc_config() const override;
    String get_class_icon_path() const override;
    //~ End ScriptExtension overrides

    /// Get the underlying script's language
    /// @return the script language instance

    /// Check whether the script is marked as a tool script
    /// @return true if the script is marked as tool-mode, false otherwise
    bool get_tool() const  { return is_tool(); }

    /// Set whether the script operates in tool-mode
    /// @param p_tool true sets the script to tool mode, false does not
    void set_tool(bool p_tool) { _tool = p_tool; }
};

#endif  // ORCHESTRATOR_SCRIPT_H
