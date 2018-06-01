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

FlexParticleBody::FlexParticleBody() :
        RIDFlex(),
        space(NULL),
        memory_chunk(NULL) {
}

void FlexParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {

    commands.particle_to_add.push_back(ParticleToAdd(p_local_position, p_mass));
}

void FlexParticleBody::remove_particle(ParticleID p_particle) {
    ERR_FAIL_COND(!is_owner(p_particle));
    commands.particle_to_remove.push_back(p_particle);
}

void FlexParticleBody::clear_commands() {
    commands.particle_to_add.clear();
    commands.particle_to_remove.clear();
}

bool FlexParticleBody::is_owner(ParticleID p_particle) const {
    return (memory_chunk && (memory_chunk->get_begin_index() + p_particle) <= memory_chunk->get_end_index());
}
