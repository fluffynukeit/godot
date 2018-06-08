/*************************************************************************/
/*  flex_memory.h                                                        */
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

#ifndef FLEX_MEMORY_H
#define FLEX_MEMORY_H

#include "flex_memory_allocator.h"
#include "flex_utility.h"
#include "thirdparty/flex/include/NvFlexExt.h"

typedef int ParticleIndex; // Particle id relative to body, can change during time
typedef int ParticleRef; // Particle Ref id relative to body never change

class ParticleBodiesMemory : public FlexMemory {

	friend class FlexSpace;

	NvFlexVector<FlVector4> particles; // XYZ world position, W inverse mass
	NvFlexVector<Vector3> velocities;
	NvFlexVector<int> phases; // This is a flag that specify behaviour of particle like collision etc.. https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/flex/manual.html#phase

public:
	ParticleBodiesMemory(NvFlexLibrary *p_flex_lib);

	void map();
	void unmap();
	void terminate();

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V

	void set_particle(const MemoryChunk *p_chunk, ParticleIndex p_particle_index, FlVector4 p_particle);
	const FlVector4 &get_particle(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const;

	void set_velocity(const MemoryChunk *p_chunk, ParticleIndex p_particle_index, Vector3 p_velocity);
	const Vector3 &get_velocity(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const;

	void set_phase(const MemoryChunk *p_chunk, ParticleIndex p_particle_index, int p_phase);
	int get_phase(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const;

protected:
	virtual void resize_memory(FlexUnit p_size);
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from);
};

class ActiveParticlesMemory : public FlexMemory {

	friend class FlexSpace;

	NvFlexVector<int> active_particles; // TODO this function can't stay here, should be handled outside this buffer.

public:
	ActiveParticlesMemory(NvFlexLibrary *p_flex_lib);

	void map();
	void unmap();
	void terminate();

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V

	void set_active_particle(const MemoryChunk *p_chunk, ActiveParticleIndex p_active_particle_index, ParticleBufferIndex p_particle_buffer_index);

protected:
	virtual void resize_memory(FlexUnit p_size);
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from);
};

class SpringMemory : public FlexMemory {

	friend class FlexSpace;

	NvFlexVector<Spring> springs;
	NvFlexVector<float> lengths;
	NvFlexVector<float> stiffness;

	bool changed;

public:
	SpringMemory(NvFlexLibrary *p_flex_lib);

	void map();
	void unmap();
	void terminate();
	bool was_changed() { return changed; }

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V

	void set_spring(const MemoryChunk *p_chunk, SpringIndex p_spring_index, const Spring &p_spring);
	const Spring &get_spring(const MemoryChunk *p_chunk, SpringIndex p_spring_index) const;

	void set_length(const MemoryChunk *p_chunk, SpringIndex p_spring_index, float p_length);
	float get_length(const MemoryChunk *p_chunk, SpringIndex p_spring_index) const;

	void set_stiffness(const MemoryChunk *p_chunk, SpringIndex p_spring_index, float p_stifness);
	float get_stiffness(const MemoryChunk *p_chunk, SpringIndex p_spring_index) const;

protected:
	virtual void resize_memory(FlexUnit p_size);
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from);
};

/// This represent primitive body
class GeometryMemory : public FlexMemory {

	friend class FlexSpace;

	NvFlexVector<NvFlexCollisionGeometry> collision_shapes;
	NvFlexVector<FlVector4> positions;
	NvFlexVector<Quat> rotations;
	NvFlexVector<int> flags;

public:
	GeometryMemory(NvFlexLibrary *p_lib);

	void map();
	void unmap();
	void terminate();

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V
	///

	void set_shape(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const NvFlexCollisionGeometry &p_shape);
	NvFlexCollisionGeometry get_shape(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const;

	void set_position(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const FlVector4 &p_position);
	const FlVector4 &get_position(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const;

	void set_rotation(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const Quat &p_rotation);
	const Quat &get_rotation(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const;

	void set_flags(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, int p_flags);
	int get_flags(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const;

protected:
	virtual void resize_memory(FlexUnit p_size);
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from);
};

#endif // FLEX_MEMORY_H
