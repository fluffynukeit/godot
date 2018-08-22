/*************************************************************************/
/*  physics_particle_glue.cpp                                            */
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

#include "physics_particle_glue.h"

#include "core/core_string_names.h"
#include "core/engine.h"
#include "scene/main/viewport.h"
#include "servers/particle_physics_server.h"

void PhysicsParticleGlue::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_body_path", "path"), &PhysicsParticleGlue::set_body_path);
	ClassDB::bind_method(D_METHOD("get_body_path"), &PhysicsParticleGlue::get_body_path);

	ClassDB::bind_method(D_METHOD("set_glued_particles", "glued_particles"), &PhysicsParticleGlue::set_glued_particles);
	ClassDB::bind_method(D_METHOD("get_glued_particles"), &PhysicsParticleGlue::get_glued_particles);

	ClassDB::bind_method(D_METHOD("set_automatic_full_body_glue", "enable"), &PhysicsParticleGlue::set_automatic_full_body_glue);
	ClassDB::bind_method(D_METHOD("get_automatic_full_body_glue"), &PhysicsParticleGlue::get_automatic_full_body_glue);

	ClassDB::bind_method(D_METHOD("set_glued_particles_offsets", "offsets"), &PhysicsParticleGlue::set_glued_particles_offsets);
	ClassDB::bind_method(D_METHOD("get_glued_particles_offsets"), &PhysicsParticleGlue::get_glued_particles_offsets);

	ClassDB::bind_method(D_METHOD("set_allow_particles_with_zero_mass", "allow"), &PhysicsParticleGlue::set_allow_particles_with_zero_mass);
	ClassDB::bind_method(D_METHOD("get_allow_particles_with_zero_mass"), &PhysicsParticleGlue::get_allow_particles_with_zero_mass);

	ClassDB::bind_method(D_METHOD("set_pull_force", "force"), &PhysicsParticleGlue::set_pull_force);
	ClassDB::bind_method(D_METHOD("get_pull_force"), &PhysicsParticleGlue::get_pull_force);

	ClassDB::bind_method(D_METHOD("get_particle_count"), &PhysicsParticleGlue::get_particle_count);
	ClassDB::bind_method(D_METHOD("find_particle", "particle_index"), &PhysicsParticleGlue::find_particle);
	ClassDB::bind_method(D_METHOD("add_particle", "particle_index"), &PhysicsParticleGlue::add_particle);
	ClassDB::bind_method(D_METHOD("remove_particle", "position"), &PhysicsParticleGlue::remove_particle);
	ClassDB::bind_method(D_METHOD("get_particle_index", "position"), &PhysicsParticleGlue::get_particle_index);

	ClassDB::bind_method(D_METHOD("particle_physics_sync", "space"), &PhysicsParticleGlue::particle_physics_sync);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "body_path"), "set_body_path", "get_body_path");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "glued_particles"), "set_glued_particles", "get_glued_particles");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "glued_particles_offsets"), "set_glued_particles_offsets", "get_glued_particles_offsets");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_particles_with_zero_mass"), "set_allow_particles_with_zero_mass", "get_allow_particles_with_zero_mass");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "automatic_full_body_glue"), "set_automatic_full_body_glue", "get_automatic_full_body_glue");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "pull_force"), "set_pull_force", "get_pull_force");
}

void PhysicsParticleGlue::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY:
			if (!Engine::get_singleton()->is_editor_hint())
				ParticlePhysicsServer::get_singleton()->connect("sync_end", this, "particle_physics_sync");
			_resolve_particle_body();
			break;
		case NOTIFICATION_EXIT_TREE:
			if (!Engine::get_singleton()->is_editor_hint())
				ParticlePhysicsServer::get_singleton()->disconnect("sync_end", this, "particle_physics_sync");
			_deferred_remove_glued_particles_physics_server();
			break;
		case NOTIFICATION_TRANSFORM_CHANGED:
			_are_particles_dirty = true;
			_compute_offsets();
			break;
	}
}

PhysicsParticleGlue::PhysicsParticleGlue() :
		Spatial(),
		particle_body(NULL),
		allow_particles_with_zero_mass(false),
		automatic_full_body_glue(false),
		pull_force(-1) {
	set_notify_transform(true);
}

PhysicsParticleGlue::~PhysicsParticleGlue() {

	if (!particle_body)
		return;

	_deferred_remove_glued_particles_physics_server();
}

void PhysicsParticleGlue::set_body_path(const NodePath &p_path) {
	particle_body_path = p_path;

	_resolve_particle_body();
}

NodePath PhysicsParticleGlue::get_body_path() const {
	return particle_body_path;
}

void PhysicsParticleGlue::set_glued_particles(Vector<int> p_particles) {
	glued_particles = p_particles;
	glued_particles_offsets.resize(glued_particles.size());
	glued_particles_data.resize(glued_particles.size());
	_compute_offsets();
}

Vector<int> PhysicsParticleGlue::get_glued_particles() const {
	return glued_particles;
}

void PhysicsParticleGlue::set_glued_particles_offsets(Vector<Vector3> p_particles) {
	glued_particles_offsets = p_particles;
}

Vector<Vector3> PhysicsParticleGlue::get_glued_particles_offsets() const {
	return glued_particles_offsets;
}

void PhysicsParticleGlue::set_allow_particles_with_zero_mass(bool p_allow) {
	allow_particles_with_zero_mass = p_allow;
}

bool PhysicsParticleGlue::get_allow_particles_with_zero_mass() const {
	return allow_particles_with_zero_mass;
}

void PhysicsParticleGlue::set_automatic_full_body_glue(bool p_enable) {
	automatic_full_body_glue = p_enable;
}

bool PhysicsParticleGlue::get_automatic_full_body_glue() const {
	return automatic_full_body_glue;
}

void PhysicsParticleGlue::set_pull_force(real_t p_force) {
	pull_force = p_force;
}

real_t PhysicsParticleGlue::get_pull_force() const {
	return pull_force;
}

int PhysicsParticleGlue::get_particle_count() const {
	return glued_particles.size();
}

int PhysicsParticleGlue::find_particle(int p_particle_index) {
	return glued_particles.find(p_particle_index);
}

void PhysicsParticleGlue::add_particle(int p_particle_index) {
	int i = find_particle(p_particle_index);
	if (i < 0) {
		i = glued_particles.size();
		glued_particles.push_back(p_particle_index);
		glued_particles_offsets.resize(i + 1);
		glued_particles_data.resize(i + 1);
		glued_particles_data.write[i].state = GluedParticleData::GLUED_PARTICLE_STATE_IN_RUNTIME;
		_are_particles_dirty = true;
		_compute_offsets();
	}
}

void PhysicsParticleGlue::remove_particle(int p_position) {
	glued_particles_data.write[p_position].state = GluedParticleData::GLUED_PARTICLE_STATE_OUT;
	_are_particles_dirty = true;
}

int PhysicsParticleGlue::get_particle_index(int p_position) {
	return glued_particles[p_position];
}

void PhysicsParticleGlue::particle_physics_sync(RID p_space) {

	if (get_world()->get_particle_space() != p_space)
		return;

	if (!_are_particles_dirty && !_is_body_new)
		return;

	if (0 > pull_force)
		_are_particles_dirty = false;

	ERR_FAIL_COND(!particle_body);

	ParticleBodyCommands *cmds = ParticlePhysicsServer::get_singleton()->body_get_commands(particle_body->get_rid());
	if (!cmds)
		return; // Not yet ready to execute commands

	if (automatic_full_body_glue && _is_body_new) {
		_is_body_new = false;

		const int size(cmds->get_particle_count());
		glued_particles.resize(size);
		glued_particles_offsets.resize(size);
		glued_particles_data.resize(size);

		for (int i(size - 1); 0 <= i; --i) {

			GluedParticleData &gp(glued_particles_data.write[i]);

			glued_particles.write[i] = i;
			glued_particles_offsets.write[i] = get_global_transform().xform_inv(cmds->get_particle_position(i));

			gp.previous_mass = cmds->get_particle_mass(i);
			gp.state = GluedParticleData::GLUED_PARTICLE_STATE_IDLE;
			apply_force(i, glued_particles_offsets[i], cmds);
		}
	} else {
		int size(glued_particles.size());
		glued_particles_offsets.resize(size); // Avoid differences
		glued_particles_data.resize(size); // Avoid differences
		for (int i(size - 1); 0 <= i; --i) {

			GluedParticleData &gp(glued_particles_data.write[i]);

			if (gp.state == GluedParticleData::GLUED_PARTICLE_STATE_IDLE) {

				apply_force(glued_particles[i], glued_particles_offsets[i], cmds);

			} else if (gp.state == GluedParticleData::GLUED_PARTICLE_STATE_OUT) {

				cmds->set_particle_mass(glued_particles[i], gp.previous_mass);
				glued_particles.write[i] = glued_particles[size - 1];
				glued_particles_offsets.write[i] = glued_particles_offsets[size - 1];
				glued_particles_data.write[i] = glued_particles_data[--size];

			} else {

				gp.previous_mass = cmds->get_particle_mass(glued_particles[i]);
				if (!allow_particles_with_zero_mass && !gp.previous_mass) {
					glued_particles.write[i] = glued_particles[size - 1];
					glued_particles_offsets.write[i] = glued_particles_offsets[size - 1];
					glued_particles_data.write[i] = glued_particles_data[--size];
					continue;
				}

				if (gp.state == GluedParticleData::GLUED_PARTICLE_STATE_IN_RUNTIME) {
					glued_particles_offsets.write[i] = get_global_transform().xform_inv(cmds->get_particle_position(glued_particles[i]));
				}

				gp.state = GluedParticleData::GLUED_PARTICLE_STATE_IDLE;

				apply_force(glued_particles[i], glued_particles_offsets[i], cmds);
			}
		}
		glued_particles.resize(size);
		glued_particles_offsets.resize(size);
		glued_particles_data.resize(size);
	}
}

void PhysicsParticleGlue::apply_force(int p_particle, const Vector3 &p_offset, ParticleBodyCommands *p_cmds) {

	if (0 > pull_force) {

		p_cmds->set_particle_position_mass(p_particle, get_global_transform().xform(p_offset), .0);
	} else {

		Vector3 target_position = get_global_transform().xform(p_offset);
		Vector3 particle_pos(p_cmds->get_particle_position(p_particle));

		Vector3 delta(target_position - particle_pos);

		p_cmds->set_particle_velocity(p_particle, delta * pull_force);
	}
}

void PhysicsParticleGlue::_changed_callback(Object *p_changed, const char *p_prop) {
	if (p_changed == particle_body)
		_compute_offsets();
}

void PhysicsParticleGlue::_resolve_particle_body() {

	ParticleBody *new_particle_body(NULL);
	if (!particle_body_path.is_empty() && is_inside_tree())
		new_particle_body = cast_to<ParticleBody>(get_node(particle_body_path));

	if (particle_body != new_particle_body) {
		if (Engine::get_singleton()->is_editor_hint() && particle_body)
			particle_body->remove_change_receptor(this);

		_deferred_remove_glued_particles_physics_server();
		particle_body = new_particle_body;
		_is_body_new = true;

		if (Engine::get_singleton()->is_editor_hint() && particle_body)
			particle_body->add_change_receptor(this);

		_compute_offsets();
	}
}

void PhysicsParticleGlue::_compute_offsets() {

	if (!Engine::get_singleton()->is_editor_hint())
		return;

	if (!particle_body)
		return;

	PoolVector<Vector3>::Read particles_r = particle_body->get_particle_body_model()->get_particles().read();

	for (int i(glued_particles.size() - 1); 0 <= i; --i) {

		glued_particles_offsets.write[i] = get_global_transform().xform_inv(particle_body->get_global_transform().xform(particles_r[glued_particles[i]]));
	}
}

void PhysicsParticleGlue::_deferred_remove_glued_particles_physics_server() {

	if (!particle_body)
		return;

	PhysicsParticleGlueRemoval *r = memnew(PhysicsParticleGlueRemoval);
	r->space = SceneTree::get_singleton()->get_root()->get_world()->get_particle_space();
	r->particle_body = particle_body;
	r->glued_particles = glued_particles;
	r->glued_particles_offsets = glued_particles_offsets;
	r->glued_particles_data = glued_particles_data;

	if (automatic_full_body_glue) {
		glued_particles.clear();
		glued_particles_offsets.clear();
		glued_particles_data.clear();
	} else {
		int size(glued_particles.size());
		for (int i(size - 1); 0 <= i; --i) {

			GluedParticleData &gp(glued_particles_data.write[i]);
			if (Engine::get_singleton()->is_editor_hint())
				gp.state == GluedParticleData::GLUED_PARTICLE_STATE_IN;
			else
				gp.state == GluedParticleData::GLUED_PARTICLE_STATE_IN_RUNTIME;
		}
	}
}

PhysicsParticleGlueRemoval::PhysicsParticleGlueRemoval() {

	ParticlePhysicsServer::get_singleton()->connect("sync_end", this, "on_sync");
}

void PhysicsParticleGlueRemoval::_bind_methods() {
	ClassDB::bind_method(D_METHOD("on_sync", "space"), &PhysicsParticleGlueRemoval::on_sync);
}

void PhysicsParticleGlueRemoval::on_sync(RID p_space) {
	if (space != p_space)
		return;

	ParticleBodyCommands *cmds = ParticlePhysicsServer::get_singleton()->body_get_commands(particle_body->get_rid());

	if (cmds) {
		int size(glued_particles.size());
		for (int i(size - 1); 0 <= i; --i) {

			cmds->set_particle_mass(glued_particles[i], glued_particles_data.write[i].previous_mass);
		}
	}

	ParticlePhysicsServer::get_singleton()->disconnect("sync_end", this, "on_sync");
	memdelete(this);
}
