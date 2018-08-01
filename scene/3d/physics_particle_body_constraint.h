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

#ifndef PHYSICS_PARTICLE_BODY_CONSTRAINT_H
#define PHYSICS_PARTICLE_BODY_CONSTRAINT_H

#include "scene/main/node.h"

class ParticleBody;

class ParticleBodyConstraint : public Node {
	GDCLASS(ParticleBodyConstraint, Node);

	enum ConstraintState {
		CONSTRAINT_STATE_IN,
		CONSTRAINT_STATE_IDLE,
		CONSTRAINT_STATE_OUT,
		CONSTRAINT_STATE_CHANGED
	};

	struct Constraint {

		Constraint();

		int body0_particle_index;
		int body1_particle_index;
		real_t length;
		real_t stiffness;
		bool created;
		ConstraintState state;
	};

	RID rid;

	NodePath particle_body0_path;
	ParticleBody *particle_body0;
	NodePath particle_body1_path;
	ParticleBody *particle_body1;

	Vector<Constraint> constraints;

	bool _set(const StringName &p_name, const Variant &p_property);
	bool _get(const StringName &p_name, Variant &r_property) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();
	void _notification(int p_what);

public:
	ParticleBodyConstraint();

	void set_particle_body0_path(NodePath p_path);
	NodePath get_particle_body0_path() const;
	ParticleBody *get_particle_body0() const;

	void set_particle_body1_path(NodePath p_path);
	NodePath get_particle_body1_path() const;
	ParticleBody *get_particle_body1() const;

	int get_constraint_count() const;

	void add_constraint(int p_body0_particle_index, int p_body1_particle_index, real_t p_length, real_t p_stiffness);

	int find_constraint(int p_body0_particle_index, int p_body1_particle_index);

	void remove_constraint(int p_index);

	void set_constraint_length(int p_index, real_t p_length);
	real_t get_constraint_length(int p_index) const;

	void set_constraint_stiffness(int p_index, real_t p_stiffness);
	real_t get_constraint_stiffness(int p_index) const;

private:
	void _reload();
	void _create();
	void _destroy();
	void addition();
	void on_sync(Object *p_cmds);
};

#endif // PHYSICS_PARTICLE_BODY_CONSTRAINT_H
