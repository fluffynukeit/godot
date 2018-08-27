/*************************************************************************/
/*  flex_primitive_shapes.h                                              */
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

#include "flex_primitive_shapes.h"
#include "flex_primitive_body.h"

FlexPrimitiveShape::FlexPrimitiveShape() :
		RIDFlex() {
}

FlexPrimitiveShape::~FlexPrimitiveShape() {
	for (int i(owners.size() - 1); 0 <= i; --i) {
		owners[i]->set_shape(NULL);
	}
}

FlexPrimitiveBoxShape::FlexPrimitiveBoxShape() :
		FlexPrimitiveShape(),
		extends(1.0, 1.0, 1.0) {}

void FlexPrimitiveShape::add_owner(FlexPrimitiveBody *p_owner) {
	owners.push_back(p_owner);
}

void FlexPrimitiveShape::remove_owner(FlexPrimitiveBody *p_owner) {
	owners.erase(p_owner);
}

void FlexPrimitiveShape::notify_change() {
	for (int i(owners.size() - 1); 0 <= i; --i) {
		owners[i]->notify_shape_changed();
	}
}

void FlexPrimitiveBoxShape::get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape) {
	r_shape->box.halfExtents[0] = extends.x;
	r_shape->box.halfExtents[1] = extends.y;
	r_shape->box.halfExtents[2] = extends.z;
}

void FlexPrimitiveBoxShape::set_data(const Variant &p_data) {
	set_extends(p_data);
}

Variant FlexPrimitiveBoxShape::get_data() const {
	return extends;
}

void FlexPrimitiveBoxShape::set_extends(const Vector3 &p_extends) {
	extends = p_extends;
}

Basis FlexPrimitiveCapsuleShape::alignment(0, 0, -1, 0, 1, 0, 1, 0, 0);

FlexPrimitiveCapsuleShape::FlexPrimitiveCapsuleShape() :
		FlexPrimitiveShape(),
		half_height(0.5),
		radius(1) {
}

void FlexPrimitiveCapsuleShape::get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape) {
	r_shape->capsule.halfHeight = half_height;
	r_shape->capsule.radius = radius;
}

void FlexPrimitiveCapsuleShape::set_data(const Variant &p_data) {
	Dictionary d = p_data;
	ERR_FAIL_COND(!d.has("radius"));
	ERR_FAIL_COND(!d.has("height"));
	half_height = static_cast<float>(d["height"]) / 2.0;
	radius = d["radius"];
}

Variant FlexPrimitiveCapsuleShape::get_data() const {
	Dictionary d;
	d["height"] = half_height * 2;
	d["radius"] = radius;
	return d;
}

const Basis &FlexPrimitiveCapsuleShape::get_alignment_basis() const {
	return alignment;
}

FlexPrimitiveSphereShape::FlexPrimitiveSphereShape() :
		radius(1) {}

void FlexPrimitiveSphereShape::get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape) {
	r_shape->sphere.radius = radius;
}

void FlexPrimitiveSphereShape::set_data(const Variant &p_data) {
	radius = p_data;
}

Variant FlexPrimitiveSphereShape::get_data() const {
	return radius;
}

FlexPrimitiveConvexShape::FlexPrimitiveConvexShape() {}

FlexPrimitiveConvexShape::~FlexPrimitiveConvexShape() {

	for (Map<FlexSpace *, MeshData>::Element *e = cache.front(); e; e = e->next()) {
		NvFlexDestroyConvexMesh(e->get().vertices_buffer->lib, e->get().mesh_id);
		e->get().vertices_buffer->destroy();
		delete e->get().vertices_buffer;
		cache.erase(e);
	}
}

void FlexPrimitiveConvexShape::get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape) {

	if (!cache.has(p_space)) {
		update_space_mesh(p_space);
	}

	r_shape->convexMesh.scale[0] = 1;
	r_shape->convexMesh.scale[1] = 1;
	r_shape->convexMesh.scale[2] = 1;
	r_shape->convexMesh.mesh = cache[p_space].mesh_id;
}

void FlexPrimitiveConvexShape::set_data(const Variant &p_data) {
	setup(p_data);
}

Variant FlexPrimitiveConvexShape::get_data() const {
	return vertices;
}

void FlexPrimitiveConvexShape::setup(const Vector<Vector3> &p_vertices) {
	vertices = p_vertices;

	for (Map<FlexSpace *, MeshData>::Element *e = cache.front(); e; e = e->next()) {
		update_space_mesh(e->key());
	}
}

void FlexPrimitiveConvexShape::update_space_mesh(FlexSpace *p_space) {

	MeshData md;
	if (cache.has(p_space)) {
		md = cache[p_space];
		md.vertices_buffer->map();
	} else {
		md.mesh_id = NvFlexCreateConvexMesh(p_space->get_flex_library());
		md.vertices_buffer = new NvFlexVector<FlVector4>(p_space->get_flex_library());
	}

	md.vertices_buffer->resize(vertices.size());

	/// This is completelly wrong aproach. I need to think a way on how to create planes from points
	/// Commented to avoid generation of wrong shapes
	AABB aabb;
	//const int vs = vertices.size();
	//for (int i = 0; i < vs; ++i) {
	//
	//	aabb.expand_to(vertices[i]);
	//	(*md.vertices_buffer)[i] = flvec4_from_vec3(vertices[i].normalized());
	//	(*md.vertices_buffer)[i].w = vertices[i].length() * -1;
	//}

	md.vertices_buffer->unmap();

	const Vector3 lower_bound = aabb.get_position() - Vector3(0.1, 0.1, 0.1);
	const Vector3 upper_bound = aabb.get_size() + aabb.get_position() + Vector3(0.1, 0.1, 0.1);

	NvFlexUpdateConvexMesh(p_space->get_flex_library(), md.mesh_id, md.vertices_buffer->buffer, md.vertices_buffer->size(), (float *)(&lower_bound), (float *)(&upper_bound));

	cache[p_space] = md;
}

FlexPrimitiveTriangleShape::FlexPrimitiveTriangleShape() {}

FlexPrimitiveTriangleShape::~FlexPrimitiveTriangleShape() {

	for (Map<FlexSpace *, MeshData>::Element *e = cache.front(); e; e = e->next()) {
		NvFlexDestroyTriangleMesh(e->get().vertices_buffer->lib, e->get().mesh_id);
		e->get().vertices_buffer->destroy();
		e->get().indices_buffer->destroy();
		delete e->get().vertices_buffer;
		delete e->get().indices_buffer;
		cache.erase(e);
	}
}

void FlexPrimitiveTriangleShape::get_shape(FlexSpace *p_space, NvFlexCollisionGeometry *r_shape) {

	if (!cache.has(p_space)) {
		update_space_mesh(p_space);
	}

	r_shape->triMesh.scale[0] = 1;
	r_shape->triMesh.scale[1] = 1;
	r_shape->triMesh.scale[2] = 1;
	r_shape->triMesh.mesh = cache[p_space].mesh_id;
}

void FlexPrimitiveTriangleShape::set_data(const Variant &p_data) {
	setup(p_data);
}

Variant FlexPrimitiveTriangleShape::get_data() const {
	return faces;
}

void FlexPrimitiveTriangleShape::setup(PoolVector<Vector3> p_faces) {
	faces = p_faces;

	for (Map<FlexSpace *, MeshData>::Element *e = cache.front(); e; e = e->next()) {
		update_space_mesh(e->key());
	}
}

void FlexPrimitiveTriangleShape::update_space_mesh(FlexSpace *p_space) {

	MeshData md;
	if (cache.has(p_space)) {
		md = cache[p_space];
		md.vertices_buffer->map();
		md.indices_buffer->map();
	} else {
		md.mesh_id = NvFlexCreateTriangleMesh(p_space->get_flex_library());
		md.vertices_buffer = new NvFlexVector<FlVector4>(p_space->get_flex_library());
		md.indices_buffer = new NvFlexVector<int>(p_space->get_flex_library());
	}

	md.vertices_buffer->resize(faces.size());
	md.indices_buffer->resize(faces.size());

	PoolVector<Vector3>::Read r = faces.read();

	AABB aabb;
	const int fs = faces.size() / 3;
	for (int i = 0; i < fs; ++i) {

		aabb.expand_to(r[i * 3 + 0]);
		aabb.expand_to(r[i * 3 + 1]);
		aabb.expand_to(r[i * 3 + 2]);

		// Seems to use normals and are invorse of normals used by Godot.
		(*md.vertices_buffer)[i * 3 + 0] = flvec4_from_vec3(r[i * 3 + 2]);
		(*md.vertices_buffer)[i * 3 + 1] = flvec4_from_vec3(r[i * 3 + 1]);
		(*md.vertices_buffer)[i * 3 + 2] = flvec4_from_vec3(r[i * 3 + 0]);

		(*md.indices_buffer)[i * 3 + 0] = i * 3 + 0;
		(*md.indices_buffer)[i * 3 + 1] = i * 3 + 1;
		(*md.indices_buffer)[i * 3 + 2] = i * 3 + 2;
	}

	md.vertices_buffer->unmap();
	md.indices_buffer->unmap();

	const Vector3 lower_bound = aabb.get_position() - Vector3(0.1, 0.1, 0.1);
	const Vector3 upper_bound = aabb.get_size() + aabb.get_position() + Vector3(0.1, 0.1, 0.1);

	NvFlexUpdateTriangleMesh(p_space->get_flex_library(), md.mesh_id, md.vertices_buffer->buffer, md.indices_buffer->buffer, md.vertices_buffer->size(), md.indices_buffer->size() / 3, (float *)(&lower_bound), (float *)(&upper_bound));

	cache[p_space] = md;
}
