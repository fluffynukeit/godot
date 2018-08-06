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

#include "scene/main/viewport.h"
#include "servers/particle_physics_server.h"

bool PhysicsParticleGlue::_set(const StringName &p_name, const Variant &p_property) {
	if (p_name == "particle_count") {
		glued_particles.resize(p_property);
		return true;
	}

	String name = p_name;
	String pos_s = name.get_slicec('/', 1);
	int pos = pos_s.to_int();

	if (glued_particles.size() <= pos)
		return false;

	String what = name.get_slicec('/', 2);
	if ("body_path" == what) {

		glued_particles.write[pos].particle_body_path = p_property;
	} else if ("particle" == what) {

		glued_particles.write[pos].particle_index = p_property;
	} else {
		return false;
	}

	_are_particles_dirty = true;
	return true;
}

bool PhysicsParticleGlue::_get(const StringName &p_name, Variant &r_property) const {
	if (p_name == "particle_count") {
		r_property = glued_particles.size();
		return true;
	}

	String name = p_name;
	String pos_s = name.get_slicec('/', 1);
	int pos = pos_s.to_int();

	if (glued_particles.size() <= pos)
		return false;

	String what = name.get_slicec('/', 2);
	if ("body_path" == what) {

		r_property = glued_particles[pos].particle_body_path;
	} else if ("particle" == what) {

		r_property = glued_particles[pos].particle_index;
	} else {
		return false;
	}

	return true;
}

void PhysicsParticleGlue::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::INT, "particle_count"));

	for (int p(0); p < glued_particles.size(); ++p) {
		p_list->push_back(PropertyInfo(Variant::NODE_PATH, "glued/" + itos(p) + "/body_path"));
		p_list->push_back(PropertyInfo(Variant::INT, "glued/" + itos(p) + "/particle"));
	}
}

void PhysicsParticleGlue::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_allow_particles_with_zero_mass", "allow"), &PhysicsParticleGlue::set_allow_particles_with_zero_mass);
	ClassDB::bind_method(D_METHOD("get_allow_particles_with_zero_mass"), &PhysicsParticleGlue::get_allow_particles_with_zero_mass);

	ClassDB::bind_method(D_METHOD("get_particle_count"), &PhysicsParticleGlue::get_particle_count);
	ClassDB::bind_method(D_METHOD("find_particle", "particle_index", "particle_body"), &PhysicsParticleGlue::find_particle);
	ClassDB::bind_method(D_METHOD("add_particle", "particle_index", "particle_body"), &PhysicsParticleGlue::add_particle);
	ClassDB::bind_method(D_METHOD("remove_particle", "position"), &PhysicsParticleGlue::remove_particle);
	ClassDB::bind_method(D_METHOD("get_particle_index", "position"), &PhysicsParticleGlue::get_particle_index);

	ClassDB::bind_method(D_METHOD("particle_physics_sync", "space"), &PhysicsParticleGlue::particle_physics_sync);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_particles_with_zero_mass"), "set_allow_particles_with_zero_mass", "get_allow_particles_with_zero_mass");
}

void PhysicsParticleGlue::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			ParticlePhysicsServer::get_singleton()->connect("sync_end", this, "particle_physics_sync");
			break;
		case NOTIFICATION_EXIT_TREE:
			ParticlePhysicsServer::get_singleton()->disconnect("sync_end", this, "particle_physics_sync");
			break;
		case NOTIFICATION_TRANSFORM_CHANGED:
			_are_particles_dirty = true;
			break;
	}
}

PhysicsParticleGlue::PhysicsParticleGlue() :
		Spatial(),
		allow_particles_with_zero_mass(false) {
	set_notify_transform(true);
}

void PhysicsParticleGlue::set_allow_particles_with_zero_mass(bool p_allow) {
	allow_particles_with_zero_mass = p_allow;
}

bool PhysicsParticleGlue::get_allow_particles_with_zero_mass() const {
	return allow_particles_with_zero_mass;
}

int PhysicsParticleGlue::get_particle_count() const {
	return glued_particles.size();
}

int PhysicsParticleGlue::find_particle(int p_particle_index, Object *p_particle_body) {
	return glued_particles.find(GluedParticle(cast_to<ParticleBody>(p_particle_body), p_particle_index));
}

void PhysicsParticleGlue::add_particle(int p_particle_index, Object *p_particle_body) {
	int i = find_particle(p_particle_index, p_particle_body);
	if (i < 0)
		glued_particles.push_back(GluedParticle(cast_to<ParticleBody>(p_particle_body), p_particle_index));
	_are_particles_dirty = true;
}

void PhysicsParticleGlue::remove_particle(int p_position) {
	glued_particles.write[p_position].state = GluedParticle::GLUED_PARTICLE_STATE_OUT;
	_are_particles_dirty = true;
}

int PhysicsParticleGlue::get_particle_index(int p_position) {
	return glued_particles[p_position].particle_index;
}

void PhysicsParticleGlue::particle_physics_sync(RID p_space) {

	if (get_world()->get_particle_space() != p_space)
		return;

	if (!_are_particles_dirty)
		return;

	_are_particles_dirty = false;

	ParticleBodyCommands *cmds;
	int size(glued_particles.size());
	for (int i(size - 1); 0 <= i; --i) {

		GluedParticle &gp = glued_particles.write[i];

		if (gp.state == GluedParticle::GLUED_PARTICLE_STATE_OUT) {

			cmds = ParticlePhysicsServer::get_singleton()->body_get_commands(glued_particles[i].particle_body->get_rid());
			cmds->set_particle_mass(gp.particle_index, gp.previous_mass);
			glued_particles.write[i] = glued_particles[--size];
			continue;

		} else if (gp.state == GluedParticle::GLUED_PARTICLE_STATE_IN) {

			if (!gp.particle_body) {
				gp.particle_body = cast_to<ParticleBody>(get_node(gp.particle_body_path));
			}
			ERR_FAIL_COND(!gp.particle_body);

			cmds = ParticlePhysicsServer::get_singleton()->body_get_commands(glued_particles[i].particle_body->get_rid());
			gp.state = GluedParticle::GLUED_PARTICLE_STATE_IDLE;
			gp.previous_mass = cmds->get_particle_mass(gp.particle_index);
			if (!allow_particles_with_zero_mass && !gp.previous_mass) {
				glued_particles.write[i] = glued_particles[--size];
				continue;
			}
			gp.offset = get_global_transform().xform_inv(cmds->get_particle_position(gp.particle_index));

			cmds->set_particle_position_mass(gp.particle_index, get_global_transform().xform(gp.offset), .0);
		} else {

			cmds = ParticlePhysicsServer::get_singleton()->body_get_commands(glued_particles[i].particle_body->get_rid());
			cmds->set_particle_position(gp.particle_index, get_global_transform().xform(gp.offset));
		}
	}
	glued_particles.resize(size);
}
