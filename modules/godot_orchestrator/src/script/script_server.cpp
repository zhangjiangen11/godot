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
#include "script/script_server.h"

#include "common/version.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/object/script_language.h"
#include "scene/main/node.h"

TypedArray<Dictionary> OrchestratorScriptServer::GlobalClass::get_property_list() const
{
    const Ref<Script> script = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
    return script.is_valid() ? script->get_script_property_list() : TypedArray<Dictionary>();
}

void OrchestratorScriptServer::GlobalClass::get_method_list(List<MethodInfo>* p_list) const
{
    const Ref<Script> script = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
    if (script.is_valid())
        script->get_script_method_list(p_list);
}

void OrchestratorScriptServer::GlobalClass::get_signal_list(List<MethodInfo>* p_list) const
{
    const Ref<Script> script = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
    if (script.is_valid())
        script->get_script_signal_list(p_list);
}

Dictionary OrchestratorScriptServer::GlobalClass::get_constants_list() const
{
    const Ref<Script> script = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
    return script.is_valid() ? script->get_script_constant_map() : Dictionary();
}

bool OrchestratorScriptServer::GlobalClass::has_method(const StringName& p_method_name) const
{
    if (!name.is_empty() && !path.is_empty())
    {
        const TypedArray<Dictionary> methods = get_method_list();
        for (int index = 0; index < methods.size(); ++index)
        {
            const Dictionary& dict = methods[index];
            if (dict.has("name") && p_method_name.match(dict["name"]))
                return true;
        }
    }
    return false;
}

bool OrchestratorScriptServer::GlobalClass::has_property(const StringName& p_property_name) const
{
    if (!name.is_empty() && !path.is_empty())
    {
        const TypedArray<Dictionary> properties = get_property_list();
        for (int index = 0; index < properties.size(); ++index)
        {
            const Dictionary& dict = properties[index];
            if (dict.has("name") && p_property_name.match(dict["name"]))
                return true;
        }
    }
    return false;
}

bool OrchestratorScriptServer::GlobalClass::has_signal(const StringName& p_signal_name) const
{
    if (!name.is_empty() && !path.is_empty())
    {
        const TypedArray<Dictionary> signals = get_signal_list();
        for (int index = 0; index < signals.size(); ++index)
        {
            const Dictionary& dict = signals[index];
            if (dict.has("name") && p_signal_name.match(dict["name"]))
                return true;
        }
    }
    return false;
}

TypedArray<Dictionary> OrchestratorScriptServer::GlobalClass::get_static_method_list() const
{
    const TypedArray<Dictionary> methods = get_method_list();

    TypedArray<Dictionary> results;
    for (int i = 0; i < methods.size(); i++)
    {
        const Dictionary& dict = methods[i];
        const uint32_t flags = dict.get("flags", METHOD_FLAGS_DEFAULT);
        if (flags & METHOD_FLAG_STATIC)
            results.append(dict);
    }

    return results;
}

Dictionary OrchestratorScriptServer::_get_global_class(const StringName& p_class_name)
{
    TypedArray<Dictionary> global_classes = ProjectSettings::get_singleton()->get_global_class_list();
    for (int index = 0; index < global_classes.size(); ++index)
    {
        const Dictionary& entry = global_classes[index];
        if (entry.has("class") && p_class_name.match(entry["class"]))
            return entry;
    }
    return {};
}

bool OrchestratorScriptServer::is_global_class(const StringName& p_class_name)
{
    return !_get_global_class(p_class_name).is_empty();
}

bool OrchestratorScriptServer::is_parent_class(const StringName& p_source_class_name,
                                               const StringName& p_target_class_name)
{
    return get_class_hierarchy(p_source_class_name, true).has(p_target_class_name);
}

PackedStringArray OrchestratorScriptServer::get_global_class_list()
{
    const TypedArray<Dictionary> global_classes = ProjectSettings::get_singleton()->get_global_class_list();

    PackedStringArray global_class_names;
    for (int index = 0; index < global_classes.size(); ++index)
    {
        const Dictionary& entry = global_classes[index];
        if (entry.has("class"))
            global_class_names.push_back(entry["class"]);
    }
    return global_class_names;
}

OrchestratorScriptServer::GlobalClass OrchestratorScriptServer::get_global_class(const StringName& p_class_name)
{
    GlobalClass global_class;

    const Dictionary entry = _get_global_class(p_class_name);
    if (!entry.is_empty())
    {
        global_class.name = entry["class"];
        global_class.base_type = entry["base"];
        global_class.path = entry["path"];
        global_class.language = entry["language"];

        if (entry.has("icon"))
            global_class.icon_path = entry["icon"];
    }
    return global_class;
}

StringName OrchestratorScriptServer::get_native_class_name(const StringName& p_class_name)
{
    PackedStringArray hierarchy = get_class_hierarchy(p_class_name, true);
    for (const String& class_name : hierarchy)
        if (!is_global_class(class_name))
            return class_name;
    return Object::get_class_static();
}

PackedStringArray OrchestratorScriptServer::get_class_hierarchy(const StringName& p_class_name,
                                                                bool p_include_native_classes)
{
    PackedStringArray hierarchy;
    StringName class_name = p_class_name;
    while (!class_name.is_empty())
        if (is_global_class(class_name))
        {
            hierarchy.push_back(class_name);
            class_name = get_global_class(class_name).base_type;
        }
        else if (p_include_native_classes)
        {
            hierarchy.push_back(class_name);
            class_name = ClassDB::get_parent_class(class_name);
        }
        else
            break;
    return hierarchy;
}

String OrchestratorScriptServer::get_global_name(const Ref<Script>& p_script)
{
    if (p_script.is_valid())
    {
#if GODOT_VERSION >= 0x040300
        return p_script->get_global_name();
#else
        TypedArray<Dictionary> global_classes = ProjectSettings::get_singleton()->get_global_class_list();
        for (int index = 0; index < global_classes.size(); ++index)
        {
            const Dictionary& entry = global_classes[index];
            if (entry.has("path") && p_script->get_path().match(entry["path"]) && entry.has("class"))
                return entry["class"];
        }
#endif
    }
    return "";
}