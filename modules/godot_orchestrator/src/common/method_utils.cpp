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
#include "common/method_utils.h"

#include "core/core_bind.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/variant/typed_array.h"
#include "dictionary_utils.h"
#include "property_utils.h"
#include "script/script_server.h"
#include "variant_operators.h"
#include "variant_utils.h"

namespace MethodUtils
{
    bool has_return_value(const MethodInfo& p_method)
    {
        // When the method specifies a non-NIL type, this means it isn't Variant, but
        // instead returns an explicit type.
        if (p_method.return_val.type != Variant::NIL)
            return true;

        // When the usage flag PROPERTY_USAGE_NIL_IS_VARIANT is set, the return value is Variant
        if (p_method.return_val.usage & PROPERTY_USAGE_NIL_IS_VARIANT)
            return true;

        // No return value
        return false;
    }

    void set_no_return_value(MethodInfo& p_method)
    {
        p_method.return_val.type = Variant::NIL;
        p_method.return_val.usage &= ~PROPERTY_USAGE_NIL_IS_VARIANT;
    }

    void set_return_value(MethodInfo& p_method)
    {
        if (p_method.return_val.type == Variant::NIL)
            p_method.return_val.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
        else
            p_method.return_val.usage &= ~PROPERTY_USAGE_NIL_IS_VARIANT;
    }

    void set_return_value_type(MethodInfo& p_method, Variant::Type p_type)
    {
        p_method.return_val.type = p_type;
        set_return_value(p_method);
    }

    String get_method_class(const String& p_class_name, const String& p_method_name)
    {
        String class_name = p_class_name;
        while (!class_name.is_empty())
        {
            List<MethodInfo> methods;
            if (OrchestratorScriptServer::is_global_class(p_class_name))
                methods = OrchestratorScriptServer::get_global_class(p_class_name).get_method_list(&methods);
            else
                ClassDB::get_method_list(class_name, &methods, true);

            for (auto& method : methods)
                if (p_method_name.match(method.name))
                    return class_name;
            class_name = ClassDB::get_parent_class(class_name);
        }
        return {};
    }

    String get_signature(const MethodInfo& p_method)
    {
        String signature = p_method.name.replace("_", " ").capitalize() + "\n\n";

        if (MethodUtils::has_return_value(p_method))
        {
            if (PropertyUtils::is_variant(p_method.return_val))
                signature += "Variant";
            else if (p_method.return_val.hint == PROPERTY_HINT_ARRAY_TYPE)
                signature += "Array[" + p_method.return_val.hint_string + "]";
            else
                signature += Variant::get_type_name(p_method.return_val.type);
        }
        else
            signature += "void";

        signature += " " + p_method.name + " (";

        int index = 0;
        for (const PropertyInfo& property : p_method.arguments)
        {
            if (!signature.ends_with("("))
                signature += ", ";

            if (property.name.is_empty())
                signature += "p" + itos(index++);
            else
                signature += property.name;

            signature += ":" + PropertyUtils::get_property_type_name(property);
        }

        if (p_method.flags & METHOD_FLAG_VARARG)
        {
            if (!p_method.arguments.empty())
                signature += ", ";
            signature += "...";
        }

        signature += ")";

        if (p_method.flags & METHOD_FLAG_CONST)
            signature += " const";
        else if (p_method.flags & METHOD_FLAG_VIRTUAL)
            signature += " virtual";

#if DEBUG_ENABLED
        signature += "\n\n";
        signature += vformat("%s", DictionaryUtils::from_method(p_method));
#endif

        return signature;
    }

    size_t get_argument_count_without_defaults(const MethodInfo& p_method)
    {
        return p_method.arguments.size() - p_method.default_arguments.size();
    }

    bool has_same_signature(const MethodInfo& p_method_a, const MethodInfo& p_method_b)
    {
        if (p_method_a.arguments.size() != p_method_b.arguments.size())
            return false;

        if (p_method_a.default_arguments.size() != p_method_b.default_arguments.size())
            return false;

        if (p_method_a.flags != p_method_b.flags)
            return false;

        if (p_method_a.name != p_method_b.name)
            return false;

        if (!PropertyUtils::are_equal(p_method_a.return_val, p_method_b.return_val))
            return false;

        for (size_t i = 0; i < p_method_a.arguments.size(); i++)
        {
            const PropertyInfo& a = p_method_a.arguments[i];
            const PropertyInfo& b = p_method_b.arguments[i];

            // PropertyUtils::are_equal does not compare names
            // But here we want to compare names
            if (a.name != b.name)
                return false;

            if (!PropertyUtils::are_equal(a, b))
                return false;
        }

        for (size_t i = 0; i < p_method_b.default_arguments.size(); i++)
        {
            const Variant& a = p_method_a.default_arguments[i];
            const Variant& b = p_method_b.default_arguments[i];
            if (!VariantUtils::evaluate(Variant::OP_EQUAL, a, b))
                return false;
        }

        return true;
    }
}
