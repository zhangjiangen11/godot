/**************************************************************************************
**
**  Copyright (C) 2006 Thomas Luft, University of Konstanz. All rights reserved.
**
**  This file is part of the Ivy Generator Tool.
**
**  This program is free software; you can redistribute it and/or modify it
**  under the terms of the GNU General Public License as published by the
**  Free Software Foundation; either version 2 of the License, or (at your
**  option) any later version.
**  This program is distributed in the hope that it will be useful, but
**  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
**  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
**  for more details.
**  You should have received a copy of the GNU General Public License along
**  with this program; if not, write to the Free Software Foundation,
**  Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA
**
***************************************************************************************/

#pragma once

#include "core/math/triangle_mesh.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/string/ustring.h"

#include "core/object/ref_counted.h"
#include "scene/3d/node_3d.h"

#include <vector>

/** 一个常春藤节点 */
class IvyNode {
public:
	IvyNode() :
			length(0.0f), floatingLength(0.0f), climb(false) {}

	/** 节点位置 */
	Vector3 pos = Vector3(0.0f, 0.0f, 0.0f);

	/** 主要生长方向，是先前方向的加权和 */
	Vector3 primaryDir = Vector3(0, 0, 1);

	/** 由其他场景对象产生的附着向量 */
	Vector3 adhesionVector = Vector3(0.0f, 0.0f, 0.0f);

	/** 在生成阶段计算和使用的平滑附着向量，
	   由于常春藤叶子通过附着向量对齐，这个平滑向量
	   允许叶子对齐的平滑过渡 */
	Vector3 smoothAdhesionVector;

	/** 此节点处相关常春藤分支的长度 */
	float length = 0.0001;

	/** 最后一个处于攀爬状态节点的长度 */
	float floatingLength = 0;

	float adhesionLength = 0.0f;

	/** 攀爬状态 */
	bool climb = true;
};

/** 一个常春藤根点 */
class IvyRoot {
public:
	/** 多个节点 */
	std::vector<IvyNode> nodes;

	/** 存活状态 */
	bool alive = true;

	/** 父节点数量，表示根层次结构中的级别 */
	int parents = 0;
};

/** 基本三角形 */
class BasicTriangle {
public:
	BasicTriangle() {}

	Vector3 v0;
	unsigned int v0id;

	Vector3 v1;
	unsigned int v1id;

	Vector3 v2;
	unsigned int v2id;

	Vector3 n0;
	unsigned int n0id;

	Vector3 n1;
	unsigned int n1id;

	Vector3 n2;
	unsigned int n2id;

	Vector2 t0;
	unsigned int t0id;

	Vector2 t1;
	unsigned int t1id;

	Vector2 t2;
	unsigned int t2id;

	Vector2 mat;
	unsigned int matid;

	Vector3 norm;
};

/** 仅包含单一纹理的简单材质 */
class BasicMaterial {
public:
	BasicMaterial() :
			id(0), texObj(0) {}

	unsigned int id;

	std::string name;

	std::string texFile;

	unsigned int texObj;
};

/** 基本网格 */
class BasicMesh {
public:
	BasicMesh() {
		boundingSphereRadius = 1.0f;

		boundingSpherePos = Vector3(0.0f, 0.0f, 0.0f);

		displayListObj = 0;
	}

	~BasicMesh() {
	}

	void reset() {
		file = "";

		path = "";
		vertices.clear();

		normals.clear();

		texCoords.clear();

		triangles.clear();

		materials.clear();

		boundingSphereRadius = 1.0f;

		boundingSpherePos = Vector3(0.0f, 0.0f, 0.0f);
	}

	/** 设置三角形指向其顶点、法线、纹理坐标和材质的指针；计算包围球 */
	void prepareData() {
		// 更新三角形指针
		for (std::vector<BasicTriangle>::iterator t = triangles.begin(); t != triangles.end(); ++t) {
			t->v0 = vertices[t->v0id - 1];
			t->v1 = vertices[t->v1id - 1];
			t->v2 = vertices[t->v2id - 1];

			if (t->n0id != 0) {
				t->n0 = normals[t->n0id - 1];
			}
			if (t->n1id != 0) {
				t->n1 = normals[t->n1id - 1];
			}
			if (t->n2id != 0) {
				t->n2 = normals[t->n2id - 1];
			}

			if (t->t0id != 0) {
				t->t0 = texCoords[t->t0id - 1];
			}
			if (t->t1id != 0) {
				t->t1 = texCoords[t->t1id - 1];
			}
			if (t->t2id != 0) {
				t->t2 = texCoords[t->t2id - 1];
			}

			//if (t->matid != 0) t->mat = materials[t->matid - 1];
		}

		// 计算包围球
		boundingSpherePos = Vector3(0.0f, 0.0f, 0.0f);

		for (std::vector<Vector3>::iterator v = vertices.begin(); v != vertices.end(); ++v) {
			boundingSpherePos += *v;
		}

		boundingSpherePos /= (float)vertices.size();

		boundingSphereRadius = 0.0f;

		for (std::vector<Vector3>::iterator v = vertices.begin(); v != vertices.end(); ++v) {
			boundingSphereRadius = MAX(boundingSphereRadius, ((*v) - boundingSpherePos).length());
		}
	}

	/** 计算顶点法线 */
	void calculateVertexNormals() {
		normals.clear();

		normals.resize(vertices.size());
	}

	/** 翻转顶点法线 */
	void flipNormals() {
	}

public:
	std::vector<Vector3> vertices;

	std::vector<Vector3> normals;

	std::vector<Vector2> texCoords;

	std::vector<BasicMaterial> materials;

	std::vector<BasicTriangle> triangles;

	Vector3 boundingSpherePos;

	float boundingSphereRadius;

	String file;

	String path;

	unsigned int displayListObj;
};

/** 常春藤本身，继承自基本网格，允许将最终的常春藤网格作为可绘制对象处理 */
class IvyMesh : public BasicMesh {
public:
	IvyMesh();

	void resetSettings();

	/** 初始化一个新的常春藤根 */
	void seed(const Vector3 &seedPos);

	/** 一次单独的生长迭代 */
	void grow();

	/** 计算场景对象在点pos处的附着力 */
	Vector3 computeAdhesion(const Vector3 &pos);

	/** 计算常春藤段oldPos->newPos的碰撞检测，如有必要将修改newPos */
	bool computeCollision(const Vector3 &oldPos, Vector3 &newPos, bool &climbingState);

	/** 创建常春藤三角形网格 */
	void birth();

	/** 常春藤根 */
	std::vector<IvyRoot> roots;

public:
	LocalVector<Ref<TriangleMesh>> meshList;
	/** 常春藤大小因子，影响生长行为 [0..0,1] */
	float ivySize;

	/** 叶子大小因子 [0..0,1] */
	float ivyLeafSize;

	/** 分支大小因子 [0..0,1] */
	float ivyBranchSize;

	/** 自由漂浮的常春藤分支段的最大长度 [0..1] */
	float maxFloatLength;

	/** 场景对象附着的最大距离 [0..1] */
	float maxAdhesionDistance;

	/** 主要生长向量的权重 [0..1] */
	float primaryWeight;

	/** 随机影响向量的权重 [0..1] */
	float randomWeight;

	/** 重力向量的权重 [0..1] */
	float gravityWeight;

	/** 附着向量的权重 [0..1] */
	float adhesionWeight;

	/** 每次迭代产生新常春藤根的概率 [0..1]*/
	float branchingProbability;

	/** 创建新常春藤叶子的概率 [0..1] */
	float leafProbability;
	float maxLength = 0.0f;
};
class Ivy : public Node3D {
	GDCLASS(Ivy,Node3D);

public:
	Ref<IvyMesh> ivyMesh;
};
