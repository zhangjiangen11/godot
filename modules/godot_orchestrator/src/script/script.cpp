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
#include "script/script.h"

#include "common/dictionary_utils.h"
#include "core/config/engine.h"
#include "core/debugger/engine_debugger.h"
#include "core/os/mutex.h"
#include "script/instances/script_instance.h"
#include "script/instances/script_instance_placeholder.h"
#include "script/nodes/script_nodes.h"

OScript::OScript()
    : Orchestration(this, OT_Script)
    , _valid(true)
    , _language(OScriptLanguage::get_singleton())
{
}

void OScript::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("_set_base_type", "p_base_type"), &OScript::set_base_type);
    ClassDB::bind_method(D_METHOD("_get_base_type"), &OScript::get_base_type);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_type", PROPERTY_HINT_TYPE_STRING, "Node"), "_set_base_type",
                 "_get_base_type");

    // Purposely hidden until tested
    ClassDB::bind_method(D_METHOD("set_tool", "p_tool"), &OScript::set_tool);
    ClassDB::bind_method(D_METHOD("get_tool"), &OScript::get_tool);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tool", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_tool",
                 "get_tool");

    ClassDB::bind_method(D_METHOD("_set_variables", "variables"), &OScript::_set_variables);
    ClassDB::bind_method(D_METHOD("_get_variables"), &OScript::_get_variables);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "variables", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_variables", "_get_variables");

    ClassDB::bind_method(D_METHOD("_set_functions", "functions"), &OScript::_set_functions);
    ClassDB::bind_method(D_METHOD("_get_functions"), &OScript::_get_functions);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "functions", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_functions", "_get_functions");

    ClassDB::bind_method(D_METHOD("_set_signals", "signals"), &OScript::_set_signals);
    ClassDB::bind_method(D_METHOD("_get_signals"), &OScript::_get_signals);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "signals", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_signals", "_get_signals");

    ClassDB::bind_method(D_METHOD("_set_connections", "connections"), &OScript::_set_connections);
    ClassDB::bind_method(D_METHOD("_get_connections"), &OScript::_get_connections);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "connections", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                 "_set_connections", "_get_connections");

    ClassDB::bind_method(D_METHOD("_set_nodes", "nodes"), &OScript::_set_nodes);
    ClassDB::bind_method(D_METHOD("_get_nodes"), &OScript::_get_nodes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_nodes",
                 "_get_nodes");

    ClassDB::bind_method(D_METHOD("_set_graphs", "graphs"), &OScript::_set_graphs);
    ClassDB::bind_method(D_METHOD("_get_graphs"), &OScript::_get_graphs);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "graphs", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "_set_graphs",
                 "_get_graphs");

    ADD_SIGNAL(MethodInfo("connections_changed", PropertyInfo(Variant::STRING, "caller")));
    ADD_SIGNAL(MethodInfo("functions_changed"));
    ADD_SIGNAL(MethodInfo("variables_changed"));
    ADD_SIGNAL(MethodInfo("signals_changed"));
}

/// ScriptExtension ////////////////////////////////////////////////////////////////////////////////////////////////////

bool OScript::_editor_can_reload_from_file()
{
    return true;
}

PlaceHolderScriptInstance* OScript::placeholder_instance_create(Object* p_object)
{
#ifdef TOOLS_ENABLED
    Ref<Script> script(this);
    OScriptPlaceHolderInstance* psi = memnew(OScriptPlaceHolderInstance(script, p_object));
    {
        MutexLock lock(*_language->lock.ptr());
        _placeholders[p_object->get_instance_id()] = psi;
    }
    psi->_script_instance = GDEXTENSION_SCRIPT_INSTANCE_CREATE(&OScriptPlaceHolderInstance::INSTANCE_INFO, psi);
    _update_exports_placeholder(nullptr, false, psi);
    return psi->_script_instance;
#else
    return nullptr;
#endif
}

void OScript::placeholder_erased(PlaceHolderScriptInstance* p_placeholder)
{
    for (const KeyValue<uint64_t, OScriptPlaceHolderInstance*>& E : _placeholders)
    {
        if (E.value == p_placeholder)
        {
            _placeholders.erase(E.key);
            break;
        }
    }
}

bool OScript::is_placeholder_fallback_enabled() const
{
    return _placeholder_fallback_enabled;
}

bool OScript::placeholder_has(Object* p_object) const
{
    return _placeholders.has(p_object->get_instance_id());
}

ScriptInstance* OScript::instance_create(Object* p_object)
{
    if (!ClassDB::is_parent_class(p_object->get_class(), _base_type))
    {
        const String message = vformat(
            "Orchestration inherits from native type '%s', so it can't be assigned to an object of type: '%s'",
            _base_type, p_object->get_class());
        if (EngineDebugger::get_singleton()->is_active())
            OScriptLanguage::get_singleton()->debug_break_parse(get_path(), -1, message);
        ERR_FAIL_V_MSG(nullptr, message);
    }

    OScriptInstance* si = memnew(OScriptInstance(Ref<Script>(this), _language, p_object));
    {
        MutexLock lock(*_language->lock.ptr());
        _instances[p_object] = si;
    }

    si->_script_instance = GDEXTENSION_SCRIPT_INSTANCE_CREATE(&OScriptInstance::INSTANCE_INFO, si);

    // Dispatch the "Init Event" if its wired
    if (has_function("_init"))
    {
        Variant result;
        GDExtensionCallError err;
        si->call("_init", nullptr, 0, &result, &err);
    }

    return si->_script_instance;
}

bool OScript::_instance_has(const Object* p_object) const
{
    return _instances.has(p_object);
}

bool OScript::can_instantiate() const
{
    bool editor = Engine::get_singleton()->is_editor_hint();
    // Built-in script languages check if scripting is enabled OR if this is a tool script
    // Scripting is disabled by default in the editor
    return _valid && (_is_tool() || !editor);
}

Ref<Script> OScript::get_base_script() const
{
    // No inheritance

    // Base in this case infers that a script inherits from another script, not that your script
    // inherits from a super type, such as Node.
    return {};
}

bool OScript::inherits_script(const Ref<Script>& p_script) const
{
    // No inheritance
    return false;
}

StringName OScript::get_global_name() const
{
    return "";
}

StringName OScript::get_instance_base_type() const
{
    return _base_type;
}

bool OScript::has_source_code() const
{
    // No source
    return false;
}

String OScript::get_source_code() const
{
    return {};
}

void OScript::set_source_code(const String& p_code)
{
}

Error OScript::_reload(bool p_keep_state)
{
    // todo: need to find a way to reload the script when requested
    _valid = true;
    return OK;
}

Vector<DocData::ClassDoc> OScript::get_documentation() const
{
    // todo:    see how to generate it from the script/node contents
    //          see doc_data & script_language_extension
    return Vector<DocData::ClassDoc>();
}

bool OScript::has_static_method(const StringName& p_method) const
{
    // Currently we don't support static methods
    return false;
}

bool OScript::_has_method(const StringName& p_method) const
{
    return _functions.has(p_method);
}

MethodInfo OScript::_get_method_info(const StringName& p_method) const
{
    return MethodInfo();
}

void OScript::get_script_method_list(List<PropertyInfo>* p_list) const
{
    for (const KeyValue<StringName, Ref<OScriptFunction>>& E : _functions)
        p_list.push_back(E.value->_method);
}

void OScript::get_script_property_list(List<PropertyInfo>* r_list) const
{
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        if (E.value->is_exported())
            results.push_back(E.value->_info));
}

bool OScript::_is_tool() const
{
    return _tool;
}

bool OScript::_is_valid() const
{
    return _valid;
}

ScriptLanguage* OScript::_get_language() const
{
    return _language;
}

bool OScript::_has_script_signal(const StringName& p_signal) const
{
    return has_custom_signal(p_signal);
}

void OScript::get_script_signal_list(List<MethodInfo>* r_signals) const
{
    for (const KeyValue<StringName, Ref<OScriptSignal>>& E : _signals)
        r_signals.push_back(E.value->_method);
}

bool OScript::has_property_default_value(const StringName& p_property) const
{
    HashMap<StringName, Ref<OScriptVariable>>::ConstIterator E = _variables.find(p_property);
    if (E)
        return true;

    return false;
}

Variant OScript::_get_property_default_value(const StringName& p_property) const
{
    for (const KeyValue<StringName, Ref<OScriptVariable>>& E : _variables)
        if (E.key.match(p_property))
            return E.value->get_default_value();

    return {};
}

void OScript::_update_exports()
{
#ifdef TOOLS_ENABLED
    _update_exports_down(false);
#endif
}

int32_t OScript::_get_member_line(const StringName& p_member) const
{
    return -1;
}

void OScript::get_constants(HashMap<StringName, Variant>* p_constants) const
{
    return {};
}

void OScript::get_members(HashSet<StringName>* p_members)
{
}

const Variant OScript::get_rpc_config() const
{
    // Gather a dictionary of all RPC calls defined
    return Dictionary();
}

String OScript::_get_class_icon_path() const
{
    return {};
}

void OScript::_update_export_values(HashMap<StringName, Variant>& r_values, List<PropertyInfo>& r_properties) const
{
    for (const Ref<OScriptVariable>& variable : get_variables())
    {
        PropertyInfo property = variable->get_info();
        if (variable->is_grouped_by_category())
            property.name = vformat("%s/%s", variable->get_category(), variable->get_variable_name());

        r_values[property.name] = variable->get_default_value();
        r_properties.push_back(property);
    }
}

bool OScript::_update_exports_placeholder(bool* r_err, bool p_recursive_call, OScriptPlaceHolderInstance* p_instance,
                                          bool p_base_exports_changed) const
{
#ifdef TOOLS_ENABLED
    HashMap<StringName, Variant> values;
    List<PropertyInfo> properties;
    _update_export_values(values, properties);

    for (const KeyValue<uint64_t, OScriptPlaceHolderInstance*>& E : _placeholders)
        E.value->update(properties, values);

    return true;
#else
    return false;
#endif
}

void OScript::_update_exports_down(bool p_base_exports_changed)
{
    bool cyclic_error = false;
    _update_exports_placeholder(&cyclic_error, false, nullptr, p_base_exports_changed);
    // todo: add inheriters_cache
}
