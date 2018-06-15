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

#define define_functions_1(clazz, name0)                     \
public:                                                      \
	clazz(NvFlexLibrary *p_flex_lib) : name0(p_flex_lib) {}  \
															 \
public:                                                      \
	void map() {                                             \
		name0.map();                                         \
	}                                                        \
															 \
	void unmap() {                                           \
		name0.unmap();                                       \
	}                                                        \
															 \
	void terminate() {                                       \
		name0.destroy();                                     \
	}                                                        \
															 \
protected:                                                   \
	virtual void resize_memory(FlexUnit p_size) {            \
		name0.resize(p_size);                                \
	}                                                        \
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from) { \
		name0[p_to] = name0[p_from];                         \
	}                                                        \
															 \
public:

#define define_functions_2(clazz, name0, name1)                                \
public:                                                                        \
	clazz(NvFlexLibrary *p_flex_lib) : name0(p_flex_lib), name1(p_flex_lib) {} \
	void map() {                                                               \
		name0.map();                                                           \
		name1.map();                                                           \
	}                                                                          \
																			   \
	void unmap() {                                                             \
		name0.unmap();                                                         \
		name1.unmap();                                                         \
	}                                                                          \
																			   \
	void terminate() {                                                         \
		name0.destroy();                                                       \
		name1.destroy();                                                       \
	}                                                                          \
																			   \
protected:                                                                     \
	virtual void resize_memory(FlexUnit p_size) {                              \
		name0.resize(p_size);                                                  \
		name1.resize(p_size);                                                  \
	}                                                                          \
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from) {                   \
		name0[p_to] = name0[p_from];                                           \
		name1[p_to] = name1[p_from];                                           \
	}                                                                          \
																			   \
public:

#define define_functions_3(clazz, name0, name1, name2)                                            \
public:                                                                                           \
	clazz(NvFlexLibrary *p_flex_lib) : name0(p_flex_lib), name1(p_flex_lib), name2(p_flex_lib) {} \
	void map() {                                                                                  \
		name0.map();                                                                              \
		name1.map();                                                                              \
		name2.map();                                                                              \
	}                                                                                             \
																								  \
	void unmap() {                                                                                \
		name0.unmap();                                                                            \
		name1.unmap();                                                                            \
		name2.unmap();                                                                            \
	}                                                                                             \
																								  \
	void terminate() {                                                                            \
		name0.destroy();                                                                          \
		name1.destroy();                                                                          \
		name2.destroy();                                                                          \
	}                                                                                             \
																								  \
protected:                                                                                        \
	virtual void resize_memory(FlexUnit p_size) {                                                 \
		name0.resize(p_size);                                                                     \
		name1.resize(p_size);                                                                     \
		name2.resize(p_size);                                                                     \
	}                                                                                             \
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from) {                                      \
		name0[p_to] = name0[p_from];                                                              \
		name1[p_to] = name1[p_from];                                                              \
		name2[p_to] = name2[p_from];                                                              \
	}                                                                                             \
																								  \
public:

#define define_functions_4(clazz, name0, name1, name2, name3)                                                        \
public:                                                                                                              \
	clazz(NvFlexLibrary *p_flex_lib) : name0(p_flex_lib), name1(p_flex_lib), name2(p_flex_lib), name3(p_flex_lib) {} \
	void map() {                                                                                                     \
		name0.map();                                                                                                 \
		name1.map();                                                                                                 \
		name2.map();                                                                                                 \
		name3.map();                                                                                                 \
	}                                                                                                                \
																													 \
	void unmap() {                                                                                                   \
		name0.unmap();                                                                                               \
		name1.unmap();                                                                                               \
		name2.unmap();                                                                                               \
		name3.unmap();                                                                                               \
	}                                                                                                                \
																													 \
	void terminate() {                                                                                               \
		name0.destroy();                                                                                             \
		name1.destroy();                                                                                             \
		name2.destroy();                                                                                             \
		name3.destroy();                                                                                             \
	}                                                                                                                \
																													 \
protected:                                                                                                           \
	virtual void resize_memory(FlexUnit p_size) {                                                                    \
		name0.resize(p_size);                                                                                        \
		name1.resize(p_size);                                                                                        \
		name2.resize(p_size);                                                                                        \
		name3.resize(p_size);                                                                                        \
	}                                                                                                                \
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from) {                                                         \
		name0[p_to] = name0[p_from];                                                                                 \
		name1[p_to] = name1[p_from];                                                                                 \
		name2[p_to] = name2[p_from];                                                                                 \
		name3[p_to] = name3[p_from];                                                                                 \
	}                                                                                                                \
																													 \
public:

#define define_functions_6(clazz, name0, name1, name2, name3, name4, name5)                                                                                \
public:                                                                                                                                                    \
	clazz(NvFlexLibrary *p_flex_lib) : name0(p_flex_lib), name1(p_flex_lib), name2(p_flex_lib), name3(p_flex_lib), name4(p_flex_lib), name5(p_flex_lib) {} \
	void map() {                                                                                                                                           \
		name0.map();                                                                                                                                       \
		name1.map();                                                                                                                                       \
		name2.map();                                                                                                                                       \
		name3.map();                                                                                                                                       \
		name4.map();                                                                                                                                       \
		name5.map();                                                                                                                                       \
	}                                                                                                                                                      \
																																						   \
	void unmap() {                                                                                                                                         \
		name0.unmap();                                                                                                                                     \
		name1.unmap();                                                                                                                                     \
		name2.unmap();                                                                                                                                     \
		name3.unmap();                                                                                                                                     \
		name4.unmap();                                                                                                                                     \
		name5.unmap();                                                                                                                                     \
	}                                                                                                                                                      \
																																						   \
	void terminate() {                                                                                                                                     \
		name0.destroy();                                                                                                                                   \
		name1.destroy();                                                                                                                                   \
		name2.destroy();                                                                                                                                   \
		name3.destroy();                                                                                                                                   \
		name4.destroy();                                                                                                                                   \
		name5.destroy();                                                                                                                                   \
	}                                                                                                                                                      \
																																						   \
protected:                                                                                                                                                 \
	virtual void resize_memory(FlexUnit p_size) {                                                                                                          \
		name0.resize(p_size);                                                                                                                              \
		name1.resize(p_size);                                                                                                                              \
		name2.resize(p_size);                                                                                                                              \
		name3.resize(p_size);                                                                                                                              \
		name4.resize(p_size);                                                                                                                              \
		name5.resize(p_size);                                                                                                                              \
	}                                                                                                                                                      \
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from) {                                                                                               \
		name0[p_to] = name0[p_from];                                                                                                                       \
		name1[p_to] = name1[p_from];                                                                                                                       \
		name2[p_to] = name2[p_from];                                                                                                                       \
		name3[p_to] = name3[p_from];                                                                                                                       \
		name4[p_to] = name4[p_from];                                                                                                                       \
		name5[p_to] = name5[p_from];                                                                                                                       \
	}                                                                                                                                                      \
																																						   \
public:

#define create_buffer_1(clazz, type, name) \
	friend class FlexSpace;                \
										   \
private:                                   \
	NvFlexVector<type> name;               \
	define_functions_1(clazz, name);

#define create_buffer_2(clazz, type0, name0, type1, name1) \
	friend class FlexSpace;                                \
														   \
private:                                                   \
	NvFlexVector<type0> name0;                             \
	NvFlexVector<type1> name1;                             \
	define_functions_2(clazz, name0, name1);

#define create_buffer_3(clazz, type0, name0, type1, name1, type2, name2) \
	friend class FlexSpace;                                              \
																		 \
private:                                                                 \
	NvFlexVector<type0> name0;                                           \
	NvFlexVector<type1> name1;                                           \
	NvFlexVector<type2> name2;                                           \
	define_functions_3(clazz, name0, name1, name2);

#define create_buffer_4(clazz, type0, name0, type1, name1, type2, name2, type3, name3) \
	friend class FlexSpace;                                                            \
																					   \
private:                                                                               \
	NvFlexVector<type0> name0;                                                         \
	NvFlexVector<type1> name1;                                                         \
	NvFlexVector<type2> name2;                                                         \
	NvFlexVector<type3> name3;                                                         \
	define_functions_4(clazz, name0, name1, name2, name3);

#define create_buffer_6(clazz, type0, name0, type1, name1, type2, name2, type3, name3, type4, name4, type5, name5) \
	friend class FlexSpace;                                                                                        \
																												   \
private:                                                                                                           \
	NvFlexVector<type0> name0;                                                                                     \
	NvFlexVector<type1> name1;                                                                                     \
	NvFlexVector<type2> name2;                                                                                     \
	NvFlexVector<type3> name3;                                                                                     \
	NvFlexVector<type4> name4;                                                                                     \
	NvFlexVector<type5> name5;                                                                                     \
	define_functions_6(clazz, name0, name1, name2, name3, name4, name5);

class ParticleBodiesMemory : public FlexMemory {

	// particles: XYZ world position, W inverse mass
	// Phases: This is a flag that specify behaviour of particle like collision etc.. https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/flex/manual.html#phase
	// TODO remove normals
	create_buffer_4(ParticleBodiesMemory, FlVector4, particles, Vector3, velocities, int, phases, FlVector4, normals);

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

	const FlVector4 &get_normal(const MemoryChunk *p_chunk, ParticleIndex p_particle_index) const;
};

class ActiveParticlesMemory : public FlexMemory {

	create_buffer_1(ActiveParticlesMemory, int, active_particles);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V

	void set_active_particle(const MemoryChunk *p_chunk, ActiveParticleIndex p_active_particle_index, ParticleBufferIndex p_particle_buffer_index);
};

class SpringMemory : public FlexMemory {

	bool changed;

	create_buffer_3(SpringMemory, Spring, springs, float, lengths, float, stiffness);

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
};

/// This represent primitive body
class GeometryMemory : public FlexMemory {

	bool changed;
	create_buffer_6(GeometryMemory, NvFlexCollisionGeometry, collision_shapes, FlVector4, positions, Quat, rotations, FlVector4, positions_prev, Quat, rotations_prev, int, flags);

	bool was_changed() { return changed; }

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

	void set_position_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const FlVector4 &p_position);
	const FlVector4 &get_position_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const;

	void set_rotation_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, const Quat &p_rotation);
	const Quat &get_rotation_prev(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const;

	void set_flags(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index, int p_flags);
	int get_flags(const MemoryChunk *p_chunk, GeometryIndex p_geometry_index) const;
};

class RigidsMemory : public FlexMemory {

	create_buffer_4(RigidsMemory, int, offsets, float, stiffness, Quat, rotation, Vector3, position);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V
	///
};

/// This memory is used to store information for each rigid
class RigidsComponentsMemory : public FlexMemory {

	create_buffer_3(RigidsComponentsMemory, int, indices, Vector3, rests, FlVector4, normals);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V
	///
};
#endif // FLEX_MEMORY_H
