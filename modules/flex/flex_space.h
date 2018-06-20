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

#include "flex_utility.h"

class NvFlexLibrary;
class NvFlexSolver;
class FlexParticleBody;
class FlexPrimitiveBody;
class FlexMemoryAllocator;
class ParticleBodiesMemory;
class ActiveParticlesMemory;
class SpringMemory;
class RigidsMemory;
class RigidsComponentsMemory;
class GeometryMemory;

class FlexSpace : public RIDFlex {
	friend class FlexBuffers;
	friend class FlexParticleBodyCommands;

	NvFlexLibrary *flex_lib;
	NvFlexSolver *solver;

	FlexMemoryAllocator *particle_bodies_allocator;
	ParticleBodiesMemory *particle_bodies_memory;

	FlexMemoryAllocator *active_particles_allocator;
	ActiveParticlesMemory *active_particles_memory;
	MemoryChunk *active_particles_mchunk;

	FlexMemoryAllocator *springs_allocator;
	SpringMemory *springs_memory;

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

	_FORCE_INLINE_ FlexMemoryAllocator *get_particle_bodies_allocator() { return particle_bodies_allocator; }
	_FORCE_INLINE_ ParticleBodiesMemory *get_particle_bodies_memory() { return particle_bodies_memory; }
	_FORCE_INLINE_ FlexMemoryAllocator *get_springs_allocator() { return springs_allocator; }
	_FORCE_INLINE_ RigidsMemory *get_rigids_memory() { return rigids_memory; }
	_FORCE_INLINE_ SpringMemory *get_springs_memory() { return springs_memory; }

	void add_particle_body(FlexParticleBody *p_body);
	void remove_particle_body(FlexParticleBody *p_body);

	void add_primitive_body(FlexPrimitiveBody *p_body);
	void remove_primitive_body(FlexPrimitiveBody *p_body);

	float get_particle_radius() const { return particle_radius; }

private:
	void dispatch_callbacks();
	void execute_delayed_commands();
	void rebuild_rigids_offsets();
	void execute_geometries_commands();

	void commands_write_buffer();
	void commands_read_buffer();

	void on_particle_removed(FlexParticleBody *p_body, ParticleBufferIndex p_index);
	void on_particle_index_changed(FlexParticleBody *p_body, ParticleBufferIndex p_index_old, ParticleBufferIndex p_index_new);
};

#endif // FLEX_SPACE_H
