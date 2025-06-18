#pragma once

#include "scene/3d/physics/area_3d.h"

#include "path_collision_tool.hpp"
#include "path_tool.hpp"
#include "path_mesh_3d.hpp"


class MultiMeshInstance3D;

class PathArea3D : public Area3D {
	GDCLASS(PathArea3D, Area3D)
	PATH_TOOL(PathArea3D, SHAPE)
	PATH_COLLISION_TOOL_HEADER(PathArea3D, Area3D, area)
};


VARIANT_ENUM_CAST(PathArea3D::Distribution)
VARIANT_ENUM_CAST(PathArea3D::Alignment)
VARIANT_ENUM_CAST(PathArea3D::Rotation)
VARIANT_ENUM_CAST(PathArea3D::RelativeTransform)
