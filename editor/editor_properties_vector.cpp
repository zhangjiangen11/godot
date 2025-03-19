/**************************************************************************/
/*  editor_properties_vector.cpp                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "editor_properties_vector.h"

#include "editor/editor_settings.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/texture_button.h"

const String EditorPropertyVectorN::COMPONENT_LABELS[4] = { "x", "y", "z", "w" };
static Mutex property_to_range_method_name_mutex;

static StringName get_range_method(const StringName& _property) {
	StringName ret;
	property_to_range_method_name_mutex.lock();
	static HashMap<StringName,StringName>* property_to_range_method_name = new (HashMap<StringName,StringName>);
	auto it = property_to_range_method_name->find(_property);
	if(it == property_to_range_method_name->end()) {
		StringName name = StringName(_property.str() + "__get_range_min_max__");
		property_to_range_method_name->insert(_property,name);
	}
	else {
		ret = it->value;
	}
	property_to_range_method_name_mutex.unlock();
	return ret;
}

void EditorPropertyVectorN::_set_read_only(bool p_read_only) {
	for (EditorSpinSlider *spin : spin_sliders) {
		spin->set_read_only(p_read_only);
	}
}

void EditorPropertyVectorN::_value_changed(double val, const String &p_name) {
	if (linked->is_pressed()) {
		int changed_component = -1;
		for (int i = 0; i < component_count; i++) {
			if (p_name == COMPONENT_LABELS[i]) {
				changed_component = i;
				break;
			}
		}
		DEV_ASSERT(changed_component >= 0);

		for (int i = 0; i < component_count - 1; i++) {
			int slider_idx = (changed_component + 1 + i) % component_count;
			int ratio_idx = changed_component * (component_count - 1) + i;

			if (ratio[ratio_idx] == 0) {
				continue;
			}

			spin_sliders[slider_idx]->set_value_no_signal(spin_sliders[changed_component]->get_value() * ratio[ratio_idx]);
		}
	}

	Variant v;
	Callable::CallError cerror;
	Variant::construct(vector_type, v, nullptr, 0, cerror);

	for (int i = 0; i < component_count; i++) {
		if (radians_as_degrees) {
			v.set(i, Math::deg_to_rad(spin_sliders[i]->get_value()));
		} else {
			v.set(i, spin_sliders[i]->get_value());
		}
	}
	emit_changed(get_edited_property(), v, linked->is_pressed() ? "" : p_name);
}

void EditorPropertyVectorN::update_property() {
	Variant val = get_edited_property_value();
	StringName name = get_range_method(get_edited_property());

	bool is_dynamic_range = false;
	Vector2 range;
	if(get_edited_object()->has_method(name)) {
		Variant ret = get_edited_object()->call(name);
		if(ret.get_type() == Variant::VECTOR2 ) {
			range = ret;
		}
		else {
			Vector2i rangei = ret;
			range.x = rangei.x;
			range.y = rangei.y;
		}
		is_dynamic_range = true;
	}
	for (int i = 0; i < component_count; i++) {
		if (radians_as_degrees) {
			if(is_dynamic_range) {
				spin_sliders[i]->set_min(range.x);
				spin_sliders[i]->set_max(range.y);
			}
			spin_sliders[i]->set_value_no_signal(Math::rad_to_deg((real_t)val.get(i)));
		} else {
			spin_sliders[i]->set_value_no_signal(val.get(i));
		}
	}

	if (!is_grabbed) {
		_update_ratio();
	}
}

void EditorPropertyVectorN::_update_ratio() {
	linked->set_modulate(Color(1, 1, 1, linked->is_pressed() ? 1.0 : 0.5));

	double *ratio_write = ratio.ptrw();
	for (int i = 0; i < ratio.size(); i++) {
		int base_slider_idx = i / (component_count - 1);
		int secondary_slider_idx = ((base_slider_idx + 1) + i % (component_count - 1)) % component_count;

		if (spin_sliders[base_slider_idx]->get_value() != 0) {
			ratio_write[i] = spin_sliders[secondary_slider_idx]->get_value() / spin_sliders[base_slider_idx]->get_value();
		}
	}
}

void EditorPropertyVectorN::_store_link(bool p_linked) {
	if (!get_edited_object()) {
		return;
	}
	const String key = vformat("%s:%s", get_edited_object()->get_class(), get_edited_property());
	EditorSettings::get_singleton()->set_project_metadata("linked_properties", key, p_linked);
}

void EditorPropertyVectorN::_grab_changed(bool p_grab) {
	if (p_grab) {
		_update_ratio();
	}
	is_grabbed = p_grab;
}

void EditorPropertyVectorN::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (linked->is_visible()) {
				if (get_edited_object()) {
					const String key = vformat("%s:%s", get_edited_object()->get_class(), get_edited_property());
					linked->set_pressed_no_signal(EditorSettings::get_singleton()->get_project_metadata("linked_properties", key, true));
					_update_ratio();
				}
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			int icon_size = get_theme_constant(SNAME("class_icon_size"), EditorStringName(Editor));

			linked->set_texture_normal(get_editor_theme_icon(SNAME("Unlinked")));
			linked->set_texture_pressed(get_editor_theme_icon(SNAME("Instance")));
			linked->set_custom_minimum_size(Size2(icon_size + 8 * EDSCALE, 0));

			const Color *colors = _get_property_colors();
			for (int i = 0; i < component_count; i++) {
				spin_sliders[i]->add_theme_color_override("label_color", colors[i]);
			}
		} break;
	}
}

void EditorPropertyVectorN::setup(double p_min, double p_max, double p_step, bool p_hide_slider, bool p_link, const String &p_suffix, bool p_radians_as_degrees, bool p_is_int) {
	radians_as_degrees = p_radians_as_degrees;

	for (EditorSpinSlider *spin : spin_sliders) {
		spin->set_min(p_min);
		spin->set_max(p_max);
		spin->set_step(p_step);
		spin->set_hide_slider(p_hide_slider);
		spin->set_allow_greater(true);
		spin->set_allow_lesser(true);
		spin->set_suffix(p_suffix);
		spin->set_editing_integer(p_is_int);
	}

	if (!p_link) {
		linked->hide();
	}
}

EditorPropertyVectorN::EditorPropertyVectorN(Variant::Type p_type, bool p_force_wide, bool p_horizontal) {
	vector_type = p_type;
	switch (vector_type) {
		case Variant::VECTOR2:
		case Variant::VECTOR2I:
			component_count = 2;
			break;

		case Variant::VECTOR3:
		case Variant::VECTOR3I:
			component_count = 3;
			break;

		case Variant::VECTOR4:
		case Variant::VECTOR4I:
			component_count = 4;
			break;

		default: // Needed to silence a warning.
			ERR_PRINT("Not a Vector type.");
			break;
	}
	bool horizontal = p_force_wide || p_horizontal;

	HBoxContainer *hb = memnew(HBoxContainer);
	hb->set_h_size_flags(SIZE_EXPAND_FILL);

	BoxContainer *bc;

	if (p_force_wide) {
		bc = memnew(HBoxContainer);
		hb->add_child(bc);
	} else if (horizontal) {
		bc = memnew(HBoxContainer);
		hb->add_child(bc);
		set_bottom_editor(hb);
	} else {
		bc = memnew(VBoxContainer);
		hb->add_child(bc);
	}
	bc->set_h_size_flags(SIZE_EXPAND_FILL);

	spin_sliders.resize(component_count);
	EditorSpinSlider **spin = spin_sliders.ptrw();

	for (int i = 0; i < component_count; i++) {
		spin[i] = memnew(EditorSpinSlider);
		bc->add_child(spin[i]);
		spin[i]->set_flat(true);
		spin[i]->set_label(String(COMPONENT_LABELS[i]));
		if (horizontal) {
			spin[i]->set_h_size_flags(SIZE_EXPAND_FILL);
		}
		spin[i]->connect(SceneStringName(value_changed), callable_mp(this, &EditorPropertyVectorN::_value_changed).bind(String(COMPONENT_LABELS[i])));
		spin[i]->connect(SNAME("grabbed"), callable_mp(this, &EditorPropertyVectorN::_grab_changed).bind(true));
		spin[i]->connect(SNAME("ungrabbed"), callable_mp(this, &EditorPropertyVectorN::_grab_changed).bind(false));
		add_focusable(spin[i]);
	}

	ratio.resize(component_count * (component_count - 1));
	ratio.fill(1.0);

	linked = memnew(TextureButton);
	linked->set_toggle_mode(true);
	linked->set_stretch_mode(TextureButton::STRETCH_KEEP_CENTERED);
	linked->set_tooltip_text(TTR("Lock/Unlock Component Ratio"));
	linked->connect(SceneStringName(pressed), callable_mp(this, &EditorPropertyVectorN::_update_ratio));
	linked->connect(SceneStringName(toggled), callable_mp(this, &EditorPropertyVectorN::_store_link));
	hb->add_child(linked);

	add_child(hb);
	if (!horizontal) {
		set_label_reference(spin_sliders[0]); // Show text and buttons around this.
	}
}

EditorPropertyVector2::EditorPropertyVector2(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR2, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector2_editing")) {}

EditorPropertyVector2i::EditorPropertyVector2i(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR2I, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector2_editing")) {}

EditorPropertyVector3::EditorPropertyVector3(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR3, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}

EditorPropertyVector3i::EditorPropertyVector3i(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR3I, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}

EditorPropertyVector4::EditorPropertyVector4(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR4, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}

EditorPropertyVector4i::EditorPropertyVector4i(bool p_force_wide) :
		EditorPropertyVectorN(Variant::VECTOR4I, p_force_wide, EDITOR_GET("interface/inspector/horizontal_vector_types_editing")) {}


void Vector2MinMaxPropertyEditor::_update_sizing() {
	update_dyn_range();
	edit_size = range_edit_widget->get_size();
	margin = Vector2(range_slider_left_icon->get_width(), (edit_size.y - range_slider_left_icon->get_height()) * 0.5);
	usable_area = edit_size - margin * 2;
}

void Vector2MinMaxPropertyEditor::_range_edit_draw() {
	ERR_FAIL_COND(range_slider_left_icon.is_null());
	ERR_FAIL_COND(range_slider_right_icon.is_null());
	_update_sizing();

	bool widget_active = mouse_inside || drag != Drag::NONE;

	// FIXME: Need to offset by 1 due to some outline bug.
	range_edit_widget->draw_rect(Rect2(margin + Vector2(1, 1), usable_area - Vector2(1, 1)), widget_active ? background_color.lerp(normal_color, 0.3) : background_color, false, 1.0);

	Color draw_color;

	if (widget_active) {
		float icon_offset = _get_left_offset() - range_slider_left_icon->get_width() - 1;

		if (drag == Drag::LEFT || drag == Drag::SCALE) {
			draw_color = drag_color;
		} else if (hover == Hover::LEFT) {
			draw_color = hovered_color;
		} else {
			draw_color = normal_color;
		}
		range_edit_widget->draw_texture(range_slider_left_icon, Vector2(icon_offset, margin.y), draw_color);

		icon_offset = _get_right_offset();

		if (drag == Drag::RIGHT || drag == Drag::SCALE) {
			draw_color = drag_color;
		} else if (hover == Hover::RIGHT) {
			draw_color = hovered_color;
		} else {
			draw_color = normal_color;
		}
		range_edit_widget->draw_texture(range_slider_right_icon, Vector2(icon_offset, margin.y), draw_color);
	}

	if (drag == Drag::MIDDLE || drag == Drag::SCALE) {
		draw_color = drag_color;
	} else if (hover == Hover::MIDDLE) {
		draw_color = hovered_color;
	} else {
		draw_color = normal_color;
	}
	range_edit_widget->draw_rect(_get_middle_rect(), draw_color);

	Rect2 midpoint_rect(Vector2(margin.x + usable_area.x * (_get_min_ratio() + _get_max_ratio()) * 0.5 - 1, margin.y + 2),
			Vector2(2, usable_area.y - 4));

	range_edit_widget->draw_rect(midpoint_rect, midpoint_color);
	Vector2 size = usable_area;
	if(size.x < 30 || size.y < 16) {
		return;
	}
	Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("LineEdit"));
	float mid_y =  (size.y - 16.0) * 0.5;
	if(mid_y < 0) {
		mid_y = 0;
	}
	if(is_int) {
		const Vector2i value = Vector2i(min_range->get_value(), max_range->get_value());
		range_edit_widget->draw_string(font,Point2(0, edit_size.y / 2.0),String::num_int64(value.x),HORIZONTAL_ALIGNMENT_LEFT,size.x * 0.5);

		String max_str = String::num_int64(value.y);

		
		int text_w = font->get_string_size(max_str).x;
		float off = size.x - text_w;
		if(off < size.x * 0.5) {
			off = size.x * 0.5;
		}
		range_edit_widget->draw_string(font,Point2(off, edit_size.y / 2.0),max_str,HORIZONTAL_ALIGNMENT_LEFT,size.x * 0.5);
	}
	else {
		const Vector2 value = Vector2(min_range->get_value(), max_range->get_value());
		range_edit_widget->draw_string(font,Point2(0, edit_size.y / 2.0),vformat(TTR("%0.4f"), value.x),HORIZONTAL_ALIGNMENT_LEFT,size.x * 0.5);

		String max_str = vformat(TTR("%0.4f"), value.y);

		
		int text_w = font->get_string_size(max_str).x;
		float off = size.x - text_w;
		if(off < size.x * 0.5) {
			off = size.x * 0.5;
		}
		range_edit_widget->draw_string(font,Point2(off, edit_size.y / 2.0),max_str,HORIZONTAL_ALIGNMENT_LEFT,size.x * 0.5);

	}
}

void Vector2MinMaxPropertyEditor::_range_edit_gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mb = p_event;
	Ref<InputEventMouseMotion> mm = p_event;
	// Prevent unnecessary computations.
	if ((mb.is_null() || mb->get_button_index() != MouseButton::LEFT) && (mm.is_null())) {
		return;
	}

	ERR_FAIL_COND(range_slider_left_icon.is_null());
	ERR_FAIL_COND(range_slider_right_icon.is_null());
	_update_sizing();

	if (mb.is_valid()) {
		const Drag prev_drag = drag;

		if (mb->is_pressed()) {
			if (mb->is_shift_pressed()) {
				drag = Drag::SCALE;
				drag_from_value = (max_range->get_value() - min_range->get_value()) * 0.5;
				drag_midpoint = (max_range->get_value() + min_range->get_value()) * 0.5;
			} else if (hover == Hover::LEFT) {
				drag = Drag::LEFT;
				drag_from_value = min_range->get_value();
			} else if (hover == Hover::RIGHT) {
				drag = Drag::RIGHT;
				drag_from_value = max_range->get_value();
			} else {
				drag = Drag::MIDDLE;
				drag_from_value = min_range->get_value();
			}
			drag_origin = mb->get_position().x;
		} else {
			drag = Drag::NONE;
		}

		if (drag != prev_drag) {
			range_edit_widget->queue_redraw();
		}
	}

	float property_length = property_range.y - property_range.x;
	if (mm.is_valid()) {
		switch (drag) {
			case Drag::NONE: {
				const Hover prev_hover = hover;
				float left_icon_offset = _get_left_offset() - range_slider_left_icon->get_width() - 1;

				if (Rect2(Vector2(left_icon_offset, 0), range_slider_left_icon->get_size()).has_point(mm->get_position())) {
					hover = Hover::LEFT;
				} else if (Rect2(Vector2(_get_right_offset(), 0), range_slider_right_icon->get_size()).has_point(mm->get_position())) {
					hover = Hover::RIGHT;
				} else if (_get_middle_rect().has_point(mm->get_position())) {
					hover = Hover::MIDDLE;
				} else {
					hover = Hover::NONE;
				}

				if (hover != prev_hover) {
					range_edit_widget->queue_redraw();
				}
			} break;

			case Drag::LEFT:
			case Drag::RIGHT: {
				float new_value = drag_from_value + (mm->get_position().x - drag_origin) / usable_area.x * property_length;
				if (drag == Drag::LEFT) {
					new_value = MIN(new_value, max_range->get_value());
					_set_clamped_values(new_value, max_range->get_value());
				} else {
					new_value = MAX(new_value, min_range->get_value());
					_set_clamped_values(min_range->get_value(), new_value);
				}
			} break;

			case Drag::MIDDLE: {
				float delta = (mm->get_position().x - drag_origin) / usable_area.x * property_length;
				float diff = max_range->get_value() - min_range->get_value();
				delta = CLAMP(drag_from_value + delta, property_range.x, property_range.y - diff) - drag_from_value;
				_set_clamped_values(drag_from_value + delta, drag_from_value + delta + diff);
			} break;

			case Drag::SCALE: {
				float delta = (mm->get_position().x - drag_origin) / usable_area.x * property_length + drag_from_value;
				_set_clamped_values(MIN(drag_midpoint, drag_midpoint - delta), MAX(drag_midpoint, drag_midpoint + delta));
			} break;
		}
	}
}

void Vector2MinMaxPropertyEditor::_set_mouse_inside(bool p_inside) {
	mouse_inside = p_inside;
	if (!p_inside) {
		hover = Hover::NONE;
	}
	range_edit_widget->queue_redraw();
}

float Vector2MinMaxPropertyEditor::_get_min_ratio() const {
	return (min_range->get_value() - property_range.x) / (property_range.y - property_range.x);
}

float Vector2MinMaxPropertyEditor::_get_max_ratio() const {
	return (max_range->get_value() - property_range.x) / (property_range.y - property_range.x);
}

float Vector2MinMaxPropertyEditor::_get_left_offset() const {
	return margin.x + usable_area.x * _get_min_ratio();
}

float Vector2MinMaxPropertyEditor::_get_right_offset() const {
	return margin.x + usable_area.x * _get_max_ratio();
}

Rect2 Vector2MinMaxPropertyEditor::_get_middle_rect() const {
	if (Math::is_equal_approx(min_range->get_value(), max_range->get_value())) {
		return Rect2();
	}

	return Rect2(
			Vector2(_get_left_offset() - 1, margin.y),
			Vector2(usable_area.x * (_get_max_ratio() - _get_min_ratio()) + 1, usable_area.y));
}

void Vector2MinMaxPropertyEditor::_set_clamped_values(float p_min, float p_max) {
	// This is required for editing widget in case the properties have or_less or or_greater hint.
	min_range->set_value(MAX(p_min, property_range.x));
	max_range->set_value(MIN(p_max, property_range.y));
	_update_slider_values();
	_sync_property();
}

void Vector2MinMaxPropertyEditor::_sync_property() {
	if(is_int){
		const Vector2i value = Vector2i(min_range->get_value(), max_range->get_value());
		emit_changed(get_edited_property(), value, "", true);
	}
	else {
		const Vector2 value = Vector2(min_range->get_value(), max_range->get_value());
		emit_changed(get_edited_property(), value, "", true);
	}
	range_edit_widget->queue_redraw();
}

void Vector2MinMaxPropertyEditor::_update_mode() {
	max_edit->set_read_only(false);

	switch (slider_mode) {
		case Mode::RANGE: {
			min_edit->set_label("min");
			max_edit->set_label("max");
			max_edit->set_block_signals(true);
			max_edit->set_min(max_range->get_min());
			max_edit->set_max(max_range->get_max());
			max_edit->set_block_signals(false);

			min_edit->set_allow_lesser(min_range->is_lesser_allowed());
			min_edit->set_allow_greater(min_range->is_greater_allowed());
			max_edit->set_allow_lesser(max_range->is_lesser_allowed());
			max_edit->set_allow_greater(max_range->is_greater_allowed());
		} break;

		case Mode::MIDPOINT: {
			min_edit->set_label("val");
			max_edit->set_label(U"±");
			max_edit->set_block_signals(true);
			max_edit->set_min(0);
			max_edit->set_block_signals(false);

			min_edit->set_allow_lesser(min_range->is_lesser_allowed());
			min_edit->set_allow_greater(max_range->is_greater_allowed());
			max_edit->set_allow_lesser(false);
			max_edit->set_allow_greater(min_range->is_lesser_allowed() && max_range->is_greater_allowed());
		} break;
	}
	_update_slider_values();
}

void Vector2MinMaxPropertyEditor::_update_slider_values() {
	switch (slider_mode) {
		case Mode::RANGE: {
			min_edit->set_value_no_signal(min_range->get_value());
			max_edit->set_value_no_signal(max_range->get_value());
		} break;

		case Mode::MIDPOINT: {
			min_edit->set_value_no_signal((min_range->get_value() + max_range->get_value()) * 0.5);
			max_edit->set_value_no_signal((max_range->get_value() - min_range->get_value()) * 0.5);

			max_edit->set_block_signals(true);
			max_edit->set_max(_get_max_spread());
			max_edit->set_read_only(max_edit->get_max() == 0);
			max_edit->set_block_signals(false);
		} break;
	}
}

float Vector2MinMaxPropertyEditor::_get_max_spread() const {
	float max_spread = max_range->get_max() - min_range->get_min();

	if (max_edit->is_greater_allowed()) {
		return max_spread;
	}

	if (!min_edit->is_lesser_allowed()) {
		max_spread = MIN(max_spread, min_edit->get_value() - min_edit->get_min());
	}

	if (!min_edit->is_greater_allowed()) {
		max_spread = MIN(max_spread, min_edit->get_max() - min_edit->get_value());
	}

	return max_spread;
}

void Vector2MinMaxPropertyEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			toggle_mode_button->set_button_icon(get_editor_theme_icon(SNAME("Anchor")));
			range_slider_left_icon = get_editor_theme_icon(SNAME("RangeSliderLeft"));
			range_slider_right_icon = get_editor_theme_icon(SNAME("RangeSliderRight"));

			min_edit->add_theme_color_override(SNAME("label_color"), get_theme_color(SNAME("property_color_x"), EditorStringName(Editor)));
			max_edit->add_theme_color_override(SNAME("label_color"), get_theme_color(SNAME("property_color_y"), EditorStringName(Editor)));

			const bool dark_theme = EditorThemeManager::is_dark_theme();
			const Color accent_color = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
			background_color = dark_theme ? Color(0.3, 0.3, 0.3) : Color(0.7, 0.7, 0.7);
			normal_color = dark_theme ? Color(0.5, 0.5, 0.5) : Color(0.8, 0.8, 0.8);
			hovered_color = dark_theme ? Color(0.8, 0.8, 0.8) : Color(0.6, 0.6, 0.6);
			drag_color = hovered_color.lerp(accent_color, 0.8);
			midpoint_color = dark_theme ? Color(1, 1, 1) : Color(0, 0, 0);

			range_edit_widget->set_custom_minimum_size(Vector2(0, range_slider_left_icon->get_height()  * 2));
		} break;
	}
}

void Vector2MinMaxPropertyEditor::setup(float p_min, float p_max, float p_step, bool p_allow_less, bool p_allow_greater, bool p_degrees,bool p_is_int) {
	property_range = Vector2(p_min, p_max);
	is_int = p_is_int;
	update_dyn_range();

	// Initially all Ranges share properties.
	for (Range *range : Vector<Range *>{ min_range, min_edit, max_range, max_edit }) {
		range->set_min(p_min);
		range->set_max(p_max);
		range->set_step(p_step);
		range->set_allow_lesser(p_allow_less);
		range->set_allow_greater(p_allow_greater);
	}

	if (p_degrees) {
		min_edit->set_suffix(U" \u00B0");
		max_edit->set_suffix(U" \u00B0");
	}
	_update_mode();
}

void Vector2MinMaxPropertyEditor::update_dyn_range() {
	if(get_edited_object() == nullptr) {
		return;
	}
	StringName name = get_range_method(get_edited_property());

	bool is_dynamic_range = false;
	if(get_edited_object()->has_method(name)) {
		Variant ret = get_edited_object()->call(name);
		if(ret.get_type() == Variant::VECTOR2 ) {
			property_range = ret;
		}
		else {
			Vector2i rangei = ret;
			property_range.x = rangei.x;
			property_range.y = rangei.y;
		}
		is_dynamic_range = true;
	}
	if(is_dynamic_range) {
		for (Range *r : Vector<Range *>{ min_range, min_edit, max_range, max_edit }) {
			r->set_min(property_range.x);
			r->set_max(property_range.y);
		}
	}

}
void Vector2MinMaxPropertyEditor::update_property() {
	update_dyn_range();
	if(is_int){
		const Vector2i value = get_edited_property_value();
		min_range->set_value(value.x);
		max_range->set_value(value.y);
	}
	else {
		const Vector2 value = get_edited_property_value();
		min_range->set_value(value.x);
		max_range->set_value(value.y);
	}
	_update_slider_values();
	range_edit_widget->queue_redraw();
}

Vector2MinMaxPropertyEditor::Vector2MinMaxPropertyEditor() {
	VBoxContainer *content_vb = memnew(VBoxContainer);
	content_vb->add_theme_constant_override(SNAME("separation"), 0);
	add_child(content_vb);

	// Helper Range objects to keep absolute min and max values.
	min_range = memnew(Range);
	min_range->hide();
	add_child(min_range);

	max_range = memnew(Range);
	max_range->hide();
	add_child(max_range);

	// Range edit widget.
	HBoxContainer *hb = memnew(HBoxContainer);
	content_vb->add_child(hb);

	range_edit_widget = memnew(Control);
	range_edit_widget->set_h_size_flags(SIZE_EXPAND_FILL);
	range_edit_widget->set_tooltip_text(TTR("Hold Shift to scale around midpoint instead of moving."));
	hb->add_child(range_edit_widget);
	range_edit_widget->set_custom_minimum_size(Vector2(0,40));
	range_edit_widget->connect(SceneStringName(draw), callable_mp(this, &Vector2MinMaxPropertyEditor::_range_edit_draw));
	range_edit_widget->connect(SceneStringName(gui_input), callable_mp(this, &Vector2MinMaxPropertyEditor::_range_edit_gui_input));
	range_edit_widget->connect(SceneStringName(mouse_entered), callable_mp(this, &Vector2MinMaxPropertyEditor::_set_mouse_inside).bind(true));
	range_edit_widget->connect(SceneStringName(mouse_exited), callable_mp(this, &Vector2MinMaxPropertyEditor::_set_mouse_inside).bind(false));

	// Range controls for actual editing. Their min/max may depend on editing mode.
	hb = memnew(HBoxContainer);
	hb->set_visible(false);
	content_vb->add_child(hb);

	min_edit = memnew(EditorSpinSlider);
	min_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	hb->add_child(min_edit);

	max_edit = memnew(EditorSpinSlider);
	max_edit->set_h_size_flags(SIZE_EXPAND_FILL);
	hb->add_child(max_edit);

	toggle_mode_button = memnew(Button);
	toggle_mode_button->set_toggle_mode(true);
	toggle_mode_button->set_tooltip_text(TTR("Toggle between minimum/maximum and base value/spread modes."));
	hb->add_child(toggle_mode_button);
	//toggle_mode_button->connect(SceneStringName(toggled), callable_mp(this, &Vector2MinMaxPropertyEditor::_toggle_mode));

	//set_bottom_editor(content_vb);
}









