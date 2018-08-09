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

	struct GluedParticle {

		enum GluedParticleState {
			GLUED_PARTICLE_STATE_IN,
			GLUED_PARTICLE_STATE_OUT,
			GLUED_PARTICLE_STATE_IDLE
		};

		NodePath particle_body_path;
		ParticleBody *particle_body;
		int particle_index;
		int state;
		Vector3 offset;
		real_t previous_mass;

		GluedParticle(ParticleBody *p_particle_body = NULL, int p_particle_index = -1) :
				particle_body(p_particle_body),
				particle_index(p_particle_index),
				state(GLUED_PARTICLE_STATE_IN) {}

		GluedParticle(const GluedParticle &p_other) :
				particle_body_path(p_other.particle_body_path),
				particle_body(p_other.particle_body),
				particle_index(p_other.particle_index),
				state(p_other.particle_index),
				offset(p_other.offset),
				previous_mass(p_other.previous_mass) {}

		bool operator==(const GluedParticle &p_other) const {
			return p_other.particle_body == particle_body && p_other.particle_index == particle_index;
		}
	};

	Vector<GluedParticle> glued_particles;
	bool allow_particles_with_zero_mass;
	real_t pull_force;
	bool _are_particles_dirty;

	bool _set(const StringName &p_name, const Variant &p_property);
	bool _get(const StringName &p_name, Variant &r_property) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;

	static void _bind_methods();

	void _notification(int p_what);

public:
	PhysicsParticleGlue();

	void set_allow_particles_with_zero_mass(bool p_allow);
	bool get_allow_particles_with_zero_mass() const;

	void set_pull_force(real_t p_force);
	real_t get_pull_force() const;

	int get_particle_count() const;
	int find_particle(int p_particle_index, Object *p_particle_body);

	void add_particle(int p_particle_index, Object *p_particle_body);
	void remove_particle(int p_position);
	int get_particle_index(int p_position);

private:
	void particle_physics_sync(RID p_space);
	void pull(const GluedParticle &p_glued_particle, ParticleBodyCommands *p_cmds);
};

#endif // PHYSICS_PARTICLE_GLUE_H
