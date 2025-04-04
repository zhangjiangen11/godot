/**************************************************************************/
/*  atlas_texture.h                                                       */
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

#include "scene/resources/texture.h"

class AtlasTexture : public Texture2D {
	GDCLASS(AtlasTexture, Texture2D);
	RES_BASE_EXTENSION("atlastex");

	Rect2 _get_region_rect() const;

protected:
	Ref<Texture2D> atlas;
	Rect2 region;
	Rect2 margin;
	bool filter_clip = false;

	static void _bind_methods();
	FillMode nine_vertical_fill;
	FillMode nine_horizontal_fill;

	int nine_left = 0;
	int nine_right = 0;
	int nine_top = 0;
	int nine_bottom = 0;

	bool nine_draw_center = true;
	bool nine_draw = false;

public:
	void set_nine_draw(bool value) {
		nine_draw = value;
	}
	bool get_nine_draw() { return nine_draw; }

	void set_nine_draw_center(bool value) {
		nine_draw_center = value;
	}
	bool get_nine_draw_center() { return nine_draw_center; }

	void set_nine_left(int value) {
		nine_left = CLAMP(value, 0, region.size.x);
	}
	int get_nine_left() { return nine_left; }

	void set_nine_right(int value) {
		nine_right = CLAMP(value, 0, region.size.x);
	}
	int get_nine_right() { return nine_right; }

	void set_nine_top(int value) {
		nine_top = CLAMP(value, 0, region.size.y);
	}
	int get_nine_top() { return nine_top; }

	void set_nine_bottom(int value) {
		nine_bottom = CLAMP(value, 0, region.size.y);
	}

	int get_nine_bottom() { return nine_bottom; }

public:
	void set_nine_horizontal_fill(FillMode value) {
		nine_horizontal_fill = value;
	}
	FillMode get_nine_horizontal_fill() { return nine_horizontal_fill; }

	void set_nine_vertical_fill(FillMode value) {
		nine_vertical_fill = value;
	}
	FillMode get_nine_vertical_fill() { return nine_vertical_fill; }

protected:
	void _update_piece_rects(Rect2 *_piece_rects, const Rect2 &p_rect) const;

public:
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;

	virtual bool has_alpha() const override;

	void set_atlas(const Ref<Texture2D> &p_atlas);
	Ref<Texture2D> get_atlas() const;

	void set_region(const Rect2 &p_region);
	Rect2 get_region() const;

	void set_margin(const Rect2 &p_margin);
	Rect2 get_margin() const;

	void set_filter_clip(const bool p_enable);
	bool has_filter_clip() const;

	virtual void draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, bool p_clip_uv = true) const override;
	virtual bool get_rect_region(const Rect2 &p_rect, const Rect2 &p_src_rect, Rect2 &r_rect, Rect2 &r_src_rect) const override;
	virtual void draw_nine(RID p_canvas_item, const Rect2 &p_rect, const Color &modulate = Color(1, 1, 1), bool p_transpose = false) const;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	virtual Ref<Image> get_image() const override;

	AtlasTexture();
};
