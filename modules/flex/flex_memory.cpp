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
    make_memory_index(p_chunk, p_particle_index);
    particles[index] = p_particle;
}

const FlVector4 &ParticleBodiesMemory::get_particle(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    make_memory_index_V(p_chunk, p_particle_index, return_err_flvec4);
    return particles[index];
}

void ParticleBodiesMemory::set_velocity(const MemoryChunk *p_chunk, ParticleID p_particle_index, Vector3 p_velocity) {
    make_memory_index(p_chunk, p_particle_index);
    velocities[index] = p_velocity;
}

const Vector3 &ParticleBodiesMemory::get_velocity(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    make_memory_index_V(p_chunk, p_particle_index, return_err_vec3);
    return velocities[index];
}

void ParticleBodiesMemory::set_phase(const MemoryChunk *p_chunk, ParticleID p_particle_index, int p_phase) {
    make_memory_index(p_chunk, p_particle_index);
    phases[index] = p_phase;
}

int ParticleBodiesMemory::get_phase(const MemoryChunk *p_chunk, ParticleID p_particle_index) const {
    make_memory_index_V(p_chunk, p_particle_index, 0);
    return phases[index];
}

void ParticleBodiesMemory::set_active_particle(const MemoryChunk *p_chunk, ParticleID p_particle_index) {
    make_memory_index(p_chunk, p_particle_index);
    active_particles[index] = p_chunk->get_buffer_index(p_particle_index);
}

SpringMemory::SpringMemory(NvFlexLibrary *p_flex_lib) :
        springs(p_flex_lib),
        lengths(p_flex_lib),
        stiffness(p_flex_lib),
        changed(false) {
}

void SpringMemory::map() {
    springs.map(eNvFlexMapWait);
    lengths.map(eNvFlexMapWait);
    stiffness.map(eNvFlexMapWait);
    changed = false;
}

void SpringMemory::unmap() {
    springs.unmap();
    lengths.unmap();
    stiffness.unmap();
}

void SpringMemory::terminate() {
    springs.destroy();
    lengths.destroy();
    stiffness.destroy();
}

void SpringMemory::resize_memory(FlexUnit p_size) {
    springs.resize(p_size);
    lengths.resize(p_size);
    stiffness.resize(p_size);
}

void SpringMemory::copy_unit(FlexUnit p_to, FlexUnit p_from) {
    springs[p_to] = springs[p_from];
    lengths[p_to] = lengths[p_from];
    stiffness[p_to] = stiffness[p_from];
}

void SpringMemory::set_spring(const MemoryChunk *p_chunk, ParticleID p_spring_index, const Spring &p_spring) {
    make_memory_index(p_chunk, p_spring_index);
    springs[index] = p_spring;
    changed = true;
}

const Spring &SpringMemory::get_spring(const MemoryChunk *p_chunk, SpringID p_spring_index) const {
    make_memory_index_V(p_chunk, p_spring_index, return_err_spring);
    return springs[index];
}

void SpringMemory::set_length(const MemoryChunk *p_chunk, ParticleID p_spring_index, float p_length) {
    make_memory_index(p_chunk, p_spring_index);
    lengths[index] = p_length;
    changed = true;
}

float SpringMemory::get_length(const MemoryChunk *p_chunk, SpringID p_spring_index) const {
    make_memory_index_V(p_chunk, p_spring_index, 0);
    return lengths[index];
}

void SpringMemory::set_stiffness(const MemoryChunk *p_chunk, ParticleID p_spring_index, float p_stiffness) {
    make_memory_index(p_chunk, p_spring_index);
    stiffness[index] = p_stiffness;
    changed = true;
}

float SpringMemory::get_stiffness(const MemoryChunk *p_chunk, SpringID p_spring_index) const {
    make_memory_index_V(p_chunk, p_spring_index, 0);
    return stiffness[index];
}
