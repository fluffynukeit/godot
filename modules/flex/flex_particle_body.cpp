/*************************************************************************/
/*  flex_particle_body.cpp                                               */
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
    @author AndreaCatania
*/

#include "flex_particle_body.h"
#include "flex_memory.h"
#include "flex_space.h"
#include "object.h"

FlexParticleBody::FlexParticleBody() :
		RIDFlex(),
		space(NULL),
		particles_mchunk(NULL),
		springs_mchunk(NULL),
		changed_parameters(0),
		group(1) {
	sync_callback.receiver = NULL;
}

void FlexParticleBody::set_callback(ParticlePhysicsServer::ParticleBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method) {

	if (p_receiver) {
		ERR_FAIL_COND(!p_receiver->has_method(p_method));
	}

	switch (p_callback_type) {
		case ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SYNC:
			sync_callback.receiver = p_receiver;
			sync_callback.method = p_method;
			break;
		case ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED:
			particle_index_changed_callback.receiver = p_receiver;
			particle_index_changed_callback.method = p_method;
			break;
		case ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED:
			spring_index_changed_callback.receiver = p_receiver;
			spring_index_changed_callback.method = p_method;
			break;
	}
}

void FlexParticleBody::set_collision_group(uint32_t p_group) {
	group = eNvFlexPhaseGroupMask & p_group;
	changed_parameters |= eChangedBodyParameterGroup;
}

uint32_t FlexParticleBody::get_collision_group() const {
	return group;
}

void FlexParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {
	delayed_commands.particle_to_add.push_back(ParticleToAdd(p_local_position, p_mass));
	changed_parameters |= eChangedBodyParameterPositionMass | eChangedBodyParameterVelocity | eChangedBodyParameterGroup;
}

void FlexParticleBody::remove_particle(ParticleIndex p_particle) {
	ERR_FAIL_COND(!is_owner_of_particle(p_particle));
	delayed_commands.particle_to_remove.insert(p_particle);
}

void FlexParticleBody::add_spring(ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness) {
	delayed_commands.springs_to_add.push_back(SpringToAdd(p_particle_0, p_particle_1, p_length, p_stiffness));
}

void FlexParticleBody::remove_spring(SpringIndex p_spring_index) {
	ERR_FAIL_COND(!is_owner_of_spring(p_spring_index));
	delayed_commands.springs_to_remove.insert(p_spring_index);
}

int FlexParticleBody::get_particle_count() const {
	return particles_mchunk ? particles_mchunk->get_size() : 0;
}

int FlexParticleBody::get_spring_count() const {
	return springs_mchunk ? springs_mchunk->get_size() : 0;
}

void FlexParticleBody::load_model(Ref<ParticleBodyModel> p_model, const Transform &initial_transform) {

	{ // Particle
		int active_p_count(get_particle_count());
		const int resource_p_count(p_model.is_null() ? 0 : p_model->get_particles_ref().size());

		if (active_p_count > resource_p_count) {

			// Remove last
			const int dif = active_p_count - resource_p_count;
			for (int i(0); i < dif; ++i) {
				remove_particle(active_p_count - i - 1);
			}

			active_p_count = resource_p_count;

		} else {

			// Add
			const int dif = resource_p_count - active_p_count;
			for (int i(0); i < dif; ++i) {
				const int p(resource_p_count - i - 1);
				add_particle(initial_transform.xform(p_model->get_particles_ref().get(p)), p_model->get_masses_ref().get(p));
			}
		}

		for (int i(0); i < active_p_count; ++i) {
			reset_particle(i, initial_transform.xform(p_model->get_particles_ref().get(i)), p_model->get_masses_ref().get(i));
		}
	}

	{ // Spring
		int active_s_count(get_spring_count());
		int resource_s_count(p_model.is_null() ? 0 : p_model->get_constraints_indexes_ref().size() / 2);

		if (active_s_count > resource_s_count) {

			// Remove last
			const int dif = active_s_count - resource_s_count;
			for (int i(0); i < dif; ++i) {
				remove_spring(active_s_count - i - 1);
			}

			active_s_count = resource_s_count;

		} else {

			// Add
			const int dif = resource_s_count - active_s_count;
			for (int i(0); i < dif; ++i) {
				const int s(resource_s_count - i - 1);
				add_spring(p_model->get_constraints_indexes_ref().get(s), p_model->get_constraints_indexes_ref().get(s + 1), p_model->get_constraints_info_ref().get(s).x, p_model->get_constraints_info_ref().get(s).y);
			}
		}

		for (int i(0); i < active_s_count; ++i) {
			reset_spring(i,
					p_model->get_constraints_indexes_ref().get(i),
					p_model->get_constraints_indexes_ref().get(i + 1),
					p_model->get_constraints_info_ref().get(i).x,
					p_model->get_constraints_info_ref().get(i).y);
		}
	}
}

void FlexParticleBody::reset_particle(ParticleIndex p_particle_index, const Vector3 &p_position, real_t p_mass) {
	if (!particles_mchunk)
		return;
	space->get_particle_bodies_memory()->set_particle(particles_mchunk, p_particle_index, CreateParticle(p_position, p_mass));
	space->get_particle_bodies_memory()->set_velocity(particles_mchunk, p_particle_index, Vector3(0, 0, 0));
	changed_parameters |= eChangedBodyParameterPositionMass | eChangedBodyParameterVelocity;
}

void FlexParticleBody::reset_spring(SpringIndex p_spring, ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness) {
	if (!springs_mchunk)
		return;
	space->get_springs_memory()->set_spring(springs_mchunk, p_spring, Spring(particles_mchunk->get_buffer_index(p_particle_0), particles_mchunk->get_buffer_index(p_particle_1)));
	space->get_springs_memory()->set_length(springs_mchunk, p_spring, p_length);
	space->get_springs_memory()->set_stiffness(springs_mchunk, p_spring, p_stiffness);
}

Vector3 FlexParticleBody::get_particle_position(ParticleIndex p_particle_index) const {
	if (!particles_mchunk)
		return return_err_vec3;
	const FlVector4 &p(space->get_particle_bodies_memory()->get_particle(particles_mchunk, p_particle_index));
	return extract_position(p);
}

const Vector3 &FlexParticleBody::get_particle_velocity(ParticleIndex p_particle_index) const {
	if (!particles_mchunk)
		return return_err_vec3;
	return space->get_particle_bodies_memory()->get_velocity(particles_mchunk, p_particle_index);
}

void FlexParticleBody::set_particle_velocity(ParticleIndex p_particle_index, const Vector3 &p_velocity) {
	if (!particles_mchunk)
		return;
	space->get_particle_bodies_memory()->set_velocity(particles_mchunk, p_particle_index, p_velocity);
	changed_parameters |= eChangedBodyParameterVelocity;
}

bool FlexParticleBody::is_owner_of_particle(ParticleIndex p_particle) const {
	if (!particles_mchunk)
		return false;
	return (particles_mchunk && (particles_mchunk->get_buffer_index(p_particle)) <= particles_mchunk->get_end_index());
}

bool FlexParticleBody::is_owner_of_spring(SpringIndex p_spring) const {
	if (!springs_mchunk)
		return false;
	return (springs_mchunk && (springs_mchunk->get_buffer_index(p_spring)) <= springs_mchunk->get_end_index());
}

void FlexParticleBody::set_clean() {
	changed_parameters = 0;
}

void FlexParticleBody::dispatch_sync_callback() {
	if (!sync_callback.receiver)
		return;
	static Variant::CallError error;
	const Variant *p = FlexParticlePhysicsServer::singleton->get_particle_body_commands_variant(this);
	sync_callback.receiver->call(sync_callback.method, &p, 1, error);
}

void FlexParticleBody::particle_index_changed(ParticleIndex p_old_particle_index, ParticleIndex p_new_particle_index) {
	if (!particle_index_changed_callback.receiver)
		return;
	particle_index_changed_callback.receiver->call(particle_index_changed_callback.method, p_old_particle_index, p_new_particle_index);
}

void FlexParticleBody::spring_index_changed(SpringIndex p_old_spring_index, SpringIndex p_new_spring_index) {
	if (!spring_index_changed_callback.receiver)
		return;
	spring_index_changed_callback.receiver->call(spring_index_changed_callback.method, p_old_spring_index, p_new_spring_index);
}

void FlexParticleBody::clear_commands() {
	delayed_commands.particle_to_add.clear();
	delayed_commands.springs_to_add.clear();
	delayed_commands.particle_to_remove.clear();
	delayed_commands.springs_to_remove.clear();
}
