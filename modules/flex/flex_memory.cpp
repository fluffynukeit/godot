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

const FlVector4 &ParticleBodiesMemory::get_normal(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const {
	make_memory_index_V(p_chunk, p_particle_index, return_err_flvec4);
	return normals[index];
}

void ActiveParticlesMemory::set_active_particle(const MemoryChunk *p_chunk, ActiveParticleIndex p_active_particle_index, ParticleBufferIndex p_particle_buffer_index) {
	make_memory_index(p_chunk, p_active_particle_index);
	active_particles[index] = p_particle_buffer_index;
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
	make_memory_index_V(p_chunk, p_geometry_index, return_err_quat);
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
	make_memory_index_V(p_chunk, p_geometry_index, return_err_quat);
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

void RawRigidsMemory::set_stiffness(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, float p_stiffness) {
	make_memory_index(p_chunk, p_rigid_index);
	stiffness[index] = p_stiffness;
	changed = true;
}

float RawRigidsMemory::get_stiffness(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const {
	make_memory_index_V(p_chunk, p_rigid_index, 0.0);
	return stiffness[index];
}

void RawRigidsMemory::set_threshold(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, float p_threshold) {
	make_memory_index(p_chunk, p_rigid_index);
	thresholds[index] = p_threshold;
}

float RawRigidsMemory::get_threshold(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const {
	make_memory_index_V(p_chunk, p_rigid_index, 0.0);
	return thresholds[index];
}

void RawRigidsMemory::set_creep(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, float p_creep) {
	make_memory_index(p_chunk, p_rigid_index);
	creeps[index] = p_creep;
}

float RawRigidsMemory::get_creep(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const {
	make_memory_index_V(p_chunk, p_rigid_index, 0.0);
	return creeps[index];
}

void RawRigidsMemory::set_rotation(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, const Quat &p_rotation) {
	make_memory_index(p_chunk, p_rigid_index);
	rotation[index] = p_rotation;
	changed = true;
}

const Quat &RawRigidsMemory::get_rotation(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const {
	make_memory_index_V(p_chunk, p_rigid_index, return_err_quat);
	return rotation[index];
}

void RawRigidsMemory::set_position(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, const Vector3 &p_position) {
	make_memory_index(p_chunk, p_rigid_index);
	position[index] = p_position;
	changed = true;
}

const Vector3 &RawRigidsMemory::get_position(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const {
	make_memory_index_V(p_chunk, p_rigid_index, return_err_vec3);
	return position[index];
}

void RigidsMemory::_on_mapped() {
	RawRigidsMemory::_on_mapped();
	buffer_offsets.map();
}

void RigidsMemory::_on_unmapped() {
	buffer_offsets.unmap();
}

void RigidsMemory::_on_resized(FlexUnit p_size) {
	offsets.resize(p_size);
	buffer_offsets.resize(p_size + 1);
}

void RigidsMemory::_on_copied_unit(FlexUnit p_to, FlexUnit p_from) {
	offsets[p_to] = offsets[p_from];
	buffer_offsets[p_to + 1] = buffer_offsets[p_from + 1];
}

void RigidsMemory::set_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, RigidComponentIndex p_offset) {
	make_memory_index(p_chunk, p_rigid_index);
	offsets[index] = p_offset;
	changed = true;
}

RigidComponentIndex RigidsMemory::get_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const {
	make_memory_index_V(p_chunk, p_rigid_index, 0);
	return offsets[index];
}

void RigidsMemory::set_buffer_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, RigidComponentBufferIndex p_offset) {
	make_memory_index(p_chunk, p_rigid_index);
	buffer_offsets[index + 1] = p_offset;
	changed = true;
}

RigidComponentBufferIndex RigidsMemory::get_buffer_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const {
	make_memory_index_V(p_chunk, p_rigid_index, 0);
	return buffer_offsets[index + 1];
}

void RigidsMemory::zeroed_first_buffer_offset() {
	buffer_offsets[0] = 0;
}

void RigidsComponentsMemory::set_index(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index, ParticleBufferIndex p_particle_buffer_index) {
	make_memory_index(p_chunk, p_rigid_comp_index);
	indices[index] = p_particle_buffer_index;
	changed = true;
}

ParticleBufferIndex RigidsComponentsMemory::get_index(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index) const {
	make_memory_index_V(p_chunk, p_rigid_comp_index, 0);
	return indices[index];
}

void RigidsComponentsMemory::set_rest(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index, const Vector3 &p_rest) {
	make_memory_index(p_chunk, p_rigid_comp_index);
	rests[index] = p_rest;
	changed = true;
}

const Vector3 &RigidsComponentsMemory::get_rest(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index) const {
	make_memory_index_V(p_chunk, p_rigid_comp_index, return_err_vec3);
	return rests[index];
}

/*
void RigidsComponentsMemory::set_normal(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index, const Vector3 &p_normal) {
	make_memory_index(p_chunk, p_rigid_comp_index);
	normals[index] = p_normal;
	changed = true;
}

const Vector3 &RigidsComponentsMemory::get_normal(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index) const {
	make_memory_index_V(p_chunk, p_rigid_comp_index, return_err_vec3);
	return normals[index];
}
*/
