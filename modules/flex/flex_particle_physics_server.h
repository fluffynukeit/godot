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

/**
	@author AndreaCatania
*/

#define MAXPARTICLES 10

class FlexparticlePhysicsServer;
class NvFlexBuffer;

struct FlexBuffers {

	// TODO this is just an initial test, is better to use NvFlexVector and then implement a better memory handling in order to avoid brute force update
	NvFlexBuffer *positions;
	NvFlexBuffer *velocities;
	NvFlexBuffer *phases;

	FlexBuffers();
	void allocate();
	void deallocate();
	void map();
	void unmap();
};

class FlexParticlePhysicsServer : public ParticlePhysicsServer {
	GDCLASS(FlexParticlePhysicsServer, ParticlePhysicsServer);

	friend class FlexBuffers;
	static FlexParticlePhysicsServer *singleton;

	class NvFlexLibrary *flex_lib;
	class NvFlexSolver *solver;
	FlexBuffers *buffers;

public:
	virtual void init();
	virtual void terminate();
	virtual void sync();
	virtual void flush_queries();
	virtual void step(real_t p_delta_time);

	FlexParticlePhysicsServer();
	virtual ~FlexParticlePhysicsServer();
};

#endif // FLEX_PARTICLE_PHYSICS_SERVER_H
