/*************************************************************************/
/*  flex_particle_body.cpp                                               */
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

/**
    @author AndreaCatania
*/

#include "flex_particle_body.h"
#include "flex_memory.h"
#include "flex_space.h"
#include "object.h"

FlexParticleBody::FlexParticleBody() :
        RIDFlex(),
        space(NULL),
        memory_chunk(NULL) {
    sync_callback.receiver = NULL;
}

void FlexParticleBody::set_sync_callback(Object *p_receiver, const StringName &p_method) {
    if (p_receiver) {
        ERR_FAIL_COND(!p_receiver->has_method(p_method));
    }
    sync_callback.receiver = p_receiver;
    sync_callback.method = p_method;
}

void FlexParticleBody::dispatch_sync_callback() {
    if (!sync_callback.receiver || !memory_chunk)
        return;
    static Variant::CallError error;
    const Variant *p = FlexParticlePhysicsServer::singleton->get_particle_body_commands_variant(this);
    sync_callback.receiver->call(sync_callback.method, &p, 1, error);
}

void FlexParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {

    delayed_commands.particle_to_add.push_back(ParticleToAdd(p_local_position, p_mass));
}

void FlexParticleBody::remove_particle(ParticleID p_particle) {
    ERR_FAIL_COND(!is_owner(p_particle));
    delayed_commands.particle_to_remove.insert(p_particle);
}

int FlexParticleBody::get_particle_count() const {
    if (!memory_chunk)
        return 0;
    return memory_chunk->get_size();
}

Vector3 FlexParticleBody::get_particle_position(ParticleID p_particle_index) const {
    const FlVector4 &p(space->get_particle_bodies_memory()->get_particle(memory_chunk, p_particle_index));
    return gvec3_from_fvec4(p);
}

bool FlexParticleBody::is_owner(ParticleID p_particle) const {
    return (memory_chunk && (memory_chunk->get_begin_index() + p_particle) <= memory_chunk->get_end_index());
}

void FlexParticleBody::clear_commands() {
    delayed_commands.particle_to_add.clear();
    delayed_commands.particle_to_remove.clear();
}
