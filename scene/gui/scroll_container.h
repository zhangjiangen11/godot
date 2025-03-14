/**************************************************************************/
/*  scroll_container.h                                                    */
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

#include "container.h"

#include "scroll_bar.h"

class ScrollContainer : public Container {
	GDCLASS(ScrollContainer, Container);

public:
	enum ScrollMode {
		SCROLL_MODE_DISABLED = 0,
		SCROLL_MODE_AUTO,
		SCROLL_MODE_SHOW_ALWAYS,
		SCROLL_MODE_SHOW_NEVER,
		SCROLL_MODE_RESERVE,
	};

private:
	HScrollBar *h_scroll = nullptr;
	VScrollBar *v_scroll = nullptr;

	mutable Size2 largest_child_min_size; // The largest one among the min sizes of all available child controls.

	void update_scrollbars();

	Vector2 drag_speed;
	Vector2 drag_accum;
	Vector2 drag_from;
	Vector2 last_drag_accum;
	float time_since_motion = 0.0f;
	bool drag_touching = false;
	bool drag_touching_deaccel = false;
	bool beyond_deadzone = false;

	ScrollMode horizontal_scroll_mode = SCROLL_MODE_AUTO;
	ScrollMode vertical_scroll_mode = SCROLL_MODE_AUTO;

	int deadzone = 0;
	bool follow_focus = false;

	struct ThemeCache {
		Ref<StyleBox> panel_style;
		Ref<StyleBox> focus_style;
	} theme_cache;

	struct UserData {
		Ref<Texture2D> background;
		Ref<Texture2D> background_focus;
	} user_data;

	void _cancel_drag();

	bool _is_h_scroll_visible() const;
	bool _is_v_scroll_visible() const;

	bool draw_focus_border = false;
	bool focus_border_is_drawn = false;
	bool child_has_focus();

protected:
	Size2 get_minimum_size() const override;

	void _gui_focus_changed(Control *p_control);
	void _reposition_children();

	void _notification(int p_what);
	static void _bind_methods();

	bool _updating_scrollbars = false;
	void _update_scrollbar_position();
	void _scroll_moved(float);
	virtual void _scroll_changed(float) {}

public:
	virtual void gui_input(const Ref<InputEvent> &p_gui_input) override;

	void set_h_scroll(int p_pos);
	int get_h_scroll() const;

	void set_v_scroll(int p_pos);
	int get_v_scroll() const;

	void set_horizontal_custom_step(float p_custom_step);
	float get_horizontal_custom_step() const;

	void set_vertical_custom_step(float p_custom_step);
	float get_vertical_custom_step() const;

	void set_horizontal_scroll_mode(ScrollMode p_mode);
	ScrollMode get_horizontal_scroll_mode() const;

	void set_vertical_scroll_mode(ScrollMode p_mode);
	ScrollMode get_vertical_scroll_mode() const;

	int get_deadzone() const;
	void set_deadzone(int p_deadzone);

	bool is_following_focus() const;
	void set_follow_focus(bool p_follow);

	void set_background(const Ref<Texture2D> &p_background) {
		user_data.background = p_background;
	}
	Ref<Texture2D> get_background() const { return user_data.background; }

	void set_background_focus(const Ref<Texture2D> &p_background_focus) {
		user_data.background_focus = p_background_focus;
	}
	Ref<Texture2D> get_background_focus() const { return user_data.background_focus; }
public:
	// hscroll bar
	void set_h_sb_background(const Ref<Texture2D> &p_background) {
		h_scroll->set_background(p_background);
	}
	Ref<Texture2D> get_h_sb_background() const { return h_scroll->get_background(); }

	void set_h_sb_background_focus(const Ref<Texture2D> &p_background_focus){
		h_scroll->set_background_focus(p_background_focus);
	}
	Ref<Texture2D> get_h_sb_background_focus() const { return h_scroll->get_background_focus(); }

	void set_h_sb_increment(const Ref<Texture2D> &p_increment){
		h_scroll->set_increment(p_increment);
	}
	Ref<Texture2D> get_h_sb_increment() const { return h_scroll->get_increment(); }

	void set_h_sb_increment_hl(const Ref<Texture2D> &p_increment_hl){
		h_scroll->set_increment_hl(p_increment_hl);
	}
	Ref<Texture2D> get_h_sb_increment_hl() const { return h_scroll->get_increment_hl(); }

	void set_h_sb_increment_pressed(const Ref<Texture2D> &p_increment_pressed){
		h_scroll->set_increment_pressed(p_increment_pressed);
	}
	Ref<Texture2D> get_h_sb_increment_pressed() const { return h_scroll->get_increment_pressed(); }

	void set_h_sb_decrement(const Ref<Texture2D> &p_decrement){
		h_scroll->set_decrement(p_decrement);
	}
	Ref<Texture2D> get_h_sb_decrement() const { return h_scroll->get_decrement(); }

	void set_h_sb_decrement_hl(const Ref<Texture2D> &p_decrement_hl){
		h_scroll->set_decrement_hl(p_decrement_hl);
	}
	Ref<Texture2D> get_h_sb_decrement_hl() const { return h_scroll->get_decrement_hl(); }

	void set_h_sb_decrement_pressed(const Ref<Texture2D> &p_decrement_pressed){
		h_scroll->set_decrement_pressed(p_decrement_pressed);
	}
	Ref<Texture2D> get_h_sb_decrement_pressed() const { return h_scroll->get_decrement_pressed(); }

	void set_h_sb_grabber(const Ref<Texture2D> &p_grabber) {
		h_scroll->set_grabber(p_grabber);
	}
	Ref<Texture2D> get_h_sb_grabber() const { return h_scroll->get_grabber(); }

	void set_h_sb_grabber_hl(const Ref<Texture2D> &p_grabber_hl){
		h_scroll->set_grabber_hl(p_grabber_hl);
	}
	Ref<Texture2D> get_h_sb_grabber_hl() const { return h_scroll->get_grabber_hl(); }

	void set_h_sb_grabber_pressed(const Ref<Texture2D> &p_grabber_pressed){
		h_scroll->set_grabber_pressed(p_grabber_pressed);
	}
	Ref<Texture2D> get_h_sb_grabber_pressed() const { return h_scroll->get_grabber_pressed(); }

public:
	// vscroll bar
	void set_v_sb_background(const Ref<Texture2D> &p_background) {
		v_scroll->set_background(p_background);
	}
	Ref<Texture2D> get_v_sb_background() const { return v_scroll->get_background(); }

	void set_v_sb_background_focus(const Ref<Texture2D> &p_background_focus){
		v_scroll->set_background_focus(p_background_focus);
	}
	Ref<Texture2D> get_v_sb_background_focus() const { return v_scroll->get_background_focus(); }

	void set_v_sb_increment(const Ref<Texture2D> &p_increment){
		v_scroll->set_increment(p_increment);
	}
	Ref<Texture2D> get_v_sb_increment() const { return v_scroll->get_increment(); }

	void set_v_sb_increment_hl(const Ref<Texture2D> &p_increment_hl){
		v_scroll->set_increment_hl(p_increment_hl);
	}
	Ref<Texture2D> get_v_sb_increment_hl() const { return v_scroll->get_increment_hl(); }

	void set_v_sb_increment_pressed(const Ref<Texture2D> &p_increment_pressed){
		v_scroll->set_increment_pressed(p_increment_pressed);
	}
	Ref<Texture2D> get_v_sb_increment_pressed() const { return v_scroll->get_increment_pressed(); }

	void set_v_sb_decrement(const Ref<Texture2D> &p_decrement){
		v_scroll->set_decrement(p_decrement);
	}
	Ref<Texture2D> get_v_sb_decrement() const { return v_scroll->get_decrement(); }

	void set_v_sb_decrement_hl(const Ref<Texture2D> &p_decrement_hl){
		v_scroll->set_decrement_hl(p_decrement_hl);
	}
	Ref<Texture2D> get_v_sb_decrement_hl() const { return v_scroll->get_decrement_hl(); }

	void set_v_sb_decrement_pressed(const Ref<Texture2D> &p_decrement_pressed){
		v_scroll->set_decrement_pressed(p_decrement_pressed);
	}
	Ref<Texture2D> get_v_sb_decrement_pressed() const { return v_scroll->get_decrement_pressed(); }

	void set_v_sb_grabber(const Ref<Texture2D> &p_grabber) {
		v_scroll->set_grabber(p_grabber);
	}
	Ref<Texture2D> get_v_sb_grabber() const { return v_scroll->get_grabber(); }

	void set_v_sb_grabber_hl(const Ref<Texture2D> &p_grabber_hl){
		v_scroll->set_grabber_hl(p_grabber_hl);
	}
	Ref<Texture2D> get_v_sb_grabber_hl() const { return v_scroll->get_grabber_hl(); }

	void set_v_sb_grabber_pressed(const Ref<Texture2D> &p_grabber_pressed){
		v_scroll->set_grabber_pressed(p_grabber_pressed);
	}
	Ref<Texture2D> get_v_sb_grabber_pressed() const { return v_scroll->get_grabber_pressed(); }
public:

	HScrollBar *get_h_scroll_bar();
	VScrollBar *get_v_scroll_bar();
	void ensure_control_visible(Control *p_control);

	PackedStringArray get_configuration_warnings() const override;

	void set_draw_focus_border(bool p_draw);
	bool get_draw_focus_border();

	ScrollContainer();
};

VARIANT_ENUM_CAST(ScrollContainer::ScrollMode);
