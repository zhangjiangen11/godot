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

#include "ivy.h"

Ivy::Ivy() {
	resetSettings();
}

void Ivy::resetSettings() {
	primaryWeight = 0.5f;

	randomWeight = 0.2f;

	gravityWeight = 1.0f;

	adhesionWeight = 0.1f;

	branchingProbability = 0.95f;

	leafProbability = 0.65f;

	ivySize = 0.02f;

	ivyLeafSize = 0.02f;

	ivyBranchSize = 0.001f;

	maxFloatLength = 0.5f;

	maxAdhesionDistance = 1.0f;
	maxLength = 0.0f;
	float sums = primaryWeight + randomWeight + adhesionWeight;
	primaryWeight /= sums;
	randomWeight /= sums;
	adhesionWeight /= sums;
}

void Ivy::seed(const Vector3 &seedPos) {
	BasicMesh::reset();

	roots.clear();

	IvyNode tmpNode;

	IvyRoot tmpRoot;

	tmpRoot.nodes.push_back(tmpNode);

	roots.push_back(tmpRoot);
}
static inline Vector3 getRandomized() {
	return (Vector3(Math::rand() / (float)RAND_MAX - 0.5f, Math::rand() / (float)RAND_MAX - 0.5f, Math::rand() / (float)RAND_MAX - 0.5f)).normalized();
}

void Ivy::grow() {
	//parameters that depend on the scene object bounding sphere

	//normalize weights of influence

	//lets grow
	for (std::vector<IvyRoot>::iterator root = roots.begin(); root != roots.end(); ++root) {
		//process only roots that are alive
		if (!root->alive) {
			continue;
		}

		//let the ivy die, if the maximum float length is reached
		if (root->nodes.back().floatingLength > maxFloatLength) {
			root->alive = false;
		}

		//grow vectors: primary direction, random influence, and adhesion of scene objects

		//primary vector = weighted sum of previous grow vectors
		Vector3 primaryVector = root->nodes.back().primaryDir;

		// Make the random vector and normalize
		Vector3 randomVector = (getRandomized() + Vector3(0.0f, 0.2f, 0.0f)).normalized();

		//adhesion influence to the nearest triangle = weighted sum of previous adhesion vectors
		Vector3 adhesionVector = computeAdhesion(root->nodes.back().pos);

		//compute grow vector
		Vector3 growVector = ivySize * (primaryVector * primaryWeight + randomVector * randomWeight + adhesionVector * adhesionWeight);

		//gravity influence

		//compute gravity vector
		Vector3 gravityVector = ivySize * Vector3(0.0f, -1.0f, 0.0f) * gravityWeight;

		//gravity depends on the floating length
		gravityVector *= pow(root->nodes.back().floatingLength / maxFloatLength, 0.7f);

		//next possible ivy node

		//climbing state of that ivy node, will be set during collision detection
		bool climbing;

		//compute position of next ivy node
		Vector3 newPos = root->nodes.back().pos + growVector + gravityVector;

		//combine alive state with result of the collision detection, e.g. let the ivy die in case of a collision detection problem
		root->alive = root->alive & computeCollision(root->nodes.back().pos, newPos, climbing);

		//update grow vector due to a changed newPos
		growVector = newPos - root->nodes.back().pos - gravityVector;

		//create next ivy node
		IvyNode tmpNode;

		tmpNode.pos = newPos;

		tmpNode.primaryDir = (root->nodes.back().primaryDir.lerp(growVector, 0.5f)).normalized();

		tmpNode.adhesionVector = adhesionVector;

		tmpNode.length = root->nodes.back().length + (newPos - root->nodes.back().pos).length();

		if (tmpNode.length > maxLength) {
			maxLength = tmpNode.length;
		}

		tmpNode.floatingLength = climbing ? 0.0f : root->nodes.back().floatingLength + (newPos - root->nodes.back().pos).length();

		tmpNode.climb = climbing;

		root->nodes.push_back(tmpNode);
	}

	// Loop through all roots to check if a new root is generated
	for (std::vector<IvyRoot>::iterator root = roots.begin(); root != roots.end(); ++root) {
		//process only roots that are alive
		if (!root->alive) {
			continue;
		}

		//process only roots up to hierarchy level 3, results in a maximum hierarchy level of 4
		if (root->parents > 3) {
			continue;
		}

		if (root->nodes.size() < 2) {
			continue;
		}

		//add child ivys on existing ivy nodes
		for (std::vector<IvyNode>::iterator node = root->nodes.begin(); node != root->nodes.end(); ++node) {
			//weight depending on ratio of node length to total length
			float weight = 1.0f - (cos(2.0f * Math_PI * node->length / root->nodes.back().length) * 0.5f + 0.5f);

			//random influence
			float probability = Math::rand() / (float)RAND_MAX;

			if (probability * weight > branchingProbability) {
				//new ivy node
				IvyNode tmpNode;

				tmpNode.pos = node->pos;

				tmpNode.primaryDir = Vector3(0.0f, 1.0f, 0.0f);

				tmpNode.adhesionVector = Vector3(0.0f, 0.0f, 0.0f);

				tmpNode.length = 0.0f;

				tmpNode.floatingLength = node->floatingLength;

				tmpNode.climb = true;

				//new ivy root
				IvyRoot tmpRoot;

				tmpRoot.nodes.push_back(tmpNode);

				tmpRoot.alive = true;

				tmpRoot.parents = root->parents + 1;

				roots.push_back(tmpRoot);

				//limit the branching to only one new root per iteration, so return
				return;
			}
		}
	}
}

static inline bool getBarycentricCoordinates(const Vector3 &vector1, const Vector3 &vector2, const Vector3 &vector3, const Vector3 &position, float &alpha, float &beta, float &gamma) {
	float area = 0.5f * ((vector2 - vector1).cross(vector3 - vector1)).length();

	alpha = 0.5f * ((vector2 - position).cross(vector3 - position)).length() / area;

	beta = 0.5f * ((vector1 - position).cross(vector3 - position)).length() / area;

	gamma = 0.5f * ((vector1 - position).cross(vector2 - position)).length() / area;

	//if (abs( 1.0f - alpha - beta - gamma ) > std::numeric_limits<float>::epsilon()) return false;
	if (abs(1.0f - alpha - beta - gamma) > 0.00001f) {
		return false;
	}

	return true;
}
Vector3 Ivy::computeAdhesion(const Vector3 &pos) {
	//the resulting adhesion vector
	Vector3 adhesionVector;

	//define a maximum distance
	float local_maxAdhesionDistance = maxAdhesionDistance;

	float minDistance = local_maxAdhesionDistance;
	float last_distance = maxAdhesionDistance * 2;
	bool is_insde = false;
	for (uint32_t m = 0; m < meshList.size(); m++) {
		Vector3 normal;
		Vector4 point = meshList[m]->get_closest_point_to(pos, maxAdhesionDistance, normal);
		if (point.w > 0) {
			float dis = adhesionVector.distance_to(pos);
			if (dis < last_distance) {
				adhesionVector = Vector3(point.x, point.y, point.z);
				last_distance = adhesionVector.distance_to(pos);
				is_insde = true;
			}
		}
	}

	return adhesionVector;
}

bool Ivy::computeCollision(const Vector3 &oldPos, Vector3 &newPos, bool &climbing) {
	//reset climbing state
	climbing = false;

	bool intersection;

	int deadlockCounter = 0;
	Vector3 direction = newPos - oldPos;

	Vector3 from = oldPos;
	Vector3 to = newPos;
	float last_distance = from.distance_to(to) * 3;

	for (uint32_t m = 0; m < meshList.size(); m++) {
		Vector3 point;
		Vector3 normal;
		int32_t surf_index = -1;
		bool is_inside = meshList[m]->intersect_segment(from, to, point, normal, &surf_index, false);
		if (is_inside) {
			if (normal.dot(direction) < 0.0) {
				float dis = point.distance_to(from);
				if (dis < last_distance) {
					Vector3 reflected_direction = (to - point).reflect(normal);
					//mirror newPos at triangle plane
					newPos = point + reflected_direction;

					intersection = true;

					climbing = true;
				}
			}
		}
	}

	return intersection;
}

static inline Vector3 rotateAroundAxis(const Vector3 &vector, const Vector3 &axisPosition, const Vector3 &axis, const float &angle) {
	//determining the sinus and cosinus of the rotation angle
	float cosTheta = cos(angle);
	float sinTheta = sin(angle);

	//Vector3 from the given axis point to the initial point
	Vector3 direction = vector - axisPosition;

	//new vector which will hold the direction from the given axis point to the new rotated point
	Vector3 newDirection;

	//x-component of the direction from the given axis point to the rotated point
	newDirection.x = (cosTheta + (1 - cosTheta) * axis.x * axis.x) * direction.x +
			((1 - cosTheta) * axis.x * axis.y - axis.z * sinTheta) * direction.y +
			((1 - cosTheta) * axis.x * axis.z + axis.y * sinTheta) * direction.z;

	//y-component of the direction from the given axis point to the rotated point
	newDirection.y = ((1 - cosTheta) * axis.x * axis.y + axis.z * sinTheta) * direction.x +
			(cosTheta + (1 - cosTheta) * axis.y * axis.y) * direction.y +
			((1 - cosTheta) * axis.y * axis.z - axis.x * sinTheta) * direction.z;

	//z-component of the direction from the given axis point to the rotated point
	newDirection.z = ((1 - cosTheta) * axis.x * axis.z - axis.y * sinTheta) * direction.x +
			((1 - cosTheta) * axis.y * axis.z + axis.x * sinTheta) * direction.y +
			(cosTheta + (1 - cosTheta) * axis.z * axis.z) * direction.z;

	//returning the result by addind the new direction vector to the given axis point
	return axisPosition + newDirection;
}
void Ivy::birth() {
	//evolve a gaussian filter over the adhesian vectors

	float gaussian[11] = { 1.0f, 2.0f, 4.0f, 7.0f, 9.0f, 10.0f, 9.0f, 7.0f, 4.0f, 2.0f, 1.0f };

	for (unsigned int r = 0; r < roots.size(); r++) {
		for (int g = 0; g < 5; ++g) {
			IvyRoot root = roots[r];
			for (unsigned int n = 0; n < root.nodes.size(); n++) {
				Vector3 e;

				for (int i = -5; i <= 5; ++i) {
					Vector3 tmpAdhesion;

					if ((n + i) < 0) {
						tmpAdhesion = root.nodes.front().adhesionVector;
					}
					if ((n + i) >= root.nodes.size()) {
						tmpAdhesion = root.nodes.back().adhesionVector;
					}
					if (((n + i) >= 0) && ((n + i) < root.nodes.size())) {
						tmpAdhesion = root.nodes[n + i].adhesionVector;
					}

					e += tmpAdhesion * gaussian[i + 5];
				}

				root.nodes[n].smoothAdhesionVector = e / 56.0f;
				root.nodes[n].adhesionLength = root.nodes[n].smoothAdhesionVector.length();
				root.nodes[n].smoothAdhesionVector = root.nodes[n].smoothAdhesionVector.normalized();
			}
		}
	}

	//parameters that depend on the scene object bounding sphere

	float local_ivyBranchSize = ivySize * ivyBranchSize;

	//reset existing geometry
	BasicMesh::reset();

	//set data path
	path = "../textures/";

	//create material for leafs
	BasicMaterial tmpMaterial;

	tmpMaterial.id = 1;
	tmpMaterial.name = "leaf_adult";
	tmpMaterial.texFile = "efeu1.png";

	materials.push_back(tmpMaterial);

	//create second material for leafs
	tmpMaterial.id = 2;
	tmpMaterial.name = "leaf_young";
	tmpMaterial.texFile = "efeu0.png";

	materials.push_back(tmpMaterial);

	//create material for branches
	tmpMaterial.id = 3;
	tmpMaterial.name = "branch";
	tmpMaterial.texFile = "efeu_branch.png";

	materials.push_back(tmpMaterial);

	//create leafs
	for (std::vector<IvyRoot>::iterator root = roots.begin(); root != roots.end(); ++root) {
		//simple multiplier, just to make it a more dense
		if (root->nodes.size() < 2) {
			continue;
		}

		for (int node_index = 0; node_index < root->nodes.size() - 1; ++node_index) {
			float alignmentWeight = root->nodes[node_index].adhesionLength;
			//srand(i + (root - roots.begin()) * 10);
			IvyNode *node = &root->nodes[node_index];
			IvyNode *nodeNext = &root->nodes[node_index + 1];
			float weight = pow(node->length / root->nodes.back().length, 0.7f);
			//test: the probability of leaves on the ground is increased
			float groundIvy = MAX(0.0f, -node->adhesionVector.y);
			weight += groundIvy * pow(1.0f - node->length / root->nodes.back().length, 2.0f);

			//horizontal angle (+ an epsilon vector, otherwise there's a problem at 0� and 90�... mmmh)
			float phi = atan2(node->adhesionVector.z, node->adhesionVector.x) - Math_PI * 0.5f;
			//random influence
			phi += (Math::rand() / (float)RAND_MAX - 0.5f) * (1.3f - alignmentWeight);

			//vertical angle, trimmed by 0.5
			float theta = node->adhesionVector.angle_to(Vector3(0.0f, -1.0f, 0.0f)) * 0.5f;
			theta += (Math::rand() / (float)RAND_MAX - 0.5f) * (1.1f - alignmentWeight);
			//size of leaf
			float sizeWeight = 1.5f - (cos(weight * 2.0f * Math_PI) * 0.5f + 0.5f);

			float leafSize = sizeWeight * ivyLeafSize;

			for (int j = 0; j < 10; ++j) {
				//weight depending on ratio of node length to total length

				//random influence
				float probability = Math::rand() / (float)RAND_MAX;

				if (probability * weight > leafProbability) {
					//alignment weight depends on the adhesion "strength"

					//center of leaf quad
					Vector3 center = node->pos.lerp(nodeNext->pos, j / 10.0f) + getRandomized() * ivyLeafSize;

					//create vertices
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

					//create texCoords
					Vector2 tmpTexCoord;

					tmpTexCoord = Vector2(0.0f, 1.0f);
					texCoords.push_back(tmpTexCoord);

					tmpTexCoord = Vector2(1.0f, 1.0f);
					texCoords.push_back(tmpTexCoord);

					tmpTexCoord = Vector2(0.0f, 0.0f);
					texCoords.push_back(tmpTexCoord);

					tmpTexCoord = Vector2(1.0f, 0.0f);
					texCoords.push_back(tmpTexCoord);

					//create triangle
					BasicTriangle tmpTriangle;

					tmpTriangle.matid = 1;

					float probability = Math::rand() / (float)RAND_MAX;
					if (probability * weight > leafProbability) {
						tmpTriangle.matid = 2;
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

	//branches
	for (std::vector<IvyRoot>::iterator root = roots.begin(); root != roots.end(); ++root) {
		//process only roots with more than one node
		if (root->nodes.size() <= 1) {
			continue;
		}

		//branch diameter depends on number of parents
		float local_ivyBranchDiameter = 1.0f / (float)(root->parents + 1) + 1.0f;

		for (std::vector<IvyNode>::iterator node = root->nodes.begin(); node != root->nodes.end() - 1; ++node) {
			//weight depending on ratio of node length to total length
			float weight = node->length / root->nodes.back().length;

			//create trihedral vertices
			Vector3 up = Vector3(0.0f, -1.0f, 0.0f);

			Vector3 basis = ((node + 1)->pos - node->pos).normalized();
			float b0t = up.dot(basis.normalized()) * local_ivyBranchDiameter * local_ivyBranchSize * (1.3f - weight);
			Vector3 b0 = Vector3(b0t, b0t, b0t) + node->pos;

			Vector3 b1 = rotateAroundAxis(b0, node->pos, basis, 2.09f);

			Vector3 b2 = rotateAroundAxis(b0, node->pos, basis, 4.18f);

			//create vertices
			Vector3 tmpVertex;

			tmpVertex = b0;
			vertices.push_back(tmpVertex);

			tmpVertex = b1;
			vertices.push_back(tmpVertex);

			tmpVertex = b2;
			vertices.push_back(tmpVertex);

			//create texCoords
			Vector2 tmpTexCoord;

			float texV = (node - root->nodes.begin()) % 2 == 0 ? 1.0f : 0.0f;

			tmpTexCoord = Vector2(0.0f, texV);
			texCoords.push_back(tmpTexCoord);

			tmpTexCoord = Vector2(0.3f, texV);
			texCoords.push_back(tmpTexCoord);

			tmpTexCoord = Vector2(0.6f, texV);
			texCoords.push_back(tmpTexCoord);

			if (node == root->nodes.begin()) {
				continue;
			}

			//create triangle
			BasicTriangle tmpTriangle;

			tmpTriangle.matid = 3;

			tmpTriangle.v0id = (unsigned int)vertices.size() - 3;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 4;

			tmpTriangle.t0id = (unsigned int)vertices.size() - 3;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 4;

			triangles.push_back(tmpTriangle);

			tmpTriangle.v0id = (unsigned int)vertices.size() - 4;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 1;

			tmpTriangle.t0id = (unsigned int)vertices.size() - 4;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 1;

			triangles.push_back(tmpTriangle);

			tmpTriangle.v0id = (unsigned int)vertices.size() - 4;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 1;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 5;

			tmpTriangle.t0id = (unsigned int)vertices.size() - 4;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 1;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 5;

			triangles.push_back(tmpTriangle);

			tmpTriangle.v0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 1;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 2;

			tmpTriangle.t0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 1;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 2;

			triangles.push_back(tmpTriangle);

			tmpTriangle.v0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 2;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 0;

			tmpTriangle.t0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 2;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 0;

			triangles.push_back(tmpTriangle);

			tmpTriangle.v0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.v1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.v2id = (unsigned int)vertices.size() - 3;

			tmpTriangle.t0id = (unsigned int)vertices.size() - 5;
			tmpTriangle.t1id = (unsigned int)vertices.size() - 0;
			tmpTriangle.t2id = (unsigned int)vertices.size() - 3;

			triangles.push_back(tmpTriangle);
		}
	}

	//initialize ivy mesh

	prepareData();

	calculateVertexNormals();

	prepareData();
}
