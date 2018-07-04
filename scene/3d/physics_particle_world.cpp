/*************************************************************************/
/*  physics_particle_world.cpp                                           */
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

#include "physics_particle_world.h"
#include "core/map.h"
#include "scene/main/viewport.h"
#include "servers/particle_physics_server.h"

void PhysicsParticleWorld::_get_property_list(List<PropertyInfo> *p_list) const {
	for (Map<StringName, Variant>::Element *e = data.front(); e; e = e->next()) {
		p_list->push_back(PropertyInfo(e->get().get_type(), e->key()));
	}
}

bool PhysicsParticleWorld::_set(const StringName &p_name, const Variant &p_property) {

	if (!data.has(p_name))
		return false;

	data[p_name] = p_property;

	if (particle_space.is_valid())
		return ParticlePhysicsServer::get_singleton()->space_set_param(particle_space, p_name, p_property);
	else
		return true;
}

bool PhysicsParticleWorld::_get(const StringName &p_name, Variant &r_property) const {
	if (data.has(p_name)) {
		r_property = data[p_name];
		return true;
	} else {
		return false;
	}
}

void PhysicsParticleWorld::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			particle_space = get_viewport()->find_world()->get_particle_space();
			reload_data();
			break;
		case NOTIFICATION_EXIT_TREE:
			particle_space = RID();
			break;
	}
}

PhysicsParticleWorld::PhysicsParticleWorld() :
		Node() {
	ParticlePhysicsServer::get_singleton()->space_get_params_defaults(&data);
}

void PhysicsParticleWorld::reload_data() {
	if (!particle_space.is_valid())
		return;

	for (Map<StringName, Variant>::Element *e = data.front(); e; e = e->next()) {
		ParticlePhysicsServer::get_singleton()->space_set_param(particle_space, e->key(), e->get());
	}
}
