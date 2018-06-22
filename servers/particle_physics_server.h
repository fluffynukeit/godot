#ifndef PARTICLE_PHYSICS_SERVER_H
#define PARTICLE_PHYSICS_SERVER_H

#include "core/math/triangle_mesh.h"
#include "object.h"
#include "resource.h"
#include "scene/resources/particle_body_model.h"

class ParticleBodyCommands : public Object {
	GDCLASS(ParticleBodyCommands, Object);

protected:
	static void _bind_methods();

public:
	virtual void load_model(Ref<ParticleBodyModel> p_shape, const Transform &initial_transform) = 0;

	virtual void reset_particle(int p_particle_index, const Vector3 &p_position, real_t p_mass) = 0;

	virtual Vector3 get_particle_position(int p_particle_index) const = 0;

	virtual const Vector3 &get_particle_velocity(int p_particle_index) const = 0;
	virtual void set_particle_velocity(int p_particle_index, const Vector3 &p_velocity) = 0;

	virtual const Vector3 &get_rigid_position(int p_index) const = 0;
	virtual const Quat &get_rigid_rotation(int p_index) const = 0;
};

class ParticlePhysicsServer : public Object {
	GDCLASS(ParticlePhysicsServer, Object);

	static ParticlePhysicsServer *singleton;

protected:
	static void _bind_methods();

public:
	static ParticlePhysicsServer *get_singleton();

public:
	/* SPACE */
	virtual RID space_create() = 0;
	virtual void space_set_active(RID p_space, bool p_active) = 0;
	virtual bool space_is_active(RID p_space) const = 0;

	virtual float space_get_particle_radius(RID p_space) = 0;

	enum ParticleBodyCallback {
		PARTICLE_BODY_CALLBACK_SYNC,
		PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED,
		PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED
	};

	enum ParticleCollisionFlag {
		PARTICLE_COLLISION_FLAG_SELF_COLLIDE,
		PARTICLE_COLLISION_FLAG_SELF_COLLIDE_FILTER,
		PARTICLE_COLLISION_FLAG_FLUID
	};

	/* BODY */
	virtual RID body_create() = 0;
	virtual void body_set_space(RID p_body, RID p_space) = 0;
	virtual void body_set_callback(RID p_body, ParticleBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method) = 0;

	virtual void body_set_collision_group(RID p_body, uint32_t p_layer) = 0;
	virtual uint32_t body_get_collision_group(RID p_body) const = 0;

	virtual void body_set_collision_flag(RID p_body, ParticleCollisionFlag p_flag, bool p_active) = 0;
	virtual bool body_get_collision_flag(RID p_body, ParticleCollisionFlag p_flag) const = 0;

	virtual void body_set_collision_primitive_mask(RID p_body, uint32_t p_mask) = 0;
	virtual uint32_t body_get_collision_primitive_mask(RID p_body) const = 0;

	virtual void body_add_particle(RID p_body, const Vector3 &p_local_position, real_t p_mass) = 0;
	virtual void body_remove_particle(RID p_body, int p_particle_index) = 0;

	virtual void body_remove_rigid(RID p_body, int p_rigid_index) = 0;

	virtual int body_get_particle_count(RID p_body) const = 0;
	virtual int body_get_spring_count(RID p_body) const = 0;
	virtual int body_get_rigid_count(RID p_body) const = 0;

	/* PRIMITIVE BODY */
	virtual RID primitive_body_create() = 0;
	virtual void primitive_body_set_space(RID p_body, RID p_space) = 0;
	virtual void primitive_body_set_shape(RID p_body, RID p_shape) = 0;

	virtual void primitive_body_set_transform(RID p_body, const Transform &p_transf, bool p_teleport) = 0;

	virtual void primitive_body_set_collision_layer(RID p_body, uint32_t p_layer) = 0;
	virtual uint32_t primitive_body_get_collision_layer(RID p_body) const = 0;

	virtual void primitive_body_set_kinematic(RID p_body, bool p_kinematic) = 0;
	virtual bool primitive_body_is_kinematic(RID p_body) const = 0;

	/* PRIMITIVE SHAPE */
	enum PrimitiveShapeType {
		PARTICLE_PRIMITIVE_SHAPE_TYPE_BOX,
		PARTICLE_PRIMITIVE_SHAPE_TYPE_CAPSULE,
		PARTICLE_PRIMITIVE_SHAPE_TYPE_SPHERE
	};

	virtual RID primitive_shape_create(PrimitiveShapeType p_type) = 0;

	virtual void primitive_shape_set_data(RID p_shape, const Variant &p_data) = 0;
	virtual Variant primitive_shape_get_data(RID p_shape) const = 0;

	/* COMMON */
	virtual void free(RID p_rid) = 0;

	virtual Ref<ParticleBodyModel> create_soft_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_global_stiffness, bool p_internal_sample, float p_particle_spacing, float p_sampling, float p_clusterSpacing, float p_clusterRadius, float p_clusterStiffness, float p_linkRadius, float p_linkStiffness, float p_plastic_threshold, float p_plastic_creep) = 0;
	virtual Ref<ParticleBodyModel> create_cloth_particle_body_model(Ref<TriangleMesh> p_mesh, float p_stretch_stiffness, float p_bend_stiffness, float p_tether_stiffness, float p_tether_give, float p_pressure) = 0;
	virtual Ref<ParticleBodyModel> create_rigid_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_expand) = 0;

	struct ParticleMeshBone {
		int particle_id;
		int bone_id;
		real_t weight;
	};
	virtual void create_skeleton(const Vector3 *bones_poses, int bone_count, const Vector3 *p_vertices, int p_vertex_count, PoolVector<float> *r_weights, PoolVector<int> *r_particle_indices, int *r_max_weight_per_vertex) = 0;

public:
	// Internals
	virtual void init() = 0;
	virtual void terminate() = 0;
	virtual void set_active(bool p_active) = 0;
	virtual void sync() = 0; // Must be called before "step" function in order to wait eventually running tasks
	virtual void flush_queries() = 0;
	virtual void step(real_t p_delta_time) = 0;

	ParticlePhysicsServer();
	virtual ~ParticlePhysicsServer();
};

VARIANT_ENUM_CAST(ParticlePhysicsServer::ParticleBodyCallback);
VARIANT_ENUM_CAST(ParticlePhysicsServer::PrimitiveShapeType);
VARIANT_ENUM_CAST(ParticlePhysicsServer::ParticleCollisionFlag);

#endif // PARTICLE_PHYSICS_SERVER_H
