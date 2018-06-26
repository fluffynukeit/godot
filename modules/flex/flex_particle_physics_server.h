/*************************************************************************/
/*  flex_particle_physics_server.h                                       */
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

#ifndef FLEX_PARTICLE_PHYSICS_SERVER_H
#define FLEX_PARTICLE_PHYSICS_SERVER_H

#include "servers/particle_physics_server.h"

#include "core/math/triangle_mesh.h"
#include "flex_particle_body.h"
#include "flex_primitive_body.h"
#include "flex_primitive_shapes.h"
#include "flex_space.h"

/**
	@author AndreaCatania
*/

class FlexParticleBodyCommands : public ParticleBodyCommands {
	GDCLASS(FlexParticleBodyCommands, ParticleBodyCommands);

public:
	FlexParticleBody *body;

	virtual void load_model(Ref<ParticleBodyModel> p_model, const Transform &initial_transform);

	virtual void add_particle(const Vector3 &p_local_position, real_t p_mass);
	virtual void set_particle(ParticleIndex p_index, const Vector3 &p_local_position, real_t p_mass);

	virtual void add_spring(ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness);
	virtual void set_spring(SpringIndex p_index, ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness);

	virtual void add_triangle(ParticleIndex p_particle_0, ParticleIndex p_particle_1, ParticleIndex p_particle_2);
	virtual void set_triangle(TriangleIndex p_index, ParticleIndex p_particle_0, ParticleIndex p_particle_1, ParticleIndex p_particle_2);

	virtual void add_rigid(const Transform &p_transform, float p_stiffness, float p_plastic_threshold, float p_plastic_creep, RigidComponentIndex p_offset);
	virtual void set_rigid(RigidIndex p_index, const Transform &p_transform, float p_stiffness, float p_plastic_threshold, float p_plastic_creep, RigidComponentIndex p_offset);

	virtual void add_rigid_component(ParticleBufferIndex p_particle_index, const Vector3 &p_rest);
	virtual void set_rigid_component(RigidComponentIndex p_index, ParticleBufferIndex p_particle_index, const Vector3 &p_rest);

	virtual void reset_particle(int p_particle_index, const Vector3 &p_position, real_t p_mass);

	virtual Vector3 get_particle_position(int p_particle_index) const;

	virtual const Vector3 &get_particle_velocity(int p_particle_index) const;
	virtual void set_particle_velocity(int p_particle_index, const Vector3 &p_velocity);

	virtual const Vector3 &get_rigid_position(int p_index) const;
	virtual const Quat &get_rigid_rotation(int p_index) const;
};

class FlexParticlePhysicsServer : public ParticlePhysicsServer {
	GDCLASS(FlexParticlePhysicsServer, ParticlePhysicsServer);

public:
	static FlexParticlePhysicsServer *singleton;

private:
	mutable RID_Owner<FlexSpace> space_owner;
	mutable RID_Owner<FlexParticleBody> body_owner;
	mutable RID_Owner<FlexPrimitiveBody> primitive_body_owner;
	mutable RID_Owner<FlexPrimitiveShape> primitive_shape_owner;

	short last_space_index;
	Vector<FlexSpace *> active_spaces;

	bool is_active;
	FlexParticleBodyCommands *particle_body_commands;
	Variant particle_body_commands_variant;

public:
	FlexParticlePhysicsServer();
	virtual ~FlexParticlePhysicsServer();

	_FORCE_INLINE_ FlexParticleBodyCommands *get_particle_body_commands(FlexParticleBody *body) {
		particle_body_commands->body = body;
		return particle_body_commands;
	}

	_FORCE_INLINE_ Variant *get_particle_body_commands_variant(FlexParticleBody *body) {
		particle_body_commands->body = body;
		return &particle_body_commands_variant;
	}

	/* SPACE */
	virtual RID space_create();
	virtual void space_set_active(RID p_space, bool p_active);
	virtual bool space_is_active(RID p_space) const;

	virtual float space_get_particle_radius(RID p_space);

	/* BODY */
	virtual RID body_create();
	virtual void body_set_space(RID p_body, RID p_space);
	virtual void body_set_callback(RID p_body, ParticleBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method);

	virtual void body_set_collision_group(RID p_body, uint32_t p_group);
	virtual uint32_t body_get_collision_group(RID p_body) const;

	virtual void body_set_collision_flag(RID p_body, ParticleCollisionFlag p_flag, bool p_active);
	virtual bool body_get_collision_flag(RID p_body, ParticleCollisionFlag p_flag) const;

	virtual void body_set_collision_primitive_mask(RID p_body, uint32_t p_mask);
	virtual uint32_t body_get_collision_primitive_mask(RID p_body) const;

	virtual void body_add_particle(RID p_body, const Vector3 &p_local_position, real_t p_mass);
	virtual void body_remove_particle(RID p_body, int p_particle_index);

	virtual void body_remove_rigid(RID p_body, int p_rigid_index);

	virtual int body_get_particle_count(RID p_body) const;
	virtual int body_get_spring_count(RID p_body) const;
	virtual int body_get_rigid_count(RID p_body) const;

	/* PRIMITIVE BODY */
	virtual RID primitive_body_create();
	virtual void primitive_body_set_space(RID p_body, RID p_space);
	virtual void primitive_body_set_shape(RID p_body, RID p_shape);

	virtual void primitive_body_set_transform(RID p_body, const Transform &p_transf, bool p_teleport);

	virtual void primitive_body_set_collision_layer(RID p_body, uint32_t p_layer);
	virtual uint32_t primitive_body_get_collision_layer(RID p_body) const;

	virtual void primitive_body_set_kinematic(RID p_body, bool p_kinematic);
	virtual bool primitive_body_is_kinematic(RID p_body) const;

	/* PRIMITIVE SHAPE */
	virtual RID primitive_shape_create(ParticlePhysicsServer::PrimitiveShapeType p_type);

	virtual void primitive_shape_set_data(RID p_shape, const Variant &p_data);
	virtual Variant primitive_shape_get_data(RID p_shape) const;

	/* COMMON */
	virtual void free(RID p_rid);

	// This should be on physics server
	virtual Ref<ParticleBodyModel> create_soft_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_global_stiffness, bool p_internal_sample, float p_particle_spacing, float p_sampling, float p_clusterSpacing, float p_clusterRadius, float p_clusterStiffness, float p_linkRadius, float p_linkStiffness, float p_plastic_threshold, float p_plastic_creep);
	virtual Ref<ParticleBodyModel> create_cloth_particle_body_model(Ref<TriangleMesh> p_mesh, float p_stretch_stiffness, float p_bend_stiffness, float p_tether_stiffness, float p_tether_give, float p_pressure);
	virtual Ref<ParticleBodyModel> create_rigid_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_expand);
	Ref<ParticleBodyModel> make_model(class NvFlexExtAsset *p_asset);
	virtual void create_skeleton(const Vector3 *bones_poses, int bone_count, const Vector3 *p_vertices, int p_vertex_count, PoolVector<float> *r_weights, PoolVector<int> *r_particle_indices, int *r_max_weight_per_vertex);

	virtual void init();
	virtual void terminate();
	virtual void set_active(bool p_active);
	virtual void sync();
	virtual void flush_queries();
	virtual void step(real_t p_delta_time);
};

#endif // FLEX_PARTICLE_PHYSICS_SERVER_H
