/**************************************************************************/
/*  scroll_bar.h                                                          */
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

#include "scene/gui/range.h"

class ScrollBar : public Range {
	GDCLASS(ScrollBar, Range);

	enum HighlightStatus {
		HIGHLIGHT_NONE,
		HIGHLIGHT_DECR,
		HIGHLIGHT_RANGE,
		HIGHLIGHT_INCR,
	};

	static bool focus_by_default;

	Orientation orientation;
	Size2 size;
	float custom_step = -1.0;

	HighlightStatus highlight = HIGHLIGHT_NONE;

	bool incr_active = false;
	bool decr_active = false;

	struct Drag {
		bool active = false;
		float pos_at_click = 0.0;
		float value_at_click = 0.0;
	} drag;

	double get_grabber_size() const;
	double get_grabber_min_size() const;
	double get_area_size() const;
	double get_grabber_offset() const;

	static void set_can_focus_by_default(bool p_can_focus);

	Node *drag_node = nullptr;
	NodePath drag_node_path;
	bool drag_node_enabled = true;

	Vector2 drag_node_speed;
	Vector2 drag_node_accum;
	Vector2 drag_node_from;
	Vector2 last_drag_node_accum;
	float last_drag_node_time = 0.0;
	float time_since_motion = 0.0;
	bool drag_node_touching = false;
	bool drag_node_touching_deaccel = false;
	bool click_handled = false;

	bool scrolling = false;
	double target_scroll = 0.0;
	bool smooth_scroll_enabled = false;

	struct ThemeCache {
		Ref<StyleBox> scroll_style;
		Ref<StyleBox> scroll_focus_style;
		Ref<StyleBox> scroll_offset_style;
		Ref<StyleBox> grabber_style;
		Ref<StyleBox> grabber_hl_style;
		Ref<StyleBox> grabber_pressed_style;

		Ref<Texture2D> increment_icon;
		Ref<Texture2D> increment_hl_icon;
		Ref<Texture2D> increment_pressed_icon;
		Ref<Texture2D> decrement_icon;
		Ref<Texture2D> decrement_hl_icon;
		Ref<Texture2D> decrement_pressed_icon;
	} theme_cache;

	struct UserData {
		// 背景
		Ref<Texture2D> background;
		Ref<Texture2D> background_focus;

		// 增加
		Ref<Texture2D> increment;
		Ref<Texture2D> increment_hl;
		Ref<Texture2D> increment_pressed;

		// 减少
		Ref<Texture2D> decrement;
		Ref<Texture2D> decrement_hl;
		Ref<Texture2D> decrement_pressed;

		// 滚动条
		Ref<Texture2D> grabber;
		Ref<Texture2D> grabber_hl;
		Ref<Texture2D> grabber_pressed;
	} user_data;

	void _drag_node_exit();
	void _drag_node_input(const Ref<InputEvent> &p_input);

	virtual void gui_input(const Ref<InputEvent> &p_event) override;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void scroll(double p_amount);
	void scroll_to(double p_position);

	void set_custom_step(float p_custom_step);
	float get_custom_step() const;

	void set_drag_node(const NodePath &p_path);
	NodePath get_drag_node() const;
	void set_drag_node_enabled(bool p_enable);

	void set_smooth_scroll_enabled(bool p_enable);
	bool is_smooth_scroll_enabled() const;

	void set_background(const Ref<Texture2D> &p_background) {
		user_data.background = p_background;
	}
	Ref<Texture2D> get_background() const { return user_data.background; }

	void set_background_focus(const Ref<Texture2D> &p_background_focus){
		user_data.background_focus = p_background_focus;
	}
	Ref<Texture2D> get_background_focus() const { return user_data.background_focus; }

	void set_increment(const Ref<Texture2D> &p_increment){
		user_data.increment = p_increment;
	}
	Ref<Texture2D> get_increment() const { return user_data.increment; }

	void set_increment_hl(const Ref<Texture2D> &p_increment_hl){
		user_data.increment_hl = p_increment_hl;
	}
	Ref<Texture2D> get_increment_hl() const { return user_data.increment_hl; }

	void set_increment_pressed(const Ref<Texture2D> &p_increment_pressed){
		user_data.increment_pressed = p_increment_pressed;
	}
	Ref<Texture2D> get_increment_pressed() const { return user_data.increment_pressed; }

	void set_decrement(const Ref<Texture2D> &p_decrement){
		user_data.decrement = p_decrement;
	}
	Ref<Texture2D> get_decrement() const { return user_data.decrement; }

	void set_decrement_hl(const Ref<Texture2D> &p_decrement_hl){
		user_data.decrement_hl = p_decrement_hl;
	}
	Ref<Texture2D> get_decrement_hl() const { return user_data.decrement_hl; }

	void set_decrement_pressed(const Ref<Texture2D> &p_decrement_pressed){
		user_data.decrement_pressed = p_decrement_pressed;
	}
	Ref<Texture2D> get_decrement_pressed() const { return user_data.decrement_pressed; }

	void set_grabber(const Ref<Texture2D> &p_grabber) {
		user_data.grabber = p_grabber;
	}
	Ref<Texture2D> get_grabber() const { return user_data.grabber; }

	void set_grabber_hl(const Ref<Texture2D> &p_grabber_hl){
		user_data.grabber_hl = p_grabber_hl;
	}
	Ref<Texture2D> get_grabber_hl() const { return user_data.grabber_hl; }

	void set_grabber_pressed(const Ref<Texture2D> &p_grabber_pressed){
		user_data.grabber_pressed = p_grabber_pressed;
	}
	Ref<Texture2D> get_grabber_pressed() const { return user_data.grabber_pressed; }

	virtual Size2 get_minimum_size() const override;
	ScrollBar(Orientation p_orientation = VERTICAL);
	~ScrollBar();
};

class HScrollBar : public ScrollBar {
	GDCLASS(HScrollBar, ScrollBar);

public:
	HScrollBar() :
			ScrollBar(HORIZONTAL) { set_v_size_flags(0); }
};

class VScrollBar : public ScrollBar {
	GDCLASS(VScrollBar, ScrollBar);

public:
	VScrollBar() :
			ScrollBar(VERTICAL) { set_h_size_flags(0); }
};
