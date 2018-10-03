/*************************************************************************/
/*  flex_primitive_shapes.cpp                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

/**
	@author AndreaCatania
*/

#ifndef FLEX_PRIMITIVE_SHAPES_H
#define FLEX_PRIMITIVE_SHAPES_H

#include "core/map.h"
#include "core/variant.h"
#include "flex_utility.h"
#include "rid_flex.h"
#include "thirdparty/flex/include/NvFlexExt.h"

class FlexPrimitiveBody;
class FlexSpace;

class FlexPrimitiveShape : public RIDFlex {

	Vector<FlexPrimitiveBody *> owners;

public:
	FlexPrimitiveShape();
	~FlexPrimitiveShape();

	void add_owner(FlexPrimitiveBody *p_owner);
	void remove_owner(FlexPrimitiveBody *p_owner);
	Vector<FlexPrimitiveBody *> get_owners() const { return owners; }
	void notify_change();

	virtual NvFlexCollisionShapeType get_type() = 0;
	virtual void get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape) = 0;
	virtual void set_data(const Variant &p_data) = 0;
	virtual Variant get_data() const = 0;
	virtual bool need_alignment() const { return false; }
	virtual const Basis &get_alignment_basis() const { return Basis(); }
};

class FlexPrimitiveBoxShape : public FlexPrimitiveShape {

	Vector3 extends;

public:
	FlexPrimitiveBoxShape();

	virtual NvFlexCollisionShapeType get_type() { return eNvFlexShapeBox; }
	virtual void get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape);
	virtual void set_data(const Variant &p_data);
	virtual Variant get_data() const;

	void set_extends(const Vector3 &p_extends);
	const Vector3 &get_extends() const { return extends; }
};

class FlexPrimitiveCapsuleShape : public FlexPrimitiveShape {

	float half_height;
	float radius;

	static Basis alignment;

public:
	FlexPrimitiveCapsuleShape();

	virtual NvFlexCollisionShapeType get_type() { return eNvFlexShapeCapsule; }
	virtual void get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape);
	virtual void set_data(const Variant &p_data);
	virtual Variant get_data() const;
	virtual bool need_alignment() const { return true; }
	virtual const Basis &get_alignment_basis() const;
};

class FlexPrimitiveSphereShape : public FlexPrimitiveShape {

	float radius;

public:
	FlexPrimitiveSphereShape();

	virtual NvFlexCollisionShapeType get_type() { return eNvFlexShapeSphere; }
	virtual void get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape);
	virtual void set_data(const Variant &p_data);
	virtual Variant get_data() const;
};

class FlexPrimitiveConvexShape : public FlexPrimitiveShape {

	struct MeshData {
		MeshData() :
				mesh_id(0),
				vertices_buffer(NULL) {}

		MeshData(const MeshData &p_other) :
				mesh_id(p_other.mesh_id),
				vertices_buffer(p_other.vertices_buffer) {}

		NvFlexConvexMeshId mesh_id;
		NvFlexVector<FlVector4> *vertices_buffer;
	};

	Vector<Vector3> vertices;
	Map<FlexSpace *, MeshData> cache;

public:
	FlexPrimitiveConvexShape();
	~FlexPrimitiveConvexShape();

	virtual NvFlexCollisionShapeType get_type() { return eNvFlexShapeConvexMesh; }
	virtual void get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape);
	virtual void set_data(const Variant &p_data);
	virtual Variant get_data() const;

private:
	void setup(const Vector<Vector3> &p_vertices);
	void update_space_mesh(FlexSpace *p_space);
};

class FlexPrimitiveTriangleShape : public FlexPrimitiveShape {

	struct MeshData {
		MeshData() :
				mesh_id(0),
				vertices_buffer(NULL),
				indices_buffer(NULL) {}

		MeshData(const MeshData &p_other) :
				mesh_id(p_other.mesh_id),
				vertices_buffer(p_other.vertices_buffer),
				indices_buffer(p_other.indices_buffer) {}

		NvFlexTriangleMeshId mesh_id;
		NvFlexVector<FlVector4> *vertices_buffer;
		NvFlexVector<int> *indices_buffer;
	};

	PoolVector<Vector3> faces;
	Map<FlexSpace *, MeshData> cache;

public:
	FlexPrimitiveTriangleShape();
	~FlexPrimitiveTriangleShape();

	virtual NvFlexCollisionShapeType get_type() { return eNvFlexShapeTriangleMesh; }
	virtual void get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape);
	virtual void set_data(const Variant &p_data);
	virtual Variant get_data() const;

private:
	void setup(PoolVector<Vector3> p_faces);
	void update_space_mesh(FlexSpace *p_space);
};
#endif // FLEX_PRIMITIVE_SHAPES_H
