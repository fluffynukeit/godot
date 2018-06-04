#ifndef PARTICLE_PHYSICS_SERVER_H
#define PARTICLE_PHYSICS_SERVER_H

#include "object.h"
#include "resource.h"
#include "scene/resources/particle_shape.h"

class ParticleBodyCommands : public Object {
    GDCLASS(ParticleBodyCommands, Object);

protected:
    static void _bind_methods();

public:
    virtual void load_shape(Ref<ParticleShape> p_shape, const Transform &initial_transform) = 0;
    virtual void reset_particle(int p_particle_index, const Vector3 &p_position, real_t p_mass) = 0;
    virtual Vector3 get_particle_position(int p_particle_index) const = 0;
    virtual Vector3 get_particle_velocity(int p_particle_index) const = 0;
};

class ParticlePhysicsServer : public Object {
	GDCLASS(ParticlePhysicsServer, Object);

	static ParticlePhysicsServer *singleton;

protected:
    static void _bind_methods();

public:
	static ParticlePhysicsServer *get_singleton();

public:
    virtual RID space_create() = 0;
    virtual void space_set_active(RID p_space, bool p_active) = 0;
    virtual bool space_is_active(const RID p_space) const = 0;

    virtual RID body_create() = 0;
    virtual void body_set_space(RID p_body, RID p_space) = 0;
    virtual void body_set_sync_callback(RID p_body, Object *p_receiver, const StringName &p_method) = 0;
    virtual void body_add_particle(RID p_body, const Vector3 &p_local_position, real_t p_mass) = 0;
    virtual void body_remove_particle(RID p_body, int p_particle_id) = 0;
    virtual int body_get_particle_count(RID p_body) const = 0;

    virtual void free(RID p_rid) = 0;

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

#endif // PARTICLE_PHYSICS_SERVER_H
