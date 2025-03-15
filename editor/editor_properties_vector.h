/**************************************************************************/
/*  editor_properties_vector.h                                            */
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

#pragma once

#include "editor/editor_inspector.h"

class EditorSpinSlider;
class TextureButton;

class EditorPropertyVectorN : public EditorProperty {
	GDCLASS(EditorPropertyVectorN, EditorProperty);

	static const String COMPONENT_LABELS[4];

	int component_count = 0;
	Variant::Type vector_type;

	Vector<EditorSpinSlider *> spin_sliders;
	TextureButton *linked = nullptr;
	Vector<double> ratio;
	bool is_grabbed = false;

	bool radians_as_degrees = false;

	void _update_ratio();
	void _store_link(bool p_linked);
	void _grab_changed(bool p_grab);
	void _value_changed(double p_val, const String &p_name);

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

public:
	virtual void update_property() override;
	void setup(double p_min, double p_max, double p_step = 1.0, bool p_hide_slider = true, bool p_link = false, const String &p_suffix = String(), bool p_radians_as_degrees = false, bool p_is_int = false);
	EditorPropertyVectorN(Variant::Type p_type, bool p_force_wide, bool p_horizontal);
};


class Vector2MinMaxPropertyEditor : public EditorProperty {
	GDCLASS(Vector2MinMaxPropertyEditor, EditorProperty);

	enum class Hover {
		NONE,
		LEFT,
		RIGHT,
		MIDDLE,
	};

	enum class Drag {
		NONE,
		LEFT,
		RIGHT,
		MIDDLE,
		SCALE,
	};

	enum class Mode {
		RANGE,
		MIDPOINT,
	};

	Ref<Texture2D> range_slider_left_icon;
	Ref<Texture2D> range_slider_right_icon;

	Color background_color;
	Color normal_color;
	Color hovered_color;
	Color drag_color;
	Color midpoint_color;

	Control *range_edit_widget = nullptr;
	Button *toggle_mode_button = nullptr;
	Range *min_range = nullptr;
	Range *max_range = nullptr;

	EditorSpinSlider *min_edit = nullptr;
	EditorSpinSlider *max_edit = nullptr;

	Vector2 edit_size;
	Vector2 margin;
	Vector2 usable_area;

	Vector2 property_range;

	bool mouse_inside = false;
	Hover hover = Hover::NONE;

	Drag drag = Drag::NONE;
	float drag_from_value = 0.0;
	float drag_midpoint = 0.0;
	float drag_origin = 0.0;

	bool allow_less = false;

	Mode slider_mode = Mode::RANGE;

	bool is_int = false;

	void _update_sizing();
	void _range_edit_draw();
	void _range_edit_gui_input(const Ref<InputEvent> &p_event);
	void _set_mouse_inside(bool p_inside);

	float _get_min_ratio() const;
	float _get_max_ratio() const;
	float _get_left_offset() const;
	float _get_right_offset() const;
	Rect2 _get_middle_rect() const;

	void _set_clamped_values(float p_min, float p_max);
	void _sync_property();

	void _update_mode();
	void _toggle_mode(bool p_edit_mode);
	void _update_slider_values();
	void _sync_sliders(float, const EditorSpinSlider *p_changed_slider);
	float _get_max_spread() const;

protected:
	void _notification(int p_what);

public:
	void setup(float p_min, float p_max, float p_step, bool p_allow_less, bool p_allow_greater, bool p_degrees,bool p_is_int = false);
	virtual void update_property() override;

	Vector2MinMaxPropertyEditor();
};



class EditorPropertyVector2 : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector2, EditorPropertyVectorN);

public:
	EditorPropertyVector2(bool p_force_wide = false);
};

class EditorPropertyVector2i : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector2i, EditorPropertyVectorN);

public:
	EditorPropertyVector2i(bool p_force_wide = false);
};

class EditorPropertyVector3 : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector3, EditorPropertyVectorN);

public:
	EditorPropertyVector3(bool p_force_wide = false);
};

class EditorPropertyVector3i : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector3i, EditorPropertyVectorN);

public:
	EditorPropertyVector3i(bool p_force_wide = false);
};

class EditorPropertyVector4 : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector4, EditorPropertyVectorN);

public:
	EditorPropertyVector4(bool p_force_wide = false);
};

class EditorPropertyVector4i : public EditorPropertyVectorN {
	GDCLASS(EditorPropertyVector4i, EditorPropertyVectorN);

public:
	EditorPropertyVector4i(bool p_force_wide = false);
};
