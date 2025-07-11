// https://github.com/yblin/instant-ngp/blob/b5ba2a3125119f85831bf37e37b7a95895c671ea/dependencies/codelibrary/world/tree/leaf_generator.h
// Copyright 2022 Yangbin Lin. All Rights Reserved.
//
// Author: yblin@jmu.edu.cn (Yangbin Lin)
//
// This file is part of the Code Library.
//

#ifndef CODELIBRARY_WORLD_TREE_LEAF_GENERATOR_H_
#define CODELIBRARY_WORLD_TREE_LEAF_GENERATOR_H_

#include "core/io/resource.h"
#include "core/templates/local_vector.h"
/**
 * 过程式树木模型的参数。
 */
#define DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(type, name) \
	void set_##name##_0(const type &v) {               \
		name[0] = v;                                   \
	}                                                  \
	const type &get_##name##_0() const {               \
		return name[0];                                \
	}                                                  \
	void set_##name##_1(const type &v) {               \
		name[1] = v;                                   \
	}                                                  \
	const type &get_##name##_1() const {               \
		return name[1];                                \
	}                                                  \
	void set_##name##_2(const type &v) {               \
		name[2] = v;                                   \
	}                                                  \
	const type &get_##name##_2() const {               \
		return name[2];                                \
	}                                                  \
	void set_##name##_3(const type &v) {               \
		name[3] = v;                                   \
	}                                                  \
	const type &get_##name##_3() const {               \
		return name[3];                                \
	}                                                  \
	ConstLocalVector<type, 4> name
#define ADD_SIMPLE_MEMBER_PROPERTY_ARRAY4(type, name, index, min, max)                                                                                                                     \
	{                                                                                                                                                                                      \
		ClassDB::bind_method(D_METHOD("set_" #name "_" #index, #name), &self_type::set_##name##_##index);                                                                                  \
		ClassDB::bind_method(D_METHOD("get_" #name "_" #index), &self_type::get_##name##_##index);                                                                                         \
		type __temp_name = type();                                                                                                                                                         \
		ADD_PROPERTY(PropertyInfo(Variant(__temp_name).get_type(), "_" #index "_" #name, PROPERTY_HINT_RANGE, #min "," #max ",0.0001"), "set_" #name "_" #index, "get_" #name "_" #index); \
	}
class ProceduralTreeParameter : public Resource {
	GDCLASS(ProceduralTreeParameter, Resource);
	static void _bind_methods();

public:
	// 整数0-8，通过改变第一层分支长度控制树的形状。
	// 预定义选项分别为圆锥形、球形、半球形、圆柱形、锥形圆柱、火焰形、倒锥形、趋向火焰形和自定义。
	// 自定义使用prune_*参数定义的包络直接控制树的形状，而不是通过修剪。
	DECL_SIMPLE_MEMBER_PROPERTY(int, shape) = 7;

	// 整棵树的缩放比例。
	DECL_SIMPLE_MEMBER_PROPERTY(float, scale) = 13;

	// 缩放比例的最大变化量。
	DECL_SIMPLE_MEMBER_PROPERTY(float, scale_v) = 3;

	// 分支的级数，通常为3或4。
	DECL_SIMPLE_MEMBER_PROPERTY(int, levels) = 3;

	// 树干长度与半径的比例。
	DECL_SIMPLE_MEMBER_PROPERTY(float, ratio) = 0.015f;

	// 分支半径在不同级别之间减少的幅度。
	DECL_SIMPLE_MEMBER_PROPERTY(float, ratio_power) = 1.2f;

	// 树干基部半径增加的比例，以树干半径的分数表示。
	DECL_SIMPLE_MEMBER_PROPERTY(float, flare) = 0.6f;

	// 树干基部高度处分叉的数量，如果为负数，则分叉数量将随机选择，最大为|base_splits|。
	DECL_SIMPLE_MEMBER_PROPERTY(int, base_splits) = 0;

public:
	// 控制子分支（第n级）相对于其父分支（第n-1级）的向下角度。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, down_angle);

	// down_angle[n]的最大变化量，如果<0，则down_angle_v[n]的值沿父分支分布。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, down_angle_v);

	// 分支上不生成子分支/叶子的比例。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, base_size);

	// 每个子分支围绕父分支（第n-1级）的角度。
	// 如果<0，则子分支在其父局部坐标系中从向下方向旋转rotate[n]度。
	// 对于扇形分支，扇形将展开总角度rotate[n]；对于轮生分支，每个轮生将旋转rotate[n]。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, rotation);

	// rotate_angle[n]的最大变化量。对于扇形和轮生分支，每个分支的角度将变化rotate_v[n]。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, rotation_v);

	// 第n级每个父茎上的最大子分支数量。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(int, branches);

	// 第n级分支长度相对于其父分支长度的比例。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, length);

	// length[n]的最大变化量。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, length_v);

	// 控制每个分支半径沿其长度的渐缩方式。
	//
	// 0   -  非渐缩圆柱
	// 0~1 -  分支在末端渐缩至其基部半径的该分数
	// 1   -  渐缩为锥形
	// 1~2 -  在锥形渐缩和圆形末端之间插值
	// 2   -  渐缩为球形末端
	// 2~3 -  导致周期性渐缩，半径的最大变化量等于基部半径的值−2
	// 3   -  一系列相邻的球体
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, taper);

	// 每个分支段的最大二分支数量，小数值使用类似于Floyd-Steinberg误差扩散的方法沿茎分布。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, segment_splits);

	// 二分支之间的角度。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, split_angle);

	// split_angle[n]的最大变化量。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, split_angle_v);

	// 每个分支（茎）中的段数。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(int, curve_resolution);

	// 茎的方向从起点到终点绕其局部x轴旋转的角度。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, curve);

	// curve[n]的最大变化量。在每个段随机应用。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, curve_v);

	// 茎在中途沿相反方向弯曲的角度，形成S形分支。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, curve_back);

	// 茎的方向从起点到终点绕其局部y轴可能变化的最大角度。
	// 在每个段随机应用。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, bend_v);

	// 控制分支沿其父茎的分布。
	// 0表示完全互生分支，插值到1时表示完全对生分支。
	// 值>1表示轮生分支，每个轮生中有n+1个分支。小数值导致每个轮生中有四舍五入的整数个分支，差异使用类似于Floyd-Steinberg误差扩散的方法传播。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, branch_dist);

	// 修改分支的基部半径，仅用于特殊情况，如垂柳，其中标准半径模型不足。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, radius_modify);
	// 对树在x、y和z方向生长方向的影响，z元素仅适用于第二级及以上的分支（n >= 2）。
	DECL_SIMPLE_MEMBER_PROPERTY_ARRAY4(float, tropism);

public:
	// 每个最深层分支上的叶子或花的数量。
	DECL_SIMPLE_MEMBER_PROPERTY(int, n_leaves) = 0;

	// 预定义的叶子形状。
	DECL_SIMPLE_MEMBER_PROPERTY(int, leaf_shape) = 0;

	// 叶子的缩放比例。
	DECL_SIMPLE_MEMBER_PROPERTY(float, leaf_scale) = 1.0f;

	// 叶子在x方向的缩放比例。
	DECL_SIMPLE_MEMBER_PROPERTY(float, leaf_scale_x) = 1.0f;

	// 叶子重新定向以面向光线（向上和向外）的比例。
	DECL_SIMPLE_MEMBER_PROPERTY(float, leaf_bend) = 0.0f;

public:
	// 修剪包络峰值宽度出现的位置，以从修剪底部向上的分数距离表示。
	DECL_SIMPLE_MEMBER_PROPERTY(float, prune_width_peak) = 0.5f;

	// 修剪包络下部的曲率。
	// < 1导致凸形，> 1导致凹形。
	DECL_SIMPLE_MEMBER_PROPERTY(float, prune_power_low) = 0.5f;

	// 修剪包络上部的曲率。
	// < 1导致凸形，> 1导致凹形。
	DECL_SIMPLE_MEMBER_PROPERTY(float, prune_power_high) = 0.5f;

public:
	ProceduralTreeParameter() {
		reset();
	}
	void reset() {
		down_angle = { 0, 60, 45, 45 };
		down_angle_v = { 0, -50, 10, 10 };
		base_size = { 0.3, 0.02, 0.02, 0.02 };
		rotation = { 0, 140, 140, 77 };
		rotation_v = { 0, 0, 0, 0 };
		branches = { 1, 50, 30, 1 };
		length = { 1, 0.3, 0.6, 0 };
		length_v = { 0, 0, 0, 0 };
		taper = { 1, 1, 1, 1 };
		segment_splits = { 0, 0, 0, 0 };
		split_angle = { 40, 0, 0, 0 };
		split_angle_v = { 5, 0, 0, 0 };
		curve_resolution = { 5, 5, 3, 1 };
		curve = { 0, -40, -40, 0 };
		curve_v = { 20, 50, 75, 0 };
		curve_back = { 0, 0, 0, 0 };
		bend_v = { 0, 50, 0, 0 };
		branch_dist = { 0, 0, 0, 0 };
		radius_modify = { 1, 1, 1, 1 };
		tropism = { 0, 0, 0.5, 0 };
	}
};

struct RenderData {
	enum {
		GL_TRIANGLES = 0,
		GL_LINES = 1,
		GL_POINTS = 2
	};
	// Type of render data. Should be one of the following value: GL_TRIANGLES,
	// GL_LINES, and GL_POINTS.
	int type = GL_TRIANGLES;

	// Vertex position.
	LocalVector<Vector3> vertices;

	// Color for each vertex.
	LocalVector<Color> colors;

	// Normal vector for each vertex.
	LocalVector<Vector3> normals;

	// Texture coordinate for each vertex.
	LocalVector<Vector2> texture_coords;

	// Element indices.
	LocalVector<int> indices;

	RenderData(int t = GL_TRIANGLES) :
			type(t) {}

	bool empty() const {
		return vertices.is_empty();
	}
	void SetRenderData(const RenderData &data) {
		type = data.type;
		vertices = data.vertices;
		colors = data.colors;
		normals = data.normals;
		texture_coords = data.texture_coords;
		indices = data.indices;
	}

	void clear() {
		vertices.clear();
		colors.clear();
		normals.clear();
		texture_coords.clear();
		indices.clear();
	}
};
/**
 * Instance node is a lot of models where each model contain the same set of
 * vertex data.
 */
class InstanceNode {
public:
	InstanceNode(const std::string &name = "") {
	}

	virtual ~InstanceNode() {
	}

	/**
	 * Clear the instances.
	 */
	void ClearInstances() {
		n_instances_ = 0;
		transforms_.clear();
		bounding_box_ = AABB();
		modified_ = true;
	}

	/**
	 * Reset the instance model.
	 */
	void Reset(const RenderData &instance) {
		n_instances_ = 0;
		transforms_.clear();
		instance_.SetRenderData(instance);
		modified_ = true;
	}

	/**
	 * Add an instance.
	 */
	void AddInstance(const Transform3D &transform) {
		++n_instances_;
		transforms_.push_back(transform);
		modified_ = true;
	}

	virtual AABB GetBoundingBox() const {
		return bounding_box_;
	}

	/**
	 * Return the number of instances.
	 */
	int n_instances() const {
		return n_instances_;
	}

private:
	// Instance and transforms modified or not.
	bool modified_ = true;

	// Number of instances.
	int n_instances_ = 0;

	// Instance model.
	RenderData instance_;

	// Transform for each instance.
	LocalVector<Transform3D> transforms_;

	// Bounding box of this node.
	AABB bounding_box_;
};
class LeafGenerator {
public:
	// 扇形索引緩衝到三角形緩衝
	void to_triangle_list(const LocalVector<int> &indices, LocalVector<int> &out_indices) const {
		if (indices.size() < 3) {
			return;
		}
		for (int i = 2; i < indices.size(); i += 3) {
			out_indices.push_back(indices[0]);
			out_indices.push_back(indices[i - 1]);
			out_indices.push_back(indices[i]);
		}
	}

	void OctaveLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0.005f, 0.0f, 0.0f },
			{ 0.005f, 0.0f, 0.1f },
			{ 0.15f, 0.0f, 0.15f },
			{ 0.2f, 0.0f, 0.3f },
			{ 0.0f, 0.0f, 1.0f },
			{ -0.2f, 0.0f, 0.6f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.005f, 0.0f, 0.0f },
			{ -0.2f, 0.0f, 0.6f },
			{ -0.25f, 0.0f, 0.15f },
			{ -0.005f, 0.0f, 0.1f },
			{ -0.005f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = { 0, 1, 9, 0, 9, 10, 1, 2, 3, 1, 3, 4, 4, 5, 6,
			6, 7, 8, 6, 8, 9, 4, 6, 9, 4, 9, 1 };
	}

	void LinearLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0.005f, 0.0f, 0.0f },
			{ 0.005f, 0.0f, 0.1f },
			{ 0.1f, 0.0f, 0.15f },
			{ 0.1f, 0.0f, 0.95f },
			{ 0.0f, 0.0f, 1.0f },
			{ -0.1f, 0.0f, 0.95f },
			{ -0.1f, 0.0f, 0.15f },
			{ -0.005f, 0.0f, 0.1f },
			{ -0.005f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = { 0, 1, 7, 0, 7, 8, 1, 2, 3, 3, 4, 5, 5, 6, 7,
			1, 3, 5, 1, 5, 7 };
	}

	void CordateLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0.005f, 0.0f, 0.0f },
			{ 0.01f, 0.0f, 0.2f },
			{ 0.2f, 0.0f, 0.1f },
			{ 0.35f, 0.0f, 0.35f },
			{ 0.25f, 0.0f, 0.6f },
			{ 0.1f, 0.0f, 0.8f },
			{ 0.0f, 0.0f, 1.0f },
			{ -0.1f, 0.0f, 0.8f },
			{ -0.25f, 0.0f, 0.6f },
			{ -0.35f, 0.0f, 0.35f },
			{ -0.2f, 0.0f, 0.1f },
			{ -0.01f, 0.0f, 0.2f },
			{ -0.005f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = { 0, 1, 11, 0, 11, 12, 1, 2, 3, 1, 3, 4,
			11, 10, 9, 11, 9, 8, 11, 1, 4, 11, 4, 8,
			8, 7, 6, 8, 6, 5, 8, 5, 4 };
	}

	void MapleLeaf(RenderData *data) const {
		////CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {

			{ 0.005, 0, 0 },
			{ 0.005, 0, 0.1 },
			{ 0.25, 0, 0.07 },
			{ 0.2, 0, 0.18 },
			{ 0.5, 0, 0.37 },
			{ 0.43, 0, 0.4 },
			{ 0.45, 0, 0.58 },
			{ 0.3, 0, 0.57 },
			{ 0.27, 0, 0.67 },
			{ 0.11, 0, 0.52 },
			{ 0.2, 0, 0.82 },
			{ 0.08, 0, 0.77 },
			{ 0, 0, 1 },
			{ -0.08, 0, 0.77 },
			{ -0.2, 0, 0.82 },
			{ -0.11, 0, 0.52 },
			{ -0.27, 0, 0.67 },
			{ -0.3, 0, 0.57 },
			{ -0.45, 0, 0.58 },
			{ -0.43, 0, 0.4 },
			{ -0.5, 0, 0.37 },
			{ -0.2, 0, 0.18 },
			{ -0.25, 0, 0.07 },
			{ -0.005, 0, 0.1 },
			{ -0.005, 0, 0 },
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));

		to_triangle_list({ 0, 1, 23, 24 }, data->indices);
		to_triangle_list({ 1, 2, 3, 4, 5 }, data->indices);
		to_triangle_list({ 23, 22, 21, 20, 19 }, data->indices);
		to_triangle_list({ 1, 5, 6, 7, 8 }, data->indices);
		to_triangle_list({ 23, 19, 18, 17, 16 }, data->indices);
		to_triangle_list({ 1, 8, 9, 10, 11 }, data->indices);
		to_triangle_list({ 23, 16, 15, 14, 13 }, data->indices);
		to_triangle_list({ 1, 11, 12, 13, 23 }, data->indices);
	}

	void PalmateLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0.005f, 0.0f, 0.0f },
			{ 0.005f, 0.0f, 0.1f },
			{ 0.25f, 0.0f, 0.1f },
			{ 0.5f, 0.0f, 0.3f },
			{ 0.2f, 0.0f, 0.45f },
			{ 0.0f, 0.0f, 0.1f },
			{ -0.2f, 0.0f, 0.45f },
			{ -0.5f, 0.0f, 0.3f },
			{ -0.25f, 0.0f, 0.1f },
			{ -0.005f, 0.0f, 0.1f },
			{ -0.005f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = {
			0, 1, 9, 0, 9, 10,
			1, 2, 3, 1, 3, 4,
			1, 4, 5, 1, 5, 6, 1, 6, 9,
			9, 8, 7, 9, 7, 6
		};
	}

	void RoundOakLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0.005f, 0.0f, 0.0f },
			{ 0.005f, 0.0f, 0.1f },
			{ 0.11f, 0.0f, 0.16f },
			{ 0.11f, 0.0f, 0.2f },
			{ 0.22f, 0.0f, 0.26f },
			{ 0.23f, 0.0f, 0.32f },
			{ 0.15f, 0.0f, 0.34f },
			{ 0.25f, 0.0f, 0.45f },
			{ 0.23f, 0.0f, 0.53f },
			{ 0.16f, 0.0f, 0.5f },
			{ 0.23f, 0.0f, 0.64f },
			{ 0.2f, 0.0f, 0.72f },
			{ 0.11f, 0.0f, 0.7f },
			{ 0.16f, 0.0f, 0.83f },
			{ 0.12f, 0.0f, 0.87f },
			{ 0.06f, 0.0f, 0.85f },
			{ 0.07f, 0.0f, 0.95f },
			{ 0.0f, 0.0f, 1.0f },
			{ -0.07f, 0.0f, 0.85f },
			{ -0.12f, 0.0f, 0.87f },
			{ -0.16f, 0.0f, 0.83f },
			{ -0.11f, 0.0f, 0.7f },
			{ -0.2f, 0.0f, 0.72f },
			{ -0.23f, 0.0f, 0.64f },
			{ -0.16f, 0.0f, 0.5f },
			{ -0.23f, 0.0f, 0.53f },
			{ -0.25f, 0.0f, 0.45f },
			{ -0.15f, 0.0f, 0.34f },
			{ -0.23f, 0.0f, 0.32f },
			{ -0.22f, 0.0f, 0.26f },
			{ -0.11f, 0.0f, 0.2f },
			{ -0.11f, 0.0f, 0.16f },
			{ -0.005f, 0.0f, 0.1f },
			{ -0.005f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = {
			0, 1, 33, 0, 33, 34,
			1, 2, 3,
			3, 4, 5, 3, 5, 6,
			6, 7, 8, 6, 8, 9,
			9, 10, 11, 9, 11, 12,
			12, 13, 14, 12, 14, 15,
			1, 3, 6, 1, 6, 9, 1, 9, 12, 1, 12, 15, 1, 15, 17, 1, 17, 19,
			1, 19, 22, 1, 22, 25, 1, 25, 28, 1, 28, 31, 1, 31, 33,
			33, 32, 31,
			31, 30, 29, 31, 29, 28,
			28, 27, 26, 28, 26, 25,
			25, 24, 23, 25, 23, 22,
			22, 21, 20, 22, 20, 19,
			19, 18, 17
		};
	}

	void SpikyOakLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0.005f, 0.0f, 0.0f },
			{ 0.005f, 0.0f, 0.1f },
			{ 0.16f, 0.0f, 0.17f },
			{ 0.11f, 0.0f, 0.2f },
			{ 0.23f, 0.0f, 0.33f },
			{ 0.15f, 0.0f, 0.34f },
			{ 0.32f, 0.0f, 0.55f },
			{ 0.16f, 0.0f, 0.5f },
			{ 0.27f, 0.0f, 0.75f },
			{ 0.11f, 0.0f, 0.7f },
			{ 0.18f, 0.0f, 0.9f },
			{ 0.07f, 0.0f, 0.86f },
			{ 0.0f, 0.0f, 1.0f },
			{ -0.07f, 0.0f, 0.86f },
			{ -0.18f, 0.0f, 0.9f },
			{ -0.11f, 0.0f, 0.7f },
			{ -0.27f, 0.0f, 0.75f },
			{ -0.16f, 0.0f, 0.5f },
			{ -0.32f, 0.0f, 0.55f },
			{ -0.15f, 0.0f, 0.34f },
			{ -0.23f, 0.0f, 0.33f },
			{ -0.11f, 0.0f, 0.2f },
			{ -0.16f, 0.0f, 0.17f },
			{ -0.005f, 0.0f, 0.1f },
			{ -0.005f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = {
			0, 1, 23, 0, 23, 24,
			1, 2, 3, 3, 4, 5, 5, 6, 7, 7, 8, 9, 9, 10, 11,
			1, 3, 5, 1, 5, 7, 1, 7, 9, 1, 9, 11, 1, 11, 12, 1, 12, 13,
			1, 13, 15, 1, 15, 17, 1, 17, 19, 1, 19, 21, 1, 21, 23,
			23, 22, 21, 21, 20, 19, 19, 18, 17, 17, 16, 15, 15, 14, 13
		};
	}

	void EllipticLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0.005f, 0.0f, 0.0f },
			{ 0.005f, 0.0f, 0.1f },
			{ 0.15f, 0.0f, 0.2f },
			{ 0.25f, 0.0f, 0.45f },
			{ 0.2f, 0.0f, 0.75f },
			{ 0.0f, 0.0f, 1.0f },
			{ -0.2f, 0.0f, 0.75f },
			{ -0.25f, 0.0f, 0.45f },
			{ -0.15f, 0.0f, 0.2f },
			{ -0.005f, 0.0f, 0.1f },
			{ -0.005f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = { 0, 1, 9, 0, 9, 10, 1, 2, 3, 1, 3, 4,
			4, 5, 6, 6, 7, 8, 6, 8, 9,
			4, 6, 9, 4, 9, 1 };
	}

	void RectLeaf(RenderData *data) const {
		//CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ -0.5, 0, 0 },
			{ -0.5, 0, 1 },
			{ 0.5, 0, 1 },
			{ 0.5, 0, 0 },
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = { 0, 1, 2, 0, 2, 3 };
	}

	void TriangleLeaf(RenderData *data) const {
		////CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ -0.5f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.5f, 0.0f, 0.0f }
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));
		data->indices = { 0, 1, 2 };
	}

	// 樱花
	void CherryLeaf(RenderData *data) const {
		////CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {

			{ 0, 0, 0 },
			{ 0.33, 0.45, 0.45 },
			{ 0.25, 0.6, 0.6 },
			{ 0, 0.7, 0.7 },
			{ -0.25, 0.6, 0.6 },
			{ -0.33, 0.45, 0.45 },
			{ 0.49, 0.42, 0.6 },
			{ 0.67, 0.22, 0.7 },
			{ 0.65, -0.05, 0.6 },
			{ 0.53, -0.17, 0.45 },
			{ 0.55, -0.33, 0.6 },
			{ 0.41, -0.57, 0.7 },
			{ 0.15, -0.63, 0.6 },
			{ 0, -0.55, 0.45 },
			{ -0.15, -0.63, 0.6 },
			{ -0.41, -0.57, 0.7 },
			{ -0.55, -0.33, 0.6 },
			{ -0.53, -0.17, 0.45 },
			{ -0.65, -0.05, 0.6 },
			{ -0.67, 0.22, 0.7 },
			{ -0.49, 0.42, 0.6 },
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));

		to_triangle_list({ 0, 1, 2, 3 }, data->indices);
		to_triangle_list({ 0, 3, 4, 5 }, data->indices);
		to_triangle_list({ 0, 1, 6, 7 }, data->indices);
		to_triangle_list({ 0, 7, 8, 9 }, data->indices);
		to_triangle_list({ 0, 9, 10, 11 }, data->indices);
		to_triangle_list({ 0, 11, 12, 13 }, data->indices);
		to_triangle_list({ 0, 13, 14, 15 }, data->indices);
		to_triangle_list({ 0, 15, 16, 17 }, data->indices);
		to_triangle_list({ 0, 17, 18, 19 }, data->indices);
		to_triangle_list({ 0, 19, 20, 5 }, data->indices);
	}
	// 橙子
	void OrangeLeaf(RenderData *data) const {
		////CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0, 0, 0 },
			{ -0.055, 0.165, 0.11 },
			{ -0.125, 0.56, 0.365 },
			{ 0, 0.7, 0.45 },
			{ 0.125, 0.56, 0.365 },
			{ 0.055, 0.165, 0.11 },
			{ 0.14, 0.10, 0.11 },
			{ 0.495, 0.29, 0.365 },
			{ 0.665, 0.215, 0.45 },
			{ 0.57, 0.055, 0.36 },
			{ 0.175, 0, 0.11 },
			{ 0.14, -0.1, 0.11 },
			{ 0.43, -0.38, 0.365 },
			{ 0.41, -0.565, 0.45 },
			{ 0.23, -0.53, 0.365 },
			{ 0.05, -0.165, 0.11 },
			{ -0.14, -0.1, 0.11 },
			{ -0.43, -0.38, 0.365 },
			{ -0.41, -0.565, 0.45 },
			{ -0.23, -0.53, 0.365 },
			{ -0.05, -0.165, 0.11 },
			{ -0.14, 0.10, 0.11 },
			{ -0.495, 0.29, 0.365 },
			{ -0.665, 0.215, 0.45 },
			{ -0.57, 0.055, 0.36 },
			{ -0.175, 0, 0.11 },
			{ 0.1, -0.1, 0.4 },
			{ -0.1, -0.1, 0.4 },
			{ -0.1, 0.1, 0.4 },
			{ 0.1, 0.1, 0.4 },
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));

		to_triangle_list({ 0, 1, 2, 3 }, data->indices);
		to_triangle_list({ 0, 3, 4, 5 }, data->indices);
		to_triangle_list({ 0, 6, 7, 8 }, data->indices);
		to_triangle_list({ 0, 8, 9, 10 }, data->indices);
		to_triangle_list({ 0, 11, 12, 13 }, data->indices);
		to_triangle_list({ 0, 13, 14, 15 }, data->indices);
		to_triangle_list({ 0, 16, 17, 18 }, data->indices);
		to_triangle_list({ 0, 18, 19, 20 }, data->indices);
		to_triangle_list({ 0, 21, 22, 23 }, data->indices);
		to_triangle_list({ 0, 23, 24, 25 }, data->indices);
		to_triangle_list({ 0, 26, 27 }, data->indices);
		to_triangle_list({ 0, 27, 28 }, data->indices);
		to_triangle_list({ 0, 28, 29 }, data->indices);
		to_triangle_list({ 0, 29, 26 }, data->indices);
	}

	// 木兰
	void MagnoliaLeaf(RenderData *data) const {
		////CHECK(data);

		data->type = RenderData::GL_TRIANGLES;
		data->vertices = {
			{ 0, 0, 0 },
			{ 0.19, -0.19, 0.06 },
			{ 0.19, -0.04, 0.06 },
			{ 0.34, -0.11, 0.35 },
			{ 0.3, -0.3, 0.6 },
			{ 0.11, -0.34, 0.35 },
			{ 0.04, -0.19, 0.06 },
			{ 0.19, 0.19, 0.06 },
			{ 0.19, 0.04, 0.06 },
			{ 0.34, 0.11, 0.35 },
			{ 0.3, 0.3, 0.6 },
			{ 0.11, 0.34, 0.35 },
			{ 0.04, 0.19, 0.06 },
			{ -0.19, -0.19, 0.06 },
			{ -0.19, -0.04, 0.06 },
			{ -0.34, -0.11, 0.35 },
			{ -0.3, -0.3, 0.6 },
			{ -0.11, -0.34, 0.35 },
			{ -0.04, -0.19, 0.06 },
			{ -0.19, 0.19, 0.06 },
			{ -0.19, 0.04, 0.06 },
			{ -0.34, 0.11, 0.35 },
			{ -0.3, 0.3, 0.6 },
			{ -0.11, 0.34, 0.35 },
			{ -0.04, 0.19, 0.06 },
			{ 0, -0.39, 0.065 },
			{ 0.15, -0.23, 0.065 },
			{ 0.23, -0.46, 0.39 },
			{ 0, -0.62, 0.65 },
			{ -0.23, -0.46, 0.39 },
			{ -0.15, -0.23, 0.065 },
			{ 0, 0.39, 0.065 },
			{ 0.15, 0.23, 0.065 },
			{ 0.23, 0.46, 0.39 },
			{ 0, 0.62, 0.65 },
			{ -0.23, 0.46, 0.39 },
			{ -0.15, 0.23, 0.065 },
			{ -0.39, 0, 0.065 },
			{ -0.23, 0.15, 0.065 },
			{ -0.46, 0.23, 0.39 },
			{ -0.62, 0, 0.65 },
			{ -0.46, -0.23, 0.39 },
			{ -0.23, -0.15, 0.065 },
			{ 0.39, 0, 0.065 },
			{ 0.23, 0.15, 0.065 },
			{ 0.46, 0.23, 0.39 },
			{ 0.62, 0, 0.65 },
			{ 0.46, -0.23, 0.39 },
			{ 0.23, -0.15, 0.065 },
		};
		data->normals.assign(data->vertices.size(),
				Vector3(0.0f, 1.0f, 0.0f));

		data->indices = {
			0,
			1,
			2,
			1,
			2,
			3,
			1,
			3,
			4,
			1,
			4,
			5,
			1,
			5,
			6,
			1,
			6,
			0,
			0,
			7,
			8,
			7,
			8,
			9,
			7,
			9,
			10,
			7,
			10,
			11,
			7,
			11,
			12,
			7,
			12,
			0,
			0,
			13,
			14,
			13,
			14,
			15,
			13,
			15,
			16,
			13,
			16,
			17,
			13,
			17,
			18,
			13,
			18,
			0,
			0,
			19,
			20,
			19,
			20,
			21,
			19,
			21,
			22,
			19,
			22,
			23,
			19,
			23,
			24,
			19,
			24,
			0,
			0,
			25,
			26,
			25,
			26,
			27,
			25,
			27,
			28,
			25,
			28,
			29,
			25,
			29,
			30,
			25,
			30,
			0,
			0,
			31,
			32,
			31,
			32,
			33,
			32,
			33,
			34,
			31,
			34,
			35,
			31,
			35,
			36,
			31,
			36,
			0,
			0,
			37,
			38,
			37,
			38,
			39,
			37,
			39,
			40,
			37,
			40,
			41,
			37,
			41,
			42,
			37,
			42,
			0,
			0,
			43,
			44,
			43,
			44,
			45,
			43,
			45,
			46,
			43,
			46,
			47,
			43,
			47,
			48,
			43,
			48,
			0,
		};
	}
};
#endif
