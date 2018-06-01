/*************************************************************************/
/*  flex_memory.cpp                                                      */
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

#include "flex_memory.h"

ParticleBodiesMemory::ParticleBodiesMemory(NvFlexLibrary *p_flex_lib) :
        particles(p_flex_lib),
        velocities(p_flex_lib),
        phases(p_flex_lib),
        active_particles(p_flex_lib) {
}

void ParticleBodiesMemory::resize_memory(int p_size) {
    particles.resize(p_size);
    velocities.resize(p_size);
    phases.resize(p_size);
    active_particles.resize(p_size);
}

void ParticleBodiesMemory::copy(FlexUnit p_from_begin_index, FlexUnit p_size, FlexUnit p_to_begin_index) {
    for (int i(0); i < p_size; ++i) {
        particles[p_to_begin_index + i] = particles[p_from_begin_index + i];
        velocities[p_to_begin_index + i] = velocities[p_from_begin_index + i];
        phases[p_to_begin_index + i] = phases[p_from_begin_index + i];
    }
}

void ParticleBodiesMemory::map() {
    particles.map(eNvFlexMapWait);
    velocities.map(eNvFlexMapWait);
    phases.map(eNvFlexMapWait);
    active_particles.map(eNvFlexMapWait);
}

void ParticleBodiesMemory::unmap() {
    particles.unmap();
    velocities.unmap();
    phases.unmap();
    active_particles.unmap();
}

void ParticleBodiesMemory::terminate() {
    particles.destroy();
    velocities.destroy();
    phases.destroy();
    active_particles.destroy();
}

void ParticleBodiesMemory::set_particle(const MemoryChunk *p_chunk, ParticleID p_particle_index, FlVector4 p_particle) {
    particles[p_chunk->get_begin_index() + p_particle_index] = p_particle;
}

const FlVector4 &ParticleBodiesMemory::get_particle(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    return particles[p_chunk->get_begin_index() + p_particle_index];
}

void ParticleBodiesMemory::set_velocity(const MemoryChunk *p_chunk, ParticleID p_particle_index, FlVector3 p_velocity) {
    velocities[p_chunk->get_begin_index() + p_particle_index] = p_velocity;
}

const FlVector3 &ParticleBodiesMemory::get_velocity(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    return velocities[p_chunk->get_begin_index() + p_particle_index];
}

void ParticleBodiesMemory::set_phase(const MemoryChunk *p_chunk, ParticleID p_particle_index, int p_phase) {
    phases[p_chunk->get_begin_index() + p_particle_index] = p_phase;
}

int ParticleBodiesMemory::get_phase(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    return phases[p_chunk->get_begin_index() + p_particle_index];
}

void ParticleBodiesMemory::set_active_particle(const MemoryChunk *p_chunk, ParticleID p_particle_index) {
    active_particles[p_chunk->get_begin_index() + p_particle_index] = p_chunk->get_begin_index() + p_particle_index;
}
