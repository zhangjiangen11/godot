/**************************************************************************/
/*  triangle_mesh.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/math/face3.h"
#include "core/object/ref_counted.h"

class TriangleMesh : public RefCounted {
	GDCLASS(TriangleMesh, RefCounted);
	static void _bind_methods();

public:
	struct Triangle {
		Vector3 normal;
		int indices[3];
		int32_t surface_index = 0;
	};

private:
	Vector<Triangle> triangles;
	Vector<Vector3> vertices;

	struct BVH {
		AABB aabb;
		Vector3 center; //used for sorting
		int left = -1;
		int right = -1;

		int32_t face_index = -1;
	};

	struct BVHCmpX {
		bool operator()(const BVH *p_left, const BVH *p_right) const {
			return p_left->center.x < p_right->center.x;
		}
	};

	struct BVHCmpY {
		bool operator()(const BVH *p_left, const BVH *p_right) const {
			return p_left->center.y < p_right->center.y;
		}
	};
	struct BVHCmpZ {
		bool operator()(const BVH *p_left, const BVH *p_right) const {
			return p_left->center.z < p_right->center.z;
		}
	};

	int _create_bvh(BVH *p_bvh, BVH **p_bb, int p_from, int p_size, int p_depth, int &max_depth, int &max_alloc);

	Vector<BVH> bvh;
	int max_depth = 0;
	bool valid = false;

public:
	bool is_valid() const;
	bool intersect_segment(const Vector3 &p_begin, const Vector3 &p_end, Vector3 &r_point, Vector3 &r_normal, int32_t *r_surf_index = nullptr, int32_t *r_face_index = nullptr, bool auto_swap_normal = true) const;
	bool intersect_ray(const Vector3 &p_begin, const Vector3 &p_dir, Vector3 &r_point, Vector3 &r_normal, int32_t *r_surf_index = nullptr, int32_t *r_face_index = nullptr, bool auto_swap_normal = true) const;
	bool inside_convex_shape(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count, Vector3 p_scale = Vector3(1, 1, 1)) const;

	// 获取最近点.返回值w 大于0 表示有交点,小于0 表示无交点
	Vector4 get_closest_point_to(const Vector3 &p_point, float max_distance, Vector3 &r_normal, bool auto_swap_normal = true) const;
	// 和线段相交
	Dictionary _intersect_segment(const Vector3 &p_begin, const Vector3 &p_end, bool auto_swap_normal = true) const;
	// 和射线相交
	Dictionary _intersect_ray(const Vector3 &p_begin, const Vector3 &p_dir, bool auto_swap_normal = true) const;

	Dictionary _get_closest_point_to(const Vector3 &p_point, float max_distance, bool auto_swap_normal = true) const;

	Vector<Face3> get_faces() const;

	const Vector<Triangle> &get_triangles() const { return triangles; }
	const Vector<Vector3> &get_vertices() const { return vertices; }
	Vector<Vector2> get_vertices_xz() const {
		Vector<Vector2> ret;
		ret.resize(vertices.size());
		Vector2 *dest = ret.ptrw();
		const Vector3 *src = vertices.ptr();
		for (int64_t i = 0; i < vertices.size(); ++i) {
			dest[i].x = src[i].x;
			dest[i].y = src[i].z;
		}
		return ret;
	}
	Vector<Vector2> get_vertices_xy() const {
		Vector<Vector2> ret;
		ret.resize(vertices.size());
		Vector2 *dest = ret.ptrw();
		const Vector3 *src = vertices.ptr();
		for (int64_t i = 0; i < vertices.size(); ++i) {
			dest[i].x = src[i].x;
			dest[i].y = src[i].y;
		}
		return ret;
	}
	void get_indices(Vector<int> *r_triangles_indices) const;
	Vector<int> _get_indices() const;
	void create(const Vector<Vector3> &p_faces, const Vector<int32_t> &p_surface_indices = Vector<int32_t>());

	// Wrapped functions for compatibility with method bindings
	// and user exposed script api that can't use more native types.
	bool create_from_faces(const Vector<Vector3> &p_faces);
	Dictionary intersect_segment_scriptwrap(const Vector3 &p_begin, const Vector3 &p_end) const;
	Dictionary intersect_ray_scriptwrap(const Vector3 &p_begin, const Vector3 &p_dir) const;
	Vector<Vector3> get_faces_scriptwrap() const;

	TriangleMesh();
};
