#ifndef MPAINTCOLOR
#define MPAINTCOLOR

#include "../mcolor_brush.h"


class MPaintColor : public MColorBrush{
    Color color;
    float hardness;
    String _get_name();
    void _set_property(String prop_name, Variant value);
    bool is_two_point_brush();
    void before_draw();
    void set_color(int32_t local_x,int32_t local_y,int32_t x,int32_t y,MImage* img);
};
#endif
