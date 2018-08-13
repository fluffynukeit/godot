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
#include "flex_particle_body_constraint.h"
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

	virtual void triangles_set_count(int p_count);
	virtual void add_triangle(ParticleIndex p_particle_0, ParticleIndex p_particle_1, ParticleIndex p_particle_2);
	virtual void set_triangle(TriangleIndex p_index, ParticleIndex p_particle_0, ParticleIndex p_particle_1, ParticleIndex p_particle_2);

	virtual void add_rigid(const Transform &p_transform, float p_stiffness, float p_plastic_threshold, float p_plastic_creep, RigidComponentIndex p_offset);
	virtual void set_rigid(RigidIndex p_index, const Transform &p_transform, float p_stiffness, float p_plastic_threshold, float p_plastic_creep, RigidComponentIndex p_offset);

	virtual void add_rigid_component(ParticleBufferIndex p_particle_index, const Vector3 &p_rest);
	virtual void set_rigid_component(RigidComponentIndex p_index, ParticleBufferIndex p_particle_index, const Vector3 &p_rest);

	virtual void set_particle_position_mass(int p_particle_index, const Vector3 &p_position, real_t p_mass);

	virtual int get_particle_count() const;

	virtual void set_particle_position(int p_particle_index, const Vector3 &p_position);
	virtual Vector3 get_particle_position(int p_particle_index) const;

	virtual void set_particle_mass(int p_particle_index, real_t p_mass);
	virtual float get_particle_mass(int p_particle_index) const;

	virtual const Vector3 &get_particle_velocity(int p_particle_index) const;
	virtual void set_particle_velocity(int p_particle_index, const Vector3 &p_velocity);

	virtual Vector3 get_particle_normal(int p_index) const;
	virtual void set_particle_normal(int p_index, const Vector3 &p_normal);

	virtual const Vector3 &get_rigid_position(int p_index) const;
	virtual const Quat &get_rigid_rotation(int p_index) const;

	virtual AABB get_aabb() const;
};

class FlexParticleBodyConstraintCommands : public ParticleBodyConstraintCommands {
	GDCLASS(FlexParticleBodyConstraintCommands, ParticleBodyConstraintCommands);

public:
	FlexParticleBodyConstraint *constraint;

	virtual int get_spring_count() const;
	virtual int add_spring(int p_body0_particle, int p_body1_particle, float p_length, float p_stiffness);
	virtual void set_spring(int p_index, int p_body0_particle, int p_body1_particle, float p_length, float p_stiffness);
	virtual real_t get_distance(int p_body0_particle, int p_body1_particle);
};

class FlexParticlePhysicsServer : public ParticlePhysicsServer {
	GDCLASS(FlexParticlePhysicsServer, ParticlePhysicsServer);

public:
	static FlexParticlePhysicsServer *singleton;

	const StringName solver_param_numIterations;
	const StringName solver_param_gravity;
	const StringName solver_param_radius;
	const StringName solver_param_solidRestDistance;
	const StringName solver_param_fluidRestDistance;
	const StringName solver_param_dynamicFriction;
	const StringName solver_param_staticFriction;
	const StringName solver_param_particleFriction;
	const StringName solver_param_restitution;
	const StringName solver_param_adhesion;
	const StringName solver_param_sleepThreshold;
	const StringName solver_param_maxSpeed;
	const StringName solver_param_maxAcceleration;
	const StringName solver_param_shockPropagation;
	const StringName solver_param_dissipation;
	const StringName solver_param_damping;
	const StringName solver_param_wind;
	const StringName solver_param_drag;
	const StringName solver_param_lift;
	const StringName solver_param_cohesion;
	const StringName solver_param_surfaceTension;
	const StringName solver_param_viscosity;
	const StringName solver_param_vorticityConfinement;
	const StringName solver_param_anisotropyScale;
	const StringName solver_param_anisotropyMin;
	const StringName solver_param_anisotropyMax;
	const StringName solver_param_smoothing;
	const StringName solver_param_solidPressure;
	const StringName solver_param_freeSurfaceDrag;
	const StringName solver_param_buoyancy;
	const StringName solver_param_diffuseThreshold;
	const StringName solver_param_diffuseBuoyancy;
	const StringName solver_param_diffuseDrag;
	const StringName solver_param_diffuseBallistic;
	const StringName solver_param_diffuseLifetime;
	const StringName solver_param_collisionDistance;
	const StringName solver_param_particleCollisionMargin;
	const StringName solver_param_shapeCollisionMargin;
	const StringName solver_param_relaxationMode;
	const StringName solver_param_relaxationFactor;

private:
	mutable RID_Owner<FlexSpace> space_owner;
	mutable RID_Owner<FlexParticleBody> body_owner;
	mutable RID_Owner<FlexParticleBodyConstraint> body_constraint_owner;
	mutable RID_Owner<FlexPrimitiveBody> primitive_body_owner;
	mutable RID_Owner<FlexPrimitiveShape> primitive_shape_owner;

	short last_space_index;
	Vector<FlexSpace *> active_spaces;

	bool is_active;
	FlexParticleBodyCommands *particle_body_commands;
	Variant particle_body_commands_variant;

	FlexParticleBodyConstraintCommands *particle_body_constraint_commands;
	Variant particle_body_constraint_commands_variant;

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

	_FORCE_INLINE_ FlexParticleBodyConstraintCommands *get_particle_body_constraint_commands(FlexParticleBodyConstraint *p_constraint) {
		particle_body_constraint_commands->constraint = p_constraint;
		return particle_body_constraint_commands;
	}

	_FORCE_INLINE_ Variant *get_particle_body_constraint_commands_variant(FlexParticleBodyConstraint *p_constraint) {
		particle_body_constraint_commands->constraint = p_constraint;
		return &particle_body_constraint_commands_variant;
	}

	/* SPACE */
	virtual RID space_create();
	virtual void space_set_active(RID p_space, bool p_active);
	virtual bool space_is_active(RID p_space) const;

	virtual void space_get_params_defaults(Map<StringName, Variant> *r_defs) const;
	virtual bool space_set_param(RID p_space, const StringName &p_name, const Variant &p_property);
	virtual bool space_get_param(RID p_space, const StringName &p_name, Variant &r_property) const;
	virtual void space_reset_params_to_default(RID p_space);
	virtual bool space_is_using_default_params(RID p_space) const;

public:
	/* BODY */
	virtual RID body_create();
	virtual void body_set_space(RID p_body, RID p_space);
	virtual void body_set_callback(RID p_body, ParticleBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method);
	virtual void body_set_object_instance(RID p_body, Object *p_object);

	virtual ParticleBodyCommands *body_get_commands(RID p_body);

	virtual void body_set_collision_group(RID p_body, uint32_t p_group);
	virtual uint32_t body_get_collision_group(RID p_body) const;

	virtual void body_set_collision_flag(RID p_body, ParticleCollisionFlag p_flag, bool p_active);
	virtual bool body_get_collision_flag(RID p_body, ParticleCollisionFlag p_flag) const;

	virtual void body_set_collision_primitive_mask(RID p_body, uint32_t p_mask);
	virtual uint32_t body_get_collision_primitive_mask(RID p_body) const;

	virtual void body_remove_particle(RID p_body, int p_particle_index);

	virtual void body_remove_rigid(RID p_body, int p_rigid_index);

	virtual int body_get_particle_count(RID p_body) const;
	virtual int body_get_spring_count(RID p_body) const;
	virtual int body_get_rigid_count(RID p_body) const;

	virtual void body_set_pressure(RID p_body, float p_pressure);
	virtual float body_get_pressure(RID p_body) const;

	virtual bool body_can_rendered_using_skeleton(RID p_body) const;
	virtual bool body_can_rendered_using_pvparticles(RID p_body) const;

	virtual void body_set_monitorable(RID p_body, bool p_monitorable);
	virtual bool body_is_monitorable(RID p_body) const;

	virtual void body_set_monitoring_primitives_contacts(RID p_body, bool p_monitoring);
	virtual bool body_is_monitoring_primitives_contacts(RID p_body) const;

	/* BODY CONSTRAINT */
	virtual RID constraint_create(RID p_body0, RID p_body1);

	virtual void constraint_set_callback(RID p_constraint, Object *p_receiver, const StringName &p_method);
	virtual void constraint_set_space(RID p_constraint, RID p_space);

	virtual void constraint_remove_spring(RID p_constraint, int p_spring_index);

	/* PRIMITIVE BODY */
	virtual RID primitive_body_create();
	virtual void primitive_body_set_space(RID p_body, RID p_space);
	virtual void primitive_body_set_shape(RID p_body, RID p_shape);

	virtual void primitive_body_set_callback(RID p_body, ParticlePrimitiveBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method);
	virtual void primitive_body_set_object_instance(RID p_body, Object *p_object);

	virtual void primitive_body_set_transform(RID p_body, const Transform &p_transf, bool p_teleport);

	virtual void primitive_body_set_collision_layer(RID p_body, uint32_t p_layer);
	virtual uint32_t primitive_body_get_collision_layer(RID p_body) const;

	virtual void primitive_body_set_kinematic(RID p_body, bool p_kinematic);
	virtual bool primitive_body_is_kinematic(RID p_body) const;

	virtual void primitive_body_set_as_area(RID p_body, bool p_area);
	virtual bool primitive_body_is_area(RID p_body) const;

	virtual void primitive_body_set_monitoring_particles_contacts(RID p_body, bool p_monitoring);
	virtual bool primitive_body_is_monitoring_particles_contacts(RID p_body) const;

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
	virtual Ref<ParticleBodyModel> create_thread_particle_body_model(real_t p_particle_radius, real_t p_extent, real_t p_spacing, real_t p_link_stiffness, int p_cluster_size, real_t p_cluster_stiffness);
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
