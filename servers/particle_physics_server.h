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
	virtual void move_particles(const Transform &transform) = 0;
	virtual void load_model(Ref<ParticleBodyModel> p_shape, const Transform &initial_transform) = 0;

	virtual void set_particle_position_mass(int p_particle_index, const Vector3 &p_position, real_t p_mass) = 0;

	virtual int get_particle_count() const = 0;

	virtual void set_particle_position(int p_particle_index, const Vector3 &p_position) = 0;
	virtual Vector3 get_particle_position(int p_particle_index) const = 0;

	virtual void set_particle_mass(int p_particle_index, real_t p_mass) = 0;
	virtual float get_particle_mass(int p_particle_index) const = 0;

	virtual const Vector3 &get_particle_velocity(int p_particle_index) const = 0;
	virtual void set_particle_velocity(int p_particle_index, const Vector3 &p_velocity) = 0;

	virtual void apply_force(int p_particle_index, const Vector3 &p_force) = 0;

	virtual Vector3 get_particle_normal(int p_index) const = 0;

	virtual const Vector3 &get_rigid_position(int p_index) const = 0;
	virtual const Quat &get_rigid_rotation(int p_index) const = 0;

	virtual AABB get_aabb() const = 0;
};

class ParticleBodyConstraintCommands : public Object {
	GDCLASS(ParticleBodyConstraintCommands, Object);

protected:
	static void _bind_methods();

public:
	virtual int get_spring_count() const = 0;
	/// Returns the ID of spring
	virtual int add_spring(int p_body0_particle, int p_body1_particle, float p_length, float p_stiffness) = 0;
	virtual void set_spring(int p_index, int p_body0_particle, int p_body1_particle, float p_length, float p_stiffness) = 0;
	virtual real_t get_distance(int p_body0_particle, int p_body1_particle) const = 0;
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

	virtual void space_get_params_defaults(Map<StringName, Variant> *r_defs) const = 0;
	virtual bool space_set_param(RID p_space, const StringName &p_name, const Variant &p_property) = 0;
	virtual bool space_get_param(RID p_space, const StringName &p_name, Variant &r_property) const = 0;
	virtual real_t space_get_particle_radius(RID p_space) const = 0;
	virtual void space_reset_params_to_default(RID p_space) = 0;
	virtual bool space_is_using_default_params(RID p_space) const = 0;

	enum ParticleBodyCallback {
		PARTICLE_BODY_CALLBACK_SYNC,
		PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED,
		PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED,
		PARTICLE_BODY_CALLBACK_PRIMITIVECONTACT
	};

	enum ParticlePrimitiveBodyCallback {
		PARTICLE_PRIMITIVE_BODY_CALLBACK_SYNC,
		PARTICLE_PRIMITIVE_BODY_CALLBACK_PARTICLECONTACT
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
	virtual void body_set_object_instance(RID p_body, Object *p_object) = 0;

	/// This MUST be called only during ParticlePhysicsServer callbacks
	/// IMPORTANT: When called all previous get commands will be unusable
	virtual ParticleBodyCommands *body_get_commands(RID p_body) = 0;

	virtual void body_set_collision_group(RID p_body, uint32_t p_layer) = 0;
	virtual uint32_t body_get_collision_group(RID p_body) const = 0;

	virtual void body_set_collision_flag(RID p_body, ParticleCollisionFlag p_flag, bool p_active) = 0;
	virtual bool body_get_collision_flag(RID p_body, ParticleCollisionFlag p_flag) const = 0;

	virtual void body_set_collision_primitive_mask(RID p_body, uint32_t p_mask) = 0;
	virtual uint32_t body_get_collision_primitive_mask(RID p_body) const = 0;

	virtual void body_remove_particle(RID p_body, int p_particle_index) = 0;

	virtual void body_remove_rigid(RID p_body, int p_rigid_index) = 0;

	virtual int body_get_particle_count(RID p_body) const = 0;
	virtual int body_get_spring_count(RID p_body) const = 0;
	virtual int body_get_rigid_count(RID p_body) const = 0;

	virtual void body_set_pressure(RID p_body, float p_pressure) = 0;
	virtual float body_get_pressure(RID p_body) const = 0;

	virtual bool body_can_rendered_using_skeleton(RID p_body) const = 0;
	virtual bool body_can_rendered_using_pvparticles(RID p_body) const = 0; // Per particle vertex

	virtual void body_set_monitorable(RID p_body, bool p_monitorable) = 0;
	virtual bool body_is_monitorable(RID p_body) const = 0;

	virtual void body_set_monitoring_primitives_contacts(RID p_body, bool p_monitoring) = 0;
	virtual bool body_is_monitoring_primitives_contacts(RID p_body) const = 0;

	/* BODY CONSTRAINT */
	virtual RID constraint_create(RID p_body0, RID p_body1) = 0;

	virtual void constraint_set_callback(RID p_constraint, Object *p_receiver, const StringName &p_method) = 0;
	virtual void constraint_set_space(RID p_constraint, RID p_space) = 0;

	virtual void constraint_remove_spring(RID p_constraint, int p_spring_index) = 0;

	/* PRIMITIVE BODY */
	virtual RID primitive_body_create() = 0;
	virtual void primitive_body_set_space(RID p_body, RID p_space) = 0;
	virtual void primitive_body_set_shape(RID p_body, RID p_shape) = 0;

	virtual void primitive_body_set_callback(RID p_body, ParticlePrimitiveBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method) = 0;
	virtual void primitive_body_set_object_instance(RID p_body, Object *p_object) = 0;

	virtual void primitive_body_set_transform(RID p_body, const Transform &p_transf, bool p_teleport) = 0;

	virtual void primitive_body_set_collision_layer(RID p_body, uint32_t p_layer) = 0;
	virtual uint32_t primitive_body_get_collision_layer(RID p_body) const = 0;

	virtual void primitive_body_set_kinematic(RID p_body, bool p_kinematic) = 0;
	virtual bool primitive_body_is_kinematic(RID p_body) const = 0;

	virtual void primitive_body_set_as_area(RID p_body, bool p_area) = 0;
	virtual bool primitive_body_is_area(RID p_body) const = 0;

	virtual void primitive_body_set_monitoring_particles_contacts(RID p_body, bool p_monitoring) = 0;
	virtual bool primitive_body_is_monitoring_particles_contacts(RID p_body) const = 0;

	/* PRIMITIVE SHAPE */
	enum PrimitiveShapeType {
		PARTICLE_PRIMITIVE_SHAPE_TYPE_BOX,
		PARTICLE_PRIMITIVE_SHAPE_TYPE_CAPSULE,
		PARTICLE_PRIMITIVE_SHAPE_TYPE_SPHERE,
		PARTICLE_PRIMITIVE_SHAPE_TYPE_CONVEX,
		PARTICLE_PRIMITIVE_SHAPE_TYPE_TRIMESH
	};

	virtual RID primitive_shape_create(PrimitiveShapeType p_type) = 0;

	virtual void primitive_shape_set_data(RID p_shape, const Variant &p_data) = 0;
	virtual Variant primitive_shape_get_data(RID p_shape) const = 0;

	/* COMMON */
	virtual void free(RID p_rid) = 0;

	virtual Ref<ParticleBodyModel> create_soft_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_global_stiffness, bool p_internal_sample, float p_particle_spacing, float p_sampling, float p_clusterSpacing, float p_clusterRadius, float p_clusterStiffness, float p_linkRadius, float p_linkStiffness, float p_plastic_threshold, float p_plastic_creep) = 0;
	virtual Ref<ParticleBodyModel> create_cloth_particle_body_model(Ref<TriangleMesh> p_mesh, float p_stretch_stiffness, float p_bend_stiffness, float p_tether_stiffness, float p_tether_give, float p_pressure) = 0;
	virtual Ref<ParticleBodyModel> create_rigid_particle_body_model(Ref<TriangleMesh> p_mesh, float p_radius, float p_expand) = 0;
	virtual Ref<ParticleBodyModel> create_thread_particle_body_model(real_t p_particle_radius, real_t p_extent, real_t p_spacing, real_t p_link_stiffness, int p_cluster_size, real_t p_cluster_stiffness) = 0;

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
VARIANT_ENUM_CAST(ParticlePhysicsServer::ParticlePrimitiveBodyCallback);
VARIANT_ENUM_CAST(ParticlePhysicsServer::PrimitiveShapeType);
VARIANT_ENUM_CAST(ParticlePhysicsServer::ParticleCollisionFlag);

#endif // PARTICLE_PHYSICS_SERVER_H
