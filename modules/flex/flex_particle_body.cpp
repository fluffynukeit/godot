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
		triangles_mchunk(NULL),
		rigids_mchunk(NULL),
		rigids_components_mchunk(NULL),
		changed_parameters(0),
		collision_group(0),
		collision_flags(0),
		collision_primitive_mask(eNvFlexPhaseShapeChannel0) {
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
	if (p_group >= (2 ^ 20))
		return;
	collision_group = p_group;
	changed_parameters |= eChangedBodyParamPhase;
}

uint32_t FlexParticleBody::get_collision_group() const {
	return collision_group;
}

uint32_t FlexParticleBody::get_collision_flag_bit(ParticlePhysicsServer::ParticleCollisionFlag p_flag) const {
	switch (p_flag) {
		case ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE:
			return eNvFlexPhaseSelfCollide;
		case ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE_FILTER:
			return eNvFlexPhaseSelfCollideFilter;
		case ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_FLUID:
			return eNvFlexPhaseFluid;
	}
	return 0;
}

void FlexParticleBody::set_collision_flag(ParticlePhysicsServer::ParticleCollisionFlag p_flag, bool p_active) {

	if (p_active) {
		collision_flags |= get_collision_flag_bit(p_flag);
	} else {
		collision_flags &= ~get_collision_flag_bit(p_flag);
	}
	changed_parameters |= eChangedBodyParamPhase;
}

bool FlexParticleBody::get_collision_flag(ParticlePhysicsServer::ParticleCollisionFlag p_flag) const {
	return collision_flags & get_collision_flag_bit(p_flag);
}

void FlexParticleBody::set_collision_primitive_mask(uint32_t p_primitive_mask) {
	collision_primitive_mask = eNvFlexPhaseShapeChannelMask & (p_primitive_mask << 24);
	changed_parameters |= eChangedBodyParamPhase;
}

uint32_t FlexParticleBody::get_collision_primitive_mask() const {
	return collision_primitive_mask >> 24;
}

void FlexParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {
	delayed_commands.particle_to_add.push_back(ParticleToAdd(p_local_position, p_mass));
	changed_parameters |= eChangedBodyParamPositionMass | eChangedBodyParamVelocity | eChangedBodyParamPhase;
}

void FlexParticleBody::remove_particle(ParticleIndex p_particle) {
	ERR_FAIL_COND(!is_owner_of_particle(p_particle));
	if (-1 == delayed_commands.particle_to_remove.find(p_particle))
		delayed_commands.particle_to_remove.push_back(p_particle);
}

void FlexParticleBody::add_spring(ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness) {
	delayed_commands.springs_to_add.push_back(SpringToAdd(p_particle_0, p_particle_1, p_length, p_stiffness));
}

void FlexParticleBody::remove_spring(SpringIndex p_spring_index) {
	ERR_FAIL_COND(!is_owner_of_spring(p_spring_index));
	delayed_commands.springs_to_remove.insert(p_spring_index);
}

void FlexParticleBody::add_triangle(const DynamicTriangle &p_triangle) {
	delayed_commands.triangles_to_add.push_back(p_triangle);
}

void FlexParticleBody::remove_triangle(const TriangleIndex p_triangle_index) {
	ERR_FAIL_COND(!is_owner_of_triangle(p_triangle_index));
	if (-1 == delayed_commands.triangles_to_remove.find(p_triangle_index))
		delayed_commands.triangles_to_remove.push_back(p_triangle_index);
}

void FlexParticleBody::add_rigid(const Transform &p_global_transform, float p_stiffness, float p_plastic_threshold, float p_plastic_creep, PoolVector<ParticleIndex> p_indices, PoolVector<Vector3> p_rests) {
	delayed_commands.rigids_to_add.push_back(RigidToAdd(p_global_transform, p_stiffness, p_plastic_threshold, p_plastic_creep, p_indices, p_rests));
}

void FlexParticleBody::remove_rigid(RigidIndex p_rigid_index) {
	ERR_FAIL_COND(!is_owner_of_rigid(p_rigid_index));
	if (-1 == delayed_commands.rigids_to_remove.find(p_rigid_index))
		delayed_commands.rigids_to_remove.push_back(p_rigid_index);
}

void FlexParticleBody::remove_rigid_component(RigidComponentIndex p_rigid_component_index) {
	ERR_FAIL_COND(!is_owner_of_rigid_component(p_rigid_component_index));
	if (-1 == delayed_commands.rigids_components_to_remove.find(p_rigid_component_index))
		delayed_commands.rigids_components_to_remove.push_back(p_rigid_component_index);
}

int FlexParticleBody::get_particle_count() const {
	return particles_mchunk ? particles_mchunk->get_size() : 0;
}

int FlexParticleBody::get_spring_count() const {
	return springs_mchunk ? springs_mchunk->get_size() : 0;
}

int FlexParticleBody::get_rigid_count() const {
	return rigids_mchunk ? rigids_mchunk->get_size() : 0;
}

PoolVector<ParticleIndex> extract_rigid_indices(int index, PoolVector<int> p_offsets, PoolVector<int> p_indices) {

	PoolVector<int>::Read offsets_r = p_offsets.read();
	PoolVector<int>::Read indices_r = p_indices.read();

	const int offset_start = index == 0 ? 0 : offsets_r[index - 1];
	const int size = offsets_r[index] - offset_start;

	PoolVector<ParticleIndex> rigid_indices;
	rigid_indices.resize(size);
	PoolVector<ParticleIndex>::Write rigid_indices_w = rigid_indices.write();

	for (int i(0); i < size; i++) {
		rigid_indices_w[i] = indices_r[offset_start + i];
	}

	return rigid_indices;
}

PoolVector<Vector3> extract_rigid_rests(PoolVector<ParticleIndex> p_rigid_indices, PoolVector<Vector3> p_particles_positions, const Vector3 &p_rigid_position) {

	PoolVector<ParticleIndex>::Read rigid_indices_r = p_rigid_indices.read();
	PoolVector<Vector3>::Read positions_r = p_particles_positions.read();

	PoolVector<Vector3> rests;
	rests.resize(p_rigid_indices.size());

	PoolVector<Vector3>::Write rests_w = rests.write();

	for (int i(p_rigid_indices.size() - 1); 0 <= i; --i) {

		rests_w[i] = positions_r[rigid_indices_r[i]] - p_rigid_position;
	}

	return rests;
}

void FlexParticleBody::load_model(Ref<ParticleBodyModel> p_model, const Transform &initial_transform) {

	{ // Particle
		int active_p_count(get_particle_count());
		const int resource_p_count(p_model.is_null() ? 0 : p_model->get_particles_ref().size());

		if (active_p_count > resource_p_count) {

			// Remove last
			const int dif = active_p_count - resource_p_count;
			for (int i(0); i < dif; ++i) {
				remove_particle(resource_p_count + i);
			}

			active_p_count = resource_p_count;

		} else {

			// Add
			const int dif = resource_p_count - active_p_count;
			for (int i(0); i < dif; ++i) {
				const int p(active_p_count + i);
				add_particle(initial_transform.xform(p_model->get_particles_ref().get(p)), p_model->get_masses_ref().get(p));
			}
		}

		for (int i(0); i < active_p_count; ++i) {
			reset_particle(i, initial_transform.xform(p_model->get_particles_ref().get(i)), p_model->get_masses_ref().get(i));
		}
	}

	{ // Spring
		uint32_t active_s_count(get_spring_count());
		int resource_s_count(p_model.is_null() ? 0 : p_model->get_constraints_indexes_ref().size() / 2);

		if (active_s_count > resource_s_count) {

			// Remove last
			const int dif = active_s_count - resource_s_count;
			for (int i(0); i < dif; ++i) {
				remove_spring(resource_s_count + i);
			}

			active_s_count = resource_s_count;

		} else {

			// Add
			const int dif = resource_s_count - active_s_count;
			for (int i(0); i < dif; ++i) {
				const int s(active_s_count + i);
				add_spring(p_model->get_constraints_indexes_ref().get(s * 2), p_model->get_constraints_indexes_ref().get(s * 2 + 1), p_model->get_constraints_info_ref().get(s).x, p_model->get_constraints_info_ref().get(s).y);
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

	{
		// Dynamic triangles
		// TODO load dynamic triangle here
	}

	{ // Rigids
		// Remove all current rigids
		for (int i(0); i < get_rigid_count(); ++i) {
			remove_rigid(i);
		}

		// Add all model rigids

		const int resource_r_count(p_model->get_clusters_offsets().size());

		PoolVector<Vector3>::Read cluster_pos_r = p_model->get_clusters_positions().read();
		PoolVector<float>::Read cluster_stiffness_r = p_model->get_clusters_stiffness().read();
		PoolVector<float>::Read cluster_plastic_threshold_r = p_model->get_clusters_plastic_threshold().read();
		PoolVector<float>::Read cluster_plastic_creep_r = p_model->get_clusters_plastic_creep().read();

		for (int i(0); i < resource_r_count; ++i) {
			PoolVector<ParticleIndex> indices = extract_rigid_indices(i, p_model->get_clusters_offsets(), p_model->get_clusters_particle_indices());
			add_rigid(initial_transform.translated(cluster_pos_r[i]), cluster_stiffness_r[i], cluster_plastic_threshold_r[i], cluster_plastic_creep_r[i], indices, extract_rigid_rests(indices, p_model->get_particles(), cluster_pos_r[i]));
		}
	}
}

void FlexParticleBody::reset_particle(ParticleIndex p_particle_index, const Vector3 &p_position, real_t p_mass) {
	if (!particles_mchunk)
		return;
	space->get_particles_memory()->set_particle(particles_mchunk, p_particle_index, make_particle(p_position, p_mass));
	space->get_particles_memory()->set_velocity(particles_mchunk, p_particle_index, Vector3(0, 0, 0));
	changed_parameters |= eChangedBodyParamPositionMass | eChangedBodyParamVelocity;
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
	const FlVector4 &p(space->get_particles_memory()->get_particle(particles_mchunk, p_particle_index));
	return extract_position(p);
}

const Vector3 &FlexParticleBody::get_particle_velocity(ParticleIndex p_particle_index) const {
	if (!particles_mchunk)
		return return_err_vec3;
	return space->get_particles_memory()->get_velocity(particles_mchunk, p_particle_index);
}

void FlexParticleBody::set_particle_velocity(ParticleIndex p_particle_index, const Vector3 &p_velocity) {
	if (!particles_mchunk)
		return;
	space->get_particles_memory()->set_velocity(particles_mchunk, p_particle_index, p_velocity);
	changed_parameters |= eChangedBodyParamVelocity;
}

const Vector3 &FlexParticleBody::get_rigid_position(RigidIndex p_rigid_index) const {
	if (!rigids_mchunk)
		return return_err_vec3;
	return space->get_rigids_memory()->get_position(rigids_mchunk, p_rigid_index);
}

const Quat &FlexParticleBody::get_rigid_rotation(RigidIndex p_rigid_index) const {
	if (!rigids_mchunk)
		return return_err_quat;
	return space->get_rigids_memory()->get_rotation(rigids_mchunk, p_rigid_index);
}

void FlexParticleBody::reload_rigids_COM() {
	for (int i(rigids_mchunk->get_size() - 1); 0 <= i; --i) {
		reload_rigid_COM(i);
	}
}

void FlexParticleBody::reload_rigid_COM(RigidIndex p_rigid) {

	// TODO This class calculates the new COM from current position of particle
	// and this is not correct because I will set a deformed rest and not the correct one
	// Improve this class by using the rest position already set in the buffer
	print_error("reload_rigid_COM is used but must be improved!");

	// calculate ne center of mass
	const RigidComponentIndex start_offset = p_rigid == 0 ? RigidComponentIndex(0) : space->get_rigids_memory()->get_offset(rigids_mchunk, p_rigid - 1);
	const RigidComponentIndex end_offset = space->get_rigids_memory()->get_offset(rigids_mchunk, p_rigid);

	const int size(end_offset - start_offset);

	if (!size)
		return;

	// In global space
	Vector3 center;
	for (RigidComponentIndex i(start_offset); i < end_offset; ++i) {
		ParticleBufferIndex particle = space->get_rigids_components_memory()->get_index(rigids_components_mchunk, i);
		const FlVector4 &particle_data(space->get_particles_memory()->get_particle(particles_mchunk, particles_mchunk->get_chunk_index(particle)));
		center += extract_position(particle_data);
	}
	center /= (float)size;

	// calculate rests
	for (RigidComponentIndex i(start_offset); i < end_offset; ++i) {
		ParticleBufferIndex particle = space->get_rigids_components_memory()->get_index(rigids_components_mchunk, i);
		const FlVector4 &particle_data(space->get_particles_memory()->get_particle(particles_mchunk, particles_mchunk->get_chunk_index(particle)));

		space->get_rigids_components_memory()->set_rest(rigids_components_mchunk, i, extract_position(particle_data) - center);
	}

	// Assign new position
	space->get_rigids_memory()->set_position(rigids_mchunk, p_rigid, center);
}

bool FlexParticleBody::is_owner_of_particle(ParticleIndex p_particle) const {
	return particles_mchunk->get_buffer_index(p_particle) <= particles_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_spring(SpringIndex p_spring) const {
	return springs_mchunk->get_buffer_index(p_spring) <= springs_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_triangle(TriangleIndex p_triangle) const {
	return triangles_mchunk->get_buffer_index(p_triangle) <= triangles_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_rigid(RigidIndex p_rigid) const {
	return rigids_mchunk->get_buffer_index(p_rigid) <= rigids_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_rigid_component(RigidComponentIndex p_rigid_component) const {
	return rigids_components_mchunk->get_buffer_index(p_rigid_component) <= rigids_components_mchunk->get_end_index();
}

void FlexParticleBody::clear_changed_params() {
	changed_parameters = 0;
}

void FlexParticleBody::clear_commands() {
	delayed_commands.particle_to_add.clear();
	delayed_commands.springs_to_add.clear();
	delayed_commands.triangles_to_add.clear();
	delayed_commands.rigids_to_add.clear();
	delayed_commands.particle_to_remove.clear();
	delayed_commands.springs_to_remove.clear();
	delayed_commands.triangles_to_remove.clear();
	delayed_commands.rigids_to_remove.clear();
	delayed_commands.rigids_components_to_remove.clear();
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
	particle_index_changed_callback.receiver->call(particle_index_changed_callback.method, (int)p_old_particle_index, (int)p_new_particle_index);
}

void FlexParticleBody::spring_index_changed(SpringIndex p_old_spring_index, SpringIndex p_new_spring_index) {
	if (!spring_index_changed_callback.receiver)
		return;
	spring_index_changed_callback.receiver->call(spring_index_changed_callback.method, (int)p_old_spring_index, (int)p_new_spring_index);
}
