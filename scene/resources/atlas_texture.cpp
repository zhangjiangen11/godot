/**************************************************************************/
/*  atlas_texture.cpp                                                     */
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

#include "atlas_texture.h"

int AtlasTexture::get_width() const {
	if (region.size.width == 0) {
		if (atlas.is_valid()) {
			return atlas->get_width();
		}
		return 1;
	} else {
		return region.size.width + margin.size.width;
	}
}

int AtlasTexture::get_height() const {
	if (region.size.height == 0) {
		if (atlas.is_valid()) {
			return atlas->get_height();
		}
		return 1;
	} else {
		return region.size.height + margin.size.height;
	}
}

RID AtlasTexture::get_rid() const {
	if (atlas.is_valid()) {
		return atlas->get_rid();
	}

	return RID();
}

bool AtlasTexture::has_alpha() const {
	if (atlas.is_valid()) {
		return atlas->has_alpha();
	}

	return false;
}

void AtlasTexture::set_atlas(const Ref<Texture2D> &p_atlas) {
	ERR_FAIL_COND(p_atlas == this);
	if (atlas == p_atlas) {
		return;
	}
	// Support recursive AtlasTextures.
	if (Ref<AtlasTexture>(atlas).is_valid()) {
		atlas->disconnect_changed(callable_mp((Resource *)this, &AtlasTexture::emit_changed));
	}
	atlas = p_atlas;
	if (Ref<AtlasTexture>(atlas).is_valid()) {
		atlas->connect_changed(callable_mp((Resource *)this, &AtlasTexture::emit_changed));
	}

	emit_changed();
}

Ref<Texture2D> AtlasTexture::get_atlas() const {
	return atlas;
}

void AtlasTexture::set_region(const Rect2 &p_region) {
	if (region == p_region) {
		return;
	}
	region = p_region;
	emit_changed();
}

Rect2 AtlasTexture::get_region() const {
	return region;
}

void AtlasTexture::set_margin(const Rect2 &p_margin) {
	if (margin == p_margin) {
		return;
	}
	margin = p_margin;
	emit_changed();
}

Rect2 AtlasTexture::get_margin() const {
	return margin;
}

void AtlasTexture::set_filter_clip(const bool p_enable) {
	filter_clip = p_enable;
	emit_changed();
}

bool AtlasTexture::has_filter_clip() const {
	return filter_clip;
}

Rect2 AtlasTexture::_get_region_rect() const {
	Rect2 rc = region;
	if (atlas.is_valid()) {
		if (rc.size.width == 0) {
			rc.size.width = atlas->get_width();
		}
		if (rc.size.height == 0) {
			rc.size.height = atlas->get_height();
		}
	}
	return rc;
}

void AtlasTexture::draw_nine(RID p_canvas_item, const Rect2 &p_rect, const Color &modulate, bool p_transpose) const {
	if (atlas.is_null()) {
		return;
	}
	if (p_rect.size.x == 0 || p_rect.size.y == 0) {
		return;
	}
	float _left = nine_left;
	float _right = nine_right;
	float _top = nine_top;
	float _bottom = nine_bottom;

	if (nine_left + nine_right > p_rect.size.x || nine_top + nine_bottom > p_rect.size.y) {
		float scale_x = p_rect.size.x / (_left + _right);
		float scale_y = p_rect.size.y / (_top + _bottom);
		float scale = MIN(scale_x, scale_y);

		_left = Math::floor(_left * scale);
		_right = Math::ceil(_right * scale);
		_top = Math::floor(_top * scale);
		_bottom = Math::ceil(_bottom * scale);
	}
	Rect2 _piece_rects[9];
	_update_piece_rects(_piece_rects, region);
	float mid_w = MAX(p_rect.size.x - (_left + _right), 0);
	float mid_h = MAX(p_rect.size.y - (_top + _bottom), 0);

	Point2 pos = p_rect.position;
	if (_top > 0) {
		if (_left > 0) {
			draw_rect_region(p_canvas_item, Rect2(pos, Vector2(_left, _top)), _piece_rects[0], modulate, p_transpose);
		}
		pos += Vector2(_left, 0);
		fill_texture(p_canvas_item, Rect2(pos, Vector2(mid_w, _top)), _piece_rects[1], nine_horizontal_fill, nine_vertical_fill, modulate);
		pos += Vector2(mid_w, 0);
		if (_right > 0) {
			draw_rect_region(p_canvas_item, Rect2(pos, Vector2(_right, _top)), _piece_rects[2], modulate);
		}
	}

	pos = Vector2(p_rect.position.x, pos.y + _top);
	if (mid_h > 0) {
		fill_texture(p_canvas_item, Rect2(pos, Vector2(_left, mid_h)), _piece_rects[3], nine_horizontal_fill, nine_vertical_fill, modulate);
		pos += Vector2(_left, 0);
		if (nine_draw_center && mid_w > 0) {
			fill_texture(p_canvas_item, Rect2(pos, Vector2(mid_w, mid_h)), _piece_rects[4], nine_horizontal_fill, nine_vertical_fill, modulate);
		}
		pos += Vector2(mid_w, 0);
		fill_texture(p_canvas_item, Rect2(pos, Vector2(_right, mid_h)), _piece_rects[5], nine_horizontal_fill, nine_vertical_fill, modulate);
	}

	pos = Vector2(p_rect.position.x, pos.y + mid_h);
	if (_bottom > 0) {
		if (_left > 0) {
			draw_rect_region(p_canvas_item, Rect2(pos, Vector2(_left, _bottom)), _piece_rects[6], modulate);
		}
		pos += Vector2(_left, 0);
		fill_texture(p_canvas_item, Rect2(pos, Vector2(mid_w, _bottom)), _piece_rects[7], nine_horizontal_fill, nine_vertical_fill, modulate);
		pos += Vector2(mid_w, 0);
		if (_right > 0) {
			draw_rect_region(p_canvas_item, Rect2(pos, Vector2(_right, _bottom)), _piece_rects[8], modulate);
		}
	}
}

void AtlasTexture::_update_piece_rects(Rect2 *_piece_rects, const Rect2 &_texture_region) const {
	float _x = _texture_region.position.x;
	float _y = _texture_region.position.y;

	float _w = _texture_region.size.x;
	float _h = _texture_region.size.y;

	float mid_w = MAX(_w - (nine_left + nine_right), 0);
	float mid_h = MAX(_h - (nine_top + nine_bottom), 0);

	_piece_rects[0] = Rect2(_x, _y, nine_left, nine_top); // TL
	_piece_rects[1] = Rect2(_x + nine_left, _y, mid_w, nine_top); // T
	_piece_rects[2] = Rect2(_x + (_w - nine_right), _y, nine_right, nine_top); // TR

	_piece_rects[3] = Rect2(_x, _y + nine_top, nine_left, mid_h); // L
	_piece_rects[4] = Rect2(_x + nine_left, _y + nine_top, mid_w, mid_h); // M
	_piece_rects[5] = Rect2(_x + (_w - nine_right), _y + nine_top, nine_right, mid_h); // R

	_piece_rects[6] = Rect2(_x, _y + (_h - nine_bottom), nine_left, nine_bottom); // BL
	_piece_rects[7] = Rect2(_x + nine_left, _y + (_h - nine_bottom), mid_w, nine_bottom); // B
	_piece_rects[8] = Rect2(_x + (_w - nine_right), _y + (_h - nine_bottom), nine_right, nine_bottom); // BR
}
void AtlasTexture::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_atlas", "atlas"), &AtlasTexture::set_atlas);
	ClassDB::bind_method(D_METHOD("get_atlas"), &AtlasTexture::get_atlas);

	ClassDB::bind_method(D_METHOD("set_region", "region"), &AtlasTexture::set_region);
	ClassDB::bind_method(D_METHOD("get_region"), &AtlasTexture::get_region);

	ClassDB::bind_method(D_METHOD("set_margin", "margin"), &AtlasTexture::set_margin);
	ClassDB::bind_method(D_METHOD("get_margin"), &AtlasTexture::get_margin);

	ClassDB::bind_method(D_METHOD("set_filter_clip", "enable"), &AtlasTexture::set_filter_clip);
	ClassDB::bind_method(D_METHOD("has_filter_clip"), &AtlasTexture::has_filter_clip);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "atlas", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_atlas", "get_atlas");
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "region", PROPERTY_HINT_NONE, "suffix:px"), "set_region", "get_region");
	ADD_PROPERTY(PropertyInfo(Variant::RECT2, "margin", PROPERTY_HINT_NONE, "suffix:px"), "set_margin", "get_margin");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "filter_clip"), "set_filter_clip", "has_filter_clip");

	ClassDB::bind_method(D_METHOD("set_nine_draw", "nine_draw"), &AtlasTexture::set_nine_draw);
	ClassDB::bind_method(D_METHOD("get_nine_draw"), &AtlasTexture::get_nine_draw);

	ClassDB::bind_method(D_METHOD("set_nine_draw_center", "center"), &AtlasTexture::set_nine_draw_center);
	ClassDB::bind_method(D_METHOD("get_nine_draw_center"), &AtlasTexture::get_nine_draw_center);

	ClassDB::bind_method(D_METHOD("set_nine_left", "nine_left"), &AtlasTexture::set_nine_left);
	ClassDB::bind_method(D_METHOD("get_nine_left"), &AtlasTexture::get_nine_left);

	ClassDB::bind_method(D_METHOD("set_nine_top", "nine_top"), &AtlasTexture::set_nine_top);
	ClassDB::bind_method(D_METHOD("get_nine_top"), &AtlasTexture::get_nine_top);

	ClassDB::bind_method(D_METHOD("set_nine_right", "nine_right"), &AtlasTexture::set_nine_right);
	ClassDB::bind_method(D_METHOD("get_nine_right"), &AtlasTexture::get_nine_right);

	ClassDB::bind_method(D_METHOD("set_nine_bottom", "nine_bottom"), &AtlasTexture::set_nine_bottom);
	ClassDB::bind_method(D_METHOD("get_nine_bottom"), &AtlasTexture::get_nine_bottom);

	ClassDB::bind_method(D_METHOD("set_nine_horizontal_fill", "nine_horizontal_fill"), &AtlasTexture::set_nine_horizontal_fill);
	ClassDB::bind_method(D_METHOD("get_nine_horizontal_fill"), &AtlasTexture::get_nine_horizontal_fill);

	ClassDB::bind_method(D_METHOD("set_nine_vertical_fill", "nine_vertical_fill"), &AtlasTexture::set_nine_vertical_fill);
	ClassDB::bind_method(D_METHOD("get_nine_vertical_fill"), &AtlasTexture::get_nine_vertical_fill);

	ADD_GROUP("Nine Patch", "nine_");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "nine_draw"), "set_nine_draw", "get_nine_draw");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "nine_draw_center"), "set_nine_draw_center", "get_nine_draw_center");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "nine_horizontal_fill", PROPERTY_HINT_ENUM, "stretch,tile,tile_fit"), "set_nine_horizontal_fill", "get_nine_horizontal_fill");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "nine_vertical_fill", PROPERTY_HINT_ENUM, "stretch,tile,tile_fit"), "set_nine_vertical_fill", "get_nine_vertical_fill");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "nine_left"), "set_nine_left", "get_nine_left");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "nine_top"), "set_nine_top", "get_nine_top");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "nine_right"), "set_nine_right", "get_nine_right");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "nine_bottom"), "set_nine_bottom", "get_nine_bottom");
}

void AtlasTexture::draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate, bool p_transpose) const {
	if (nine_draw) {
		const Rect2 rc = _get_region_rect();
		draw_nine(p_canvas_item, Rect2(p_pos + margin.position, rc.size), p_modulate, p_transpose);
		return;
	}
	if (atlas.is_null()) {
		return;
	}
	const Rect2 rc = _get_region_rect();
	atlas->draw_rect_region(p_canvas_item, Rect2(p_pos + margin.position, rc.size), rc, p_modulate, p_transpose, filter_clip);
}

void AtlasTexture::draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile, const Color &p_modulate, bool p_transpose) const {
	if (nine_draw) {
		draw_nine(p_canvas_item, p_rect, p_modulate, p_transpose);
		return;
	}
	if (atlas.is_null()) {
		return;
	}

	Rect2 src_rect = Rect2(0, 0, get_width(), get_height());

	Rect2 dr;
	Rect2 src_c;
	if (get_rect_region(p_rect, src_rect, dr, src_c)) {
		atlas->draw_rect_region(p_canvas_item, dr, src_c, p_modulate, p_transpose, filter_clip);
	}
}

void AtlasTexture::draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate, bool p_transpose, bool p_clip_uv) const {
	// This might not necessarily work well if using a rect, needs to be fixed properly.
	if (atlas.is_null()) {
		return;
	}

	Rect2 dr;
	Rect2 src_c;
	if (get_rect_region(p_rect, p_src_rect, dr, src_c)) {
		atlas->draw_rect_region(p_canvas_item, dr, src_c, p_modulate, p_transpose, filter_clip);
	}
}

bool AtlasTexture::get_rect_region(const Rect2 &p_rect, const Rect2 &p_src_rect, Rect2 &r_rect, Rect2 &r_src_rect) const {
	if (atlas.is_null()) {
		return false;
	}

	Rect2 src = p_src_rect;
	if (src.size == Size2()) {
		src.size = region.size;
	}
	if (src.size == Size2() && atlas.is_valid()) {
		src.size = atlas->get_size();
	}
	Vector2 scale = p_rect.size / src.size;

	src.position += (region.position - margin.position);
	Rect2 src_clipped = _get_region_rect().intersection(src);
	if (src_clipped.size == Size2()) {
		return false;
	}

	Vector2 ofs = (src_clipped.position - src.position);
	if (scale.x < 0) {
		ofs.x += (src_clipped.size.x - src.size.x);
	}
	if (scale.y < 0) {
		ofs.y += (src_clipped.size.y - src.size.y);
	}

	r_rect = Rect2(p_rect.position + ofs * scale, src_clipped.size * scale);
	r_src_rect = src_clipped;
	return true;
}

bool AtlasTexture::is_pixel_opaque(int p_x, int p_y) const {
	if (atlas.is_null()) {
		return true;
	}

	int x = p_x + region.position.x - margin.position.x;
	int y = p_y + region.position.y - margin.position.y;

	// Margin edge may outside of atlas.
	if (x < 0 || x >= atlas->get_width()) {
		return false;
	}
	if (y < 0 || y >= atlas->get_height()) {
		return false;
	}

	return atlas->is_pixel_opaque(x, y);
}

Ref<Image> AtlasTexture::get_image() const {
	if (atlas.is_null()) {
		return Ref<Image>();
	}

	const Ref<Image> &atlas_image = atlas->get_image();
	if (atlas_image.is_null()) {
		return Ref<Image>();
	}

	return atlas_image->get_region(_get_region_rect());
}

AtlasTexture::AtlasTexture() {}
