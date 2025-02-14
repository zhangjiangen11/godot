#include "path_extrude_3d.hpp"

#include "scene/3d/physics/collision_object_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "core/math/geometry_2d.h"



//using namespace godot;

void PathExtrude3D::set_path_3d(Path3D *p_path) {
    if (p_path != path3d) {
        if (path3d != nullptr && path3d->is_connected("curve_changed", callable_mp(this, &PathExtrude3D::_on_curve_changed))) {
            path3d->disconnect("curve_changed", callable_mp(this, &PathExtrude3D::_on_curve_changed));
        }

        path3d = p_path;

        if (path3d != nullptr) {
            path3d->connect("curve_changed", callable_mp(this, &PathExtrude3D::_on_curve_changed));
        }

        _on_curve_changed();
    }
}

Path3D *PathExtrude3D::get_path_3d() const {
    return path3d;
}

void PathExtrude3D::set_profile(const Ref<PathExtrudeProfileBase> &p_profile) {
    if (p_profile != profile) {
        if (profile.is_valid() && profile->is_connected("changed", callable_mp(this, &PathExtrude3D::_on_profile_changed))) {
            profile->disconnect("changed", callable_mp(this, &PathExtrude3D::_on_profile_changed));
        }
        profile = p_profile;
        if (profile.is_valid()) {
            profile->connect("changed", callable_mp(this, &PathExtrude3D::_on_profile_changed));
        }

        _on_profile_changed();
    }
}

Ref<PathExtrudeProfileBase> PathExtrude3D::get_profile() const {
    return profile;
}

void PathExtrude3D::set_tessellation_max_stages(const int32_t p_tesselation_max_stages) {
    if (tessellation_max_stages != p_tesselation_max_stages) {
        tessellation_max_stages = p_tesselation_max_stages;
        queue_rebuild();
    }
}

int32_t PathExtrude3D::get_tessellation_max_stages() const {
    return tessellation_max_stages;
}

void PathExtrude3D::set_tessellation_tolerance_degrees(const double p_tessellation_tolerance_degrees) {
    if (tessellation_tolerance_degrees != p_tessellation_tolerance_degrees) {
        tessellation_tolerance_degrees = p_tessellation_tolerance_degrees;
        queue_rebuild();
    }
}

double PathExtrude3D::get_tessellation_tolerance_degrees() const {
    return tessellation_tolerance_degrees;
}

void PathExtrude3D::set_end_cap_mode(const BitField<EndCaps> p_end_cap_mode) {
    if (end_cap_mode != p_end_cap_mode) {
        end_cap_mode = EndCaps(int(p_end_cap_mode));
        queue_rebuild();
    }
}

BitField<PathExtrude3D::EndCaps> PathExtrude3D::get_end_cap_mode() const {
    return end_cap_mode;
}

void PathExtrude3D::set_offset(const Vector2 p_offset) {
    if (offset != p_offset) {
        offset = p_offset;
        queue_rebuild();
    }
}

Vector2 PathExtrude3D::get_offset() const {
    return offset;
}

void PathExtrude3D::set_offset_angle(const double p_offset_angle) {
    if (offset_angle != p_offset_angle) {
        offset_angle = p_offset_angle;
        queue_rebuild();
    }
}

double PathExtrude3D::get_offset_angle() const {
    return offset_angle;
}

void PathExtrude3D::set_sample_cubic(const bool p_cubic) {
    if (sample_cubic != p_cubic) {
        sample_cubic = p_cubic;
        queue_rebuild();
    }
}

bool PathExtrude3D::get_sample_cubic() const {
    return sample_cubic;
}

void PathExtrude3D::set_tilt(const bool p_tilt) {
    if (tilt != p_tilt) {
        tilt = p_tilt;
        queue_rebuild();
    }
}

bool PathExtrude3D::get_tilt() const {
    return tilt;
}

Ref<ArrayMesh> PathExtrude3D::get_baked_mesh() const {
    return generated_mesh->duplicate();
}

Node *PathExtrude3D::create_trimesh_collision_node() {
    return _setup_collision_node(generated_mesh->create_trimesh_shape());
}

void PathExtrude3D::create_trimesh_collision() {
    _add_child_collision_node(create_trimesh_collision_node());
}

Node *PathExtrude3D::create_convex_collision_node(bool p_clean, bool p_simplify) {
    return _setup_collision_node(generated_mesh->create_convex_shape(p_clean, p_simplify));
}

void PathExtrude3D::create_convex_collision(bool p_clean, bool p_simplify) {
    _add_child_collision_node(create_convex_collision_node(p_clean, p_simplify));
}

Node *PathExtrude3D::create_multiple_convex_collision_node(const Ref<MeshConvexDecompositionSettings> &p_settings) {
    Ref<MeshConvexDecompositionSettings> settings = p_settings;
    if (settings.is_null()) {
        settings.instantiate();
    }

    // # TODO: GDExtension doesn't have API parity here...
    Vector<Ref<Shape3D>> shapes; // = generated_mesh->convex_decompose(settings); 
    if (shapes.is_empty()) {
        return nullptr;
    }

    StaticBody3D *static_body = memnew(StaticBody3D);
    for (int i = 0; i < shapes.size(); ++i) {
        CollisionShape3D *cshape = memnew(CollisionShape3D);
        cshape->set_shape(shapes[i]);
        static_body->add_child(cshape, true);
    }
    return static_body;
}

void PathExtrude3D::create_multiple_convex_collision(const Ref<MeshConvexDecompositionSettings> &p_settings) {
    StaticBody3D *static_body = Object::cast_to<StaticBody3D>(create_multiple_convex_collision_node(p_settings));
    ERR_FAIL_NULL(static_body);
    static_body->set_name(String(get_name()) + "Collision");

    add_child(static_body, true);
    if (get_owner() != nullptr) {
        static_body->set_owner(get_owner());
        int count = static_body->get_child_count();
        for (int i = 0; i < count; ++i) {
            CollisionShape3D *cshape = Object::cast_to<CollisionShape3D>(static_body->get_child(i));
            cshape->set_owner(get_owner());
        }
    }
}

PathExtrude3D::PathExtrude3D() {
    generated_mesh.instantiate();
}

PathExtrude3D::~PathExtrude3D() {
    generated_mesh.unref();
}

void PathExtrude3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("queue_rebuild"), &PathExtrude3D::queue_rebuild);
    ClassDB::bind_method(D_METHOD("get_baked_mesh"), &PathExtrude3D::get_baked_mesh);
    ClassDB::bind_method(D_METHOD("create_trimesh_collision"), &PathExtrude3D::create_trimesh_collision);
    ClassDB::bind_method(D_METHOD("create_convex_collision", "clean", "simplify"), &PathExtrude3D::create_convex_collision);
    ClassDB::bind_method(D_METHOD("create_multiple_convex_collision", "settings"), &PathExtrude3D::create_multiple_convex_collision);

    ClassDB::bind_method(D_METHOD("set_path_3d", "path"), &PathExtrude3D::set_path_3d);
    ClassDB::bind_method(D_METHOD("get_path_3d"), &PathExtrude3D::get_path_3d);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "path_3d", PROPERTY_HINT_NODE_TYPE, "Path3D"), "set_path_3d", "get_path_3d");

    ClassDB::bind_method(D_METHOD("set_profile", "profile"), &PathExtrude3D::set_profile);
    ClassDB::bind_method(D_METHOD("get_profile"), &PathExtrude3D::get_profile);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, "PathExtrudeProfileBase"), "set_profile", "get_profile");

    ClassDB::bind_method(D_METHOD("set_tessellation_max_stages", "tessellation_max_stages"), &PathExtrude3D::set_tessellation_max_stages);
    ClassDB::bind_method(D_METHOD("get_tessellation_max_stages"), &PathExtrude3D::get_tessellation_max_stages);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "tessellation_max_stages"), "set_tessellation_max_stages", "get_tessellation_max_stages");

    ClassDB::bind_method(D_METHOD("set_tessellation_tolerance_degrees", "tessellation_tolerance_degrees"), &PathExtrude3D::set_tessellation_tolerance_degrees);
    ClassDB::bind_method(D_METHOD("get_tessellation_tolerance_degrees"), &PathExtrude3D::get_tessellation_tolerance_degrees);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tessellation_tolerance_degrees"), "set_tessellation_tolerance_degrees", "get_tessellation_tolerance_degrees");

    ClassDB::bind_method(D_METHOD("set_end_cap_mode", "end_cap_mode"), &PathExtrude3D::set_end_cap_mode);
    ClassDB::bind_method(D_METHOD("get_end_cap_mode"), &PathExtrude3D::get_end_cap_mode);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "end_cap_mode", PROPERTY_HINT_FLAGS, "Start,End"), "set_end_cap_mode", "get_end_cap_mode");

    ClassDB::bind_method(D_METHOD("set_offset", "offset"), &PathExtrude3D::set_offset);
    ClassDB::bind_method(D_METHOD("get_offset"), &PathExtrude3D::get_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_offset", "get_offset");

    ClassDB::bind_method(D_METHOD("set_offset_angle", "offset_angle"), &PathExtrude3D::set_offset_angle);
    ClassDB::bind_method(D_METHOD("get_offset_angle"), &PathExtrude3D::get_offset_angle);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "offset_angle", PROPERTY_HINT_RANGE, "0.0,360.0,0.01,radians_as_degrees"), "set_offset_angle", "get_offset_angle");

    ClassDB::bind_method(D_METHOD("set_sample_cubic", "sample_cubic"), &PathExtrude3D::set_sample_cubic);
    ClassDB::bind_method(D_METHOD("get_sample_cubic"), &PathExtrude3D::get_sample_cubic);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sample_cubic"), "set_sample_cubic", "get_sample_cubic");

    ClassDB::bind_method(D_METHOD("set_tilt", "tilt"), &PathExtrude3D::set_tilt);
    ClassDB::bind_method(D_METHOD("get_tilt"), &PathExtrude3D::get_tilt);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tilt"), "set_tilt", "get_tilt");

    ClassDB::bind_method(D_METHOD("set_material", "material"), &PathExtrude3D::set_material);
    ClassDB::bind_method(D_METHOD("get_material"), &PathExtrude3D::get_material);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_material", "get_material");

    ADD_SIGNAL(MethodInfo("profile_changed"));
    ADD_SIGNAL(MethodInfo("curve_changed"));

    BIND_BITFIELD_FLAG(END_CAPS_NONE);
    BIND_BITFIELD_FLAG(END_CAPS_START);
    BIND_BITFIELD_FLAG(END_CAPS_END);
    BIND_BITFIELD_FLAG(END_CAPS_BOTH);
}

void PathExtrude3D::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_INTERNAL_PROCESS: {
            if (!initial_dirty) {
                break;
            }
            initial_dirty = false;
            set_process_internal(false);
        }
        case NOTIFICATION_READY: {
            set_process_internal(true);
            queue_rebuild();
        } break;
    }
}

Node *PathExtrude3D::_setup_collision_node(const Ref<Shape3D> &shape) {
    StaticBody3D *static_body = memnew(StaticBody3D);
    CollisionShape3D *cshape = memnew(CollisionShape3D);
    cshape->set_shape(shape);
    static_body->add_child(cshape, true);
    return static_body;
}

void PathExtrude3D::_add_child_collision_node(Node *p_node) {
    if (p_node != nullptr) {
        add_child(p_node, true);
        if (get_owner() != nullptr) {
            CollisionShape3D *cshape = Object::cast_to<CollisionShape3D>(p_node->get_child(0));
            p_node->set_owner(get_owner());
            if (cshape != nullptr) {
                cshape->set_owner(get_owner());
            }
        }
    }
}

void PathExtrude3D::queue_rebuild() {
    dirty = true;
    callable_mp(this, &PathExtrude3D::_rebuild_mesh).call_deferred();
}

void PathExtrude3D::_rebuild_mesh() {
    if (!dirty) {
        return;
    }
    dirty = false;

    generated_mesh->clear_surfaces();

    if (profile.is_null() || path3d == nullptr || path3d->get_curve().is_null()) {
        return;
    }

    Ref<Curve3D> curve = path3d->get_curve();
    if (curve->get_point_count() < 2) {
        return;
    }

    PackedVector3Array tessellated_points = curve->tessellate(tessellation_max_stages, tessellation_tolerance_degrees);
    uint64_t n_slices = tessellated_points.size();
    if (n_slices < 2) {
        return;
    }

    double baked_l = curve->get_baked_length();

    Array arrays = profile->get_mesh_arrays();
    if (arrays.size() == 0 || arrays[0].get_type() != Variant::PACKED_VECTOR2_ARRAY) {
        return;
    }

    PackedVector2Array cross_section = arrays[0];
    uint64_t n_vertices = cross_section.size();
    if (n_vertices < 2) {
        return;
    }

    Vector<bool> has_column;
    has_column.resize(Mesh::ARRAY_MAX);
    for (int idx_type = 0; idx_type < Mesh::ARRAY_MAX; ++idx_type) {
        has_column.write[idx_type] = arrays.size() > idx_type && arrays[idx_type].get_type() != Variant::NIL;
    }

    #define MAKE_OLD_ARRAY(m_type, m_name, m_index) \
        m_type m_name = has_column[m_index] ? m_type(arrays[m_index]) : m_type()
    MAKE_OLD_ARRAY(PackedVector2Array, old_norms, Mesh::ARRAY_NORMAL);
    MAKE_OLD_ARRAY(PackedFloat32Array, old_tang, Mesh::ARRAY_TANGENT);
    MAKE_OLD_ARRAY(PackedFloat64Array, old_v1, Mesh::ARRAY_TEX_UV);
    MAKE_OLD_ARRAY(PackedFloat64Array, old_v2, Mesh::ARRAY_TEX_UV2);
    MAKE_OLD_ARRAY(PackedColorArray, old_colors, Mesh::ARRAY_COLOR);
    MAKE_OLD_ARRAY(PackedByteArray, old_custom0, Mesh::ARRAY_CUSTOM0);
    MAKE_OLD_ARRAY(PackedByteArray, old_custom1, Mesh::ARRAY_CUSTOM1);
    MAKE_OLD_ARRAY(PackedByteArray, old_custom2, Mesh::ARRAY_CUSTOM2);
    MAKE_OLD_ARRAY(PackedByteArray, old_custom3, Mesh::ARRAY_CUSTOM3);
    MAKE_OLD_ARRAY(PackedInt32Array, old_bones, Mesh::ARRAY_BONES);
    MAKE_OLD_ARRAY(PackedFloat32Array, old_weights, Mesh::ARRAY_WEIGHTS);
    #undef MAKE_OLD_ARRAY

    arrays.clear();

    Transform2D cs_transform = Transform2D(offset_angle, offset);
    for (Vector2 &vert : cross_section) {
        Vector2 tmp = cs_transform.xform(vert);
        vert.x = tmp.x;
        vert.y = tmp.y;
    }
    for (Vector2 &norm : old_norms) {
        Vector2 tmp = cs_transform.basis_xform(norm);
        norm.x = tmp.x;
        norm.y = tmp.y;
    }

    Vector<Transform3D> transforms;
    transforms.resize(n_slices);
    for (uint64_t idx_slice = 0; idx_slice < n_slices; ++idx_slice) {
        transforms.write[idx_slice] = curve->sample_baked_with_rotation(
                curve->get_closest_offset(tessellated_points[idx_slice]), sample_cubic, tilt);
    }

    uint64_t new_size = n_slices * n_vertices * 6;
    PackedVector3Array new_vertices; new_vertices.resize(new_size);
    PackedVector3Array new_normals; if (has_column[Mesh::ARRAY_NORMAL])new_normals.resize(new_size);
    PackedFloat64Array new_tangents; if (has_column[Mesh::ARRAY_TANGENT])new_tangents.resize(new_size * 4);
    PackedVector2Array new_uv1; if (has_column[Mesh::ARRAY_TEX_UV]) new_uv1.resize(new_size);
    PackedVector2Array new_uv2; if (has_column[Mesh::ARRAY_TEX_UV2]) new_uv2.resize(new_size);
    PackedColorArray new_colors; if (has_column[Mesh::ARRAY_COLOR]) new_colors.resize(new_size);
    PackedByteArray new_custom0; if (has_column[Mesh::ARRAY_CUSTOM0]) new_custom0.resize(new_size);
    PackedByteArray new_custom1; if (has_column[Mesh::ARRAY_CUSTOM1]) new_custom1.resize(new_size);
    PackedByteArray new_custom2; if (has_column[Mesh::ARRAY_CUSTOM2])new_custom2.resize(new_size);
    PackedByteArray new_custom3;  if (has_column[Mesh::ARRAY_CUSTOM3]) new_custom3.resize(new_size);
    PackedInt32Array new_bones; if (has_column[Mesh::ARRAY_BONES]) new_bones.resize(new_size);
    PackedFloat32Array new_weights; if (has_column[Mesh::ARRAY_WEIGHTS]) new_weights.resize(new_size);

    uint64_t k = 0;

    for (uint64_t idx_slice = 0; idx_slice < n_slices - 1; ++idx_slice) {
        const Transform3D &this_transform = transforms[idx_slice];
        const Transform3D &next_transform = transforms[idx_slice + 1];

        double u = curve->get_closest_offset(this_transform.origin) / baked_l;

        for (uint64_t idx_seg = 0; idx_seg < cross_section.size() - 1; ++idx_seg) {
            Vector2 p1 = cross_section[idx_seg];
            Vector2 p2 = cross_section[idx_seg + 1];
            Vector3 this_p1 = this_transform.xform(Vector3(p1.x, p1.y, 0.0));
            Vector3 this_p2 = this_transform.xform(Vector3(p2.x, p2.y, 0.0));
            Vector3 next_p1 = next_transform.xform(Vector3(p1.x, p1.y, 0.0));
            Vector3 next_p2 = next_transform.xform(Vector3(p2.x, p2.y, 0.0));
            
            new_vertices.write[k] = this_p1;
            new_vertices.write[k + 1] = this_p2;
            new_vertices.write[k + 2] = next_p1;
            new_vertices.write[k + 3] = this_p2;
            new_vertices.write[k + 4] = next_p2;
            new_vertices.write[k + 5] = next_p1;

            if (has_column[Mesh::ARRAY_NORMAL]) {
                Vector2 n1 = old_norms[idx_seg];
                Vector2 n2 = old_norms[idx_seg + 1];

                Vector3 this_n1 = this_transform.basis.xform(Vector3(n1.x, n1.y, 0.0));
                Vector3 this_n2 = this_transform.basis.xform(Vector3(n2.x, n2.y, 0.0));
                Vector3 next_n1 = next_transform.basis.xform(Vector3(n1.x, n1.y, 0.0));
                Vector3 next_n2 = next_transform.basis.xform(Vector3(n2.x, n2.y, 0.0));

                new_normals.write[k] = this_n1;
                new_normals.write[k + 1] = this_n2;
                new_normals.write[k + 2] = next_n1;
                new_normals.write[k + 3] = this_n2;
                new_normals.write[k + 4] = next_n2;
                new_normals.write[k + 5] = next_n1;
            }

            if (has_column[Mesh::ARRAY_TANGENT]) {
                double t11 = old_tang[4 * idx_seg];
                double t12 = old_tang[4 * idx_seg + 1];
                double t13 = old_tang[4 * idx_seg + 2];
                double t14 = old_tang[4 * idx_seg + 3];
                double t21 = old_tang[4 * (idx_seg + 1)];
                double t22 = old_tang[4 * (idx_seg + 1) + 1];
                double t23 = old_tang[4 * (idx_seg + 1) + 2];
                double t24 = old_tang[4 * (idx_seg + 1) + 3];

                Vector3 this_t1 = this_transform.basis.xform(Vector3(t11, t12, t13));
                Vector3 this_t2 = this_transform.basis.xform(Vector3(t21, t22, t23));
                Vector3 next_t1 = next_transform.basis.xform(Vector3(t11, t12, t13));
                Vector3 next_t2 = next_transform.basis.xform(Vector3(t21, t22, t23));

                new_tangents.write[k*4] = this_t1.x;
                new_tangents.write[k*4 + 1] = this_t1.y;
                new_tangents.write[k*4 + 2] = this_t1.z;
                new_tangents.write[k*4 + 3] = t14;
                new_tangents.write[k*4 + 4] = this_t2.x;
                new_tangents.write[k*4 + 5] = this_t2.y;
                new_tangents.write[k*4 + 6] = this_t2.z;
                new_tangents.write[k*4 + 7] = t24;
                new_tangents.write[k*4 + 8] = next_t1.x;
                new_tangents.write[k*4 + 9] = next_t1.y;
                new_tangents.write[k*4 + 10] = next_t1.z;
                new_tangents.write[k*4 + 11] = t14;
                new_tangents.write[k*4 + 12] = this_t2.x;
                new_tangents.write[k*4 + 13] = this_t2.y;
                new_tangents.write[k*4 + 14] = this_t2.z;
                new_tangents.write[k*4 + 15] = t24;
                new_tangents.write[k*4 + 16] = next_t2.x;
                new_tangents.write[k*4 + 17] = next_t2.y;
                new_tangents.write[k*4 + 18] = next_t2.z;
                new_tangents.write[k*4 + 19] = t24;
                new_tangents.write[k*4 + 20] = next_t1.x;
                new_tangents.write[k*4 + 21] = next_t1.y;
                new_tangents.write[k*4 + 22] = next_t1.z;
                new_tangents.write[k*4 + 23] = t14;
            }

            Vector<std::pair<uint64_t, uint64_t>> idx_verts { 
                {k, idx_seg}, 
                {k + 1, idx_seg + 1},
                {k + 2, idx_seg},
                {k + 3, idx_seg + 1},
                {k + 4, idx_seg + 1},
                {k + 5, idx_seg},
            };

            if (has_column[Mesh::ARRAY_TEX_UV]) {
                for (const std::pair<uint64_t, uint64_t> &idx_vert : idx_verts) {
                    const uint64_t tmp_k = idx_vert.first;
                    const uint64_t i = idx_vert.second;
                    new_uv1.write[tmp_k] = Vector2(u, old_v1[i] / 2.0); // top half of UV space
                    new_uv2.write[tmp_k] = Vector2(u, old_v2[i] / 2.0);
                }
            }

            #define APPEND_ARRAY(m_new, m_old, m_idx) \
                if (has_column[m_idx]) { \
                    for (const std::pair<uint64_t, uint64_t> &idx_vert : idx_verts) { \
                        const uint64_t tmp_k = idx_vert.first; \
                        const uint64_t i = idx_vert.second; \
                        m_new.write[tmp_k] = m_old[i]; \
                    } \
                }
            
            APPEND_ARRAY(new_colors, old_colors, Mesh::ARRAY_COLOR);
            APPEND_ARRAY(new_custom0, old_custom0, Mesh::ARRAY_CUSTOM0);
            APPEND_ARRAY(new_custom1, old_custom1, Mesh::ARRAY_CUSTOM1);
            APPEND_ARRAY(new_custom2, old_custom2, Mesh::ARRAY_CUSTOM2);
            APPEND_ARRAY(new_custom3, old_custom3, Mesh::ARRAY_CUSTOM3);
            APPEND_ARRAY(new_bones, old_bones, Mesh::ARRAY_BONES);
            APPEND_ARRAY(new_weights, old_weights, Mesh::ARRAY_WEIGHTS);
            #undef APPEND_ARRAY

            k += 6;
        }
    }

    if (end_cap_mode != END_CAPS_NONE) {
        PackedInt32Array cap = Geometry2D::triangulate_polygon(cross_section);
        Vector2 ll = Vector2(INFINITY, INFINITY);
        Vector2 ur = Vector2(-INFINITY, -INFINITY);
        for (const int32_t &i : cap) {
            Vector2 vec = cross_section[i];
            ll.x = MIN(ll.x, vec.x);
            ll.y = MIN(ll.y, vec.y);
            ur.x = MAX(ur.x, vec.x);
            ur.y = MAX(ur.y, vec.y);
        }
        PackedVector2Array cap_uv; cap_uv.resize(cap.size());
        for (uint64_t idx = 0; idx < cap.size(); ++idx) {
            const int32_t i = cap[idx];
            Vector2 vec = cross_section[i];
            cap_uv.write[idx] = Vector2((vec.x - ll.x) / (ur.x - ll.x), (vec.y - ll.y) / (ur.y - ll.y));
        }
        Vector3 cap_normal = Vector3(0.0, 0.0, 1.0);
        if (profile->get_flip_normals()) {
            cap_normal = -cap_normal;
        } else {
            cap.reverse();
        }

        if (end_cap_mode & END_CAPS_START) {
            const Transform3D &this_transform = transforms[0];
            new_vertices.resize(new_vertices.size() + cap.size());
            uint64_t tmp_k = k;
            for (const int &i : cap) {
                new_vertices.write[tmp_k] = this_transform.xform(Vector3(cross_section[i].x, cross_section[i].y, 0.0));
                tmp_k++;
            }

            if (has_column[Mesh::ARRAY_NORMAL]) {
                new_normals.resize(new_normals.size() + cap.size());
                uint64_t tmp_k = k;
                for (const int &i : cap) {
                    new_normals.write[tmp_k] = this_transform.basis.xform(cap_normal);
                    tmp_k++;
                }
            }

            if (has_column[Mesh::ARRAY_TANGENT]) {
                new_tangents.resize(new_tangents.size() + cap.size() * 4);
                uint64_t tmp_k = k * 4;
                for (const int &i : cap) {
                    Vector3 tang = this_transform.basis.xform(Vector3(old_tang[4*i], old_tang[4*i + 1], old_tang[4*i + 2]));
                    new_tangents.write[tmp_k] = tang.x;
                    new_tangents.write[tmp_k + 1] = tang.y;
                    new_tangents.write[tmp_k + 2] = tang.z;
                    new_tangents.write[tmp_k + 3] = old_tang[4*i + 3];
                    tmp_k += 4;
                }
            }

            if (has_column[Mesh::ARRAY_TEX_UV]) {
                new_uv1.resize(new_uv1.size() + cap.size());
                uint64_t tmp_k = k;
                for (const int &i : cap) {
                    new_uv1.write[tmp_k] = cap_uv[i] / 2.0 + Vector2(0.0, 0.5); // start cap covers bottom left of UV space
                    tmp_k++;
                }
            }

            if (has_column[Mesh::ARRAY_TEX_UV2]) {
                new_uv1.resize(new_uv2.size() + cap.size());
                uint64_t tmp_k = k;
                for (const int &i : cap) {
                    new_uv2.write[tmp_k] = cap_uv[i] / 2.0 + Vector2(0.0, 0.5); // start cap covers bottom left of UV space
                    tmp_k++;
                }
            }

            #define ADD_CAP_ARRAY(m_new, m_old, m_idx) \
                if (has_column[m_idx]) { \
                    m_new.resize(m_new.size() + cap.size()); \
                    uint64_t tmp_k = k; \
                    for (const int &i : cap) { \
                        m_new.write[tmp_k] = m_old[i]; \
                        tmp_k++; \
                    } \
                }
            ADD_CAP_ARRAY(new_colors, old_colors, Mesh::ARRAY_COLOR);
            ADD_CAP_ARRAY(new_custom0, old_custom0, Mesh::ARRAY_CUSTOM0);
            ADD_CAP_ARRAY(new_custom1, old_custom1, Mesh::ARRAY_CUSTOM1);
            ADD_CAP_ARRAY(new_custom2, old_custom2, Mesh::ARRAY_CUSTOM2);
            ADD_CAP_ARRAY(new_custom3, old_custom3, Mesh::ARRAY_CUSTOM3);
            ADD_CAP_ARRAY(new_bones, old_bones, Mesh::ARRAY_BONES);
            ADD_CAP_ARRAY(new_weights, old_weights, Mesh::ARRAY_WEIGHTS);
            #undef ADD_CAP_ARRAY
            k += cap.size();
        }

        if (end_cap_mode & END_CAPS_END) {
            cap.reverse();
            cap_normal = -cap_normal;
            const Transform3D &this_transform = transforms[transforms.size() - 1];

            new_vertices.resize(new_vertices.size() + cap.size());
            uint64_t tmp_k = k;
            for (const int &i : cap) {
                new_vertices.write[tmp_k] = this_transform.xform(Vector3(cross_section[i].x, cross_section[i].y, 0.0));
                tmp_k++;
            }

            if (has_column[Mesh::ARRAY_NORMAL]) {
                new_normals.resize(new_normals.size() + cap.size());
                uint64_t tmp_k = k;
                for (const int &i : cap) {
                    new_normals.write[tmp_k] = this_transform.basis.xform(cap_normal);
                    tmp_k++;
                }
            }

            if (has_column[Mesh::ARRAY_TANGENT]) {
                new_tangents.resize(new_tangents.size() + cap.size() * 4);
                uint64_t tmp_k = k * 4;
                for (const int &i : cap) {
                    Vector3 tang = this_transform.basis.xform(Vector3(old_tang[4*i], old_tang[4*i + 1], old_tang[4*i + 2]));
                    new_tangents.write[tmp_k] = tang.x;
                    new_tangents.write[tmp_k + 1] = tang.y;
                    new_tangents.write[tmp_k + 2] = tang.z;
                    new_tangents.write[tmp_k + 3] = old_tang[4*i + 3];
                    tmp_k += 4;
                }
            }

            if (has_column[Mesh::ARRAY_TEX_UV]) {
                new_uv1.resize(new_uv1.size() + cap.size());
                uint64_t tmp_k = k;
                for (const int &i : cap) {
                    new_uv1.write[tmp_k] = cap_uv[i] / 2.0 + Vector2(0.5, 0.5); // end cap covers bottom right of UV space
                    tmp_k++;
                }
            }

            if (has_column[Mesh::ARRAY_TEX_UV2]) {
                new_uv1.resize(new_uv2.size() + cap.size());
                uint64_t tmp_k = k;
                for (const int &i : cap) {
                    new_uv2.write[tmp_k] = cap_uv[i] / 2.0 + Vector2(0.5, 0.5); // end cap covers bottom right of UV space
                    tmp_k++;
                }
            }

            #define ADD_CAP_ARRAY(m_new, m_old, m_idx) \
                if (has_column[m_idx]) { \
                    m_new.resize(m_new.size() + cap.size()); \
                    uint64_t tmp_k = k; \
                    for (const int &i : cap) { \
                        m_new.write[tmp_k] = m_old[i]; \
                        tmp_k++; \
                    } \
                }
            ADD_CAP_ARRAY(new_colors, old_colors, Mesh::ARRAY_COLOR);
            ADD_CAP_ARRAY(new_custom0, old_custom0, Mesh::ARRAY_CUSTOM0);
            ADD_CAP_ARRAY(new_custom1, old_custom1, Mesh::ARRAY_CUSTOM1);
            ADD_CAP_ARRAY(new_custom2, old_custom2, Mesh::ARRAY_CUSTOM2);
            ADD_CAP_ARRAY(new_custom3, old_custom3, Mesh::ARRAY_CUSTOM3);
            ADD_CAP_ARRAY(new_bones, old_bones, Mesh::ARRAY_BONES);
            ADD_CAP_ARRAY(new_weights, old_weights, Mesh::ARRAY_WEIGHTS);
            #undef ADD_CAP_ARRAY
            k += cap.size();
        }
    }

    arrays.resize(Mesh::ARRAY_MAX);
    arrays.fill(Variant());
    arrays[Mesh::ARRAY_VERTEX] = new_vertices;
    #define MAKE_NEW_ARRAY(m_index, m_name) arrays[m_index] = m_name.is_empty() ? Variant() : Variant(m_name)
    MAKE_NEW_ARRAY(Mesh::ARRAY_NORMAL, new_normals);
    MAKE_NEW_ARRAY(Mesh::ARRAY_TANGENT, new_tangents);
    MAKE_NEW_ARRAY(Mesh::ARRAY_TEX_UV, new_uv1);
    MAKE_NEW_ARRAY(Mesh::ARRAY_TEX_UV2, new_uv2);
    MAKE_NEW_ARRAY(Mesh::ARRAY_COLOR, new_colors);
    MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM0, new_custom0);
    MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM1, new_custom1);
    MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM2, new_custom2);
    MAKE_NEW_ARRAY(Mesh::ARRAY_CUSTOM3, new_custom3);
    MAKE_NEW_ARRAY(Mesh::ARRAY_BONES, new_bones);
    MAKE_NEW_ARRAY(Mesh::ARRAY_WEIGHTS, new_weights);
    #undef MAKE_NEW_ARRAY

    generated_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    generated_mesh->surface_set_material(0, material);
    set_base(generated_mesh->get_rid());
}

void PathExtrude3D::_on_profile_changed() {
    queue_rebuild();
    emit_signal("profile_changed");
}

void PathExtrude3D::_on_curve_changed() {
    queue_rebuild();
    emit_signal("curve_changed");
}
