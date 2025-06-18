#pragma once

#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/multimesh.h"

#include "path_collision_tool.hpp"
#include "path_tool.hpp"
class MultiMeshInstance3D;

class PathStaticBody3D : public StaticBody3D {
	GDCLASS(PathStaticBody3D, StaticBody3D)
	PATH_TOOL(PathStaticBody3D, SHAPE)
	PATH_COLLISION_TOOL_HEADER(PathStaticBody3D, StaticBody3D, static_body)
};

VARIANT_ENUM_CAST(PathStaticBody3D::Distribution)
VARIANT_ENUM_CAST(PathStaticBody3D::Alignment)
VARIANT_ENUM_CAST(PathStaticBody3D::Rotation)
VARIANT_ENUM_CAST(PathStaticBody3D::RelativeTransform)
