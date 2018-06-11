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

#define make_memory_index(p_chunk, p_index)         \
	int index = p_chunk->get_buffer_index(p_index); \
	ERR_FAIL_COND(index > p_chunk->get_end_index());

#define make_memory_index_V(p_chunk, p_index, ret)  \
	int index = p_chunk->get_buffer_index(p_index); \
	ERR_FAIL_COND_V(index > p_chunk->get_end_index(), ret);

ParticleBodiesMemory::ParticleBodiesMemory(NvFlexLibrary *p_flex_lib) :
		particles(p_flex_lib),
		velocities(p_flex_lib),
		phases(p_flex_lib) {
}

void ParticleBodiesMemory::resize_memory(FlexUnit p_size) {
	particles.resize(p_size);
	velocities.resize(p_size);
	phases.resize(p_size);
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
}

void ParticleBodiesMemory::unmap() {
	particles.unmap();
	velocities.unmap();
	phases.unmap();
}

void ParticleBodiesMemory::terminate() {
	particles.destroy();
	velocities.destroy();
	phases.destroy();
}

void ParticleBodiesMemory::set_particle(const MemoryChunk *p_chunk, ParticleIndex p_particle_index, FlVector4 p_particle) {
	make_memory_index(p_chunk, p_particle_index);
	particles[index] = p_particle;
}

const FlVector4 &ParticleBodiesMemory::get_particle(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const {
	make_memory_index_V(p_chunk, p_particle_index, return_err_flvec4);
	return particles[index];
}

void ParticleBodiesMemory::set_velocity(const MemoryChunk *p_chunk, ParticleIndex p_particle_index, Vector3 p_velocity) {
	make_memory_index(p_chunk, p_particle_index);
	velocities[index] = p_velocity;
}

const Vector3 &ParticleBodiesMemory::get_velocity(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const {
	make_memory_index_V(p_chunk, p_particle_index, return_err_vec3);
	return velocities[index];
}

void ParticleBodiesMemory::set_phase(const MemoryChunk *p_chunk, ParticleIndex p_particle_index, int p_phase) {
	make_memory_index(p_chunk, p_particle_index);
	phases[index] = p_phase;
}

int ParticleBodiesMemory::get_phase(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const {
	make_memory_index_V(p_chunk, p_particle_index, 0);
	return phases[index];
}

ActiveParticlesMemory::ActiveParticlesMemory(NvFlexLibrary *p_flex_lib) :
		active_particles(p_flex_lib) {
}

void ActiveParticlesMemory::map() {
	active_particles.map(eNvFlexMapWait);
}

void ActiveParticlesMemory::unmap() {
	active_particles.unmap();
}

void ActiveParticlesMemory::terminate() {
	active_particles.destroy();
}

void ActiveParticlesMemory::set_active_particle(const MemoryChunk *p_chunk, ActiveParticleIndex p_active_particle_index, ParticleBufferIndex p_particle_buffer_index) {
	make_memory_index(p_chunk, p_active_particle_index);
	active_particles[index] = p_particle_buffer_index;
}

void ActiveParticlesMemory::resize_memory(FlexUnit p_size) {
	active_particles.resize(p_size);
}

void ActiveParticlesMemory::copy_unit(FlexUnit p_to, FlexUnit p_from) {
	active_particles[p_to] = active_particles[p_from];
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

void SpringMemory::set_spring(const MemoryChunk *p_chunk, SpringIndex p_spring_index, const Spring &p_spring) {
	make_memory_index(p_chunk, p_spring_index);
	springs[index] = p_spring;
	changed = true;
}

const Spring &SpringMemory::get_spring(const MemoryChunk *p_chunk, SpringIndex p_spring_index) const {
	make_memory_index_V(p_chunk, p_spring_index, return_err_spring);
	return springs[index];
}

void SpringMemory::set_length(const MemoryChunk *p_chunk, SpringIndex p_spring_index, float p_length) {
	make_memory_index(p_chunk, p_spring_index);
	lengths[index] = p_length;
	changed = true;
}

float SpringMemory::get_length(const MemoryChunk *p_chunk, SpringIndex p_spring_index) const {
	make_memory_index_V(p_chunk, p_spring_index, 0);
	return lengths[index];
}

void SpringMemory::set_stiffness(const MemoryChunk *p_chunk, SpringIndex p_spring_index, float p_stiffness) {
	make_memory_index(p_chunk, p_spring_index);
	stiffness[index] = p_stiffness;
	changed = true;
}

float SpringMemory::get_stiffness(const MemoryChunk *p_chunk, SpringIndex p_spring_index) const {
	make_memory_index_V(p_chunk, p_spring_index, 0);
	return stiffness[index];
}

GeometryMemory::GeometryMemory(NvFlexLibrary *p_lib) :
		collision_shapes(p_lib),
		positions(p_lib),
		rotations(p_lib),
		positions_prev(p_lib),
		rotations_prev(p_lib),
		flags(p_lib),
		changed(false) {
}

void GeometryMemory::map() {
	collision_shapes.map(eNvFlexMapWait);
	positions.map(eNvFlexMapWait);
	rotations.map(eNvFlexMapWait);
	positions_prev.map(eNvFlexMapWait);
	rotations_prev.map(eNvFlexMapWait);
	flags.map(eNvFlexMapWait);
	changed = false;
}

void GeometryMemory::unmap() {
	collision_shapes.unmap();
	positions.unmap();
	rotations.unmap();
	positions_prev.unmap();
	rotations_prev.unmap();
	flags.unmap();
}

void GeometryMemory::terminate() {
	collision_shapes.destroy();
	positions.destroy();
	rotations.destroy();
	positions_prev.destroy();
	rotations_prev.destroy();
	flags.destroy();
}

void GeometryMemory::set_shape(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const NvFlexCollisionGeometry &p_shape) {
	make_memory_index(p_chunk, p_geometry_index);
	collision_shapes[index] = p_shape;
	changed = true;
}

NvFlexCollisionGeometry GeometryMemory::get_shape(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const {
	make_memory_index_V(p_chunk, p_geometry_index, NvFlexCollisionGeometry());
	return collision_shapes[index];
}

void GeometryMemory::set_position(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const FlVector4 &p_position) {
	make_memory_index(p_chunk, p_geometry_index);
	positions[index] = p_position;
	changed = true;
}

const FlVector4 &GeometryMemory::get_position(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const {
	make_memory_index_V(p_chunk, p_geometry_index, return_err_flvec4);
	return positions[index];
}

void GeometryMemory::set_rotation(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const Quat &p_rotation) {
	make_memory_index(p_chunk, p_geometry_index);
	rotations[index] = p_rotation;
	changed = true;
}

const Quat &GeometryMemory::get_rotation(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const {
	make_memory_index_V(p_chunk, p_geometry_index, return_err_flquat);
	return rotations[index];
}

void GeometryMemory::set_position_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const FlVector4 &p_position) {
	make_memory_index(p_chunk, p_geometry_index);
	positions_prev[index] = p_position;
	changed = true;
}

const FlVector4 &GeometryMemory::get_position_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const {
	make_memory_index_V(p_chunk, p_geometry_index, return_err_flvec4);
	return positions_prev[index];
}

void GeometryMemory::set_rotation_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const Quat &p_rotation) {
	make_memory_index(p_chunk, p_geometry_index);
	rotations_prev[index] = p_rotation;
	changed = true;
}

const Quat &GeometryMemory::get_rotation_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const {
	make_memory_index_V(p_chunk, p_geometry_index, return_err_flquat);
	return rotations_prev[index];
}

void GeometryMemory::set_flags(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, int p_flags) {
	make_memory_index(p_chunk, p_geometry_index);
	flags[index] = p_flags;
	changed = true;
}

int GeometryMemory::get_flags(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const {
	make_memory_index_V(p_chunk, p_geometry_index, 0);
	return flags[index];
}

void GeometryMemory::resize_memory(FlexUnit p_size) {
	collision_shapes.resize(p_size);
	positions.resize(p_size);
	rotations.resize(p_size);
	positions_prev.resize(p_size);
	rotations_prev.resize(p_size);
	flags.resize(p_size);
}

void GeometryMemory::copy_unit(FlexUnit p_to, FlexUnit p_from) {
	collision_shapes[p_to] = collision_shapes[p_from];
	positions[p_to] = positions[p_from];
	rotations[p_to] = rotations[p_from];
	positions_prev[p_to] = positions_prev[p_from];
	rotations_prev[p_to] = rotations_prev[p_from];
	flags[p_to] = flags[p_from];
}
