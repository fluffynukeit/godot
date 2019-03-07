/*************************************************************************/
/*  flex_particle_physics_server.cpp                                     */
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

#include "flex_particle_physics_server.h"

#include "flex_memory.h"
#include "flex_utility.h"

#include "profiler.h"
#include "thirdparty/flex/include/NvFlex.h"
#include "thirdparty/flex/include/NvFlexExt.h"

#include "scene/3d/physics_particle_body_mesh_instance.h"

/**
@author AndreaCatania
*/

#define CreateThenReturnRID(owner, rid_data) \
	RID rid = owner.make_rid(rid_data);      \
	rid_data->__set_self(rid);               \
	rid_data->__set_physics_server(this);    \
	return rid;

void FlexParticleBodyCommands::move_particles(const Transform &transform) {

	for (int i(get_particle_count() - 1); 0 <= i; --i) {
		set_particle_position(i, transform.xform(get_particle_position(i)));
	}
}

void FlexParticleBodyCommands::load_model(Ref<ParticleBodyModel> p_model, const Transform &initial_transform) {
	if (p_model.is_null())
		return;

	PoolVector<Vector3>::Read particle_positions_r = p_model->get_particles().read();

	{ // Particle
		const int resource_p_count(p_model->get_particles().size());

		// Reset particle memory
		body->space->particles_allocator->resize_chunk(
				body->particles_mchunk,
				0);

		ParticleIndex first_added_particle = body->CMD_add_particles(resource_p_count);
		ERR_FAIL_COND(first_added_particle != 0);

		PoolVector<real_t>::Read masses_r = p_model->get_masses().read();

		for (int i(0); i < resource_p_count; ++i) {
			initialize_particle(i, initial_transform.xform(particle_positions_r[i]), masses_r[i]);
		}
	}

	{ // Spring
		const int resource_s_count(p_model->get_constraints_indexes_ref().size() / 2);
		body->space->springs_allocator->resize_chunk(
				body->springs_mchunk,
				resource_s_count);

		for (int i(0); i < resource_s_count; ++i) {
			set_spring(i,
					p_model->get_constraints_indexes_ref().get(i * 2 + 0),
					p_model->get_constraints_indexes_ref().get(i * 2 + 1),
					p_model->get_constraints_info_ref().get(i).x,
					p_model->get_constraints_info_ref().get(i).y);
		}
	}

	{ // Dynamic triangle
		const int resource_t_count(p_model->get_dynamic_triangles_indices().size() / 3);
		// Resize existing memory chunk
		triangles_set_count(resource_t_count);

		PoolVector<int>::Read triangles_indices_r(p_model->get_dynamic_triangles_indices().read());
		for (int t(0); t < resource_t_count; ++t) {
			set_triangle(t, triangles_indices_r[t * 3], triangles_indices_r[t * 3 + 1], triangles_indices_r[t * 3 + 2]);
		}
	}

	{ // inflatables
		if (p_model->get_want_inflatable()) {

			body->space->inflatables_allocator->resize_chunk(body->inflatable_mchunk, 1);

			body->set_rest_volume(p_model->get_rest_volume());
			body->set_constraint_scale(p_model->get_constraint_scale());
		} else {
			body->space->inflatables_allocator->resize_chunk(body->inflatable_mchunk, 0);
		}
	}

	{ // Rigids

		const int resource_r_count(p_model->get_clusters_offsets().size());

		body->space->rigids_allocator->resize_chunk(body->rigids_mchunk, resource_r_count);

		PoolVector<Vector3>::Read cluster_pos_r = p_model->get_clusters_positions().read();
		PoolVector<float>::Read cluster_stiffness_r = p_model->get_clusters_stiffness().read();
		PoolVector<float>::Read cluster_plastic_threshold_r = p_model->get_clusters_plastic_threshold().read();
		PoolVector<float>::Read cluster_plastic_creep_r = p_model->get_clusters_plastic_creep().read();
		PoolVector<int>::Read cluster_offsets_r = p_model->get_clusters_offsets().read();

		for (int i(0); i < resource_r_count; ++i) {
			set_rigid(i, initial_transform.translated(cluster_pos_r[i]), cluster_stiffness_r[i], cluster_plastic_threshold_r[i], cluster_plastic_creep_r[i], cluster_offsets_r[i]);
		}

		// Rigids components

		const int resource_rc_count(p_model->get_clusters_particle_indices().size());
		body->space->rigids_components_allocator->resize_chunk(body->rigids_components_mchunk, resource_rc_count);

		PoolVector<int>::Read indices_r = p_model->get_clusters_particle_indices().read();

		int cluster_index(-1);
		int cluster_offset(-1);
		Vector3 cluster_position;

		for (int i(0); i < resource_rc_count; ++i) {
			if (i >= cluster_offset) {
				++cluster_index;
				cluster_offset = cluster_offsets_r[cluster_index];
				cluster_position = cluster_pos_r[cluster_index];
			}

			set_rigid_component(i, body->particles_mchunk->get_buffer_index(indices_r[i]), particle_positions_r[indices_r[i]] - cluster_position);
		}
	}

	{ // Tearing
		if (p_model->get_allow_tearing()) {

			body->set_tearing_active(p_model->get_allow_tearing());

			body->tearing_data->spring_rest_lengths_2.resize(
					body->get_spring_count());

			for (int i = body->get_spring_count() - 1; 0 <= i; --i) {

				const int p1 =
						p_model->get_constraints_indexes_ref().get(i * 2 + 0);
				const int p2 =
						p_model->get_constraints_indexes_ref().get(i * 2 + 1);

				body->tearing_data->spring_rest_lengths_2.write[i] =
						(particle_positions_r[p2] - particle_positions_r[p1])
								.length_squared();
			}

			{ // Load dynamic triangles

				const int triangle_count(
						p_model->get_dynamic_triangles_indices().size() / 3);
				body->tearing_data->triangles.resize(triangle_count);
				body->tearing_data->sides.resize(triangle_count);

				PoolVector<int>::Read triangles_indices_r(
						p_model->get_dynamic_triangles_indices().read());

				VectorWriteProxy<ParticlePhysicsServer::Triangle> &triangles =
						body->tearing_data->triangles.write;

				// Step 1 build triangle and edges
				for (int t(0); t < triangle_count; ++t) {

					triangles[t] =
							ParticlePhysicsServer::Triangle(
									triangles_indices_r[t * 3 + 0],
									triangles_indices_r[t * 3 + 1],
									triangles_indices_r[t * 3 + 2]);

					triangles[t].self_id = t;

					triangles[t].edges[0].particle_index0 = triangles_indices_r[t * 3 + 0];
					triangles[t].edges[0].particle_index1 = triangles_indices_r[t * 3 + 1];

					triangles[t].edges[1].particle_index0 = triangles_indices_r[t * 3 + 1];
					triangles[t].edges[1].particle_index1 = triangles_indices_r[t * 3 + 2];

					triangles[t].edges[2].particle_index0 = triangles_indices_r[t * 3 + 2];
					triangles[t].edges[2].particle_index1 = triangles_indices_r[t * 3 + 0];

					// Find springs
					for (int j = 2; 0 <= j; --j) {
						int particle0 = triangles[t].edges[j].particle_index0;
						int particle1 = triangles[t].edges[j].particle_index1;

						for (int x = body->get_spring_count() - 1; 0 <= x; --x) {

							const int p0 =
									p_model->get_constraints_indexes_ref().get(x * 2 + 0);
							const int p1 =
									p_model->get_constraints_indexes_ref().get(x * 2 + 1);

							if ((p0 == particle0 && p1 == particle1) ||
									(p0 == particle1 && p1 == particle0)) {

								triangles[t].springs[j] = x;
								break;
							}
						}
					}
				}

				// Step 2 Finalize edges

				for (int t0(0); t0 < triangle_count; ++t0) {

					for (int t1(0); t1 < triangle_count; ++t1) {

						if (t0 == t1)
							continue;

						int other_edge_id;
						ParticlePhysicsServer::Edge *edge =
								triangles[t0].find_adjacent(
										triangles[t1],
										other_edge_id);

						if (edge) {
							edge->adjacent_triangle_index = t1;
							edge->adjacent_edge_index = other_edge_id;

							int bending_particle0 = triangles[t0].get_opposit_vertex(edge);
							int bending_particle1 = triangles[t1].get_opposit_vertex(edge);

							for (int x = body->get_spring_count() - 1; 0 <= x; --x) {

								const int p0 =
										p_model->get_constraints_indexes_ref().get(x * 2 + 0);
								const int p1 =
										p_model->get_constraints_indexes_ref().get(x * 2 + 1);

								if ((p0 == bending_particle0 && p1 == bending_particle1) ||
										(p0 == bending_particle1 && p1 == bending_particle0)) {

									edge->bending_spring_index = x;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

void FlexParticleBodyCommands::add_unactive_particles(int p_particle_count) {
	body->CMD_add_unactive_particles(p_particle_count);
}

int FlexParticleBodyCommands::add_particles(int p_particle_count) {
	return body->CMD_add_particles(p_particle_count);
}

void FlexParticleBodyCommands::initialize_particle(int p_index, const Vector3 &p_global_position, real_t p_mass) {

	ERR_FAIL_COND(!body->is_owner_of_particle(p_index));

	body->space->particles_memory->set_particle(
			body->particles_mchunk,
			p_index,
			make_particle(p_global_position, p_mass));

	body->space->particles_memory->set_velocity(
			body->particles_mchunk,
			p_index,
			Vector3());

	body->space->particles_memory->set_phase(
			body->particles_mchunk,
			p_index,
			NvFlexMakePhaseWithChannels(
					body->collision_group,
					body->collision_flags,
					body->collision_primitive_mask));

	body->changed_parameters |= eChangedBodyParamParticleJustAdded;
}

void FlexParticleBodyCommands::set_particle_position_mass(int p_particle_index, const Vector3 &p_position, real_t p_mass) {
	body->CMD_set_particle_position_mass(p_particle_index, p_position, p_mass);
}

int FlexParticleBodyCommands::get_particle_count() const {
	return body->get_particle_count();
}

int FlexParticleBodyCommands::get_particle_buffer_stride() const {
	return 4;
}

const float *FlexParticleBodyCommands::get_particle_buffer() const {
	return (const float *)
			body->space->particles_memory->get_particles_buffers(
					body->particles_mchunk);
}

int FlexParticleBodyCommands::get_particle_velocities_buffer_stride() const {
	return 3;
}

const float *FlexParticleBodyCommands::get_particle_velocities_buffer() const {
	return (const float *)
			body->space->particles_memory->get_velocities_buffer(
					body->particles_mchunk);
}

void FlexParticleBodyCommands::set_particle_position(int p_particle_index, const Vector3 &p_position) {
	body->CMD_set_particle_position(p_particle_index, p_position);
}

Vector3 FlexParticleBodyCommands::get_particle_position(int p_particle_index) const {
	return body->CMD_get_particle_position(p_particle_index);
}

void FlexParticleBodyCommands::set_particle_mass(int p_particle_index, real_t p_mass) {
	body->CMD_set_particle_mass(p_particle_index, p_mass);
}

float FlexParticleBodyCommands::get_particle_mass(int p_particle_index) const {
	return body->CMD_get_particle_mass(p_particle_index);
}

float FlexParticleBodyCommands::get_particle_inv_mass(int p_particle_index) const {
	return body->CMD_get_particle_inv_mass(p_particle_index);
}

const Vector3 &FlexParticleBodyCommands::get_particle_velocity(int p_particle_index) const {
	return body->CMD_get_particle_velocity(p_particle_index);
}

void FlexParticleBodyCommands::set_particle_velocity(int p_particle_index, const Vector3 &p_velocity) {
	body->CMD_set_particle_velocity(p_particle_index, p_velocity);
}

void FlexParticleBodyCommands::apply_force(int p_particle_index, const Vector3 &p_force) {
	const real_t inv_mass = get_particle_inv_mass(p_particle_index);
	if (inv_mass <= 0.0)
		return;
	const Vector3 velocity = p_force * inv_mass * FlexParticlePhysicsServer::singleton->get_delta_time();
	set_particle_velocity(p_particle_index, get_particle_velocity(p_particle_index) + velocity);
}

Vector3 FlexParticleBodyCommands::get_particle_normal(int p_index) const {
	return vec3_from_flvec4(body->CMD_get_particle_normal(p_index));
}

void FlexParticleBodyCommands::set_particle_normal(int p_index, const Vector3 &p_normal) {
	body->CMD_set_particle_normal(p_index, p_normal);
}

const AABB FlexParticleBodyCommands::get_aabb() const {
	return AABB(); //body->get_space()->particle_bodies_aabb[body->id];
}

const Vector3 &FlexParticleBodyCommands::get_rigid_position(int p_index) const {
	return body->CMD_get_rigid_position(p_index);
}

const Quat &FlexParticleBodyCommands::get_rigid_rotation(int p_index) const {
	return body->CMD_get_rigid_rotation(p_index);
}

Transform FlexParticleBodyCommands::get_rigid_transform(int p_index) const {
	return Transform(Basis(get_rigid_rotation(p_index)), get_rigid_position(p_index));
}

int FlexParticleBodyCommands::get_rigid_index_of_particle(int p_particle_index) {

	RigidComponentIndex rigid_component_index = -1;
	for (RigidComponentIndex i(body->rigids_components_mchunk->get_size() - 1); 0 <= i; --i) {
		const ParticleBufferIndex pbindex = body->space->rigids_components_memory->get_index(body->rigids_components_mchunk, i);
		const ParticleIndex pindex = body->particles_mchunk->get_chunk_index(pbindex);
		if (pindex == p_particle_index) {
			rigid_component_index = i;
			break;
		}
	}

	if (0 <= rigid_component_index) {
		for (RigidIndex i(0); i < body->rigids_mchunk->get_size(); ++i) {
			const RigidComponentIndex rcindex = body->space->rigids_memory->get_offset(body->rigids_mchunk, i);
			if (rcindex > rigid_component_index) {
				return i;
			}
		}
	}

	return -1;
}

void FlexParticleBodyCommands::set_rigid_velocity_toward_position(int p_rigid_index, const Transform &p_new_position) {

	const real_t dt(FlexParticlePhysicsServer::singleton->get_delta_time());

	if (!dt)
		return;

	Transform current_transform_inv(Basis(get_rigid_rotation(p_rigid_index)), get_rigid_position(p_rigid_index));
	current_transform_inv.invert();

	const RigidComponentIndex initial_component_index(p_rigid_index ? body->space->rigids_memory->get_offset(body->rigids_mchunk, p_rigid_index - 1) : RigidComponentIndex(0));
	const RigidComponentIndex last_component_index(body->space->rigids_memory->get_offset(body->rigids_mchunk, p_rigid_index));

	RigidsComponentsMemory *components_memory = body->space->rigids_components_memory;

	for (RigidComponentIndex i(initial_component_index); i < last_component_index; ++i) {

		const ParticleIndex pindex = body->particles_mchunk->get_chunk_index(components_memory->get_index(body->rigids_components_mchunk, i));

		const Vector3 current_ppos(get_particle_position(pindex));
		const Vector3 new_ppos(p_new_position.xform(current_transform_inv.xform(current_ppos)));

		set_particle_velocity(pindex, (new_ppos - current_ppos) / dt);
	}
}

void FlexParticleBodyCommands::set_particle_group(int p_particle_index, int p_group) {
	FlexSpace *space = body->space;
	int phase = space->particles_memory->get_phase(
			body->particles_mchunk,
			p_particle_index);

	const int current_group = eNvFlexPhaseGroupMask & phase;
	if (current_group == p_group)
		return;

	// Reset phase
	phase = (~eNvFlexPhaseGroupMask) & phase;

	space->particles_memory->set_phase(
			body->particles_mchunk,
			p_particle_index,
			(p_group & eNvFlexPhaseGroupMask) | phase);

	body->changed_parameters |= eChangedBodyParamPhaseSingle;
}

int FlexParticleBodyCommands::get_particle_group(int p_particle_index) const {
	FlexSpace *space = body->space;
	const int phase = space->particles_memory->get_phase(
			body->particles_mchunk,
			p_particle_index);
	return phase & eNvFlexPhaseGroupMask;
}

void FlexParticleBodyCommands::add_spring(int p_particle_0, int p_particle_1, float p_length, float p_stiffness) {
	body->CMD_add_spring(p_particle_0, p_particle_1, p_length, p_stiffness);
}

void FlexParticleBodyCommands::set_spring(SpringIndex p_index, ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness) {
	body->CMD_set_spring(p_index, p_particle_0, p_particle_1, p_length, p_stiffness);
}

int FlexParticleBodyCommands::get_spring_count() const {
	return body->get_spring_count();
}

void FlexParticleBodyCommands::get_spring_positions(
		int p_spring_index,
		Vector3 &r_begin,
		Vector3 &r_end) const {

	const Spring &s = body->CMD_get_spring_indices(p_spring_index);
	const FlVector4 &begin = body->space->get_particles_memory()->get_particle(s.index0);
	const FlVector4 &end = body->space->get_particles_memory()->get_particle(s.index1);

	r_begin = vec3_from_flvec4(begin);
	r_end = vec3_from_flvec4(end);
}

void FlexParticleBodyCommands::triangles_set_count(int p_count) {
	body->space->triangles_allocator->resize_chunk(body->triangles_mchunk, p_count);
}

void FlexParticleBodyCommands::add_triangle(ParticleIndex p_particle_0, ParticleIndex p_particle_1, ParticleIndex p_particle_2) {
	const int previous_size(body->get_triangle_count());
	triangles_set_count(previous_size + 1);
	set_triangle(previous_size, p_particle_0, p_particle_1, p_particle_2);
}

void FlexParticleBodyCommands::set_triangle(TriangleIndex p_index, ParticleIndex p_particle_0, ParticleIndex p_particle_1, ParticleIndex p_particle_2) {

	ERR_FAIL_COND(!body->is_owner_of_triangle(p_index));

	body->space->triangles_memory->set_triangle(body->triangles_mchunk, p_index, DynamicTriangle(body->particles_mchunk->get_buffer_index(p_particle_0), body->particles_mchunk->get_buffer_index(p_particle_1), body->particles_mchunk->get_buffer_index(p_particle_2)));
}

void FlexParticleBodyCommands::add_rigid(const Transform &p_transform, float p_stiffness, float p_plastic_threshold, float p_plastic_creep, RigidComponentIndex p_offset) {
	const int previous_size(body->get_rigid_count());
	body->space->rigids_allocator->resize_chunk(body->rigids_mchunk, previous_size + 1);
	set_rigid(previous_size, p_transform, p_stiffness, p_plastic_threshold, p_plastic_creep, p_offset);
}

void FlexParticleBodyCommands::set_rigid(RigidIndex p_index, const Transform &p_transform, float p_stiffness, float p_plastic_threshold, float p_plastic_creep, RigidComponentIndex p_offset) {

	ERR_FAIL_COND(!body->is_owner_of_rigid(p_index));

	FlexSpace *space(body->space);

	space->rigids_memory->set_position(body->rigids_mchunk, p_index, p_transform.origin);
	space->rigids_memory->set_rotation(body->rigids_mchunk, p_index, p_transform.basis.get_quat());
	space->rigids_memory->set_stiffness(body->rigids_mchunk, p_index, p_stiffness);
	space->rigids_memory->set_threshold(body->rigids_mchunk, p_index, p_plastic_threshold);
	space->rigids_memory->set_creep(body->rigids_mchunk, p_index, p_plastic_creep);
	space->rigids_memory->set_offset(body->rigids_mchunk, p_index, p_offset);
}

void FlexParticleBodyCommands::add_rigid_component(ParticleBufferIndex p_particle_index, const Vector3 &p_rest) {
	const int previous_size(body->rigids_components_mchunk->get_size());
	body->space->rigids_components_allocator->resize_chunk(body->rigids_components_mchunk, previous_size + 1);
	set_rigid_component(previous_size, p_particle_index, p_rest);
}

void FlexParticleBodyCommands::set_rigid_component(RigidComponentIndex p_index, ParticleBufferIndex p_particle_index, const Vector3 &p_rest) {

	ERR_FAIL_COND(!body->is_owner_of_rigid_component(p_index));

	body->space->rigids_components_memory->set_index(body->rigids_components_mchunk, p_index, p_particle_index);
	body->space->rigids_components_memory->set_rest(body->rigids_components_mchunk, p_index, p_rest);
}

int FlexParticleBodyConstraintCommands::get_spring_count() const {
	return constraint->get_spring_count();
}

int FlexParticleBodyConstraintCommands::add_spring(int p_body0_particle, int p_body1_particle, float p_length, float p_stiffness) {
	const int previous_size = constraint->get_spring_count();
	constraint->space->springs_allocator->resize_chunk(constraint->springs_mchunk, previous_size + 1);
	set_spring(previous_size, p_body0_particle, p_body1_particle, p_length, p_stiffness);
	return previous_size;
}

void FlexParticleBodyConstraintCommands::set_spring(int p_index, int p_body0_particle, int p_body1_particle, float p_length, float p_stiffness) {

	ERR_FAIL_COND(!constraint->is_owner_of_spring(p_index));

	constraint->space->get_springs_memory()->set_spring(constraint->springs_mchunk, p_index, Spring(constraint->body0->particles_mchunk->get_buffer_index(p_body0_particle), constraint->body1->particles_mchunk->get_buffer_index(p_body1_particle)));
	constraint->space->get_springs_memory()->set_length(constraint->springs_mchunk, p_index, p_length);
	constraint->space->get_springs_memory()->set_stiffness(constraint->springs_mchunk, p_index, p_stiffness);
}

real_t FlexParticleBodyConstraintCommands::get_distance(int p_body0_particle, int p_body1_particle) const {
	return (
			constraint->body1->CMD_get_particle_position(p_body1_particle) -
			constraint->body0->CMD_get_particle_position(p_body0_particle))
			.length();
}

void FlexParticleBodyConstraintCommands::get_spring_positions(
		int p_body0_particle,
		int p_body1_particle,
		Vector3 &r_begin,
		Vector3 &r_end) const {

	r_begin = constraint->body0->CMD_get_particle_position(p_body0_particle);
	r_end = constraint->body1->CMD_get_particle_position(p_body1_particle);
}

FlexParticlePhysicsServer *FlexParticlePhysicsServer::singleton = NULL;

FlexParticlePhysicsServer::FlexParticlePhysicsServer() :
		ParticlePhysicsServer(),
		solver_param_numIterations("numIterations"),
		solver_param_gravity("gravity"),
		solver_param_radius("radius_particle_particle"),
		solver_param_collisionDistance("radius_primitive_particle"),
		solver_param_solidRestDistance("solidRestDistance"),
		solver_param_fluidRestDistance("fluidRestDistance"),
		solver_param_dynamicFriction("dynamicFriction"),
		solver_param_staticFriction("staticFriction"),
		solver_param_particleFriction("particleFriction"),
		solver_param_restitution("restitution"),
		solver_param_adhesion("adhesion"),
		solver_param_sleepThreshold("sleepThreshold"),
		solver_param_maxSpeed("maxSpeed"),
		solver_param_maxAcceleration("maxAcceleration"),
		solver_param_shockPropagation("shockPropagation"),
		solver_param_dissipation("dissipation"),
		solver_param_damping("damping"),
		solver_param_wind("wind"),
		solver_param_drag("drag"),
		solver_param_lift("lift"),
		solver_param_cohesion("cohesion"),
		solver_param_surfaceTension("surfaceTension"),
		solver_param_viscosity("viscosity"),
		solver_param_vorticityConfinement("vorticityConfinement"),
		solver_param_anisotropyScale("anisotropyScale"),
		solver_param_anisotropyMin("anisotropyMin"),
		solver_param_anisotropyMax("anisotropyMax"),
		solver_param_smoothing("smoothing"),
		solver_param_solidPressure("solidPressure"),
		solver_param_freeSurfaceDrag("freeSurfaceDrag"),
		solver_param_buoyancy("buoyancy"),
		solver_param_diffuseThreshold("diffuseThreshold"),
		solver_param_diffuseBuoyancy("diffuseBuoyancy"),
		solver_param_diffuseDrag("diffuseDrag"),
		solver_param_diffuseBallistic("diffuseBallistic"),
		solver_param_diffuseLifetime("diffuseLifetime"),
		solver_param_particleCollisionMargin("particleCollisionMargin"),
		solver_param_shapeCollisionMargin("shapeCollisionMargin"),
		solver_param_relaxationMode("relaxationMode"),
		solver_param_relaxationFactor("relaxationFactor"),
		solver_param_tearing_max_splits("tearing_max_splits"),
		solver_param_tearing_max_spring_checks("tearing_max_spring_checks"),
		is_active(true),
		delta_time(0.0),
		last_space_index(-1) {

	ERR_FAIL_COND(singleton);
	singleton = this;

	clear_timers();
}

FlexParticlePhysicsServer::~FlexParticlePhysicsServer() {
}

RID FlexParticlePhysicsServer::space_create() {
	FlexSpace *space = memnew(FlexSpace);
	CreateThenReturnRID(space_owner, space);
}

void FlexParticlePhysicsServer::space_set_active(RID p_space, bool p_active) {
	FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND(!space);

	if (space_is_active(p_space) == p_active)
		return;

	if (p_active) {
		active_spaces.push_back(space);
	} else {
		auto it = std::find(
				active_spaces.begin(),
				active_spaces.end(),
				space);
		if (it != active_spaces.end())
			active_spaces.erase(it, it + 1);
	}
	last_space_index = static_cast<short>(active_spaces.size() - 1);
}

bool FlexParticlePhysicsServer::space_is_active(RID p_space) const {
	FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, false);

	auto it = std::find(
			active_spaces.begin(),
			active_spaces.end(),
			space);
	return it != active_spaces.end();
}

void FlexParticlePhysicsServer::space_get_params_defaults(Map<StringName, Variant> *r_defs) const {

	real_t def_radius = 0.1;

	(*r_defs)[solver_param_numIterations] = 3;
	(*r_defs)[solver_param_gravity] = Vector3(0, -10, 0);
	(*r_defs)[solver_param_radius] = def_radius * 1.2;
	(*r_defs)[solver_param_collisionDistance] = def_radius;
	(*r_defs)[solver_param_solidRestDistance] = def_radius;
	(*r_defs)[solver_param_fluidRestDistance] = def_radius;
	(*r_defs)[solver_param_dynamicFriction] = 0.1;
	(*r_defs)[solver_param_staticFriction] = 0.1;
	(*r_defs)[solver_param_particleFriction] = 0.1;
	(*r_defs)[solver_param_restitution] = 0.01;
	(*r_defs)[solver_param_adhesion] = 0.0;
	(*r_defs)[solver_param_sleepThreshold] = 0.5;
	(*r_defs)[solver_param_maxSpeed] = FLT_MAX;
	(*r_defs)[solver_param_maxAcceleration] = Vector3((*r_defs)[solver_param_gravity]).length() * 10.0;
	(*r_defs)[solver_param_shockPropagation] = 0.0;
	(*r_defs)[solver_param_dissipation] = 0.5;
	(*r_defs)[solver_param_damping] = 0.1;
	(*r_defs)[solver_param_wind] = Vector3();
	(*r_defs)[solver_param_drag] = 0.0;
	(*r_defs)[solver_param_lift] = 0.0;
	(*r_defs)[solver_param_cohesion] = 0.025;
	(*r_defs)[solver_param_surfaceTension] = 0.0;
	(*r_defs)[solver_param_viscosity] = 0.0;
	(*r_defs)[solver_param_vorticityConfinement] = 0.0;
	(*r_defs)[solver_param_anisotropyScale] = 0.0;
	(*r_defs)[solver_param_anisotropyMin] = 0.0;
	(*r_defs)[solver_param_anisotropyMax] = 0.0;
	(*r_defs)[solver_param_smoothing] = 0.0;
	(*r_defs)[solver_param_solidPressure] = 1.0;
	(*r_defs)[solver_param_freeSurfaceDrag] = 0.0;
	(*r_defs)[solver_param_buoyancy] = 0.0;
	(*r_defs)[solver_param_diffuseThreshold] = 0.0;
	(*r_defs)[solver_param_diffuseBuoyancy] = 0.0;
	(*r_defs)[solver_param_diffuseDrag] = 0.0;
	(*r_defs)[solver_param_diffuseBallistic] = 0.0;
	(*r_defs)[solver_param_diffuseLifetime] = 0.0;
	(*r_defs)[solver_param_particleCollisionMargin] = 0.001;
	(*r_defs)[solver_param_shapeCollisionMargin] = 0.001;
	(*r_defs)[solver_param_relaxationMode] = "local";
	(*r_defs)[solver_param_relaxationFactor] = 0.8;
	(*r_defs)[solver_param_tearing_max_splits] = 100;
	(*r_defs)[solver_param_tearing_max_spring_checks] = 200;
}

bool FlexParticlePhysicsServer::space_set_param(RID p_space, const StringName &p_name, const Variant &p_property) {
	FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, false);

	return space->set_param(p_name, p_property);
}

bool FlexParticlePhysicsServer::space_get_param(RID p_space, const StringName &p_name, Variant &r_property) const {
	const FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, false);

	return space->get_param(p_name, r_property);
}

real_t FlexParticlePhysicsServer::space_get_particle_radius(RID p_space) const {
	const FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, 0);

	return space->get_particle_radius();
}

void FlexParticlePhysicsServer::space_reset_params_to_default(RID p_space) {
	FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND(!space);

	space->reset_params_to_defaults();
}

bool FlexParticlePhysicsServer::space_is_using_default_params(RID p_space) const {
	const FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, false);

	return space->is_using_default_params();
}

RID FlexParticlePhysicsServer::body_create() {
	FlexParticleBody *particle_body = memnew(FlexParticleBody);
	CreateThenReturnRID(body_owner, particle_body);
}

void FlexParticlePhysicsServer::body_set_space(RID p_body, RID p_space) {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	if (p_space == RID()) {
		// Remove
		if (body->get_space()) {
			body->get_space()->remove_particle_body(body);
		}
	} else {
		// Add
		FlexSpace *space = space_owner.get(p_space);
		ERR_FAIL_COND(!space);
		space->add_particle_body(body);
	}
}

void FlexParticlePhysicsServer::body_set_callback(RID p_body, ParticleBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method) {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);
	body->set_callback(p_callback_type, p_receiver, p_method);
}

void FlexParticlePhysicsServer::body_set_object_instance(RID p_body, Object *p_object) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);
	body->set_object_instance(p_object);
}

ParticleBodyCommands *FlexParticlePhysicsServer::body_get_commands(RID p_body) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, NULL);
	ERR_FAIL_COND_V(!body->get_space(), NULL);
	ERR_FAIL_COND_V(!body->get_space()->can_commands_be_executed(), NULL);
	return get_particle_body_commands(body);
}

void FlexParticlePhysicsServer::body_set_collision_group(RID p_body, uint32_t p_group) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);
	body->set_collision_group(p_group);
}

uint32_t FlexParticlePhysicsServer::body_get_collision_group(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_collision_group();
}

void FlexParticlePhysicsServer::body_set_collision_flag(RID p_body, ParticleCollisionFlag p_flag, bool p_active) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_collision_flag(p_flag, p_active);
}

bool FlexParticlePhysicsServer::body_get_collision_flag(RID p_body, ParticleCollisionFlag p_flag) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);
	return body->get_collision_flag(p_flag);
}

void FlexParticlePhysicsServer::body_set_collision_primitive_mask(RID p_body, uint32_t p_mask) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);
	body->set_collision_primitive_mask(p_mask);
}

uint32_t FlexParticlePhysicsServer::body_get_collision_primitive_mask(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);
	return body->get_collision_primitive_mask();
}

void FlexParticlePhysicsServer::body_remove_particle(RID p_body, int p_particle_index, bool p_unactive) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	if (p_unactive)
		body->unactive_particle(p_particle_index);
	else
		body->remove_particle(p_particle_index);
}

void FlexParticlePhysicsServer::body_remove_rigid(RID p_body, int p_rigid_index) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->remove_rigid(p_rigid_index);
}

int FlexParticlePhysicsServer::body_get_particle_count(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_particle_count();
}

const float *FlexParticlePhysicsServer::body_get_particle_particle_buffer(
		RID p_body) const {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, NULL);

	return reinterpret_cast<const float *>(
			body->get_space()->get_particle_particle_buffer(body));
}

bool FlexParticlePhysicsServer::body_get_particle_position(
		RID p_body,
		int p_particle_index,
		Vector3 &r_pos) const {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	const FlVector4 &pos_mass =
			body->get_space()->get_particle_position(
					body,
					p_particle_index);

	r_pos = extract_position(pos_mass);
	return true;
}

bool FlexParticlePhysicsServer::body_get_particle_normal(
		RID p_body,
		int p_particle_index,
		Vector3 &r_norm) const {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	const FlVector4 &normal =
			body->get_space()->get_particle_normal(
					body,
					p_particle_index);

	r_norm = vec3_from_flvec4(normal);
	return true;
}

real_t FlexParticlePhysicsServer::body_get_particle_mass(
		RID p_body,
		int p_particle_index) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	const FlVector4 &pos_mass =
			body->get_space()->get_particle_position(
					body,
					p_particle_index);

	return extract_mass(pos_mass);
}

const float *FlexParticlePhysicsServer::body_get_particle_velocity_buffer(
		RID p_body) const {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, NULL);

	return reinterpret_cast<const float *>(
			body->get_space()->get_particle_velocity_buffer(body));
}

bool FlexParticlePhysicsServer::body_get_particle_velocity(
		RID p_body,
		int p_particle_index,
		Vector3 &r_vel) const {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	r_vel = body->get_space()->get_particle_velocity(
			body,
			p_particle_index);

	return true;
}

bool FlexParticlePhysicsServer::body_is_particle_active(
		RID p_body,
		int p_particle_index) const {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_particle_active(p_particle_index);
}

int FlexParticlePhysicsServer::body_get_spring_count(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_spring_count();
}

int FlexParticlePhysicsServer::body_get_rigid_count(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_rigid_count();
}

void FlexParticlePhysicsServer::body_set_pressure(RID p_body, float p_pressure) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_pressure(p_pressure);
}

float FlexParticlePhysicsServer::body_get_pressure(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_pressure();
}

bool FlexParticlePhysicsServer::body_can_rendered_using_skeleton(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return 0 < body->get_rigid_count();
}

bool FlexParticlePhysicsServer::body_can_rendered_using_pvparticles(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->get_triangle_count();
}

void FlexParticlePhysicsServer::body_set_monitorable(RID p_body, bool p_monitorable) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_monitorable(p_monitorable);
}

bool FlexParticlePhysicsServer::body_is_monitorable(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_monitorable();
}

void FlexParticlePhysicsServer::body_set_monitoring_primitives_contacts(RID p_body, bool p_monitoring) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_monitoring_primitives_contacts(p_monitoring);
}

bool FlexParticlePhysicsServer::body_is_monitoring_primitives_contacts(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_monitoring_primitives_contacts();
}

const ParticlePhysicsServer::TearingData *FlexParticlePhysicsServer::body_get_tearing_data(RID p_body) const {
	const FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, NULL);

	return body->get_tearing_data();
}

void FlexParticlePhysicsServer::body_set_tearing_max_extension(
		RID p_body,
		real_t p_tearing_max_extension) {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	return body->set_tearing_max_extension(p_tearing_max_extension);
}

real_t FlexParticlePhysicsServer::body_get_tearing_max_extension(RID p_body) const {
	const FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, -1);

	return body->get_tearing_max_extension();
}

void FlexParticlePhysicsServer::body_add_force_tearing(
		RID p_body,
		int p_particle_index,
		const Vector3 &p_split_plane) {

	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->add_force_tearing(p_particle_index, p_split_plane);
}

RID FlexParticlePhysicsServer::constraint_create(RID p_body0, RID p_body1) {
	FlexParticleBody *body0 = body_owner.get(p_body0);
	ERR_FAIL_COND_V(!body0, RID());

	FlexParticleBody *body1 = body_owner.get(p_body1);
	ERR_FAIL_COND_V(!body1, RID());

	FlexParticleBodyConstraint *constraint = memnew(FlexParticleBodyConstraint(body0, body1));
	CreateThenReturnRID(body_constraint_owner, constraint);
}

void FlexParticlePhysicsServer::constraint_set_callback(RID p_constraint, Object *p_receiver, const StringName &p_method) {
	FlexParticleBodyConstraint *constraint = body_constraint_owner.get(p_constraint);
	ERR_FAIL_COND(!constraint);

	constraint->set_callback(p_receiver, p_method);
}

void FlexParticlePhysicsServer::constraint_set_space(RID p_constraint, RID p_space) {

	FlexParticleBodyConstraint *constraint = body_constraint_owner.get(p_constraint);
	ERR_FAIL_COND(!constraint);

	if (p_space == RID()) {

		if (constraint->get_space())
			constraint->get_space()->remove_particle_body_constraint(constraint);

	} else {
		FlexSpace *space = space_owner.get(p_space);
		ERR_FAIL_COND(!space);

		space->add_particle_body_constraint(constraint);
	}
}

void FlexParticlePhysicsServer::constraint_remove_spring(RID p_constraint, int p_spring_index) {
	FlexParticleBodyConstraint *constraint = body_constraint_owner.get(p_constraint);
	ERR_FAIL_COND(!constraint);

	constraint->remove_spring(p_spring_index);
}

RID FlexParticlePhysicsServer::primitive_body_create() {
	FlexPrimitiveBody *primitive = memnew(FlexPrimitiveBody);
	CreateThenReturnRID(primitive_body_owner, primitive);
}

void FlexParticlePhysicsServer::primitive_body_set_space(RID p_body, RID p_space) {

	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	if (p_space == RID()) {
		if (body->get_space())
			body->get_space()->remove_primitive_body(body);
	} else {

		FlexSpace *space = space_owner.get(p_space);
		ERR_FAIL_COND(!space);

		space->add_primitive_body(body);
	}
}

void FlexParticlePhysicsServer::primitive_body_set_shape(RID p_body, RID p_shape) {

	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	if (p_shape == RID()) {
		body->set_shape(NULL);
	} else {
		FlexPrimitiveShape *shape = primitive_shape_owner.get(p_shape);
		ERR_FAIL_COND(!shape);
		body->set_shape(shape);
	}
}

void FlexParticlePhysicsServer::primitive_body_set_callback(RID p_body, ParticlePrimitiveBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_callback(p_callback_type, p_receiver, p_method);
}

void FlexParticlePhysicsServer::primitive_body_set_object_instance(RID p_body, Object *p_object) {

	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_object_instance(p_object);
}

void FlexParticlePhysicsServer::primitive_body_set_use_custom_friction(RID p_body, bool p_use) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_use_custom_friction(p_use);
}

void FlexParticlePhysicsServer::primitive_body_set_custom_friction(RID p_body, real_t p_friction) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_custom_friction(p_friction);
}

void FlexParticlePhysicsServer::primitive_body_set_custom_friction_threshold(RID p_body, real_t p_threshold) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_custom_friction_threshold(p_threshold);
}

void FlexParticlePhysicsServer::primitive_body_set_transform(RID p_body, const Transform &p_transf, bool p_teleport) {

	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_transform(p_transf, p_teleport);
}

void FlexParticlePhysicsServer::primitive_body_set_collision_layer(RID p_body, uint32_t p_layer) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_layer(p_layer);
}

uint32_t FlexParticlePhysicsServer::primitive_body_get_collision_layer(RID p_body) const {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_layer();
}

void FlexParticlePhysicsServer::primitive_body_set_kinematic(RID p_body, bool p_kinematic) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_kinematic(p_kinematic);
}

bool FlexParticlePhysicsServer::primitive_body_is_kinematic(RID p_body) const {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_kinematic();
}

void FlexParticlePhysicsServer::primitive_body_set_as_area(RID p_body, bool p_area) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_area(p_area);
}

bool FlexParticlePhysicsServer::primitive_body_is_area(RID p_body) const {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_area();
}

void FlexParticlePhysicsServer::primitive_body_set_monitoring_particles_contacts(RID p_body, bool p_monitoring) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_monitoring_particles_contacts(p_monitoring);
}

bool FlexParticlePhysicsServer::primitive_body_is_monitoring_particles_contacts(RID p_body) const {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_monitoring_particles_contacts();
}

RID FlexParticlePhysicsServer::primitive_shape_create(PrimitiveShapeType p_type) {

	FlexPrimitiveShape *primitive_shape = NULL;
	switch (p_type) {
		case PARTICLE_PRIMITIVE_SHAPE_TYPE_BOX: {
			primitive_shape = memnew(FlexPrimitiveBoxShape);
		} break;
		case PARTICLE_PRIMITIVE_SHAPE_TYPE_CAPSULE: {
			primitive_shape = memnew(FlexPrimitiveCapsuleShape);
		} break;
		case PARTICLE_PRIMITIVE_SHAPE_TYPE_SPHERE: {
			primitive_shape = memnew(FlexPrimitiveSphereShape);
		} break;
		case PARTICLE_PRIMITIVE_SHAPE_TYPE_CONVEX: {
			primitive_shape = memnew(FlexPrimitiveConvexShape);
		} break;
		case PARTICLE_PRIMITIVE_SHAPE_TYPE_TRIMESH: {
			primitive_shape = memnew(FlexPrimitiveTriangleShape);
		} break;
	}

	if (primitive_shape) {
		CreateThenReturnRID(primitive_shape_owner, primitive_shape);
	} else {
		ERR_FAIL_V(RID());
	}
}

void FlexParticlePhysicsServer::primitive_shape_set_data(RID p_shape, const Variant &p_data) {
	FlexPrimitiveShape *shape = primitive_shape_owner.get(p_shape);
	ERR_FAIL_COND(!shape);

	shape->set_data(p_data);
}

Variant FlexParticlePhysicsServer::primitive_shape_get_data(RID p_shape) const {
	FlexPrimitiveShape *shape = primitive_shape_owner.get(p_shape);
	ERR_FAIL_COND_V(!shape, Variant());

	return shape->get_data();
}

void FlexParticlePhysicsServer::free(RID p_rid) {
	if (space_owner.owns(p_rid)) {

		FlexSpace *space = space_owner.get(p_rid);
		space_owner.free(p_rid);
		memdelete(space);

	} else if (body_owner.owns(p_rid)) {

		FlexParticleBody *body = body_owner.get(p_rid);
		body_owner.free(p_rid);
		memdelete(body);

	} else if (body_constraint_owner.owns(p_rid)) {

		FlexParticleBodyConstraint *constraint = body_constraint_owner.get(p_rid);
		body_constraint_owner.free(p_rid);
		memdelete(constraint);

	} else if (primitive_body_owner.owns(p_rid)) {

		FlexPrimitiveBody *primitive = primitive_body_owner.get(p_rid);
		primitive_body_owner.free(p_rid);
		memdelete(primitive);

	} else if (primitive_shape_owner.owns(p_rid)) {

		FlexPrimitiveShape *primitive_shape = primitive_shape_owner.get(p_rid);
		primitive_shape_owner.free(p_rid);
		memdelete(primitive_shape);

	} else {
		ERR_EXPLAIN("Can't delete RID, owner not found");
		ERR_FAIL();
	}
}

Ref<ParticleBodyModel> FlexParticlePhysicsServer::create_soft_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_global_stiffness, bool p_internal_sample, float p_particle_spacing, float p_sampling, float p_clusterSpacing, float p_clusterRadius, float p_clusterStiffness, float p_linkRadius, float p_linkStiffness, float p_plastic_threshold, float p_plastic_creep) {
	ERR_FAIL_COND_V(p_mesh.is_null(), Ref<ParticleBodyModel>());

	PoolVector<Vector3>::Read vertices_read = p_mesh->get_vertices().read();

	PoolVector<int> indices;
	p_mesh->get_indices(&indices);
	PoolVector<int>::Read indices_read = indices.read();

	NvFlexExtAsset *generated_assets = NvFlexExtCreateSoftFromMesh(
			((const float *)vertices_read.ptr()),
			p_mesh->get_vertices().size(),
			static_cast<const int *>(indices_read.ptr()),
			indices.size(),

			p_radius * p_particle_spacing, // Distance between 2 particle
			p_internal_sample ? p_sampling : 0.0, // (0-1) This parameter regulate the number of particle that should be put inside the mesh (in case of cloth it should be 0)
			p_internal_sample ? 0.0 : p_sampling, // (0-1) This parameter regulate the number of particle that should be put on the surface of mesh (in case of cloth it should be 1)
			p_clusterSpacing * p_radius,
			p_clusterRadius * p_radius,
			p_clusterStiffness,
			p_linkRadius * p_radius,
			p_linkStiffness,
			p_global_stiffness,
			p_plastic_threshold,
			p_plastic_creep);

	ERR_FAIL_COND_V(!generated_assets, Ref<ParticleBodyModel>());

	Ref<ParticleBodyModel> model = make_model(generated_assets);

	NvFlexExtDestroyAsset(generated_assets);
	generated_assets = NULL;

	return model;
}

Ref<ParticleBodyModel> FlexParticlePhysicsServer::create_cloth_particle_body_model(
		Ref<TriangleMesh> p_mesh,
		float p_stretch_stiffness,
		float p_bend_stiffness,
		float p_tether_stiffness,
		float p_tether_give,
		float p_pressure,
		bool p_allow_tearing) {

	ERR_FAIL_COND_V(p_mesh.is_null(), Ref<ParticleBodyModel>());

	PoolVector<FlVector4> welded_particles_positions;
	PoolVector<int> welded_particles_indices;

	{ // Merge all overlapping vertices
		PoolVector<Vector3>::Read mesh_vertices_read = p_mesh->get_vertices().read();

		PoolVector<int> mesh_indices;
		p_mesh->get_indices(&mesh_indices);
		const int mesh_index_count(mesh_indices.size());
		const int mesh_vertex_count(p_mesh->get_vertices().size());

		// A list of unique vertex index
		PoolVector<int> welded_vertex_indices;
		welded_vertex_indices.resize(mesh_vertex_count);

		// The list that map all vertex indices from original to unique
		PoolVector<int> original_to_unique;
		original_to_unique.resize(mesh_vertex_count);

		int unique_vertices(0);

		{ // Merge vertices
			PoolVector<int>::Write welded_vertex_indices_w = welded_vertex_indices.write();
			PoolVector<int>::Write original_to_unique_w = original_to_unique.write();

			unique_vertices = NvFlexExtCreateWeldedMeshIndices(
					(float *)mesh_vertices_read.ptr(),
					mesh_vertex_count,
					welded_vertex_indices_w.ptr(),
					original_to_unique_w.ptr(),
					0.00005);
		}

		// In this way I don't need to edit the mesh resource
		ERR_EXPLAIN("Please provide a mesh with no duplicate vertices");
		ERR_FAIL_COND_V(unique_vertices != mesh_vertex_count, Ref<ParticleBodyModel>());

		PoolVector<int>::Read mesh_indices_r = mesh_indices.read();
		PoolVector<int>::Read welded_vertex_indices_r = welded_vertex_indices.read();
		PoolVector<int>::Read original_to_unique_r = original_to_unique.read();

		welded_particles_positions.resize(unique_vertices);
		welded_particles_indices.resize(mesh_index_count);

		{ // Populate vertices and indices

			PoolVector<FlVector4>::Write welded_particles_w = welded_particles_positions.write();
			PoolVector<int>::Write welded_particles_indices_w = welded_particles_indices.write();

			for (int i(0); i < unique_vertices; ++i) {
				Vector3 pos(mesh_vertices_read[welded_vertex_indices_r[original_to_unique_r[i]]]);
				welded_particles_w[i] = make_particle(pos, 1);
			}

			for (int i(0); i < mesh_index_count; ++i) {
				welded_particles_indices_w[i] =
						welded_vertex_indices_r[original_to_unique_r[mesh_indices_r[i]]];
			}
		}
	}

	PoolVector<FlVector4>::Read vertices_particles_r = welded_particles_positions.read();
	PoolVector<int>::Read vertices_to_particles_r = welded_particles_indices.read();

	NvFlexExtAsset *generated_assets;
	if (!p_allow_tearing) {

		generated_assets = NvFlexExtCreateClothFromMesh(
				(float *)vertices_particles_r.ptr(),
				welded_particles_positions.size(),
				vertices_to_particles_r.ptr(),
				welded_particles_indices.size() / 3,
				p_stretch_stiffness,
				p_bend_stiffness,
				p_tether_stiffness,
				p_tether_give,
				p_pressure);
	} else {

		generated_assets = NvFlexExtCreateTearingClothFromMesh(
				(float *)vertices_particles_r.ptr(),
				welded_particles_positions.size(),
				welded_particles_positions.size() * 3,
				vertices_to_particles_r.ptr(),
				welded_particles_indices.size() / 3,
				p_stretch_stiffness,
				p_bend_stiffness,
				p_pressure);
	}

	ERR_FAIL_COND_V(!generated_assets, Ref<ParticleBodyModel>());

	Ref<ParticleBodyModel> model = make_model(generated_assets);

	model->set_allow_tearing(p_allow_tearing);

	NvFlexExtDestroyAsset(generated_assets);
	generated_assets = NULL;

	return model;
}

Ref<ParticleBodyModel> FlexParticlePhysicsServer::create_rigid_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_expand) {
	ERR_FAIL_COND_V(p_mesh.is_null(), Ref<ParticleBodyModel>());

	PoolVector<Vector3>::Read vertices_read = p_mesh->get_vertices().read();

	PoolVector<int> indices;
	p_mesh->get_indices(&indices);
	PoolVector<int>::Read indices_read = indices.read();

	NvFlexExtAsset *generated_assets = NvFlexExtCreateRigidFromMesh(
			((const float *)vertices_read.ptr()),
			p_mesh->get_vertices().size(),
			static_cast<const int *>(indices_read.ptr()),
			indices.size(),

			p_radius,
			p_expand);

	ERR_FAIL_COND_V(!generated_assets, Ref<ParticleBodyModel>());

	Ref<ParticleBodyModel> model = make_model(generated_assets);

	NvFlexExtDestroyAsset(generated_assets);
	generated_assets = NULL;

	return model;
}

Ref<ParticleBodyModel> FlexParticlePhysicsServer::create_thread_particle_body_model(real_t p_particle_radius, real_t p_extent, real_t p_spacing, real_t p_link_stiffness, int p_cluster_size, real_t p_cluster_stiffness, int p_attach_to_particle, Ref<ParticleBodyModel> p_current_model) {

	ERR_FAIL_COND_V(p_cluster_size < 2, NULL);

	real_t particle_radius = p_particle_radius * p_spacing;
	real_t size = p_extent * 2.f;

	Ref<ParticleBodyModel> model;

	bool want_to_attach = false;

	Vector3 position_offset;

	int particle_count_start = 0;
	int masses_count_start = 0;
	int constraint_indices_count_start = 0;
	int constraint_info_count_start = 0;
	int cluster_position_count_start = 0;
	int cluster_stiffness_count_start = 0;
	int cluster_plastic_thresold_count_start = 0;
	int cluster_plastic_creep_count_start = 0;
	int cluster_offset_count_start = 0;
	int cluster_particle_indices_counst_start = 0;

	if (p_current_model.is_valid() && p_attach_to_particle >= 0) {
		// Copy
		model = p_current_model->duplicate();

		want_to_attach = true;

		position_offset = model->get_particles().get(p_attach_to_particle);

		particle_count_start = model->get_particles().size();
		masses_count_start = model->get_masses().size();
		constraint_indices_count_start = model->get_constraints_indexes().size();
		constraint_info_count_start = model->get_constraints_info().size();
		cluster_position_count_start = model->get_clusters_positions().size();
		cluster_stiffness_count_start = model->get_clusters_stiffness().size();
		cluster_plastic_thresold_count_start = model->get_clusters_plastic_threshold().size();
		cluster_plastic_creep_count_start = model->get_clusters_plastic_creep().size();
		cluster_offset_count_start = model->get_clusters_offsets().size();
		cluster_particle_indices_counst_start = model->get_clusters_particle_indices().size();

	} else {
		model.instance();
	}

	int particle_count = (size / (particle_radius * 2)) + 1;

	/// Create particles
	PoolVector<Vector3> &particles = model->get_particles_ref();
	particles.resize(particle_count_start + particle_count);

	PoolVector<real_t> &masses = model->get_masses_ref();
	masses.resize(masses_count_start + particle_count);

	{
		PoolVector<Vector3>::Write particles_w = particles.write();
		PoolVector<real_t>::Write masses_w = masses.write();

		for (int i(0); i < particle_count; ++i) {
			particles_w[particle_count_start + i] = position_offset + Vector3(0, (particle_radius * i * 2), 0);
			masses_w[masses_count_start + i] = 1.f;
		}
	}

	model->set_particles(particles);
	model->set_masses(masses);

	/// Create clusters
	int cluster_count = particle_count / p_cluster_size;
	const int spare_particles = particle_count % p_cluster_size;

	if (1 < spare_particles) {
		++cluster_count;
	}

	PoolVector<Vector3> &clusters_positions = model->get_clusters_positions_ref();
	PoolVector<real_t> &cluster_stiffness = model->get_clusters_stiffness_ref();
	PoolVector<real_t> &cluster_plastic_thresold = model->get_clusters_plastic_threshold_ref();
	PoolVector<real_t> &cluster_plastic_creep = model->get_clusters_plastic_creep_ref();
	PoolVector<int> &cluster_offsets = model->get_clusters_offsets_ref();
	PoolVector<int> &cluster_indices = model->get_clusters_particle_indices_ref();

	clusters_positions.resize(cluster_position_count_start + cluster_count);
	cluster_stiffness.resize(cluster_stiffness_count_start + cluster_count);
	cluster_plastic_thresold.resize(cluster_plastic_thresold_count_start + cluster_count);
	cluster_plastic_creep.resize(cluster_plastic_creep_count_start + cluster_count);
	cluster_offsets.resize(cluster_offset_count_start + cluster_count);
	cluster_indices.resize(cluster_particle_indices_counst_start + cluster_count * p_cluster_size);

	{
		PoolVector<Vector3>::Read particles_r = particles.read();

		PoolVector<Vector3>::Write clusters_positions_w = clusters_positions.write();
		PoolVector<real_t>::Write cluster_stiffness_w = cluster_stiffness.write();
		PoolVector<real_t>::Write cluster_plastic_thresold_w = cluster_plastic_thresold.write();
		PoolVector<real_t>::Write cluster_plastic_creep_w = cluster_plastic_creep.write();
		PoolVector<int>::Write cluster_offsets_w = cluster_offsets.write();
		PoolVector<int>::Write cluster_indices_w = cluster_indices.write();

		for (int i(0); i < cluster_count; ++i) {

			AABB aabb(particles_r[particle_count_start + (i * p_cluster_size)], Vector3());
			for (int p(1); p < p_cluster_size; ++p) {
				aabb.expand_to(particles_r[particle_count_start + (i * p_cluster_size + p)]);
			}

			clusters_positions_w[cluster_position_count_start + i] = aabb.get_position() + (aabb.get_size() * 0.5);
			cluster_stiffness_w[cluster_stiffness_count_start + i] = p_cluster_stiffness;
			cluster_plastic_thresold_w[cluster_plastic_thresold_count_start + i] = 0;
			cluster_plastic_creep_w[cluster_plastic_creep_count_start + i] = 0;
			cluster_offsets_w[cluster_offset_count_start + i] = particle_count_start + (i * p_cluster_size + p_cluster_size);

			for (int p(0); p < p_cluster_size; ++p) {
				cluster_indices_w[cluster_particle_indices_counst_start + (i * p_cluster_size + p)] = particle_count_start + (i * p_cluster_size + p);
			}
		}
	}

	model->set_clusters_positions(clusters_positions);
	model->set_clusters_stiffness(cluster_stiffness);
	model->set_clusters_plastic_threshold(cluster_plastic_thresold);
	model->set_clusters_plastic_creep(cluster_plastic_creep);
	model->set_clusters_offsets(cluster_offsets);
	model->set_clusters_particle_indices(cluster_indices);

	/// Craete springs

	int springs_count = cluster_count - 1;

	if (1 == spare_particles) {
		++springs_count;
	}

	PoolVector<int> &springs_indices = model->get_constraints_indexes_ref();
	PoolVector<Vector2> &springs_info = model->get_constraints_info_ref();

	if (want_to_attach) {
		// Add 1 extra spring for Needle <-> Thread attachment
		springs_indices.resize(constraint_indices_count_start + (springs_count + 1) * 2);
		springs_info.resize(constraint_info_count_start + (springs_count + 1));
	} else {
		springs_indices.resize(constraint_indices_count_start + springs_count * 2);
		springs_info.resize(constraint_info_count_start + springs_count);
	}

	{
		PoolVector<int>::Write springs_indices_w = springs_indices.write();
		PoolVector<Vector2>::Write springs_info_w = springs_info.write();

		for (int i(0); i < springs_count; ++i) {
			springs_indices_w[constraint_indices_count_start + (i * 2 + 0)] = particle_count_start + (i * p_cluster_size + p_cluster_size - 1);
			springs_indices_w[constraint_indices_count_start + (i * 2 + 1)] = particle_count_start + (i * p_cluster_size + p_cluster_size);
			springs_info_w[constraint_info_count_start + i] = Vector2(particle_radius * 2, p_link_stiffness);
		}

		if (want_to_attach) {
			springs_indices_w[springs_count * 2 + 0] = p_attach_to_particle;
			springs_indices_w[springs_count * 2 + 1] = particle_count_start;
			springs_info_w[springs_count + 0] = Vector2(0, 1); // Needle <-> Thread no gap with max stiffness
		}
	}

	model->set_constraints_indexes(springs_indices);
	model->set_constraints_info(springs_info);

	return model;
}

Ref<ParticleBodyModel> FlexParticlePhysicsServer::make_model(NvFlexExtAsset *p_assets) {
	ERR_FAIL_COND_V(!p_assets, Ref<ParticleBodyModel>());

	Ref<ParticleBodyModel> model;
	model.instance();

	model->get_masses_ref().resize(p_assets->numParticles);
	model->get_particles_ref().resize(p_assets->numParticles);

	for (int i(0); i < p_assets->numParticles; ++i) {
		FlVector4 particle(((FlVector4 *)p_assets->particles)[i]);
		model->get_masses_ref().set(i, extract_inverse_mass(particle) == 0 ? 0.01 : 1 / extract_inverse_mass(particle));
		model->get_particles_ref().set(i, extract_position(particle));
	}

	model->get_constraints_indexes_ref().resize(p_assets->numSprings * 2);
	for (int i(0); i < model->get_constraints_indexes_ref().size(); ++i) {
		model->get_constraints_indexes_ref().set(i, p_assets->springIndices[i]);
	}

	model->get_constraints_info_ref().resize(p_assets->numSprings);
	for (int i(0); i < p_assets->numSprings; ++i) {
		model->get_constraints_info_ref().set(i, Vector2(p_assets->springRestLengths[i], p_assets->springCoefficients[i]));
	}

	model->get_clusters_offsets_ref().resize(p_assets->numShapes);
	model->get_clusters_positions_ref().resize(p_assets->numShapes);
	model->get_clusters_stiffness_ref().resize(p_assets->numShapes);
	model->get_clusters_plastic_threshold_ref().resize(p_assets->numShapes);
	model->get_clusters_plastic_creep_ref().resize(p_assets->numShapes);
	for (int i(0); i < p_assets->numShapes; ++i) {
		model->get_clusters_offsets_ref().set(i, p_assets->shapeOffsets[i]);
		model->get_clusters_positions_ref().set(i, ((Vector3 *)p_assets->shapeCenters)[i]);
		model->get_clusters_stiffness_ref().set(i, p_assets->shapeCoefficients[i]);
		model->get_clusters_plastic_threshold_ref().set(i, p_assets->shapePlasticThresholds ? p_assets->shapePlasticThresholds[i] : 0);
		model->get_clusters_plastic_creep_ref().set(i, p_assets->shapePlasticCreeps ? p_assets->shapePlasticCreeps[i] : 0);
	}

	model->get_clusters_particle_indices_ref().resize(p_assets->numShapeIndices);
	for (int i(0); i < p_assets->numShapeIndices; ++i) {
		model->get_clusters_particle_indices_ref().set(i, p_assets->shapeIndices[i]);
	}

	model->get_dynamic_triangles_indices_ref().resize(p_assets->numTriangles * 3);
	PoolVector<int>::Write dynamic_triangles_indices_w = model->get_dynamic_triangles_indices_ref().write();
	for (int i(0); i < p_assets->numTriangles; ++i) {
		dynamic_triangles_indices_w[i * 3 + 0] = p_assets->triangleIndices[i * 3 + 0];
		dynamic_triangles_indices_w[i * 3 + 1] = p_assets->triangleIndices[i * 3 + 1];
		dynamic_triangles_indices_w[i * 3 + 2] = p_assets->triangleIndices[i * 3 + 2];
	}

	model->set_want_inflatable(p_assets->inflatable);
	model->set_rest_volume(p_assets->inflatableVolume);
	model->set_constraint_scale(p_assets->inflatableStiffness);

	return model;
}

void FlexParticlePhysicsServer::create_skeleton(real_t weight_fall_off, real_t weight_max_distance, const Vector3 *bones_poses, int bone_count, const Vector3 *p_vertices, int p_vertex_count, PoolVector<float> *r_weights, PoolVector<int> *r_particle_indices, int *r_max_weight_per_vertex) {

	ERR_FAIL_COND(0 >= bone_count);
	ERR_FAIL_COND(0 >= p_vertex_count);

	*r_max_weight_per_vertex = 4;
	r_weights->resize(p_vertex_count * 4);
	r_particle_indices->resize(p_vertex_count * 4);

	PoolVector<float>::Write weight_w = r_weights->write();
	PoolVector<int>::Write indices_w = r_particle_indices->write();

	NvFlexExtCreateSoftMeshSkinning(
			((const float *)p_vertices),
			p_vertex_count,
			((const float *)bones_poses),
			bone_count,
			weight_fall_off,
			weight_max_distance,
			weight_w.ptr(),
			indices_w.ptr());
}

void FlexParticlePhysicsServer::init() {
	particle_body_commands = memnew(FlexParticleBodyCommands);
	particle_body_commands_variant = particle_body_commands;

	particle_body_constraint_commands = memnew(FlexParticleBodyConstraintCommands);
	particle_body_constraint_commands_variant = particle_body_constraint_commands;
}

void FlexParticlePhysicsServer::terminate() {

	memdelete(particle_body_commands);
	particle_body_commands = NULL;
	particle_body_commands_variant = Variant();

	memdelete(particle_body_constraint_commands);
	particle_body_constraint_commands = NULL;
	particle_body_constraint_commands_variant = Variant();
}

void FlexParticlePhysicsServer::set_active(bool p_active) {
	is_active = p_active;
}

void FlexParticlePhysicsServer::profile() {

#ifdef DEBUG_ENABLED
	if (!ScriptDebugger::get_singleton() || !ScriptDebugger::get_singleton()->is_profiling())
		return;
	{
		Array values;
		for (
				std::map<String, ProfileData>::const_iterator i = Profiler::get_data().begin();
				i != Profiler::get_data().end();
				++i) {

			values.push_back(i->first);
			values.push_back(i->second.get_execution_time_sec());
		}
		ScriptDebugger::get_singleton()->add_profiling_frame_data("FlexServer", values);
		Profiler::clear();
	}

	{
		Array values;
		values.push_back("flex_predict");
		values.push_back(timers.predict);
		values.push_back("flex_create_cell_indices");
		values.push_back(timers.createCellIndices);
		values.push_back("flex_sort_cell_indices");
		values.push_back(timers.sortCellIndices);
		values.push_back("flex_create_grid");
		values.push_back(timers.createGrid);
		values.push_back("flex_reorder");
		values.push_back(timers.reorder);
		values.push_back("flex_collide_particles");
		values.push_back(timers.collideParticles);
		values.push_back("flex_collide_shapes");
		values.push_back(timers.collideShapes);
		values.push_back("flex_collide_triangles");
		values.push_back(timers.collideTriangles);
		values.push_back("flex_collide_fields");
		values.push_back(timers.collideFields);
		values.push_back("flex_calculate_density");
		values.push_back(timers.calculateDensity);
		values.push_back("flex_solve_densities");
		values.push_back(timers.solveDensities);
		values.push_back("flex_solve_velocities");
		values.push_back(timers.solveVelocities);
		values.push_back("flex_solve_shapes");
		values.push_back(timers.solveShapes);
		values.push_back("flex_solve_springs");
		values.push_back(timers.solveSprings);
		values.push_back("flex_solve_contacts");
		values.push_back(timers.solveContacts);
		values.push_back("flex_solve_inflatables");
		values.push_back(timers.solveInflatables);
		values.push_back("flex_apply_deltas");
		values.push_back(timers.applyDeltas);
		values.push_back("flex_calculate_anisotropy");
		values.push_back(timers.calculateAnisotropy);
		values.push_back("flex_update_diffuse");
		values.push_back(timers.updateDiffuse);
		values.push_back("flex_update_triangles");
		values.push_back(timers.updateTriangles);
		values.push_back("flex_update_normals");
		values.push_back(timers.updateNormals);
		values.push_back("flex_finalize");
		values.push_back(timers.finalize);
		values.push_back("flex_update_bounds");
		values.push_back(timers.updateBounds);
		//values.push_back("flex_total");
		//values.push_back(timers.total);

		ScriptDebugger::get_singleton()->add_profiling_frame_data("Flex", values);

		clear_timers();
	}
#endif // ~DEBUG_ENABLED
}

void FlexParticlePhysicsServer::sync() {
	// Deactivated for now so useless
	//if (!is_active)
	//	return;
	//
	//for (short i = last_space_index; 0 <= i; --i) {
	//	active_spaces[i]->sync();
	//}
}

void FlexParticlePhysicsServer::flush_queries() {

#ifdef DEBUG_ENABLED
	if (!ScriptDebugger::get_singleton() || !ScriptDebugger::get_singleton()->is_profiling())
		return;

	for (short i = last_space_index; 0 <= i; --i) {
		NvFlexTimers solv_timers;
		NvFlexGetTimers(active_spaces[i]->get_solver(), &solv_timers);

		timers.predict += solv_timers.predict / 1000;
		timers.createCellIndices += solv_timers.createCellIndices / 1000;
		timers.sortCellIndices += solv_timers.sortCellIndices / 1000;
		timers.createGrid += solv_timers.createGrid / 1000;
		timers.reorder += solv_timers.reorder / 1000;
		timers.collideParticles += solv_timers.collideParticles / 1000;
		timers.collideShapes += solv_timers.collideShapes / 1000;
		timers.collideTriangles += solv_timers.collideTriangles / 1000;
		timers.collideFields += solv_timers.collideFields / 1000;
		timers.calculateDensity += solv_timers.calculateDensity / 1000;
		timers.solveDensities += solv_timers.solveDensities / 1000;
		timers.solveVelocities += solv_timers.solveVelocities / 1000;
		timers.solveShapes += solv_timers.solveShapes / 1000;
		timers.solveSprings += solv_timers.solveSprings / 1000;
		timers.solveContacts += solv_timers.solveContacts / 1000;
		timers.solveInflatables += solv_timers.solveInflatables / 1000;
		timers.applyDeltas += solv_timers.applyDeltas / 1000;
		timers.calculateAnisotropy += solv_timers.calculateAnisotropy / 1000;
		timers.updateDiffuse += solv_timers.updateDiffuse / 1000;
		timers.updateTriangles += solv_timers.updateTriangles / 1000;
		timers.updateNormals += solv_timers.updateNormals / 1000;
		timers.finalize += solv_timers.finalize / 1000;
		timers.updateBounds += solv_timers.updateBounds / 1000;
		//timers.total += solv_timers.total / 1000;
	}
#endif // ~DEBUG_ENABLED
}

void FlexParticlePhysicsServer::step(real_t p_delta_time) {

	if (!is_active)
		return;

	const bool profiling =
#ifdef DEBUG_ENABLED
			ScriptDebugger::get_singleton() &&
			ScriptDebugger::get_singleton()->is_profiling();
#else
			false;
#endif // ~ DEBUG_ENABLED

	delta_time = p_delta_time;

	for (short i = last_space_index; 0 <= i; --i) {
		active_spaces[i]->step(p_delta_time, profiling);
	}
}

int FlexParticlePhysicsServer::get_process_info(ProcessInfo p_info) {
	switch (p_info) {
		case INFO_ACTIVE_PRIMITIVES: {
			int c(0);
			for (short i = last_space_index; 0 <= i; --i) {
				c += active_spaces[i]->get_primitive_body_count();
			}
			return c;
		} break;
		case INFO_ACTIVE_PARTICLES: {
			int c(0);
			for (short i = last_space_index; 0 <= i; --i) {
				c += active_spaces[i]->get_particle_count();
			}
			return c;
		} break;
		case INFO_SOLVER_MAX_PARTICLES: {
			int c(0);
			for (short i = last_space_index; 0 <= i; --i) {
				c += active_spaces[i]->get_solver_max_particles();
			}
			return c;
		} break;
	}
	ERR_FAIL_V(0);
}

void FlexParticlePhysicsServer::clear_timers() {
	timers.predict = 0;
	timers.createCellIndices = 0;
	timers.sortCellIndices = 0;
	timers.createGrid = 0;
	timers.reorder = 0;
	timers.collideParticles = 0;
	timers.collideShapes = 0;
	timers.collideTriangles = 0;
	timers.collideFields = 0;
	timers.calculateDensity = 0;
	timers.solveDensities = 0;
	timers.solveVelocities = 0;
	timers.solveShapes = 0;
	timers.solveSprings = 0;
	timers.solveContacts = 0;
	timers.solveInflatables = 0;
	timers.applyDeltas = 0;
	timers.calculateAnisotropy = 0;
	timers.updateDiffuse = 0;
	timers.updateTriangles = 0;
	timers.updateNormals = 0;
	timers.finalize = 0;
	timers.updateBounds = 0;
	timers.total = 0;
}
