#pragma once

#include "scene/3d/path_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/multimesh.h"
#include "scene/resources/packed_scene.h"

#include "path_tool.hpp"
//namespace godot {

class PathScene3D : public Node3D {
	GDCLASS(PathScene3D, Node3D)
	PATH_TOOL(PathScene3D, SCENE)

public:
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

	void set_scene(const Ref<PackedScene> &p_scene);
	Ref<PackedScene> get_scene() const;

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

	TypedArray<Node3D> bake_instances();

	PackedStringArray get_configuration_warnings() const override;

	~PathScene3D() override;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &property) const;

private:
	Ref<PackedScene> scene;
	Distribution distribution = DISTRIBUTE_BY_COUNT;
	Alignment alignment = ALIGN_FROM_START;
	uint64_t count = 1;
	double distance = 1.0;
	Rotation rotation_mode = ROTATE_PATH;
	Vector3 rotation = Vector3();
	bool sample_cubic = false;

	LocalVector<Node3D *> instances;

	void _on_scene_changed();
};

VARIANT_ENUM_CAST(PathScene3D::RelativeTransform)
VARIANT_ENUM_CAST(PathScene3D::Distribution)
VARIANT_ENUM_CAST(PathScene3D::Rotation)
VARIANT_ENUM_CAST(PathScene3D::Alignment)
