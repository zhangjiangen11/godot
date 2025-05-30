/**
 * bb_param.cpp
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#include "bb_param.h"

#include "../compat/variant.h"
#include "../util/limbo_utility.h"

VARIANT_ENUM_CAST(BBParam::ValueSource);

void BBParam::set_value_source(ValueSource p_value) {
	value_source = p_value;
	notify_property_list_changed();
	_update_name();
	emit_changed();
}

Variant BBParam::get_saved_value() {
	return saved_value;
}

void BBParam::set_saved_value(Variant p_value) {
	if (p_value.get_type() == Variant::NIL) {
		_assign_default_value();
	} else {
		saved_value = p_value;
	}
	_update_name();
	emit_changed();
}

void BBParam::set_variable(const StringName &p_variable) {
	variable = p_variable;
	_update_name();
	emit_changed();
}

String BBParam::to_string() {
	if (value_source == SAVED_VALUE) {
		String s = saved_value.stringify();
		switch (get_type()) {
			case Variant::STRING: {
				s = "\"" + s.c_escape() + "\"";
			} break;
			case Variant::STRING_NAME: {
				s = "&\"" + s.c_escape() + "\"";
			} break;
			case Variant::NODE_PATH: {
				s = "^\"" + s.c_escape() + "\"";
			} break;
			default: {
			} break;
		}
		return s;
	} else {
		return LimboUtility::get_singleton()->decorate_var(variable);
	}
}

Variant BBParam::get_value(Node *p_scene_root, const Ref<Blackboard> &p_blackboard, const Variant &p_default) {
	ERR_FAIL_COND_V(!p_blackboard.is_valid(), p_default);

	if (value_source == SAVED_VALUE) {
		if (saved_value == Variant()) {
			_assign_default_value();
		}
		return saved_value;
	} else {
		ERR_FAIL_COND_V_MSG(!p_blackboard->has_var(variable), p_default, vformat("BBParam: Blackboard variable \"%s\" doesn't exist.", variable));
		return p_blackboard->get_var(variable, p_default);
	}
}

void BBParam::_assign_default_value() {
	saved_value = VARIANT_DEFAULT(get_type());
}

void BBParam::_get_property_list(List<PropertyInfo> *p_list) const {
	if (value_source == ValueSource::SAVED_VALUE) {
		p_list->push_back(PropertyInfo(get_type(), "saved_value"));
	} else {
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, "variable"));
	}
}

void BBParam::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_value_source", "value_source"), &BBParam::set_value_source);
	ClassDB::bind_method(D_METHOD("get_value_source"), &BBParam::get_value_source);
	ClassDB::bind_method(D_METHOD("set_saved_value", "value"), &BBParam::set_saved_value);
	ClassDB::bind_method(D_METHOD("get_saved_value"), &BBParam::get_saved_value);
	ClassDB::bind_method(D_METHOD("set_variable", "variable_name"), &BBParam::set_variable);
	ClassDB::bind_method(D_METHOD("get_variable"), &BBParam::get_variable);
	ClassDB::bind_method(D_METHOD("get_type"), &BBParam::get_type);
	ClassDB::bind_method(D_METHOD("get_value", "scene_root", "blackboard", "default"), &BBParam::get_value, Variant());

	ADD_PROPERTY(PropertyInfo(Variant::INT, "value_source", PROPERTY_HINT_ENUM, "Saved Value,Blackboard Var"), "set_value_source", "get_value_source");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "variable", PROPERTY_HINT_NONE, "", 0), "set_variable", "get_variable");
	ADD_PROPERTY(PropertyInfo(Variant::NIL, "saved_value", PROPERTY_HINT_NONE, "", 0), "set_saved_value", "get_saved_value");

	BIND_ENUM_CONSTANT(SAVED_VALUE);
	BIND_ENUM_CONSTANT(BLACKBOARD_VAR);
}

BBParam::BBParam() {
	value_source = SAVED_VALUE;
}
