/*************************************************************************/
/*  flex_space.h                                                         */
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

#ifndef FLEX_SPACE_H
#define FLEX_SPACE_H

#include "rid_flex.h"

#include "flex_memory_allocator.h"
#include "flex_utility.h"

class NvFlexLibrary;
class NvFlexSolver;
class FlexParticleBody;
class FlexPrimitiveBody;
class FlexMemoryAllocator;
class ParticlesMemory;
class ActiveParticlesMemory;
class SpringMemory;
class DynamicTrianglesMemory;
class RigidsMemory;
class RigidsComponentsMemory;
class GeometryMemory;

class FlexSpace : public RIDFlex {
	friend class FlexBuffers;
	friend class FlexParticleBodyCommands;

	NvFlexLibrary *flex_lib;
	NvFlexSolver *solver;

	FlexMemoryAllocator *particles_allocator;
	ParticlesMemory *particles_memory;

	FlexMemoryAllocator *active_particles_allocator;
	ActiveParticlesMemory *active_particles_memory;
	MemoryChunk *active_particles_mchunk;

	FlexMemoryAllocator *springs_allocator;
	SpringMemory *springs_memory;

	FlexMemoryAllocator *triangles_allocator;
	DynamicTrianglesMemory *triangles_memory;

	FlexMemoryAllocator *rigids_allocator;
	RigidsMemory *rigids_memory;

	FlexMemoryAllocator *rigids_components_allocator;
	RigidsComponentsMemory *rigids_components_memory;

	FlexMemoryAllocator *geometries_allocator;
	GeometryMemory *geometries_memory;

	Vector<FlexParticleBody *> particle_bodies;
	Vector<FlexPrimitiveBody *> primitive_bodies;

	Vector<MemoryChunk *> geometry_chunks_to_deallocate;

	float particle_radius;

public:
	FlexSpace();
	~FlexSpace();

	void init();
	void terminate();
	void sync();
	void step(real_t p_delta_time);

	_FORCE_INLINE_ FlexMemoryAllocator *get_particles_allocator() { return particles_allocator; }
	_FORCE_INLINE_ ParticlesMemory *get_particles_memory() { return particles_memory; }
	_FORCE_INLINE_ FlexMemoryAllocator *get_springs_allocator() { return springs_allocator; }
	_FORCE_INLINE_ RigidsMemory *get_rigids_memory() { return rigids_memory; }
	_FORCE_INLINE_ RigidsComponentsMemory *get_rigids_components_memory() { return rigids_components_memory; }
	_FORCE_INLINE_ SpringMemory *get_springs_memory() { return springs_memory; }

	void add_particle_body(FlexParticleBody *p_body);
	void remove_particle_body(FlexParticleBody *p_body);

	void add_primitive_body(FlexPrimitiveBody *p_body);
	void remove_primitive_body(FlexPrimitiveBody *p_body);

	float get_particle_radius() const { return particle_radius; }

	// internals
	void dispatch_callbacks();
	void execute_delayed_commands();
	void rebuild_rigids_offsets();
	void execute_geometries_commands();

	void commands_write_buffer();
	void commands_read_buffer();

	void on_particle_removed(FlexParticleBody *p_body, ParticleBufferIndex p_index);
	void on_particle_index_changed(FlexParticleBody *p_body, ParticleBufferIndex p_index_old, ParticleBufferIndex p_index_new);
};

class FlexMemorySweeper : public FlexMemoryModificator {
};

// Change index of last index
class FlexMemorySweeperFast : public FlexMemorySweeper {
protected:
	FlexMemoryAllocator *allocator;
	MemoryChunk *&mchunk;
	Vector<FlexChunkIndex> &indices_to_remove;

public:
	FlexMemorySweeperFast(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove);

	virtual void on_element_removed(FlexBufferIndex on_element_removed) {} // Just after removal
	virtual void on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index) {}

	virtual void exec();
};

class ParticlesMemorySweeper : public FlexMemorySweeperFast {
	FlexSpace *space;
	FlexParticleBody *body;

public:
	ParticlesMemorySweeper(FlexSpace *p_space, FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove);

	virtual void on_element_removed(FlexBufferIndex on_element_removed);
	virtual void on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index);
};

class SpringsMemorySweeper : public FlexMemorySweeperFast {
	FlexParticleBody *body;

public:
	SpringsMemorySweeper(FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove);

	virtual void on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index);
};

class TrianglesMemorySweeper : public FlexMemorySweeperFast {
	FlexParticleBody *body;

public:
	TrianglesMemorySweeper(FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove);

	virtual void on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index);
};

/// Maintain order but change indices
/// r_indices_to_remove will be unusable after this
class FlexMemorySweeperSlow : public FlexMemorySweeper {
protected:
	FlexMemoryAllocator *allocator;
	MemoryChunk *&mchunk;
	Vector<FlexChunkIndex> &indices_to_remove;

public:
	FlexMemorySweeperSlow(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove);

	virtual void on_element_remove(FlexChunkIndex on_element_removed) {} // Before removal
	virtual void on_element_removed(FlexChunkIndex on_element_removed) {} // Just after removal

	virtual void exec();
};

class RigidsComponentsMemorySweeper : public FlexMemorySweeperSlow {

	RigidsMemory *rigids_memory;
	MemoryChunk *&rigids_mchunk;

public:
	RigidsComponentsMemorySweeper(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove, RigidsMemory *p_rigids_memory, MemoryChunk *&r_rigids_mchunk);

	virtual void on_element_removed(RigidComponentIndex on_element_removed);
};

class RigidsMemorySweeper : public FlexMemorySweeperSlow {

	RigidsMemory *rigids_memory;

	FlexMemoryAllocator *rigids_components_allocator;
	RigidsComponentsMemory *rigids_components_memory;
	MemoryChunk *&rigids_components_mchunk;

	int rigid_particle_index_count;

public:
	RigidsMemorySweeper(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove, RigidsMemory *p_rigids_memory, FlexMemoryAllocator *p_rigids_components_allocator, RigidsComponentsMemory *p_rigids_components_memory, MemoryChunk *&r_rigids_components_mchunk);

	virtual void on_element_remove(RigidIndex on_element_removed);
	virtual void on_element_removed(RigidIndex on_element_removed);
};

#endif // FLEX_SPACE_H
