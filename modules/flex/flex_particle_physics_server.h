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

class FlexParticleBodyCommands : public ParticleBodyCommands {
    GDCLASS(FlexParticleBodyCommands, ParticleBodyCommands);

public:
    FlexParticleBody *body;

    virtual void load_shape(Ref<ParticleShape> p_shape, const Transform &initial_transform);
    virtual void reset_particle(int p_particle_index, const Vector3 &p_position, real_t p_mass);
    virtual Vector3 get_particle_position(int p_particle_id) const;
    virtual const Vector3 &get_particle_velocity(int p_particle_index) const;
};

class FlexParticlePhysicsServer : public ParticlePhysicsServer {
	GDCLASS(FlexParticlePhysicsServer, ParticlePhysicsServer);

public:
    static FlexParticlePhysicsServer *singleton;

private:
    mutable RID_Owner<FlexSpace> space_owner;
    mutable RID_Owner<FlexParticleBody> body_owner;

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

    virtual RID space_create();
    virtual void space_set_active(RID p_space, bool p_active);
    virtual bool space_is_active(const RID p_space) const;

    virtual RID body_create();
    virtual void body_set_space(RID p_body, RID p_space);
    virtual void body_set_sync_callback(RID p_body, Object *p_receiver, const StringName &p_method);
    virtual void body_add_particle(RID p_body, const Vector3 &p_local_position, real_t p_mass);
    virtual void body_remove_particle(RID p_body, int p_particle_id);
    virtual int body_get_particle_count(RID p_body) const;

    virtual void free(RID p_rid);

    virtual void init();
    virtual void terminate();
    virtual void set_active(bool p_active);
    virtual void sync();
    virtual void flush_queries();
    virtual void step(real_t p_delta_time);
};

#endif // FLEX_PARTICLE_PHYSICS_SERVER_H
