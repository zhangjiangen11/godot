#pragma once

#include "../mcolor_brush.h"

class MPaintColor : public MColorBrush {
	Color color;
	float hardness = 0.0f;
	String _get_name();
	void _set_property(String prop_name, Variant value);
	bool is_two_point_brush();
	void before_draw();
	void set_color(uint32_t local_x, uint32_t local_y, uint32_t x, uint32_t y, MImage *img);
};
