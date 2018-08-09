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

#ifndef PHYSICS_PARTICLE_GLUE_H
#define PHYSICS_PARTICLE_GLUE_H

#include "spatial.h"

#include "physics_particle_body.h"

class PhysicsParticleGlue : public Spatial {
	GDCLASS(PhysicsParticleGlue, Spatial);

	struct GluedParticleData {

		enum GluedParticleState {
			GLUED_PARTICLE_STATE_IN,
			GLUED_PARTICLE_STATE_OUT,
			GLUED_PARTICLE_STATE_IDLE
		};

		int state;
		real_t previous_mass;

		GluedParticleData() :
				state(GLUED_PARTICLE_STATE_IN) {}

		GluedParticleData(const GluedParticleData &p_other) :
				state(p_other.state),
				offset(p_other.offset),
				previous_mass(p_other.previous_mass) {}
	};

	NodePath particle_body_path;
	ParticleBody *particle_body;

	Vector<int> glued_particles;
	Vector<Vector3> glued_particles_offsets;
	Vector<GluedParticleData> glued_particles_data;
	bool allow_particles_with_zero_mass;
	real_t pull_force;
	bool _are_particles_dirty;

	static void _bind_methods();

	void _notification(int p_what);

public:
	PhysicsParticleGlue();

	void set_body_path(const NodePath &p_path);
	NodePath get_body_path() const;

	void set_glued_particles(Vector<int> p_particles);
	Vector<int> get_glued_particles() const;

	void set_glued_particles_offsets(Vector<Vector3> p_particles);
	Vector<Vector3> get_glued_particles_offsets() const;

	void set_allow_particles_with_zero_mass(bool p_allow);
	bool get_allow_particles_with_zero_mass() const;

	void set_pull_force(real_t p_force);
	real_t get_pull_force() const;

	int get_particle_count() const;
	int find_particle(int p_particle_index);

	void add_particle(int p_particle_index);
	void remove_particle(int p_position);
	int get_particle_index(int p_position);

private:
	void particle_physics_sync(RID p_space);
	void pull(int p_particle, const GluedParticleData &p_glued_particle, ParticleBodyCommands *p_cmds);
};

#endif // PHYSICS_PARTICLE_GLUE_H
