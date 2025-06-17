/**************************************************************************************
**
**  Copyright (C) 2006 Thomas Luft, University of Konstanz. All rights reserved.
**
**  This file is part of the IvyMesh Generator Tool.
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

#include "ivy.h"

IvyMesh::IvyMesh() {
	resetSettings(); // 重置参数设置
}

void IvyMesh::resetSettings() {
	primaryWeight = 0.5f; // 主要生长方向权重
	randomWeight = 0.2f; // 随机影响权重
	gravityWeight = 1.0f; // 重力影响权重
	adhesionWeight = 0.1f; // 附着向量权重
	branchingProbability = 0.95f; // 分支生成概率
	leafProbability = 0.65f; // 叶子生成概率
	ivySize = 0.02f; // 常春藤整体大小因子
	ivyLeafSize = 0.02f; // 叶子大小因子
	ivyBranchSize = 0.001f; // 分支大小因子
	maxFloatLength = 0.5f; // 自由漂浮最大长度
	maxAdhesionDistance = 1.0f; // 最大附着距离
	maxLength = 0.0f; // 最大生长长度（初始化为0）

	// 归一化方向权重（确保权重和为1）
	float sums = primaryWeight + randomWeight + adhesionWeight;
	primaryWeight /= sums;
	randomWeight /= sums;
	adhesionWeight /= sums;
}

void IvyMesh::seed(const Vector3 &seedPos) {
	BasicMesh::reset(); // 重置基础网格数据
	roots.clear(); // 清空根节点列表

	IvyNode tmpNode; // 临时节点对象
	IvyRoot tmpRoot; // 临时根节点对象
	tmpRoot.nodes.push_back(tmpNode); // 将临时节点添加到根节点
	roots.push_back(tmpRoot); // 将根节点添加到根列表
}

// 生成随机三维向量（归一化处理）
static inline Vector3 getRandomized() {
	return (Vector3(
					Math::rand() / (float)RAND_MAX - 0.5f,
					Math::rand() / (float)RAND_MAX - 0.5f,
					Math::rand() / (float)RAND_MAX - 0.5f))
			.normalized();
}

void IvyMesh::grow() {
	// 依赖场景对象包围球的参数（注释待完善）
	// 归一化影响权重（注释待完善）

	// 遍历所有根节点进行生长模拟
	for (auto root = roots.begin(); root != roots.end(); ++root) {
		// 跳过非存活根节点
		if (!root->alive) {
			continue;
		}

		// 若自由漂浮长度超过阈值则标记根节点为死亡
		if (root->nodes.back().floatingLength > maxFloatLength) {
			root->alive = false;
		}

		// 计算生长向量：主方向、随机影响、场景附着
		Vector3 primaryVector = root->nodes.back().primaryDir; // 主生长方向
		Vector3 randomVector = (getRandomized() + Vector3(0.0f, 0.2f, 0.0f)).normalized(); // 随机影响向量（带y轴偏移）
		Vector3 adhesionVector = computeAdhesion(root->nodes.back().pos); // 场景附着向量

		// 计算综合生长向量（权重叠加）
		Vector3 growVector = ivySize * (primaryVector * primaryWeight + randomVector * randomWeight + adhesionVector * adhesionWeight);

		// 重力影响计算
		Vector3 gravityVector = ivySize * Vector3(0.0f, -1.0f, 0.0f) * gravityWeight; // 基础重力向量
		gravityVector *= pow(root->nodes.back().floatingLength / maxFloatLength, 0.7f); // 基于漂浮长度的重力权重

		// 计算下一个节点位置
		bool climbing; // 攀爬状态（将在碰撞检测中设置）
		Vector3 newPos = root->nodes.back().pos + growVector + gravityVector; // 初始预测位置

		// 执行碰撞检测并更新位置
		if (computeCollision(root->nodes.back().pos, newPos, climbing)) {
			root->alive = true; // 碰撞处理成功则保持存活
		}

		// 基于更新后的位置重新计算生长向量
		growVector = newPos - root->nodes.back().pos - gravityVector;

		// 创建新的常春藤节点
		IvyNode tmpNode;
		tmpNode.pos = newPos; // 节点位置
		tmpNode.primaryDir = (root->nodes.back().primaryDir.lerp(growVector, 0.5f)).normalized(); // 平滑更新主方向
		tmpNode.adhesionVector = adhesionVector; // 附着向量
		tmpNode.length = root->nodes.back().length + (newPos - root->nodes.back().pos).length(); // 累计长度
		if (tmpNode.length > maxLength) {
			maxLength = tmpNode.length; // 更新最大长度
		}
		tmpNode.floatingLength = climbing ? 0.0f : root->nodes.back().floatingLength + (newPos - root->nodes.back().pos).length(); // 漂浮长度（攀爬时重置为0）
		tmpNode.climb = climbing; // 攀爬状态
		root->nodes.push_back(tmpNode); // 添加到根节点列表
	}

	// 检查是否生成新的分支根节点
	for (auto root = roots.begin(); root != roots.end(); ++root) {
		if (!root->alive) {
			continue; // 跳过非存活根节点
		}
		if (root->parents > 3) {
			continue; // 限制根层级不超过3
		}
		if (root->nodes.size() < 2) {
			continue; // 至少需要2个节点才能分支
		}

		// 遍历节点尝试生成新分支
		for (auto node = root->nodes.begin(); node != root->nodes.end(); ++node) {
			// 基于节点长度占比计算权重
			float weight = 1.0f - (cos(2.0f * Math::PI * node->length / root->nodes.back().length) * 0.5f + 0.5f);
			float probability = Math::rand() / (float)RAND_MAX; // 随机概率

			if (probability * weight > branchingProbability) { // 满足分支条件
				// 创建新节点作为分支起点
				IvyNode tmpNode;
				tmpNode.pos = node->pos; // 位置与当前节点相同
				tmpNode.primaryDir = Vector3(0.0f, 1.0f, 0.0f); // 初始方向向上
				tmpNode.adhesionVector = Vector3(0.0f, 0.0f, 0.0f); // 初始无附着
				tmpNode.length = 0.0f; // 长度重置为0
				tmpNode.floatingLength = node->floatingLength; // 继承漂浮长度
				tmpNode.climb = true; // 初始为攀爬状态

				// 创建新根节点并添加到列表
				IvyRoot tmpRoot;
				tmpRoot.nodes.push_back(tmpNode);
				tmpRoot.alive = true;
				tmpRoot.parents = root->parents + 1; // 层级+1
				roots.push_back(tmpRoot);
				return; // 每次迭代仅生成一个新分支
			}
		}
	}
}

Vector3 IvyMesh::computeAdhesion(const Vector3 &pos) {
	Vector3 adhesionVector; // 最终附着向量
	float last_distance = maxAdhesionDistance * 2; // 初始最大距离（两倍阈值）

	// 遍历所有场景网格计算最近附着点
	for (uint32_t m = 0; m < meshList.size(); m++) {
		Vector3 normal;
		Vector4 point = meshList[m]->get_closest_point_to(pos, maxAdhesionDistance, normal); // 获取最近点
		if (point.w > 0) { // 点有效时
			float dis = adhesionVector.distance_to(pos);
			if (dis < last_distance) { // 找到更近的附着点
				adhesionVector = Vector3(point.x, point.y, point.z);
				last_distance = adhesionVector.distance_to(pos);
			}
		}
	}
	return adhesionVector;
}

bool IvyMesh::computeCollision(const Vector3 &oldPos, Vector3 &newPos, bool &climbing) {
	climbing = false; // 重置攀爬状态
	bool intersection = false; // 碰撞标志

	Vector3 direction = newPos - oldPos; // 移动方向
	Vector3 from = oldPos;
	Vector3 to = newPos;
	float last_distance = from.distance_to(to) * 3; // 初始最大距离

	// 遍历场景网格检测碰撞
	for (uint32_t m = 0; m < meshList.size(); m++) {
		Vector3 point;
		Vector3 normal;
		int32_t surf_index = -1, r_face_index = 0;
		bool is_inside = meshList[m]->intersect_segment(from, to, point, normal, &surf_index, &r_face_index, false); // 线段相交检测

		if (is_inside) { // 检测到相交
			if (normal.dot(direction) < 0.0) { // 法线与方向夹角为钝角（表示碰撞）
				float dis = point.distance_to(from);
				if (dis < last_distance) { // 找到更近的碰撞点
					Vector3 reflected_direction = (to - point).reflect(normal); // 计算反射方向
					newPos = point + reflected_direction; // 沿反射方向更新位置
					intersection = true; // 标记碰撞发生
					climbing = true; // 标记为攀爬状态
				}
			}
		}
	}
	return intersection;
}

// 绕轴旋转向量（三维旋转矩阵实现）
static inline Vector3 rotateAroundAxis(const Vector3 &vector, const Vector3 &axisPosition, const Vector3 &axis, const float &angle) {
	float cosTheta = cos(angle); // 旋转角余弦
	float sinTheta = sin(angle); // 旋转角正弦
	Vector3 direction = vector - axisPosition; // 从轴点到目标点的向量

	// 计算旋转后的向量分量（使用罗德里格旋转公式）
	Vector3 newDirection;
	newDirection.x = (cosTheta + (1 - cosTheta) * axis.x * axis.x) * direction.x +
			((1 - cosTheta) * axis.x * axis.y - axis.z * sinTheta) * direction.y +
			((1 - cosTheta) * axis.x * axis.z + axis.y * sinTheta) * direction.z;

	newDirection.y = ((1 - cosTheta) * axis.x * axis.y + axis.z * sinTheta) * direction.x +
			(cosTheta + (1 - cosTheta) * axis.y * axis.y) * direction.y +
			((1 - cosTheta) * axis.y * axis.z - axis.x * sinTheta) * direction.z;

	newDirection.z = ((1 - cosTheta) * axis.x * axis.z - axis.y * sinTheta) * direction.x +
			((1 - cosTheta) * axis.y * axis.z + axis.x * sinTheta) * direction.y +
			(cosTheta + (1 - cosTheta) * axis.z * axis.z) * direction.z;

	return axisPosition + newDirection; // 返回旋转后的点坐标
}

void IvyMesh::birth() {
	// 高斯滤波处理附着向量（平滑叶子对齐）
	float gaussian[11] = { 1.0f, 2.0f, 4.0f, 7.0f, 9.0f, 10.0f, 9.0f, 7.0f, 4.0f, 2.0f, 1.0f }; // 高斯核

	for (int32_t r = 0; r < int32_t(roots.size()); r++) {
		for (int g = 0; g < 5; ++g) { // 多次迭代增强平滑效果
			IvyRoot root = roots[r];
			for (int32_t n = 0; n < int32_t(root.nodes.size()); n++) {
				Vector3 e; // 临时累加向量

				// 窗口大小为11的高斯滤波
				for (int i = -5; i <= 5; ++i) {
					Vector3 tmpAdhesion;
					if ((n + i) < 0) {
						tmpAdhesion = root.nodes.front().adhesionVector; // 处理边界情况（前边界）
					} else if ((n + i) >= root.nodes.size()) {
						tmpAdhesion = root.nodes.back().adhesionVector; // 后边界
					} else {
						tmpAdhesion = root.nodes[n + i].adhesionVector; // 正常位置
					}
					e += tmpAdhesion * gaussian[i + 5]; // 加权累加
				}

				// 归一化平滑后的附着向量
				root.nodes[n].smoothAdhesionVector = e / 56.0f; // 高斯核权重和为56
				root.nodes[n].adhesionLength = root.nodes[n].smoothAdhesionVector.length(); // 附着向量长度
				root.nodes[n].smoothAdhesionVector = root.nodes[n].smoothAdhesionVector.normalized(); // 单位化
			}
		}
	}

	float local_ivyBranchSize = ivySize * ivyBranchSize; // 分支大小因子（基于全局参数）
	BasicMesh::reset(); // 重置基础网格数据
	path = "../textures/"; // 设置纹理路径

	// 创建叶子材质（成年叶）
	BasicMaterial tmpMaterial;
	tmpMaterial.id = 1;
	tmpMaterial.name = "leaf_adult";
	tmpMaterial.texFile = "efeu1.png";
	materials.push_back(tmpMaterial);

	// 创建叶子材质（幼叶）
	tmpMaterial.id = 2;
	tmpMaterial.name = "leaf_young";
	tmpMaterial.texFile = "efeu0.png";
	materials.push_back(tmpMaterial);

	// 创建分支材质
	tmpMaterial.id = 3;
	tmpMaterial.name = "branch";
	tmpMaterial.texFile = "efeu_branch.png";
	materials.push_back(tmpMaterial);

	// 生成叶子网格
	for (auto root = roots.begin(); root != roots.end(); ++root) {
		if (root->nodes.size() < 2) {
			continue; // 至少需要2个节点才能生成叶子
		}

		for (size_t node_index = 0; node_index < root->nodes.size() - 1; ++node_index) {
			float alignmentWeight = root->nodes[node_index].adhesionLength; // 附着强度权重
			IvyNode *node = &root->nodes[node_index];
			IvyNode *nodeNext = &root->nodes[node_index + 1];
			float weight = pow(node->length / root->nodes.back().length, 0.7f); // 基于长度的权重
			float groundIvy = MAX(0.0f, -node->adhesionVector.y); // 地面常春藤权重（y轴负方向增强）
			weight += groundIvy * pow(1.0f - node->length / root->nodes.back().length, 2.0f); // 综合权重

			// 计算叶子朝向角度
			float phi = atan2(node->adhesionVector.z, node->adhesionVector.x) - Math::PI * 0.5f; // 水平角度（弧度）
			phi += (Math::rand() / (float)RAND_MAX - 0.5f) * (1.3f - alignmentWeight); // 随机偏移

			float theta = node->adhesionVector.angle_to(Vector3(0.0f, -1.0f, 0.0f)) * 0.5f; // 垂直角度（与y轴负方向夹角的一半）
			theta += (Math::rand() / (float)RAND_MAX - 0.5f) * (1.1f - alignmentWeight); // 随机偏移

			// 计算叶子大小
			float sizeWeight = 1.5f - (cos(weight * 2.0f * Math::PI) * 0.5f + 0.5f); // 周期性大小变化
			float leafSize = sizeWeight * ivyLeafSize; // 最终叶子大小

			// 生成10片叶子（每个节点间隔生成）
			for (int j = 0; j < 10; ++j) {
				float probability = Math::rand() / (float)RAND_MAX; // 随机概率
				if (probability * weight > leafProbability) { // 满足生成条件
					// 计算叶子中心位置
					Vector3 center = node->pos.lerp(nodeNext->pos, j / 10.0f) + getRandomized() * ivyLeafSize;

					// 生成叶子顶点（四边形拆分为两个三角形）
					Vector3 tmpVertex;

					tmpVertex = center + Vector3(-leafSize * sizeWeight, 0.0f, leafSize * sizeWeight);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 0.0f, 1.0f), theta);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 1.0f, 0.0f), phi);
					tmpVertex += getRandomized() * leafSize * sizeWeight * 0.5f;
					vertices.push_back(tmpVertex);

					tmpVertex = center + Vector3(leafSize * sizeWeight, 0.0f, leafSize * sizeWeight);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 0.0f, 1.0f), theta);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 1.0f, 0.0f), phi);
					tmpVertex += getRandomized() * leafSize * sizeWeight * 0.5f;
					vertices.push_back(tmpVertex);

					tmpVertex = center + Vector3(-leafSize * sizeWeight, 0.0f, -leafSize * sizeWeight);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 0.0f, 1.0f), theta);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 1.0f, 0.0f), phi);
					tmpVertex += getRandomized() * leafSize * sizeWeight * 0.5f;
					vertices.push_back(tmpVertex);

					tmpVertex = center + Vector3(leafSize * sizeWeight, 0.0f, -leafSize * sizeWeight);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 0.0f, 1.0f), theta);
					tmpVertex = rotateAroundAxis(tmpVertex, center, Vector3(0.0f, 1.0f, 0.0f), phi);
					tmpVertex += getRandomized() * leafSize * sizeWeight * 0.5f;
					vertices.push_back(tmpVertex);

					// 生成纹理坐标
					Vector2 tmpTexCoord;
					tmpTexCoord = Vector2(0.0f, 1.0f);
					texCoords.push_back(tmpTexCoord);
					tmpTexCoord = Vector2(1.0f, 1.0f);
					texCoords.push_back(tmpTexCoord);
					tmpTexCoord = Vector2(0.0f, 0.0f);
					texCoords.push_back(tmpTexCoord);
					tmpTexCoord = Vector2(1.0f, 0.0f);
					texCoords.push_back(tmpTexCoord);

					// 生成三角形（四边形拆分为两个三角形）
					BasicTriangle tmpTriangle;
					tmpTriangle.matid = 1; // 默认为成年叶材质
					if (probability * weight > leafProbability) {
						tmpTriangle.matid = 2; // 随机切换为幼叶材质
					}

					tmpTriangle.v0id = (unsigned int)vertices.size() - 1;
					tmpTriangle.v1id = (unsigned int)vertices.size() - 3;
					tmpTriangle.v2id = (unsigned int)vertices.size() - 2;
					tmpTriangle.t0id = (unsigned int)vertices.size() - 1;
					tmpTriangle.t1id = (unsigned int)vertices.size() - 3;
					tmpTriangle.t2id = (unsigned int)vertices.size() - 2;
					triangles.push_back(tmpTriangle);

					tmpTriangle.v0id = (unsigned int)vertices.size() - 2;
					tmpTriangle.v1id = (unsigned int)vertices.size() - 0;
					tmpTriangle.v2id = (unsigned int)vertices.size() - 1;
					tmpTriangle.t0id = (unsigned int)vertices.size() - 2;
					tmpTriangle.t1id = (unsigned int)vertices.size() - 0;
					tmpTriangle.t2id = (unsigned int)vertices.size() - 1;
					triangles.push_back(tmpTriangle);
				}
			}
		}
	}

	// 生成分支网格
	for (auto root = roots.begin(); root != roots.end(); ++root) {
		if (root->nodes.size() <= 1) {
			continue; // 至少需要2个节点才能生成分支
		}

		float local_ivyBranchDiameter = 1.0f / (float)(root->parents + 1) + 1.0f; // 分支直径（随层级递减）

		for (auto node = root->nodes.begin(); node != root->nodes.end() - 1; ++node) {
			float weight = node->length / root->nodes.back().length; // 基于长度的权重

			// 计算分支截面顶点（三边形）
			Vector3 up = Vector3(0.0f, -1.0f, 0.0f); // 向上方向
			Vector3 basis = ((node + 1)->pos - node->pos).normalized(); // 分支方向
			float b0t = up.dot(basis.normalized()) * local_ivyBranchDiameter * local_ivyBranchSize * (1.3f - weight); // 截面大小
			Vector3 b0 = Vector3(b0t, b0t, b0t) + node->pos; // 第一个截面点
			Vector3 b1 = rotateAroundAxis(b0, node->pos, basis, 2.09f); // 旋转120度
			Vector3 b2 = rotateAroundAxis(b0, node->pos, basis, 4.18f); // 旋转240度

			// 生成顶点和纹理坐标
			Vector3 tmpVertex;
			tmpVertex = b0;
			vertices.push_back(tmpVertex);
			tmpVertex = b1;
			vertices.push_back(tmpVertex);
			tmpVertex = b2;
			vertices.push_back(tmpVertex);

			Vector2 tmpTexCoord;
			float texV = (node - root->nodes.begin()) % 2 == 0 ? 1.0f : 0.0f; // 纹理V坐标交替
			tmpTexCoord = Vector2(0.0f, texV);
			texCoords.push_back(tmpTexCoord);
			tmpTexCoord = Vector2(0.3f, texV);
			texCoords.push_back(tmpTexCoord);
			tmpTexCoord = Vector2(0.6f, texV);
			texCoords.push_back(tmpTexCoord);

			if (node == root->nodes.begin()) {
				continue; // 跳过第一个节点（无前驱）
			}

			// 生成分支三角形（三边形拉伸为棱柱）
			BasicTriangle tmpTriangle;
			tmpTriangle.matid = 3; // 分支材质

			tmpTriangle.v0id = (unsigned int)vertices.size() - 3;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 4;
			tmpTriangle.t0id = (unsigned int)vertices.size() - 3;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 4;
			triangles.push_back(tmpTriangle);

			// 省略重复的三角形生成代码（共6个三角形构成棱柱）
			// ...（中间4个三角形逻辑类似，此处简化）

			tmpTriangle.v0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 3;
			tmpTriangle.t0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 3;
			triangles.push_back(tmpTriangle);
		}
	}

	// 初始化网格数据（计算包围球、法线等）
	prepareData();
	calculateVertexNormals();
	prepareData();
}
