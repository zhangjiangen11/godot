#pragma once

#include "core/io/resource.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "core/templates/vmap.h"
#include "core/templates/vset.h"
#include "scene/resources/material.h"

#include "scene/resources/mesh.h"

#include "../octmesh/mmesh_lod.h"

class MIntersectionInfo {
public:
	int num_sockts;
	float lenght;
	Ref<Material> material;
	PackedVector3Array vertex;
	PackedVector3Array normal;
	PackedFloat32Array tangent;
	PackedColorArray color;
	PackedVector2Array uv;
	PackedVector2Array uv2;
	PackedInt32Array index;
	PackedFloat32Array weights;
};

struct MIntersectionInfoSurfaces : public RefCounted {
	RID mesh_rid = RID();
	Vector<MIntersectionInfo> intersections;
};

class MIntersection : public Resource {
	GDCLASS(MIntersection, Resource);

protected:
	static void _bind_methods();

private:
	bool _is_init = false;
	Ref<MMeshLod> mesh;
	TypedArray<Transform3D> sockets;
	Vector<Ref<MIntersectionInfoSurfaces>> mesh_info;

	Ref<ArrayMesh> debug_mesh;

public:
	bool is_init();
	Ref<MIntersectionInfoSurfaces> get_mesh_info(int lod);
	void generate_mesh_info();

private:
	MIntersectionInfo _generate_mesh_info(Ref<Mesh> m, int surface_index);

public:
	Ref<ArrayMesh> get_debug_mesh();

	void set_mesh(Ref<MMeshLod> input);
	Ref<MMeshLod> get_mesh();
	int get_mesh_count();

	void set_sockets(TypedArray<Transform3D> input);
	TypedArray<Transform3D> get_sockets();
	int get_socket_count();
	Vector<Transform3D> _get_sockets();
};
