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

ParticleBodyConstraint::Constraint::Constraint() :
		body0_particle_index(-1),
		body1_particle_index(-1),
		length(1),
		stiffness(0.5),
		index(-1),
		state(ConstraintState::CONSTRAINT_STATE_IN) {}

void ParticleBodyConstraint::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_particle_body0_path", "path"), &ParticleBodyConstraint::set_particle_body0_path);
	ClassDB::bind_method(D_METHOD("get_particle_body0_path"), &ParticleBodyConstraint::get_particle_body0_path);

	ClassDB::bind_method(D_METHOD("get_particle_body0"), &ParticleBodyConstraint::get_particle_body0);

	ClassDB::bind_method(D_METHOD("set_particle_body1_path", "path"), &ParticleBodyConstraint::set_particle_body1_path);
	ClassDB::bind_method(D_METHOD("get_particle_body1_path"), &ParticleBodyConstraint::get_particle_body1_path);

	ClassDB::bind_method(D_METHOD("get_particle_body1"), &ParticleBodyConstraint::get_particle_body1);

	ClassDB::bind_method(D_METHOD("get_constraint_count"), &ParticleBodyConstraint::get_constraint_count);

	ClassDB::bind_method(D_METHOD("add_constraint", "body0_particle_index", "body1_particle_index", "length", "stiffness"), &ParticleBodyConstraint::add_constraint);

	ClassDB::bind_method(D_METHOD("find_constraint", "body0_particle_index", "body1_particle_index"), &ParticleBodyConstraint::find_constraint);

	ClassDB::bind_method(D_METHOD("remove_constraint", "constraint_index"), &ParticleBodyConstraint::remove_constraint);

	ClassDB::bind_method(D_METHOD("set_constraint_length", "constraint_index", "length"), &ParticleBodyConstraint::set_constraint_length);
	ClassDB::bind_method(D_METHOD("get_constraint_length", "constraint_index"), &ParticleBodyConstraint::get_constraint_length);

	ClassDB::bind_method(D_METHOD("set_constraint_stiffness", "constraint_index", "stiffness"), &ParticleBodyConstraint::set_constraint_stiffness);
	ClassDB::bind_method(D_METHOD("get_constraint_stiffness", "constraint_index"), &ParticleBodyConstraint::get_constraint_stiffness);

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

	bool just_created = false;
	if (constraints.size() <= pos) {
		constraints.push_back(Constraint());
		just_created = true;
		if (Engine::get_singleton()->is_editor_hint())
			ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, this, "on_sync");
	}

	ERR_FAIL_INDEX_V(pos, constraints.size(), false);

	String what = name.get_slicec('/', 2);
	if ("body0_particle_index" == what) {

		constraints[pos].body0_particle_index = p_property;
	} else if ("body1_particle_index" == what) {

		constraints[pos].body1_particle_index = p_property;
	} else if ("constraint_length" == what) {

		constraints[pos].length = p_property;
	} else if ("constraint_stiffness" == what) {

		constraints[pos].stiffness = p_property;
	} else {
		return false;
	}

	if (!just_created && CONSTRAINT_STATE_IN != constraints[pos].state) {
		constraints[pos].state = CONSTRAINT_STATE_CHANGED;
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

	ERR_FAIL_INDEX_V(pos, constraints.size(), false);

	String what = name.get_slicec('/', 2);
	if ("body0_particle_index" == what) {

		r_property = constraints[pos].body0_particle_index;
	} else if ("body1_particle_index" == what) {

		r_property = constraints[pos].body1_particle_index;
	} else if ("constraint_length" == what) {

		r_property = constraints[pos].length;
	} else if ("constraint_stiffness" == what) {

		r_property = constraints[pos].stiffness;
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

int ParticleBodyConstraint::get_constraint_count() const {
	return constraints.size();
}

void ParticleBodyConstraint::add_constraint(int p_body0_particle_index, int p_body1_particle_index, real_t p_length, real_t p_stiffness) {

	Constraint c;
	c.body0_particle_index = p_body0_particle_index;
	c.body1_particle_index = p_body1_particle_index;
	c.length = p_length;
	c.stiffness = p_stiffness;

	constraints.push_back(c);

	ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, this, "on_sync");
}

int ParticleBodyConstraint::find_constraint(int p_body0_particle_index, int p_body1_particle_index) {

	for (int i(constraints.size() - 1); 0 <= i; --i) {
		if (p_body0_particle_index != constraints[i].body0_particle_index)
			continue;

		if (p_body1_particle_index != constraints[i].body1_particle_index)
			continue;

		return i;
	}
	return -1;
}

void ParticleBodyConstraint::remove_constraint(int p_index) {
	ERR_FAIL_INDEX(p_index, constraints.size());

	constraints[p_index].state = CONSTRAINT_STATE_OUT;

	ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, this, "on_sync");
}

void ParticleBodyConstraint::set_constraint_length(int p_index, real_t p_length) {
	ERR_FAIL_INDEX(p_index, constraints.size());
	constraints[p_index].length = p_length;
	constraints[p_index].state = CONSTRAINT_STATE_CHANGED;
	ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, this, "on_sync");
}

real_t ParticleBodyConstraint::get_constraint_length(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, constraints.size(), 0.0);
	return constraints[p_index].length;
}

void ParticleBodyConstraint::set_constraint_stiffness(int p_index, real_t p_stiffness) {
	ERR_FAIL_INDEX(p_index, constraints.size());
	constraints[p_index].stiffness = p_stiffness;
	constraints[p_index].state = CONSTRAINT_STATE_CHANGED;
	ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, this, "on_sync");
}

real_t ParticleBodyConstraint::get_constraint_stiffness(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, constraints.size(), 0.0);
	return constraints[p_index].stiffness;
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

	int size(constraints.size());
	for (int i(size - 1); 0 <= i; --i) {

		Constraint &constraint(constraints[i]);

		if (-1 == constraint.body0_particle_index || -1 == constraint.body1_particle_index)
			continue;

		switch (constraint.state) {
			case CONSTRAINT_STATE_IN:
			case CONSTRAINT_STATE_CHANGED: {

				if (constraint.index == -1) {
					constraint.index = cmds->add_spring(constraint.body0_particle_index, constraint.body1_particle_index, constraint.length, constraint.stiffness);
				} else {
					cmds->set_spring(constraint.index, constraint.body0_particle_index, constraint.body1_particle_index, constraint.length, constraint.stiffness);
				}
				constraint.state = CONSTRAINT_STATE_IDLE;

			} break;
			case CONSTRAINT_STATE_IDLE: {
				// Nothing
			} break;
			case CONSTRAINT_STATE_OUT: {
				ParticlePhysicsServer::get_singleton()->constraint_remove_spring(rid, i);
				constraints[i] = constraints[--size]; // This is the same way on how the server remove springs
			} break;
		}
	}

	constraints.resize(size);

	ParticlePhysicsServer::get_singleton()->constraint_set_callback(rid, NULL, "");
}
