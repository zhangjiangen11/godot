/**
 * editor_property_bb_param.cpp
 * =============================================================================
 * Copyright (c) 2023-present Serhii Snitsaruk and the LimboAI contributors.
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 * =============================================================================
 */

#ifdef TOOLS_ENABLED

#include "editor_property_bb_param.h"

#include "../blackboard/bb_param/bb_variant.h"
#include "../blackboard/compat/object.h"
#include "../blackboard/compat/translation.h"
#include "../blackboard/util/limbo_string_names.h"
#include "editor_property_variable_name.h"

#include "editor/editor_interface.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/menu_button.h"

Ref<BBParam> EditorPropertyBBParam::_get_edited_param() {
	Ref<BBParam> param;
	if (get_edited_object()) {
		param = get_edited_object()->get(get_edited_property());
	}
	if (param.is_null()) {
		// Create parameter resource if null.
		param = ClassDB::instantiate(param_type);
		get_edited_object()->set(get_edited_property(), param);
	}
	return param;
}

void EditorPropertyBBParam::_create_value_editor(Object *p_object, const String &p_property, Variant::Type p_type) {
	if (value_editor) {
		if (value_editor->get_meta(LW_NAME(_param_type)) == Variant(p_type)) {
			return;
		}
		_remove_value_editor();
	}

	const String hint_text = p_type == Variant::OBJECT ? "Resource" : "";
	value_editor = EditorInterface::get_singleton()->get_inspector()->instantiate_property_editor(p_object, p_type, p_property, property_hint, hint_text, PROPERTY_USAGE_EDITOR);

	bool is_bottom = false;
	switch (p_type) {
		case Variant::STRING:
		case Variant::STRING_NAME: {
			is_bottom = (property_hint == PROPERTY_HINT_MULTILINE_TEXT);
		} break;

		case Variant::NIL:
		case Variant::BOOL:
		case Variant::INT:
		case Variant::FLOAT:
		case Variant::VECTOR2:
		case Variant::VECTOR2I:
		case Variant::COLOR:
		case Variant::NODE_PATH: {
			is_bottom = false;
		} break;

		case Variant::RECT2:
		case Variant::RECT2I:
		case Variant::VECTOR3:
		case Variant::VECTOR3I:
		case Variant::VECTOR4:
		case Variant::VECTOR4I:
		case Variant::TRANSFORM2D:
		case Variant::PLANE:
		case Variant::QUATERNION:
		case Variant::AABB:
		case Variant::BASIS:
		case Variant::TRANSFORM3D:
		case Variant::PROJECTION:
		case Variant::OBJECT:
		case Variant::DICTIONARY:
		case Variant::ARRAY:
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY: {
			is_bottom = true;
		} break;

		default: {
			ERR_PRINT("Unexpected variant type!");
		}
	}

	value_editor->set_name_split_ratio(0.0);
	value_editor->set_use_folding(is_using_folding());
	value_editor->set_selectable(false);
	value_editor->set_h_size_flags(SIZE_EXPAND_FILL);
	value_editor->set_meta(LW_NAME(_param_type), p_type);
	value_editor->connect(LW_NAME(property_changed), callable_mp(this, &EditorPropertyBBParam::_value_edited));
	if (is_bottom) {
		bottom_container->add_child(value_editor);
		set_bottom_editor(bottom_container);
		bottom_container->show();
	} else {
		set_bottom_editor(nullptr);
		editor_hbox->add_child(value_editor);
		bottom_container->hide();
	}
}

void EditorPropertyBBParam::_remove_value_editor() {
	if (value_editor) {
		value_editor->get_parent()->remove_child(value_editor);
		value_editor->queue_free();
		value_editor = nullptr;
	}
}

void EditorPropertyBBParam::_value_edited(const String &p_property, Variant p_value, const String &p_name, bool p_changing) {
	_get_edited_param()->set_saved_value(p_value);
}

void EditorPropertyBBParam::_type_selected(int p_index) {
	Ref<BBParam> param = _get_edited_param();
	ERR_FAIL_COND(param.is_null());
	if (p_index == ID_BIND_VAR) {
		param->set_value_source(BBParam::BLACKBOARD_VAR);
	} else {
		param->set_value_source(BBParam::SAVED_VALUE);
		Ref<BBVariant> variant_param = param;
		if (variant_param.is_valid()) {
			variant_param->set_type(Variant::Type(p_index));
		}
	}
	update_property();
}

void EditorPropertyBBParam::_variable_edited(const String &p_property, Variant p_value, const String &p_name, bool p_changing) {
	_get_edited_param()->set_variable(p_value);
}

void EditorPropertyBBParam::update_property() {
	if (!initialized) {
		// Initialize UI -- needed after https://github.com/godotengine/godot/commit/db7175458a0532f1efe733f303ad2b55a02a52a5
		_notification(NOTIFICATION_THEME_CHANGED);
	}

	Ref<BBParam> param = _get_edited_param();

	if (param->get_value_source() == BBParam::BLACKBOARD_VAR) {
		_remove_value_editor();
		variable_editor->set_object_and_property(param.ptr(), LW_NAME(variable));
		variable_editor->setup(plan, false, param->get_variable_expected_type());
		variable_editor->update_property();
		variable_editor->show();
		bottom_container->hide();
		type_choice->set_button_icon(LimboUtility::get_singleton()->get_task_icon(LW_NAME(LimboExtraVariable)));
	} else {
		// _create_value_editor(param->get_type());
		_create_value_editor(param.ptr(), LW_NAME(saved_value), param->get_type());
		variable_editor->hide();
		value_editor->show();
		value_editor->set_object_and_property(param.ptr(), LW_NAME(saved_value));
		value_editor->update_property();
		type_choice->set_button_icon(get_theme_icon(Variant::get_type_name(param->get_type()), LW_NAME(EditorIcons)));
	}
}

void EditorPropertyBBParam::setup(PropertyHint p_hint, const String &p_hint_text, const Ref<BlackboardPlan> &p_plan) {
	param_type = p_hint_text;
	property_hint = p_hint;
	plan = p_plan;
	variable_editor->set_name_split_ratio(0.0);
}

void EditorPropertyBBParam::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED: {
			if (!get_edited_object()) {
				// Null check needed after https://github.com/godotengine/godot/commit/db7175458a0532f1efe733f303ad2b55a02a52a5
				return;
			}

			{
				String type = Variant::get_type_name(_get_edited_param()->get_type());
				type_choice->set_button_icon(get_theme_icon(type, LW_NAME(EditorIcons)));
			}

			// Initialize type choice.
			PopupMenu *type_menu = type_choice->get_popup();
			type_menu->clear();
			type_menu->add_icon_item(LimboUtility::get_singleton()->get_task_icon(LW_NAME(LimboExtraVariable)), TTR("Blackboard Variable"), ID_BIND_VAR);
			type_menu->add_separator();
			Ref<BBParam> param = _get_edited_param();
			bool is_variant_param = IS_CLASS(param, BBVariant);
			if (is_variant_param) {
				for (int i = 0; i < Variant::VARIANT_MAX; i++) {
					if (i == Variant::RID || i == Variant::CALLABLE || i == Variant::SIGNAL) {
						continue;
					}
					String type = Variant::get_type_name(Variant::Type(i));
					type_menu->add_icon_item(get_theme_icon(type, LW_NAME(EditorIcons)), type, i);
				}
			} else { // Not a variant param.
				String type = Variant::get_type_name(param->get_type());
				type_menu->add_icon_item(get_theme_icon(type, LW_NAME(EditorIcons)), type, param->get_type());
			}

			initialized = true;
		} break;
	}
}

EditorPropertyBBParam::EditorPropertyBBParam() {
	hbox = memnew(HBoxContainer);
	add_child(hbox);
	hbox->add_theme_constant_override(LW_NAME(separation), 0);

	bottom_container = memnew(MarginContainer);
	bottom_container->set_theme_type_variation("MarginContainer4px");
	add_child(bottom_container);

	type_choice = memnew(MenuButton);
	hbox->add_child(type_choice);
	type_choice->get_popup()->connect(LW_NAME(id_pressed), callable_mp(this, &EditorPropertyBBParam::_type_selected));
	type_choice->set_tooltip_text(TTR("Click to choose type"));
	type_choice->set_flat(false);

	editor_hbox = memnew(HBoxContainer);
	hbox->add_child(editor_hbox);
	editor_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
	editor_hbox->add_theme_constant_override(LW_NAME(separation), 0);

	variable_editor = memnew(EditorPropertyVariableName);
	editor_hbox->add_child(variable_editor);
	variable_editor->set_h_size_flags(SIZE_EXPAND_FILL);
	variable_editor->connect(LW_NAME(property_changed), callable_mp(this, &EditorPropertyBBParam::_variable_edited));

	param_type = LW_NAME(BBString);
}

//***** EditorInspectorPluginBBParam

bool EditorInspectorPluginBBParam::can_handle(Object *p_object) {
	return true; // Handles everything.
}

bool EditorInspectorPluginBBParam::parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide) {
	if (p_hint == PROPERTY_HINT_RESOURCE_TYPE && p_hint_text.begins_with("BB")) {
		// TODO: Add more rigid hint check.
		EditorPropertyBBParam *editor = memnew(EditorPropertyBBParam());
		editor->setup(p_hint, p_hint_text, plan_getter.call());
		add_property_editor(p_path, editor);
		return true;
	}
	return false;
}

#endif // ! TOOLS_ENABLED
