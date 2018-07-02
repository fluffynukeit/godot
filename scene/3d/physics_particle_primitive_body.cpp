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
#include "engine.h"
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

	ClassDB::bind_method(D_METHOD("set_monitoring_particles_contacts", "monitoring"), &ParticlePrimitiveBody::set_monitoring_particles_contacts);
	ClassDB::bind_method(D_METHOD("is_monitoring_particles_contacts"), &ParticlePrimitiveBody::is_monitoring_particles_contacts);

	ClassDB::bind_method(D_METHOD("on_particle_contact", "particle_body_commands", "particle_body", "particle_index", "velocity", "normal"), &ParticlePrimitiveBody::on_particle_contact);

	ClassDB::bind_method(D_METHOD("resource_changed", "resource"), &ParticlePrimitiveBody::resource_changed);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape"), "set_shape", "get_shape");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "kinematic"), "set_kinematic", "is_kinematic");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitoring_particles_contacts"), "set_monitoring_particles_contacts", "is_monitoring_particles_contacts");

	ADD_GROUP("Collision", "collision_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_layer", "get_collision_layer");

	ADD_SIGNAL(MethodInfo("particle_contact", PropertyInfo(Variant::OBJECT, "particle_body_commands", PROPERTY_HINT_RESOURCE_TYPE, "ParticleBodyCommands"), PropertyInfo(Variant::OBJECT, "particle_body"), PropertyInfo(Variant::INT, "particle_index"), PropertyInfo(Variant::VECTOR3, "velocity"), PropertyInfo(Variant::VECTOR3, "normal")));
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

	ParticlePhysicsServer::get_singleton()->primitive_body_set_object_instance(rid, this);
	set_notify_transform(true);

	ParticlePhysicsServer::get_singleton()->primitive_body_set_callback(rid, ParticlePhysicsServer::PARTICLE_PRIMITIVE_BODY_CALLBACK_PARTICLECONTACT, this, "on_particle_contact");
}

ParticlePrimitiveBody::~ParticlePrimitiveBody() {

	ParticlePhysicsServer::get_singleton()->primitive_body_set_callback(rid, ParticlePhysicsServer::PARTICLE_PRIMITIVE_BODY_CALLBACK_PARTICLECONTACT, NULL, "");
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

void ParticlePrimitiveBody::set_monitoring_particles_contacts(bool p_monitoring) {
	ParticlePhysicsServer::get_singleton()->primitive_body_set_monitoring_particles_contacts(rid, p_monitoring);
}

bool ParticlePrimitiveBody::is_monitoring_particles_contacts() const {
	return ParticlePhysicsServer::get_singleton()->primitive_body_is_monitoring_particles_contacts(rid);
}

void ParticlePrimitiveBody::on_particle_contact(Object *p_particle_body_commands, Object *p_particle_body, int p_particle_index, Vector3 p_velocity, Vector3 p_normal) {

	emit_signal("particle_contact", p_particle_body_commands, p_particle_body, p_particle_index, p_velocity, p_normal);
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

void ParticlePrimitiveBody::resource_changed(RES res) {

	update_gizmo();
}

ParticlePrimitiveArea::ParticleBodyContacts::ParticleBodyContacts(Object *p_particle_object) :
		particle_body(p_particle_object),
		just_entered(true),
		particle_count(0) {}

void ParticlePrimitiveArea::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_monitor_particle_bodies_entering", "monitor"), &ParticlePrimitiveArea::set_monitor_particle_bodies_entering);
	ClassDB::bind_method(D_METHOD("get_monitor_particle_bodies_entering"), &ParticlePrimitiveArea::get_monitor_particle_bodies_entering);

	ClassDB::bind_method(D_METHOD("set_monitor_particles_entering", "monitor"), &ParticlePrimitiveArea::set_monitor_particles_entering);
	ClassDB::bind_method(D_METHOD("get_monitor_particles_entering"), &ParticlePrimitiveArea::get_monitor_particles_entering);

	ClassDB::bind_method(D_METHOD("get_overlapping_particles", "body"), &ParticlePrimitiveArea::get_overlapping_particles);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitor_particle_bodies_entering"), "set_monitor_particle_bodies_entering", "get_monitor_particle_bodies_entering");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitor_particles_entering"), "set_monitor_particles_entering", "get_monitor_particles_entering");

	ADD_SIGNAL(MethodInfo("on_body_enter", PropertyInfo(Variant::OBJECT, "particle_body")));
	ADD_SIGNAL(MethodInfo("on_body_exit", PropertyInfo(Variant::OBJECT, "particle_body")));
}

void ParticlePrimitiveArea::_notification(int p_what) {

	ParticlePrimitiveBody::_notification(p_what);

	if (NOTIFICATION_INTERNAL_PHYSICS_PROCESS != p_what)
		return;

	for (int i(body_contacts.size() - 1); 0 <= i; --i) {
		if (!body_contacts[i].particle_count) {

			emit_signal("on_body_exit", body_contacts[i].particle_body);
			body_contacts.remove(i);
			continue;
		}

		if (body_contacts[i].just_entered) {

			body_contacts[i].just_entered = false;
			emit_signal("on_body_enter", body_contacts[i].particle_body);
		}

		// Reset
		body_contacts[i].particle_count = 0;
	}
}

ParticlePrimitiveArea::ParticlePrimitiveArea() :
		ParticlePrimitiveBody() {
	ParticlePhysicsServer::get_singleton()->primitive_body_set_as_area(rid, true);

	set_physics_process_internal(false);
}

void ParticlePrimitiveArea::set_monitor_particle_bodies_entering(bool p_monitor) {
	monitor_particle_bodies_entering = p_monitor;
	if (!Engine::get_singleton()->is_editor_hint())
		set_physics_process_internal(monitor_particle_bodies_entering && monitor_particles_entering);
}

void ParticlePrimitiveArea::set_monitor_particles_entering(bool p_monitor) {
	monitor_particles_entering = p_monitor;
	if (!Engine::get_singleton()->is_editor_hint())
		set_physics_process_internal(monitor_particle_bodies_entering && monitor_particles_entering);
}

Vector<int> ParticlePrimitiveArea::get_overlapping_particles(Object *p_particle_body) {
	int i = body_contacts.find(ParticleBodyContacts(p_particle_body));
	if (-1 != i)
		return body_contacts[i].particles;
	return Vector<int>();
}

void ParticlePrimitiveArea::on_particle_contact(Object *p_particle_body_commands, Object *p_particle_body, int p_particle_index, Vector3 p_velocity, Vector3 p_normal) {

	ParticlePrimitiveBody::on_particle_contact(p_particle_body_commands, p_particle_body, p_particle_index, p_velocity, p_normal);

	if (!monitor_particle_bodies_entering && !monitor_particles_entering)
		return;

	int data_id = body_contacts.find(ParticleBodyContacts(p_particle_body));
	if (-1 == data_id) {
		data_id = body_contacts.size();
		body_contacts.push_back(ParticleBodyContacts(p_particle_body));
	}

	if (monitor_particles_entering) {
		if (!body_contacts[data_id].particle_count) {
			// When particle_count is 0 at this stage mean that is the first particle of body to be readed so is required to perform a cleaning of old particles contacts
			body_contacts[data_id].particles.clear();
		}
		body_contacts[data_id].particles.push_back(p_particle_index);
	}

	++(body_contacts[data_id].particle_count);
}
