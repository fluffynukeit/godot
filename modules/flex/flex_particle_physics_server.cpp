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

void FlexParticleBodyCommands::load_shape(Ref<ParticleShape> p_shape, const Transform &initial_transform) {
	body->load_shape(p_shape, initial_transform);
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

void FlexParticlePhysicsServer::free(RID p_rid) {
	if (space_owner.owns(p_rid)) {
		FlexSpace *space = space_owner.get(p_rid);
		space_owner.free(p_rid);
		memdelete(space);
	} else if (body_owner.owns(p_rid)) {
		FlexParticleBody *body = body_owner.get(p_rid);
		body_owner.free(p_rid);
		memdelete(body);
	} else {
		ERR_EXPLAIN("Can't delete RID, owner not found");
		ERR_FAIL();
	}
}

Ref<ParticleShape> FlexParticlePhysicsServer::create_soft_particle_shape(Ref<TriangleMesh> p_mesh) {
	ERR_FAIL_COND_V(p_mesh.is_null(), Ref<ParticleShape>());

	PoolVector<Vector3>::Read vertices_read = p_mesh->get_vertices().read();

	PoolVector<int> indices;
	p_mesh->get_indices(&indices);
	PoolVector<int>::Read indices_read = indices.read();

	NvFlexExtAsset *generated_assets = NvFlexExtCreateSoftFromMesh(
			((const float *)vertices_read.ptr()),
			p_mesh->get_vertices().size(),
			static_cast<const int *>(indices_read.ptr()),
			indices.size(),
			0.2,
			0.5,
			0.5,
			0.3,
			0.5,
			0.5,
			0.1,
			0.5,
			0.5,
			0.5,
			0.5);

	ERR_FAIL_COND_V(!generated_assets, Ref<ParticleShape>());

	Ref<ParticleShape> shape;
	shape.instance();

	shape->get_particles().resize(generated_assets->numParticles);

	for (int i(0); i < generated_assets->numParticles; ++i) {

		FlVector4 particle(((FlVector4 *)generated_assets->particles)[i]);
		shape->get_particles()[i].mass = particle.w;
		shape->get_particles()[i].relative_position.x = particle.x;
		shape->get_particles()[i].relative_position.y = particle.y;
		shape->get_particles()[i].relative_position.z = particle.z;
	}

	shape->get_constraints().resize(generated_assets->numSprings);
	for (int i(0); i < generated_assets->numSprings; ++i) {

		Spring spring(((Spring *)generated_assets->springIndices)[i]);
		shape->get_constraints()[i].particle_index_0 = spring.index0;
		shape->get_constraints()[i].particle_index_1 = spring.index1;
		shape->get_constraints()[i].length = generated_assets->springRestLengths[i];
		shape->get_constraints()[i].stiffness = generated_assets->springCoefficients[i];
	}

	NvFlexExtDestroyAsset(generated_assets);
	generated_assets = NULL;

	return shape;
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
