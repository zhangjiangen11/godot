#pragma once

#include "scene/3d/path_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/multimesh.h"

//namespace godot {

class PathMultiMesh3D : public GeometryInstance3D {
	GDCLASS(PathMultiMesh3D, GeometryInstance3D)

public:
	enum MeshTransform {
		TRANSFORM_MESH_LOCAL,
		TRANSFORM_MESH_PATH_NODE,
		TRANSFORM_MESH_MAX,
	};
	enum Distribution {
		DISTRIBUTE_BY_COUNT = 0,
		DISTRIBUTE_BY_DISTANCE = 1,
		DISTRIBUTE_MAX = 2,
	};
	enum Rotation {
		ROTATE_FIXED = 0,
		ROTATE_PATH = 1,
		ROTATE_RANDOM = 2,
		ROTATE_MAX = 3,
	};
	enum Alignment {
		ALIGN_FROM_START = 0,
		ALIGN_CENTERED = 1,
		ALIGN_FROM_END = 2,
		ALIGN_MAX = 3,
	};

	void set_multi_mesh(const Ref<MultiMesh> &p_multi_mesh);
	Ref<MultiMesh> get_multi_mesh() const;

	void set_path_3d(Path3D *p_path);
	Path3D *get_path_3d() const;

	void set_mesh_transform(MeshTransform p_transform);
	MeshTransform get_mesh_transform() const;

	void set_distribution(Distribution p_distribution);
	Distribution get_distribution() const;

	void set_alignment(Alignment p_alignment);
	Alignment get_alignment() const;

	void set_count(uint64_t p_count);
	uint64_t get_count() const;

	void set_distance(double p_distance);
	double get_distance() const;

	void set_rotation_mode(Rotation p_rotation_mode);
	Rotation get_rotation_mode() const;

	void set_rotation(const Vector3 &p_rotation);
	Vector3 get_rotation() const;

	void set_sample_cubic(bool p_cubic);
	bool get_sample_cubic() const;

	void queue_rebuild();

	~PathMultiMesh3D() override;

protected:
	static void _bind_methods();
	void _notification(int p_what);
	void _validate_property(PropertyInfo &property) const;

private:
	Ref<MultiMesh> multi_mesh;
	Path3D *path3d = nullptr;
	MeshTransform mesh_transform = TRANSFORM_MESH_LOCAL;
	Distribution distribution = DISTRIBUTE_BY_COUNT;
	Alignment alignment = ALIGN_FROM_START;
	uint64_t count = 1;
	double distance = 1.0;
	Rotation rotation_mode = ROTATE_PATH;
	Vector3 rotation = Vector3();
	bool sample_cubic = false;
	bool dirty = true;

	Transform3D local_transform = Transform3D();
	Transform3D path_transform = Transform3D();

	void _on_mesh_changed();
	void _on_curve_changed();
	void _rebuild_mesh();
};

VARIANT_ENUM_CAST(PathMultiMesh3D::MeshTransform);
VARIANT_ENUM_CAST(PathMultiMesh3D::Distribution);
VARIANT_ENUM_CAST(PathMultiMesh3D::Rotation);
VARIANT_ENUM_CAST(PathMultiMesh3D::Alignment);
