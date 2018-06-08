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
	body->load_model(p_model, initial_transform);
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

bool FlexParticlePhysicsServer::space_is_active(const RID p_space) const {
	const FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, false);

	return active_spaces.find(space) != -1;
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

void FlexParticlePhysicsServer::body_set_collision_layer(RID p_body, uint32_t p_layer) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);
	body->set_collision_group(p_layer);
}

uint32_t FlexParticlePhysicsServer::body_get_collision_layer(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_collision_group();
}

void FlexParticlePhysicsServer::body_add_particle(RID p_body, const Vector3 &p_local_position, real_t p_mass) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->add_particle(p_local_position, p_mass);
}

void FlexParticlePhysicsServer::body_remove_particle(RID p_body, int p_particle_index) {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND(!body);

	body->remove_particle(p_particle_index);
}

int FlexParticlePhysicsServer::body_get_particle_count(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_particle_count();
}

RID FlexParticlePhysicsServer::primitive_body_create() {
	FlexPrimitiveBody *primitive = memnew(FlexPrimitiveBody);
	CreateThenReturnRID(primitive_body_owner, primitive);
}

RID FlexParticlePhysicsServer::primitive_shape_create() {
	FlexPrimitiveShape *primitive_shape = memnew(FlexPrimitiveShape);
	CreateThenReturnRID(primitive_shape_owner, primitive_shape);
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

Ref<ParticleBodyModel> FlexParticlePhysicsServer::create_soft_particle_body_model(Ref<TriangleMesh> p_mesh, bool p_cloth, float p_sampling, float p_clusterSpacing, float p_clusterRadius, float p_clusterStiffness, float p_linkRadius, float p_linkStiffness) {
	ERR_FAIL_COND_V(p_mesh.is_null(), Ref<ParticleBodyModel>());

	PoolVector<Vector3>::Read vertices_read = p_mesh->get_vertices().read();

	PoolVector<int> indices;
	p_mesh->get_indices(&indices);
	PoolVector<int>::Read indices_read = indices.read();

	float radius = 0.1;
	float globalStiffness = 0;

	NvFlexExtAsset *generated_assets = NvFlexExtCreateSoftFromMesh(
			/*vertices*/ ((const float *)vertices_read.ptr()),
			/*numVertices*/ p_mesh->get_vertices().size(),
			/*indices*/ static_cast<const int *>(indices_read.ptr()),
			/*numIndices*/ indices.size(),

			/*particleSpacing*/ radius, // Distance between 2 particle
			/*volumeSampling*/ p_cloth ? 0.0 : p_sampling, // (0-1) This parameter regulate the number of particle that should be put inside the mesh (in case of cloth it should be 0)
			/*surfaceSampling*/ p_cloth ? p_sampling : 0.0, // (0-1) This parameter regulate the number of particle that should be put on the surface of mesh (in case of cloth it should be 1)
			/*clusterSpacing*/ p_clusterSpacing * radius,
			/*clusterRadius*/ p_clusterRadius * radius,
			/*clusterStiffness*/ p_clusterStiffness,
			/*linkRadius*/ p_linkRadius * radius,
			/*linkStiffness*/ p_linkStiffness,
			/*globalStiffness*/ globalStiffness,
			/*clusterPlasticThreshold*/ 0.0,
			/*clusterPlasticCreep*/ 0.0);

	ERR_FAIL_COND_V(!generated_assets, Ref<ParticleBodyModel>());

	Ref<ParticleBodyModel> model;
	model.instance();

	model->get_masses_ref().resize(generated_assets->numParticles);
	model->get_particles_ref().resize(generated_assets->numParticles);

	for (int i(0); i < generated_assets->numParticles; ++i) {
		FlVector4 particle(((FlVector4 *)generated_assets->particles)[i]);
		model->get_masses_ref().set(i, extract_mass(particle) == 0 ? 0.01 : 1 / extract_mass(particle));
		model->get_particles_ref().set(i, extract_position(particle));
	}

	model->get_constraints_indexes_ref().resize(generated_assets->numSprings * 2);
	for (int i(0); i < model->get_constraints_indexes_ref().size(); ++i) {
		model->get_constraints_indexes_ref().set(i, generated_assets->springIndices[i]);
	}

	model->get_constraints_info_ref().resize(generated_assets->numSprings);
	for (int i(0); i < generated_assets->numSprings; ++i) {
		model->get_constraints_info_ref().set(i, Vector2(generated_assets->springRestLengths[i], generated_assets->springCoefficients[i]));
	}

	NvFlexExtDestroyAsset(generated_assets);
	generated_assets = NULL;

	return model;
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
