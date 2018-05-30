#ifndef PARTICLE_PHYSICS_SERVER_H
#define PARTICLE_PHYSICS_SERVER_H

#include "object.h"
#include "resource.h"

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
    virtual void space_add_particle_body(RID p_space, RID p_body) = 0;
    virtual void space_remove_particle_body(RID p_space, RID p_body) = 0;

    virtual RID body_create() = 0;

    virtual void free(RID p_rid) = 0;

public:
    // Internals
	virtual void init() = 0;
	virtual void terminate() = 0;
	virtual void sync() = 0; // Must be called before "step" function in order to wait eventually running tasks
	virtual void flush_queries() = 0;
	virtual void step(real_t p_delta_time) = 0;

	ParticlePhysicsServer();
	virtual ~ParticlePhysicsServer();
};

#endif // PARTICLE_PHYSICS_SERVER_H
