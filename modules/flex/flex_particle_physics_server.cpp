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

float FlexParticlePhysicsServer::space_get_particle_radius(RID p_space) {
	const FlexSpace *space = space_owner.get(p_space);
	ERR_FAIL_COND_V(!space, false);

	return space->get_particle_radius();
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

int FlexParticlePhysicsServer::body_get_rigid_count(RID p_body) const {
	FlexParticleBody *body = body_owner.get(p_body);
	ERR_FAIL_COND_V(!body, 0);

	return body->get_rigids_count();
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

Ref<ParticleBodyModel> FlexParticlePhysicsServer::create_soft_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_global_stiffness, bool p_cloth, float p_particle_spacing, float p_sampling, float p_clusterSpacing, float p_clusterRadius, float p_clusterStiffness, float p_linkRadius, float p_linkStiffness, float p_plastic_threshold, float p_plastic_creep) {
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
			p_cloth ? 0.0 : p_sampling, // (0-1) This parameter regulate the number of particle that should be put inside the mesh (in case of cloth it should be 0)
			p_cloth ? p_sampling : 0.0, // (0-1) This parameter regulate the number of particle that should be put on the surface of mesh (in case of cloth it should be 1)
			p_clusterSpacing * p_radius,
			p_clusterRadius * p_radius,
			p_clusterStiffness,
			p_linkRadius * p_radius,
			p_linkStiffness,
			p_global_stiffness,
			p_plastic_threshold,
			p_plastic_creep);

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

	return model;
}

void FlexParticlePhysicsServer::create_skeleton(const Vector3 *bones_poses, int bone_count, const Vector3 *p_vertices, int p_vertex_count, PoolVector<float> *r_weights, PoolVector<int> *r_particle_indices, int *r_max_weight_per_vertex) {

	ERR_FAIL_COND(0 >= bone_count);
	ERR_FAIL_COND(0 >= p_vertex_count);

	float radius = 0.1;

	float falloff = radius * 0.5f;
	float max_distance = radius * 2.f;

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
