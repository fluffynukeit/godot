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

void ParticleBodiesMemory::resize_memory(FlexUnit p_size) {
    particles.resize(p_size);
    velocities.resize(p_size);
    phases.resize(p_size);
    active_particles.resize(p_size);
}

void ParticleBodiesMemory::copy_unit(FlexUnit p_to, FlexUnit p_from) {
    particles[p_to] = particles[p_from];
    velocities[p_to] = velocities[p_from];
    phases[p_to] = phases[p_from];
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
    get_memory_index();
    particles[index] = p_particle;
}

const FlVector4 &ParticleBodiesMemory::get_particle(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    get_memory_index_V(return_err_flvec4);
    return particles[index];
}

void ParticleBodiesMemory::set_velocity(const MemoryChunk *p_chunk, ParticleID p_particle_index, Vector3 p_velocity) {
    get_memory_index();
    velocities[index] = p_velocity;
}

const Vector3 &ParticleBodiesMemory::get_velocity(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    get_memory_index_V(return_err_vec3);
    return velocities[index];
}

void ParticleBodiesMemory::set_phase(const MemoryChunk *p_chunk, ParticleID p_particle_index, int p_phase) {
    get_memory_index();
    phases[index] = p_phase;
}

int ParticleBodiesMemory::get_phase(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    get_memory_index_V(0);
    return phases[index];
}

void ParticleBodiesMemory::set_active_particle(const MemoryChunk *p_chunk, ParticleID p_particle_index) {
    get_memory_index();
    active_particles[index] = p_chunk->get_begin_index() + p_particle_index;
}

SpringBodiesMemory::SpringBodiesMemory(NvFlexLibrary *p_flex_lib) :
        springs(p_flex_lib),
        lengths(p_flex_lib),
        stifness(p_flex_lib) {
}

void SpringBodiesMemory::resize_memory(FlexUnit p_size) {
    springs.resize(p_size);
    lengths.resize(p_size);
    stifness.resize(p_size);
}

void SpringBodiesMemory::copy_unit(FlexUnit p_to, FlexUnit p_from) {
    springs[p_to] = springs[p_from];
    lengths[p_to] = lengths[p_from];
    stifness[p_to] = stifness[p_from];
}

void SpringBodiesMemory::set_spring(const MemoryChunk *p_chunk, ParticleID p_particle_index, const Spring &p_spring) {
    get_memory_index();
    springs[index] = p_spring;
}

void SpringBodiesMemory::set_length(const MemoryChunk *p_chunk, ParticleID p_particle_index, float p_length) {
    get_memory_index();
    lengths[index] = p_length;
}

void SpringBodiesMemory::set_stifness(const MemoryChunk *p_chunk, ParticleID p_particle_index, float p_stifness) {
    get_memory_index();
    stifness[index] = p_stifness;
}
