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

/**
	@author AndreaCatania
*/

#define CreateThenReturnRID(owner, rid_data) \
    RID rid = owner.make_rid(rid_data);      \
    rid_data->__set_self(rid);               \
    rid_data->__set_physics_server(this);    \
    return rid;

FlexParticlePhysicsServer *FlexParticlePhysicsServer::singleton = NULL;

FlexParticlePhysicsServer::FlexParticlePhysicsServer() :
        ParticlePhysicsServer(),
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

bool FlexParticlePhysicsServer::space_is_active(const RID p_space) const {
    const FlexSpace *space = space_owner.get(p_space);
    ERR_FAIL_COND_V(!space, false);

    return active_spaces.find(space) != -1;
}

void FlexParticlePhysicsServer::space_add_particle_body(RID p_space, RID p_body) {
    FlexSpace *space = space_owner.get(p_space);
    FlexParticleBody *particle_body = body_owner.get(p_body);
    ERR_FAIL_COND(!space);
    ERR_FAIL_COND(!particle_body);
}

void FlexParticlePhysicsServer::space_remove_particle_body(RID p_space, RID p_body) {
}

RID FlexParticlePhysicsServer::body_create() {
    FlexParticleBody *particle_body = memnew(FlexParticleBody);
    CreateThenReturnRID(body_owner, particle_body);
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
    } else {
        ERR_EXPLAIN("Can't delete RID, owner not found");
        ERR_FAIL();
    }
}

void FlexParticlePhysicsServer::init() {}

void FlexParticlePhysicsServer::terminate() {}

void FlexParticlePhysicsServer::sync() {
    for (short i = last_space_index; 0 <= i; --i) {
        active_spaces[i]->sync();
    }
}

void FlexParticlePhysicsServer::flush_queries() {}

void FlexParticlePhysicsServer::step(real_t p_delta_time) {
    for (short i = last_space_index; 0 <= i; --i) {
        active_spaces[i]->step(p_delta_time);
    }
}
