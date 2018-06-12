/*************************************************************************/
/*  physics_particle_primitive_body.cpp                                  */
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
 * @author AndreaCatania
 */

#include "physics_particle_primitive_body.h"
#include "scene/3d/mesh_instance.h"

void ParticlePrimitiveBody::_bind_methods() {

	ClassDB::bind_method(D_METHOD("move", "transform"), &ParticlePrimitiveBody::move);

	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &ParticlePrimitiveBody::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &ParticlePrimitiveBody::get_shape);

	ClassDB::bind_method(D_METHOD("set_kinematic", "kinematic"), &ParticlePrimitiveBody::set_kinematic);
	ClassDB::bind_method(D_METHOD("is_kinematic"), &ParticlePrimitiveBody::is_kinematic);

	ClassDB::bind_method(D_METHOD("set_collision_layer", "layer"), &ParticlePrimitiveBody::set_collision_layer);
	ClassDB::bind_method(D_METHOD("get_collision_layer"), &ParticlePrimitiveBody::get_collision_layer);
	ClassDB::bind_method(D_METHOD("set_collision_layer_bit", "bit", "value"), &ParticlePrimitiveBody::set_collision_layer_bit);
	ClassDB::bind_method(D_METHOD("get_collision_layer_bit", "bit"), &ParticlePrimitiveBody::get_collision_layer_bit);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape"), "set_shape", "get_shape");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "kinematic"), "set_kinematic", "is_kinematic");

	ADD_GROUP("Collision", "collision_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_layer", "get_collision_layer");
}

void ParticlePrimitiveBody::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			ParticlePhysicsServer::get_singleton()->primitive_body_set_space(rid, get_world()->get_particle_space());
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			ParticlePhysicsServer::get_singleton()->primitive_body_set_transform(rid, get_global_transform(), true);
		} break;
		case NOTIFICATION_EXIT_WORLD: {
			ParticlePhysicsServer::get_singleton()->primitive_body_set_space(rid, RID());
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (get_tree()->is_debugging_collisions_hint()) {
				_create_debug_shape();
			}
		} break;
	}
}

ParticlePrimitiveBody::ParticlePrimitiveBody() :
		ParticleObject(ParticlePhysicsServer::get_singleton()->primitive_body_create()),
		collision_layer(1),
		debug_shape(NULL) {

	set_notify_transform(true);
}

ParticlePrimitiveBody::~ParticlePrimitiveBody() {

	debug_shape = NULL;
}

void ParticlePrimitiveBody::move(const Transform &p_transform) {
	set_notify_transform(false);
	set_global_transform(p_transform);
	ParticlePhysicsServer::get_singleton()->primitive_body_set_transform(rid, p_transform, false);
	set_notify_transform(true);
}

void ParticlePrimitiveBody::set_shape(const Ref<Shape> &p_shape) {

	if (!shape.is_null())
		shape->unregister_owner(this);
	shape = p_shape;
	if (!shape.is_null()) {

		shape->register_owner(this);
		ParticlePhysicsServer::get_singleton()->primitive_body_set_shape(rid, shape->get_particle_rid());
	} else {

		ParticlePhysicsServer::get_singleton()->primitive_body_set_shape(rid, RID());
	}

	update_gizmo();
	update_configuration_warning();
}

Ref<Shape> ParticlePrimitiveBody::get_shape() const {
	return shape;
}

void ParticlePrimitiveBody::set_kinematic(bool p_kinematic) {
	ParticlePhysicsServer::get_singleton()->primitive_body_set_kinematic(rid, p_kinematic);
}

bool ParticlePrimitiveBody::is_kinematic() const {
	return ParticlePhysicsServer::get_singleton()->primitive_body_is_kinematic(rid);
}

void ParticlePrimitiveBody::set_collision_layer(uint32_t p_layer) {

	collision_layer = p_layer;
	ParticlePhysicsServer::get_singleton()->primitive_body_set_collision_layer(rid, p_layer);
}

uint32_t ParticlePrimitiveBody::get_collision_layer() const {

	return collision_layer;
}

void ParticlePrimitiveBody::set_collision_layer_bit(int p_bit, bool p_value) {

	uint32_t mask = get_collision_layer();
	if (p_value)
		mask |= 1 << p_bit;
	else
		mask &= ~(1 << p_bit);
	set_collision_layer(mask);
}

bool ParticlePrimitiveBody::get_collision_layer_bit(int p_bit) const {

	return get_collision_layer() & (1 << p_bit);
}

void ParticlePrimitiveBody::_create_debug_shape() {

	if (debug_shape) {
		debug_shape->queue_delete();
		debug_shape = NULL;
	}

	Ref<Shape> s = get_shape();

	if (s.is_null())
		return;

	Ref<Mesh> mesh = s->get_debug_mesh();

	MeshInstance *mi = memnew(MeshInstance);
	mi->set_mesh(mesh);

	add_child(mi);
	debug_shape = mi;
}
