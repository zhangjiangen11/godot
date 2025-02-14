#include "path_extrude_profile_rect.hpp"

//using namespace godot;


Array PathExtrudeProfileRect::_generate_cross_section() {
    PackedVector2Array cs;
    PackedVector2Array norms;
    
    const double height_subdiv = rect.size.y / (subdivisions.y + 1);
    const double width_subdiv = rect.size.x / (subdivisions.x + 1);
    const double norm_length = 1.0 / Math::sqrt(2.0);

    double x = rect.position.x;
    double y = rect.position.y;

    cs.push_back(Vector2(x, y));
    if (smooth_normals) {
        norms.push_back(Vector2(-norm_length, -norm_length));
    } else {
        norms.push_back(Vector2(0.0, -1.0));
    }

    for (uint64_t idx = 0; idx < subdivisions.x; ++idx) {
        x += width_subdiv;
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(0.0, -1.0));
    }
    x += width_subdiv;

    if (smooth_normals) {
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(norm_length, -norm_length));
    } else {
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(0.0, -1.0));
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(1.0, 0.0));
    }

    for (uint64_t idx = 0; idx < subdivisions.y; ++idx) {
        y += height_subdiv;
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(1.0, 0.0));
    }
    y += height_subdiv;

    if (smooth_normals) {
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(norm_length, norm_length));
    } else {
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(1.0, 0.0));
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(0.0, 1.0));
    }

    for (uint64_t idx = 0; idx < subdivisions.x; ++idx) {
        x -= width_subdiv;
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(0.0, 1.0));
    }
    x -= width_subdiv;

    if (smooth_normals) {
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(-norm_length, norm_length));
    } else {
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(0.0, 1.0));
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(-1.0, 0.0));
    }

    for (uint64_t idx = 0; idx < subdivisions.y; ++idx) {
        y -= height_subdiv;
        cs.push_back(Vector2(x, y));
        norms.push_back(Vector2(-1.0, 0.0));
    }
    y -= height_subdiv;

    cs.push_back(Vector2(x, y));
    if (smooth_normals) {
        norms.push_back(Vector2(-norm_length, -norm_length));
    } else {
        norms.push_back(Vector2(-1.0, 0.0));
    }

	Array ret;
	ret.push_back(cs);
	ret.push_back(norms);
    return ret;
}

void PathExtrudeProfileRect::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_rect", "rect"), &PathExtrudeProfileRect::set_rect);
    ClassDB::bind_method(D_METHOD("get_rect"), &PathExtrudeProfileRect::get_rect);
    ADD_PROPERTY(PropertyInfo(Variant::RECT2, "rect"), "set_rect", "get_rect");

    ClassDB::bind_method(D_METHOD("set_subdivisions", "subdivisions"), &PathExtrudeProfileRect::set_subdivisions);
    ClassDB::bind_method(D_METHOD("get_subdivisions"), &PathExtrudeProfileRect::get_subdivisions);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "subdivisions", PROPERTY_HINT_RANGE, "0,256,1,or_greater"), "set_subdivisions", "get_subdivisions");

    ClassDB::bind_method(D_METHOD("set_smooth_normals", "smooth_normals"), &PathExtrudeProfileRect::set_smooth_normals);
    ClassDB::bind_method(D_METHOD("get_smooth_normals"), &PathExtrudeProfileRect::get_smooth_normals);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "smooth_normals"), "set_smooth_normals", "get_smooth_normals");
}
