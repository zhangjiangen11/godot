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


#ifndef IVY_H
#define IVY_H

#include "core/math/vector3.h"
#include "core/math/vector2.h"
#include "core/math/triangle_mesh.h"
#include "core/string/ustring.h"

#include "core/object/ref_counted.h"

#include <vector>


/** an ivy node */
class IvyNode
{
public:

    IvyNode() : length(0.0f), floatingLength(0.0f), climb(false) {}

	/** node position */
	Vector3 pos = Vector3(0.0f, 0.0f, 0.0f);			

	/** primary grow direction, a weighted sum of the previous directions */
	Vector3 primaryDir = Vector3(0, 0, 1);

	/** adhesion vector as a result from other scene objects */
	Vector3 adhesionVector = Vector3(0.0f, 0.0f, 0.0f);

	/** a smoothed adhesion vector computed and used during the birth phase,
	   since the ivy leaves are align by the adhesion vector, this smoothed vector
	   allows for smooth transitions of leaf alignment */
	   Vector3 smoothAdhesionVector;

	/** length of the associated ivy branch at this node */
	float length = 0.0001;

	/** length at the last node that was climbing */
	float floatingLength = 0;

	float adhesionLength = 0.0f;

	/** climbing state */
	bool climb = true;
};


/** an ivy root point */
class IvyRoot
{
public:

	/** a number of nodes */
	std::vector<IvyNode> nodes;

	/** alive state */
	bool alive = true;

	/** number of parents, represents the level in the root hierarchy */
	int parents = 0;
};
class BasicTriangle
{
public:

    BasicTriangle()  {}

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
/** a simple material containing only a single texture */
class BasicMaterial
{
public:

	BasicMaterial() : id(0), texObj(0) {}

	unsigned int id;

	std::string name;

	std::string texFile;

	unsigned int texObj;
};
class BasicMesh 
{
public:

	BasicMesh()
	{
		boundingSphereRadius = 1.0f;
	
		boundingSpherePos = Vector3(0.0f, 0.0f, 0.0f);
	
		displayListObj = 0;
	}

	~BasicMesh()
	{

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

	/** setup the triangles pointer to their vertices, normals, texCoords, and materials; computes the bounding sphere */
	void prepareData() {
		//update pointers of triangle
		for (std::vector<BasicTriangle>::iterator t = triangles.begin(); t != triangles.end(); ++t)
		{
			t->v0 = vertices[t->v0id - 1];
			t->v1 = vertices[t->v1id - 1];
			t->v2 = vertices[t->v2id - 1];

			if (t->n0id != 0) t->n0 = normals[t->n0id - 1];
			if (t->n1id != 0) t->n1 = normals[t->n1id - 1];
			if (t->n2id != 0) t->n2 = normals[t->n2id - 1];

			if (t->t0id != 0) t->t0 = texCoords[t->t0id - 1];
			if (t->t1id != 0) t->t1 = texCoords[t->t1id - 1];
			if (t->t2id != 0) t->t2 = texCoords[t->t2id - 1];
	
			//if (t->matid != 0) t->mat = materials[t->matid - 1];
		}

		//compute bounding sphere
		boundingSpherePos = Vector3(0.0f, 0.0f, 0.0f);

		for (std::vector<Vector3>::iterator v = vertices.begin(); v != vertices.end(); ++v)
		{
			boundingSpherePos += *v;
		}

		boundingSpherePos /= (float)vertices.size();

		boundingSphereRadius = 0.0f;

		for (std::vector<Vector3>::iterator v = vertices.begin(); v != vertices.end(); ++v)
		{
			boundingSphereRadius = MAX(boundingSphereRadius, ((*v) - boundingSpherePos).length());
		}
	}

	/** computes the vertex normals */
	void calculateVertexNormals() {
		normals.clear();

		normals.resize( vertices.size() );
	
	
	}

	/** flips the vertex normals */
	void flipNormals()
	{
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

/** the ivy itself, derived from basic mesh that allows to handle the final ivy mesh as a drawable object */
class Ivy : public BasicMesh
{
public:

	Ivy();

	void resetSettings();

	/** initialize a new ivy root */
	void seed(const Vector3 &seedPos);

	/** one single grow iteration */
	void grow();

	/** compute the adhesion of scene objects at a point pos*/
	Vector3 computeAdhesion(const Vector3& pos);

	/** computes the collision detection for an ivy segment oldPos->newPos, newPos will be modified if necessary */
	bool computeCollision(const Vector3& oldPos, Vector3& newPos, bool &climbingState);

	/** creates the ivy triangle mesh */
	void birth();
	
	/** the ivy roots */
	std::vector<IvyRoot> roots;	

public:
	LocalVector<Ref<TriangleMesh>> meshList;
	/** the ivy size factor, influences the grow behaviour [0..0,1] */
	float ivySize;

	/** leaf size factor [0..0,1] */
	float ivyLeafSize;

	/** branch size factor [0..0,1] */
	float ivyBranchSize;

    /** maximum length of an ivy branch segment that is freely floating [0..1] */
	float maxFloatLength;

	/** maximum distance for adhesion of scene object [0..1] */
	float maxAdhesionDistance;

	/** weight for the primary grow vector [0..1] */
	float primaryWeight;

	/** weight for the random influence vector [0..1] */
	float randomWeight;

	/** weight for the gravity vector [0..1] */
	float gravityWeight;

	/** weight for the adhesion vector [0..1] */
	float adhesionWeight;

	/** the probability of producing a new ivy root per iteration [0..1]*/
	float branchingProbability;

	/** the probability of creating a new ivy leaf [0..1] */
	float leafProbability;
	float maxLength = 0.0f;
};


#endif
