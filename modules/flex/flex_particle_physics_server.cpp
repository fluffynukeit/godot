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

#include "thirdparty/flex/include/NvFlex.h"
#include "thirdparty/flex/include/NvFlexExt.h"

/**
	@author AndreaCatania
*/

#define CreateThenReturnRID(owner, rid_data) \
	RID rid = owner.make_rid(rid_data);      \
	rid_data->__set_self(rid);               \
	rid_data->__set_physics_server(this);    \
	return rid;

void FlexParticleBodyCommands::load_model(Ref<ParticleBodyModel> p_model, const Transform &initial_transform) {
	if (p_model.is_null())
		return;

	PoolVector<Vector3>::Read particle_positions_r = p_model->get_particles().read();

	{ // Particle
		const int resource_p_count(p_model->get_particles_ref().size());
		body->space->particles_allocator->resize_chunk(body->particles_mchunk, resource_p_count);

		PoolVector<real_t>::Read masses_r = p_model->get_masses().read();

		for (int i(0); i < resource_p_count; ++i) {
			set_particle(i, initial_transform.xform(particle_positions_r[i]), masses_r[i]);
		}
	}

	{ // Spring
		const int resource_s_count(p_model->get_constraints_indexes_ref().size() / 2);
		body->space->springs_allocator->resize_chunk(body->springs_mchunk, resource_s_count);

		for (int i(0); i < resource_s_count; ++i) {
			set_spring(i,
					p_model->get_constraints_indexes_ref().get(i * 2),
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
}

void FlexParticleBodyCommands::add_particle(const Vector3 &p_local_position, real_t p_mass) {
	const int previous_size = body->get_particle_count();
	body->space->particles_allocator->resize_chunk(body->particles_mchunk, previous_size + 1);
	set_particle(previous_size, p_local_position, p_mass);
}

void FlexParticleBodyCommands::set_particle(ParticleIndex p_index, const Vector3 &p_local_position, real_t p_mass) {

	ERR_FAIL_COND(!body->is_owner_of_particle(p_index));

	body->space->particles_memory->set_particle(body->particles_mchunk, p_index, make_particle(p_local_position, p_mass));
	body->space->particles_memory->set_velocity(body->particles_mchunk, p_index, Vector3());
	body->space->particles_memory->set_phase(body->particles_mchunk, p_index, NvFlexMakePhaseWithChannels(body->collision_group, body->collision_flags, body->collision_primitive_mask));
	body->changed_parameters |= eChangedBodyParamParticleJustAdded;
}

void FlexParticleBodyCommands::add_spring(ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness) {
	const int previous_size = body->get_spring_count();
	body->space->springs_allocator->resize_chunk(body->springs_mchunk, previous_size + 1);
	set_spring(previous_size, p_particle_0, p_particle_1, p_length, p_stiffness);
}

void FlexParticleBodyCommands::set_spring(SpringIndex p_index, ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness) {

	ERR_FAIL_COND(!body->is_owner_of_spring(p_index));

	body->space->get_springs_memory()->set_spring(body->springs_mchunk, p_index, Spring(body->particles_mchunk->get_buffer_index(p_particle_0), body->particles_mchunk->get_buffer_index(p_particle_1)));
	body->space->get_springs_memory()->set_length(body->springs_mchunk, p_index, p_length);
	body->space->get_springs_memory()->set_stiffness(body->springs_mchunk, p_index, p_stiffness);
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

void FlexParticleBodyCommands::reset_particle(int p_particle_index, const Vector3 &p_position, real_t p_mass) {
	body->reset_particle(p_particle_index, p_position, p_mass);
}

Vector3 FlexParticleBodyCommands::get_particle_position(int p_particle_index) const {
	return body->get_particle_position(p_particle_index);
}

const Vector3 &FlexParticleBodyCommands::get_particle_velocity(int p_particle_index) const {
	return body->get_particle_velocity(p_particle_index);
}

void FlexParticleBodyCommands::set_particle_velocity(int p_particle_index, const Vector3 &p_velocity) {
	body->set_particle_velocity(p_particle_index, p_velocity);
}

const Vector3 &FlexParticleBodyCommands::get_rigid_position(int p_index) const {
	return body->get_rigid_position(p_index);
}

const Quat &FlexParticleBodyCommands::get_rigid_rotation(int p_index) const {
	return body->get_rigid_rotation(p_index);
}

FlexParticlePhysicsServer *FlexParticlePhysicsServer::singleton = NULL;

FlexParticlePhysicsServer::FlexParticlePhysicsServer() :
		ParticlePhysicsServer(),
		is_active(true),
		last_space_index(-1) {

	ERR_FAIL_COND(singleton);
	singleton = this;
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
		active_spaces.erase(space);
	}
	last_space_index = static_cast<short>(active_spaces.size() - 1);
}

bool FlexParticlePhysicsServer::space_is_active(RID p_space) const {
	const FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, false);

	return active_spaces.find(space) != -1;
}

void FlexParticlePhysicsServer::space_get_params_defaults(Map<String, Variant> *r_defs) const {
	(*r_defs)["numIterations"] = 3;
	(*r_defs)["gravity"] = Vector3(0, -10, 0);
	(*r_defs)["radius"] = 0.1f;
	(*r_defs)["solidRestDistance"] = real_t((*r_defs)["radius"]) * 0.9;
	(*r_defs)["fluidRestDistance"] = real_t((*r_defs)["radius"]) * 0.5;
	(*r_defs)["dynamicFriction"] = 0.1;
	(*r_defs)["staticFriction"] = 0.1;
	(*r_defs)["particleFriction"] = 0.1;
	(*r_defs)["maxSpeed"] = FLT_MAX;
	(*r_defs)["maxAcceleration"] = Vector3((*r_defs)["gravity"]).length() * 10.0;
	(*r_defs)["solidPressure"] = 1.0;
	(*r_defs)["collisionDistance"] = MAX(real_t((*r_defs)["solidRestDistance"]), real_t((*r_defs)["fluidRestDistance"])) * 0.5;
	(*r_defs)["shapeCollisionMargin"] = real_t((*r_defs)["collisionDistance"]) * 0.5;
	(*r_defs)["relaxationMode"] = "global";
	(*r_defs)["relaxationFactor"] = 0.8;
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

void FlexParticlePhysicsServer::body_remove_particle(RID p_body, int p_particle_index) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

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

void FlexParticlePhysicsServer::body_set_monitoring_primitives(RID p_body, bool p_monitoring) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_monitoring_primitives(p_monitoring);
}

bool FlexParticlePhysicsServer::body_is_monitoring_primitives(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_monitoring_primitives();
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

void FlexParticlePhysicsServer::primitive_body_set_monitoring_particles(RID p_body, bool p_monitoring) {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->set_monitoring_particles(p_monitoring);
}

bool FlexParticlePhysicsServer::primitive_body_is_monitoring_particles(RID p_body) const {
	FlexPrimitiveBody *body = primitive_body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, false);

	return body->is_monitoring_particles();
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

Ref<ParticleBodyModel> FlexParticlePhysicsServer::create_cloth_particle_body_model(Ref<TriangleMesh> p_mesh, float p_stretch_stiffness, float p_bend_stiffness, float p_tether_stiffness, float p_tether_give, float p_pressure) {
	ERR_FAIL_COND_V(p_mesh.is_null(), Ref<ParticleBodyModel>());

	PoolVector<FlVector4> welded_particles;
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

		PoolVector<int>::Read mesh_indices_r = mesh_indices.read();
		PoolVector<int>::Read welded_vertex_indices_r = welded_vertex_indices.read();
		PoolVector<int>::Read original_to_unique_r = original_to_unique.read();

		welded_particles.resize(unique_vertices);
		welded_particles_indices.resize(mesh_index_count);

		{ // Populate vertices and indices
			PoolVector<FlVector4>::Write welded_particles_w = welded_particles.write();
			PoolVector<int>::Write welded_particles_indices_w = welded_particles_indices.write();

			for (int i(0); i < unique_vertices; ++i) {
				Vector3 pos(mesh_vertices_read[welded_vertex_indices_r[original_to_unique_r[i]]]);
				welded_particles_w[i] = make_particle(pos, 1);
			}

			for (int i(0); i < mesh_index_count; ++i) {
				welded_particles_indices_w[i] = welded_vertex_indices_r[original_to_unique_r[mesh_indices_r[i]]];
			}
		}
	}

	PoolVector<FlVector4>::Read welded_vertices_r = welded_particles.read();
	PoolVector<int>::Read welded_indices_r = welded_particles_indices.read();

	NvFlexExtAsset *generated_assets = NvFlexExtCreateClothFromMesh(
			(float *)welded_vertices_r.ptr(),
			welded_particles.size(),
			welded_indices_r.ptr(),
			welded_particles_indices.size() / 3,
			p_stretch_stiffness,
			p_bend_stiffness,
			p_tether_stiffness,
			p_tether_give,
			p_pressure);

	ERR_FAIL_COND_V(!generated_assets, Ref<ParticleBodyModel>());

	Ref<ParticleBodyModel> model = make_model(generated_assets);

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

Ref<ParticleBodyModel> FlexParticlePhysicsServer::make_model(NvFlexExtAsset *p_assets) {
	ERR_FAIL_COND_V(!p_assets, Ref<ParticleBodyModel>());

	Ref<ParticleBodyModel> model;
	model.instance();

	model->get_masses_ref().resize(p_assets->numParticles);
	model->get_particles_ref().resize(p_assets->numParticles);

	for (int i(0); i < p_assets->numParticles; ++i) {
		FlVector4 particle(((FlVector4 *)p_assets->particles)[i]);
		model->get_masses_ref().set(i, extract_mass(particle) == 0 ? 0.01 : 1 / extract_mass(particle));
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

void FlexParticlePhysicsServer::create_skeleton(const Vector3 *bones_poses, int bone_count, const Vector3 *p_vertices, int p_vertex_count, PoolVector<float> *r_weights, PoolVector<int> *r_particle_indices, int *r_max_weight_per_vertex) {

	ERR_FAIL_COND(0 >= bone_count);
	ERR_FAIL_COND(0 >= p_vertex_count);

	float falloff = 10.f;
	float max_distance = 100.f;

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
			falloff,
			max_distance,
			weight_w.ptr(),
			indices_w.ptr());
}

void FlexParticlePhysicsServer::init() {
	particle_body_commands = memnew(FlexParticleBodyCommands);
	particle_body_commands_variant = particle_body_commands;
}

void FlexParticlePhysicsServer::terminate() {
	memdelete(particle_body_commands);
	particle_body_commands = NULL;
	particle_body_commands_variant = Variant();
}

void FlexParticlePhysicsServer::set_active(bool p_active) {
	is_active = p_active;
}

void FlexParticlePhysicsServer::sync() {

	if (!is_active)
		return;

	for (short i = last_space_index; 0 <= i; --i) {
		active_spaces[i]->sync();
	}
}

void FlexParticlePhysicsServer::flush_queries() {}

void FlexParticlePhysicsServer::step(real_t p_delta_time) {

	if (!is_active)
		return;

	for (short i = last_space_index; 0 <= i; --i) {
		active_spaces[i]->step(p_delta_time);
	}
}
