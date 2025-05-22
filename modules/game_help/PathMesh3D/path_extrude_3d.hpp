#pragma once

#include "scene/3d/path_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#include "scene/resources/mesh.h"

#include "path_extrude_profile_base.hpp"

//namespace godot {

class PathExtrude3D : public GeometryInstance3D {
	GDCLASS(PathExtrude3D, GeometryInstance3D)

public:
	enum MeshTransform {
		TRANSFORM_MESH_LOCAL,
		TRANSFORM_MESH_PATH_NODE,
		TRANSFORM_MESH_MAX,
	};
	enum EndCaps {
		END_CAPS_NONE = 0,
		END_CAPS_START = 1 << 0,
		END_CAPS_END = 1 << 1,
		END_CAPS_BOTH = END_CAPS_START | END_CAPS_END
	};

	void set_path_3d(Path3D *p_path);
	Path3D *get_path_3d() const;

	void set_profile(const Ref<PathExtrudeProfileBase> &p_profile);
	Ref<PathExtrudeProfileBase> get_profile() const;

	void set_mesh_transform(const MeshTransform p_transform);
	MeshTransform get_mesh_transform() const;

	void set_tessellation_max_stages(const int32_t p_tessellation_max_stages);
	int32_t get_tessellation_max_stages() const;

	void set_tessellation_tolerance_degrees(const double p_tessellation_tolerance_degrees);
	double get_tessellation_tolerance_degrees() const;

	void set_end_cap_mode(const BitField<EndCaps> p_end_cap_mode);
	BitField<EndCaps> get_end_cap_mode() const;

	void set_offset(const Vector2 p_offset);
	Vector2 get_offset() const;

	void set_offset_angle(const double p_offset_angle);
	double get_offset_angle() const;

	void set_sample_cubic(const bool p_cubic);
	bool get_sample_cubic() const;

	void set_tilt(const bool p_tilt);
	bool get_tilt() const;

	void set_material(const Ref<Material> &p_material);
	Ref<Material> get_material() const;

	void queue_rebuild();

	Ref<ArrayMesh> get_baked_mesh() const;

	uint64_t get_triangle_count() const;

	Node *create_trimesh_collision_node();
	void create_trimesh_collision();

	Node *create_convex_collision_node(bool p_clean = true, bool p_simplify = false);
	void create_convex_collision(bool p_clean = true, bool p_simplify = false);

	Node *create_multiple_convex_collision_node(const Ref<MeshConvexDecompositionSettings> &p_settings = nullptr);
	void create_multiple_convex_collision(const Ref<MeshConvexDecompositionSettings> &p_settings = nullptr);

	PathExtrude3D();
	~PathExtrude3D();

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	Ref<PathExtrudeProfileBase> profile;
	Ref<ArrayMesh> generated_mesh;
	Path3D *path3d = nullptr;

	MeshTransform mesh_transform = TRANSFORM_MESH_LOCAL;

	int32_t tessellation_max_stages = 5;
	double tessellation_tolerance_degrees = 4.0;
	Vector2 offset = Vector2();
	double offset_angle = 0.0;
	bool sample_cubic = false;
	bool tilt = true;
	EndCaps end_cap_mode = END_CAPS_BOTH;
	Ref<Material> material;
	bool dirty = true;
	uint64_t n_tris = 0;

	Transform3D local_transform = Transform3D();
	Transform3D path_transform = Transform3D();

	void _rebuild_mesh();
	void _on_profile_changed();
	void _on_curve_changed();
	Node *_setup_collision_node(const Ref<Shape3D> &shape);
	void _add_child_collision_node(Node *p_node);
};

VARIANT_ENUM_CAST(PathExtrude3D::MeshTransform);
VARIANT_BITFIELD_CAST(PathExtrude3D::EndCaps);
