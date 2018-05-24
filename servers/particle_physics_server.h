#ifndef PARTICLE_PHYSICS_SERVER_H
#define PARTICLE_PHYSICS_SERVER_H

#include "object.h"
#include "resource.h"

class ParticlePhysicsServer : public Object {
	GDCLASS(ParticlePhysicsServer, Object);

	static ParticlePhysicsServer *singleton;

public:
	static ParticlePhysicsServer *get_singleton();

public:
	virtual void init() = 0;
	virtual void step(float p_delta_time) = 0;

	ParticlePhysicsServer();
	virtual ~ParticlePhysicsServer();
};

#endif // PARTICLE_PHYSICS_SERVER_H
