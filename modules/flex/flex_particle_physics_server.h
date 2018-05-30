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

#include "flex_particle_body.h"
#include "flex_space.h"

/**
	@author AndreaCatania
*/

class FlexParticlePhysicsServer : public ParticlePhysicsServer {
	GDCLASS(FlexParticlePhysicsServer, ParticlePhysicsServer);

	static FlexParticlePhysicsServer *singleton;

    mutable RID_Owner<FlexSpace> space_owner;
    mutable RID_Owner<FlexParticleBody> body_owner;

    char last_space_index;
    Vector<FlexSpace *> active_spaces;

public:
    FlexParticlePhysicsServer();
    virtual ~FlexParticlePhysicsServer();

    virtual RID space_create();
    virtual void space_set_active(RID p_space, bool p_active);
    virtual bool space_is_active(const RID p_space) const;

    virtual RID body_create();

    virtual void free(RID p_rid);

    virtual void init();
    virtual void terminate();
    virtual void sync();
    virtual void flush_queries();
    virtual void step(real_t p_delta_time);
};

#endif // FLEX_PARTICLE_PHYSICS_SERVER_H
