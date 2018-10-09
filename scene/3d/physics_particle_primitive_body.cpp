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
#include "core/engine.h"
#include "physics_particle_body.h"
#include "scene/3d/mesh_instance.h"

void ParticlePrimitiveBody::_bind_methods() {

	ClassDB::bind_method(D_METHOD("teleport", "transform"), &ParticlePrimitiveBody::teleport);

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

	ClassDB::bind_method(D_METHOD("set_callback_sync", "enabled"), &ParticlePrimitiveBody::set_callback_sync);
	ClassDB::bind_method(D_METHOD("is_callback_sync_enabled"), &ParticlePrimitiveBody::is_callback_sync_enabled);

	ClassDB::bind_method(D_METHOD("on_particle_contact", "particle_body", "particle_index", "velocity", "normal"), &ParticlePrimitiveBody::_on_particle_contact);
	ClassDB::bind_method(D_METHOD("_on_sync"), &ParticlePrimitiveBody::_on_sync);

	ClassDB::bind_method(D_METHOD("resource_changed", "resource"), &ParticlePrimitiveBody::resource_changed);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape"), "set_shape", "get_shape");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "kinematic"), "set_kinematic", "is_kinematic");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitoring_particles_contacts"), "set_monitoring_particles_contacts", "is_monitoring_particles_contacts");

	ADD_GROUP("Collision", "collision_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_layer", "get_collision_layer");

	ADD_SIGNAL(MethodInfo("particle_contact", PropertyInfo(Variant::OBJECT, "particle_body_commands", PROPERTY_HINT_RESOURCE_TYPE, "ParticleBodyCommands"), PropertyInfo(Variant::OBJECT, "particle_body"), PropertyInfo(Variant::INT, "particle_index"), PropertyInfo(Variant::VECTOR3, "velocity"), PropertyInfo(Variant::VECTOR3, "normal")));
	ADD_SIGNAL(MethodInfo("sync"));
}

void ParticlePrimitiveBody::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			teleport(get_global_transform());
			ParticlePhysicsServer::get_singleton()->primitive_body_set_space(rid, get_world()->get_particle_space());
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			ParticlePhysicsServer::get_singleton()->primitive_body_set_transform(rid, get_global_transform(), false);
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

	set_shape(NULL);
	ParticlePhysicsServer::get_singleton()->primitive_body_set_callback(rid, ParticlePhysicsServer::PARTICLE_PRIMITIVE_BODY_CALLBACK_PARTICLECONTACT, NULL, "");
	ParticlePhysicsServer::get_singleton()->primitive_body_set_callback(rid, ParticlePhysicsServer::PARTICLE_PRIMITIVE_BODY_CALLBACK_SYNC, NULL, "");
	debug_shape = NULL;
}

void ParticlePrimitiveBody::teleport(const Transform &p_transform) {
	set_notify_transform(false);
	set_global_transform(p_transform);
	ParticlePhysicsServer::get_singleton()->primitive_body_set_transform(rid, p_transform, false);
	set_notify_transform(true);
}

void ParticlePrimitiveBody::set_shape(const Ref<Shape> &p_shape) {

	if (shape.is_valid())
		shape->unregister_owner(this);

	shape = p_shape;
	if (shape.is_valid()) {

		shape->register_owner(this);
		ParticlePhysicsServer::get_singleton()->primitive_body_set_shape(rid, shape->get_particle_rid());

	} else
		ParticlePhysicsServer::get_singleton()->primitive_body_set_shape(rid, RID());

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

void ParticlePrimitiveBody::set_callback_sync(bool p_enabled) {
	_is_callback_sync_enabled = p_enabled;
	if (_is_callback_sync_enabled) {
		ParticlePhysicsServer::get_singleton()->primitive_body_set_callback(rid, ParticlePhysicsServer::PARTICLE_PRIMITIVE_BODY_CALLBACK_SYNC, this, "_on_sync");
	} else {
		ParticlePhysicsServer::get_singleton()->primitive_body_set_callback(rid, ParticlePhysicsServer::PARTICLE_PRIMITIVE_BODY_CALLBACK_SYNC, NULL, "");
	}
}

bool ParticlePrimitiveBody::is_callback_sync_enabled() const {
	return _is_callback_sync_enabled;
}

void ParticlePrimitiveBody::_on_particle_contact(Object *p_particle_body, int p_particle_index, Vector3 p_velocity, Vector3 p_normal) {

	emit_signal("particle_contact", ParticlePhysicsServer::get_singleton()->body_get_commands(cast_to<ParticleBody>(p_particle_body)->get_rid()), p_particle_body, p_particle_index, p_velocity, p_normal);
}

void ParticlePrimitiveBody::_on_sync() {
	emit_signal("sync", NULL, 0);
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

ParticlePrimitiveArea::ParticleContacts::ParticleContacts(int p_index) :
		particle_index(p_index),
		stage(0) {}

ParticlePrimitiveArea::ParticleBodyContacts::ParticleBodyContacts(Object *p_particle_object) :
		particle_body(p_particle_object),
		just_entered(true),
		particle_count(0) {}

void ParticlePrimitiveArea::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_monitor_particle_bodies_entering", "monitor"), &ParticlePrimitiveArea::set_monitor_particle_bodies_entering);
	ClassDB::bind_method(D_METHOD("get_monitor_particle_bodies_entering"), &ParticlePrimitiveArea::get_monitor_particle_bodies_entering);

	ClassDB::bind_method(D_METHOD("set_monitor_particles_entering", "monitor"), &ParticlePrimitiveArea::set_monitor_particles_entering);
	ClassDB::bind_method(D_METHOD("get_monitor_particles_entering"), &ParticlePrimitiveArea::get_monitor_particles_entering);

	ClassDB::bind_method(D_METHOD("get_overlapping_body_count"), &ParticlePrimitiveArea::get_overlapping_body_count);
	ClassDB::bind_method(D_METHOD("find_overlapping_body_pos", "particle_body"), &ParticlePrimitiveArea::find_overlapping_body_pos);
	ClassDB::bind_method(D_METHOD("get_overlapping_body", "id"), &ParticlePrimitiveArea::get_overlapping_body);
	ClassDB::bind_method(D_METHOD("get_overlapping_particles_count", "id"), &ParticlePrimitiveArea::get_overlapping_particles_count);
	ClassDB::bind_method(D_METHOD("get_overlapping_particle_index", "body_id", "particle_id"), &ParticlePrimitiveArea::get_overlapping_particle_index);

	ClassDB::bind_method(D_METHOD("reset_inside_bodies"), &ParticlePrimitiveArea::reset_inside_bodies);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitor_particle_bodies_entering"), "set_monitor_particle_bodies_entering", "get_monitor_particle_bodies_entering");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitor_particles_entering"), "set_monitor_particles_entering", "get_monitor_particles_entering");

	ADD_SIGNAL(MethodInfo("body_enter", PropertyInfo(Variant::OBJECT, "particle_body")));
	ADD_SIGNAL(MethodInfo("body_exit", PropertyInfo(Variant::OBJECT, "particle_body")));

	ADD_SIGNAL(MethodInfo("particle_enter", PropertyInfo(Variant::OBJECT, "particle_body"), PropertyInfo(Variant::INT, "particle_index")));
	ADD_SIGNAL(MethodInfo("particle_exit", PropertyInfo(Variant::OBJECT, "particle_body"), PropertyInfo(Variant::INT, "particle_index")));
}

ParticlePrimitiveArea::ParticlePrimitiveArea() :
		ParticlePrimitiveBody(),
		monitor_particle_bodies_entering(false),
		monitor_particles_entering(false) {
	ParticlePhysicsServer::get_singleton()->primitive_body_set_as_area(rid, true);

	set_callback_sync(false);
}

String ParticlePrimitiveArea::get_configuration_warning() const {
	String warning = ParticlePrimitiveBody::get_configuration_warning();

	if (monitor_particle_bodies_entering || monitor_particles_entering) {
		if (!is_monitoring_particles_contacts()) {
			if (warning != String()) {
				warning += "\n\n";
			}
			warning += TTR("The Area can't monitor body or particles untill you set monitoring_particles_contacts to TRUE.");
		}
	}

	return warning;
}

void ParticlePrimitiveArea::set_monitor_particle_bodies_entering(bool p_monitor) {
	monitor_particle_bodies_entering = p_monitor;

	if (!Engine::get_singleton()->is_editor_hint())
		set_callback_sync(monitor_particle_bodies_entering || monitor_particles_entering);
}

void ParticlePrimitiveArea::set_monitor_particles_entering(bool p_monitor) {
	monitor_particles_entering = p_monitor;

	if (!Engine::get_singleton()->is_editor_hint())
		set_callback_sync(monitor_particle_bodies_entering || monitor_particles_entering);
}

int ParticlePrimitiveArea::get_overlapping_body_count() const {
	return body_contacts.size();
}

int ParticlePrimitiveArea::find_overlapping_body_pos(Object *p_particle_body) {
	return body_contacts.find(ParticleBodyContacts(p_particle_body));
}

Object *ParticlePrimitiveArea::get_overlapping_body(int id) const {
	ERR_FAIL_INDEX_V(id, body_contacts.size(), NULL);
	return body_contacts[id].particle_body;
}

int ParticlePrimitiveArea::get_overlapping_particles_count(int id) {
	ERR_FAIL_INDEX_V(id, body_contacts.size(), -1);
	return body_contacts[id].particles.size();
}

int ParticlePrimitiveArea::get_overlapping_particle_index(int body_id, int particle_id) {
	ERR_FAIL_INDEX_V(body_id, body_contacts.size(), -1);
	ERR_FAIL_INDEX_V(particle_id, body_contacts[body_id].particles.size(), -1);
	return body_contacts[body_id].particles[particle_id].particle_index;
}

void ParticlePrimitiveArea::reset_inside_bodies() {
	body_contacts.clear();
}

void ParticlePrimitiveArea::_on_particle_contact(Object *p_particle_body, int p_particle_index, Vector3 p_velocity, Vector3 p_normal) {

	ParticlePrimitiveBody::_on_particle_contact(p_particle_body, p_particle_index, p_velocity, p_normal);

	if (!monitor_particle_bodies_entering && !monitor_particles_entering)
		return;

	int data_id = body_contacts.find(ParticleBodyContacts(p_particle_body));
	if (-1 == data_id) {
		data_id = body_contacts.size();
		body_contacts.push_back(ParticleBodyContacts(p_particle_body));
	}

	if (monitor_particles_entering) {
		int p = body_contacts[data_id].particles.find(ParticleContacts(p_particle_index));
		if (-1 == p) {
			body_contacts.write[data_id].particles.push_back(ParticleContacts(p_particle_index));
		} else {
			body_contacts.write[data_id].particles.write[p].stage = 1;
		}
	}

	++body_contacts.write[data_id].particle_count;
}

void ParticlePrimitiveArea::_on_sync() {

	for (int i(body_contacts.size() - 1); 0 <= i; --i) {
		ParticleBodyContacts &body_contact(body_contacts.write[i]);
		if (!body_contact.particle_count) {

			for (int p(body_contact.particles.size() - 1); 0 <= p; --p) {
				emit_signal("particle_exit", body_contact.particle_body, body_contact.particles[p].particle_index);
			}

			emit_signal("body_exit", body_contact.particle_body);

			body_contacts.remove(i);
			continue;
		}

		if (body_contact.just_entered) {

			body_contact.just_entered = false;
			emit_signal("body_enter", body_contact.particle_body);
		}

		for (int p(body_contact.particles.size() - 1); 0 <= p; --p) {
			ParticleContacts &particle_contact = body_contact.particles.write[p];
			if (2 == particle_contact.stage) {

				emit_signal("particle_exit", body_contact.particle_body, particle_contact.particle_index);
				body_contact.particles.remove(p);
				continue;

			} else if (0 == particle_contact.stage) {

				emit_signal("particle_enter", body_contact.particle_body, particle_contact.particle_index);
			}

			particle_contact.stage = 2;
		}

		// Reset
		body_contact.particle_count = 0;
	}

	ParticlePrimitiveBody::_on_sync();
}
