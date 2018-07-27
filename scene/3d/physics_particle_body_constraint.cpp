/*************************************************************************/
/*  physics_particle_body_constraint.h.h                                 */
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

#include "physics_particle_body_constraint.h"

#include "core/engine.h"
#include "physics_particle_body.h"
#include "scene/main/viewport.h"
#include "servers/particle_physics_server.h"

void ParticleBodyConstraint::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_particle_body0_path", "path"), &ParticleBodyConstraint::set_particle_body0_path);
	ClassDB::bind_method(D_METHOD("get_particle_body0_path"), &ParticleBodyConstraint::get_particle_body0_path);

	ClassDB::bind_method(D_METHOD("get_particle_body0"), &ParticleBodyConstraint::get_particle_body0);

	ClassDB::bind_method(D_METHOD("set_particle_body1_path", "path"), &ParticleBodyConstraint::set_particle_body1_path);
	ClassDB::bind_method(D_METHOD("get_particle_body1_path"), &ParticleBodyConstraint::get_particle_body1_path);

	ClassDB::bind_method(D_METHOD("get_particle_body1"), &ParticleBodyConstraint::get_particle_body1);

	ClassDB::bind_method(D_METHOD("on_sync", "cmds"), &ParticleBodyConstraint::on_sync);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "particle_body0_path"), "set_particle_body0_path", "get_particle_body0_path");
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "particle_body1_path"), "set_particle_body1_path", "get_particle_body1_path");
}

bool ParticleBodyConstraint::_set(const StringName &p_name, const Variant &p_property) {
	if (p_name == "constraint_count") {
		constraints.resize(p_property);
		return true;
	}

	String name = p_name;
	String pos_s = name.get_slicec('/', 1);
	int pos = pos_s.to_int();

	if (constraints.size() <= pos)
		return false;

	String what = name.get_slicec('/', 2);
	if ("body0_particle_index" == what) {

		constraints[pos].body0_particle_index = p_property;
	} else if ("body1_particle_index" == what) {

		constraints[pos].body1_particle_index = p_property;
	} else if ("constraint_length" == what) {

		constraints[pos].constraint_length = p_property;
	} else if ("constraint_stiffness" == what) {

		constraints[pos].constraint_stiffness = p_property;
	} else {
		return false;
	}

	return true;
}

bool ParticleBodyConstraint::_get(const StringName &p_name, Variant &r_property) const {
	if (p_name == "constraint_count") {
		r_property = constraints.size();
		return true;
	}

	String name = p_name;
	String pos_s = name.get_slicec('/', 1);
	int pos = pos_s.to_int();

	if (constraints.size() <= pos)
		return false;

	String what = name.get_slicec('/', 2);
	if ("body0_particle_index" == what) {

		r_property = constraints[pos].body0_particle_index;
	} else if ("body1_particle_index" == what) {

		r_property = constraints[pos].body1_particle_index;
	} else if ("constraint_length" == what) {

		r_property = constraints[pos].constraint_length;
	} else if ("constraint_stiffness" == what) {

		r_property = constraints[pos].constraint_stiffness;
	} else {
		return false;
	}

	return true;
}

void ParticleBodyConstraint::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::INT, "constraint_count"));

	for (int p(0); p < constraints.size(); ++p) {
		p_list->push_back(PropertyInfo(Variant::INT, "constraints/" + itos(p) + "/body0_particle_index"));
		p_list->push_back(PropertyInfo(Variant::INT, "constraints/" + itos(p) + "/body1_particle_index"));
		p_list->push_back(PropertyInfo(Variant::REAL, "constraints/" + itos(p) + "/constraint_length"));
		p_list->push_back(PropertyInfo(Variant::REAL, "constraints/" + itos(p) + "/constraint_stiffness"));
	}
}

void ParticleBodyConstraint::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_reload();
			break;
		case NOTIFICATION_EXIT_TREE:
			_destroy();
			break;
	}
}

ParticleBodyConstraint::ParticleBodyConstraint() :
		Node(),
		particle_body0(NULL),
		particle_body1(NULL) {
}

void ParticleBodyConstraint::set_particle_body0_path(NodePath p_path) {
	particle_body0_path = p_path;
	_reload();
}

NodePath ParticleBodyConstraint::get_particle_body0_path() const {
	return particle_body0_path;
}

ParticleBody *ParticleBodyConstraint::get_particle_body0() const {
	return particle_body0;
}

void ParticleBodyConstraint::set_particle_body1_path(NodePath p_path) {
	particle_body1_path = p_path;
	_reload();
}

NodePath ParticleBodyConstraint::get_particle_body1_path() const {
	return particle_body1_path;
}

ParticleBody *ParticleBodyConstraint::get_particle_body1() const {
	return particle_body0;
}

void ParticleBodyConstraint::_reload() {

	if (Engine::get_singleton()->is_editor_hint())
		return;

	bool body_changed = false;

	if (is_inside_tree() && !particle_body0_path.is_empty()) {
		ParticleBody *pb = cast_to<ParticleBody>(get_node(particle_body0_path));
		if (pb != particle_body0) {
			particle_body0 = pb;
			body_changed = true;
		}
	} else
		particle_body0 = NULL;

	if (is_inside_tree() && !particle_body1_path.is_empty()) {
		ParticleBody *pb = cast_to<ParticleBody>(get_node(particle_body1_path));
		if (pb != particle_body1) {
			particle_body1 = pb;
			body_changed = true;
		}
	} else
		particle_body1 = NULL;

	if (body_changed)
		_destroy();

	_create();
}

void ParticleBodyConstraint::_create() {

	if (!particle_body0 || !particle_body1)
		return;

	if (rid == RID())
		rid = ParticlePhysicsServer::get_singleton()->constraint_create(particle_body0->get_rid(), particle_body1->get_rid());

	if (!is_inside_tree())
		return;

	ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, this, "on_sync");
	ParticlePhysicsServer::get_singleton()->constraint_set_space(rid, get_viewport()->find_world()->get_particle_space());
}

void ParticleBodyConstraint::_destroy() {

	if (rid != RID()) {
		ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, NULL, "");
		ParticlePhysicsServer::get_singleton()->constraint_set_space(rid, RID());
		ParticlePhysicsServer::get_singleton()->free(rid);
		rid = RID();
	}
}

void ParticleBodyConstraint::on_sync(Object *p_cmds) {
	ParticleBodyConstraintCommands *cmds(static_cast<ParticleBodyConstraintCommands *>(p_cmds));

	print_line("execution");
	//cmds->
}
