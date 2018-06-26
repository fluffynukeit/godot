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

#define FLEXBUFFERCLASS_1(clazz, type0, name0) \
	friend class FlexSpace;                    \
											   \
	NvFlexVector<type0> name0;                 \
											   \
public:                                        \
	clazz(NvFlexLibrary *p_flex_lib) :         \
			name0(p_flex_lib) {                \
		__add_buffer(&name0);                  \
	}

#define FLEXBUFFERCLASS_2(clazz, type0, name0, type1, name1) \
	friend class FlexSpace;                                  \
															 \
	NvFlexVector<type0> name0;                               \
	NvFlexVector<type1> name1;                               \
															 \
public:                                                      \
	clazz(NvFlexLibrary *p_flex_lib) :                       \
			name0(p_flex_lib),                               \
			name1(p_flex_lib) {                              \
		__add_buffer(&name0);                                \
		__add_buffer(&name1);                                \
	}

#define FLEXBUFFERCLASS_3(clazz, type0, name0, type1, name1, type2, name2) \
	friend class FlexSpace;                                                \
																		   \
	NvFlexVector<type0> name0;                                             \
	NvFlexVector<type1> name1;                                             \
	NvFlexVector<type2> name2;                                             \
																		   \
public:                                                                    \
	clazz(NvFlexLibrary *p_flex_lib) :                                     \
			name0(p_flex_lib),                                             \
			name1(p_flex_lib),                                             \
			name2(p_flex_lib) {                                            \
		__add_buffer(&name0);                                              \
		__add_buffer(&name1);                                              \
		__add_buffer(&name2);                                              \
	}

#define FLEXBUFFERCLASS_4(clazz, type0, name0, type1, name1, type2, name2, type3, name3) \
	friend class FlexSpace;                                                              \
																						 \
	NvFlexVector<type0> name0;                                                           \
	NvFlexVector<type1> name1;                                                           \
	NvFlexVector<type2> name2;                                                           \
	NvFlexVector<type3> name3;                                                           \
																						 \
public:                                                                                  \
	clazz(NvFlexLibrary *p_flex_lib) :                                                   \
			name0(p_flex_lib),                                                           \
			name1(p_flex_lib),                                                           \
			name2(p_flex_lib),                                                           \
			name3(p_flex_lib) {                                                          \
		__add_buffer(&name0);                                                            \
		__add_buffer(&name1);                                                            \
		__add_buffer(&name2);                                                            \
		__add_buffer(&name3);                                                            \
	}

#define FLEXBUFFERCLASS_5(clazz, type0, name0, type1, name1, type2, name2, type3, name3, type4, name4) \
	friend class FlexSpace;                                                                            \
																									   \
	NvFlexVector<type0> name0;                                                                         \
	NvFlexVector<type1> name1;                                                                         \
	NvFlexVector<type2> name2;                                                                         \
	NvFlexVector<type3> name3;                                                                         \
	NvFlexVector<type4> name4;                                                                         \
																									   \
public:                                                                                                \
	clazz(NvFlexLibrary *p_flex_lib) :                                                                 \
			name0(p_flex_lib),                                                                         \
			name1(p_flex_lib),                                                                         \
			name2(p_flex_lib),                                                                         \
			name3(p_flex_lib),                                                                         \
			name4(p_flex_lib) {                                                                        \
		__add_buffer(&name0);                                                                          \
		__add_buffer(&name1);                                                                          \
		__add_buffer(&name2);                                                                          \
		__add_buffer(&name3);                                                                          \
		__add_buffer(&name4);                                                                          \
	}

#define FLEXBUFFERCLASS_6(clazz, type0, name0, type1, name1, type2, name2, type3, name3, type4, name4, type5, name5) \
	friend class FlexSpace;                                                                                          \
																													 \
	NvFlexVector<type0> name0;                                                                                       \
	NvFlexVector<type1> name1;                                                                                       \
	NvFlexVector<type2> name2;                                                                                       \
	NvFlexVector<type3> name3;                                                                                       \
	NvFlexVector<type4> name4;                                                                                       \
	NvFlexVector<type5> name5;                                                                                       \
																													 \
public:                                                                                                              \
	clazz(NvFlexLibrary *p_flex_lib) :                                                                               \
			name0(p_flex_lib),                                                                                       \
			name1(p_flex_lib),                                                                                       \
			name2(p_flex_lib),                                                                                       \
			name3(p_flex_lib),                                                                                       \
			name4(p_flex_lib),                                                                                       \
			name5(p_flex_lib) {                                                                                      \
		__add_buffer(&name0);                                                                                        \
		__add_buffer(&name1);                                                                                        \
		__add_buffer(&name2);                                                                                        \
		__add_buffer(&name3);                                                                                        \
		__add_buffer(&name4);                                                                                        \
		__add_buffer(&name5);                                                                                        \
	}

class FlexBufferMemory : public FlexMemory {

	Vector<NvFlexVector<int> *> buffers_int;
	Vector<NvFlexVector<float> *> buffers_float;
	Vector<NvFlexVector<Quat> *> buffers_quat;
	Vector<NvFlexVector<Vector3> *> buffers_vec3;
	Vector<NvFlexVector<Spring> *> buffers_spring;
	Vector<NvFlexVector<DynamicTriangle> *> buffers_dintriangles;
	Vector<NvFlexVector<FlVector4> *> buffers_flvec4;
	Vector<NvFlexVector<NvFlexCollisionGeometry> *> buffers_colgeo;

protected:
	bool changed;

protected:
	void __add_buffer(NvFlexVector<int> *buffer) {
		buffers_int.push_back(buffer);
	}
	void __add_buffer(NvFlexVector<float> *buffer) {
		buffers_float.push_back(buffer);
	}
	void __add_buffer(NvFlexVector<Quat> *buffer) {
		buffers_quat.push_back(buffer);
	}
	void __add_buffer(NvFlexVector<Vector3> *buffer) {
		buffers_vec3.push_back(buffer);
	}
	void __add_buffer(NvFlexVector<Spring> *buffer) {
		buffers_spring.push_back(buffer);
	}
	void __add_buffer(NvFlexVector<DynamicTriangle> *buffer) {
		buffers_dintriangles.push_back(buffer);
	}
	void __add_buffer(NvFlexVector<FlVector4> *buffer) {
		buffers_flvec4.push_back(buffer);
	}
	void __add_buffer(NvFlexVector<NvFlexCollisionGeometry> *buffer) {
		buffers_colgeo.push_back(buffer);
	}

public:
	FlexBufferMemory() {}

	void map() {
		for (int i(buffers_int.size() - 1); 0 <= i; --i) {
			buffers_int[i]->map();
		}
		for (int i(buffers_float.size() - 1); 0 <= i; --i) {
			buffers_float[i]->map();
		}
		for (int i(buffers_quat.size() - 1); 0 <= i; --i) {
			buffers_quat[i]->map();
		}
		for (int i(buffers_vec3.size() - 1); 0 <= i; --i) {
			buffers_vec3[i]->map();
		}
		for (int i(buffers_spring.size() - 1); 0 <= i; --i) {
			buffers_spring[i]->map();
		}
		for (int i(buffers_dintriangles.size() - 1); 0 <= i; --i) {
			buffers_dintriangles[i]->map();
		}
		for (int i(buffers_flvec4.size() - 1); 0 <= i; --i) {
			buffers_flvec4[i]->map();
		}
		for (int i(buffers_colgeo.size() - 1); 0 <= i; --i) {
			buffers_colgeo[i]->map();
		}
		_on_mapped();
	}

	void unmap() {
		for (int i(buffers_int.size() - 1); 0 <= i; --i) {
			buffers_int[i]->unmap();
		}
		for (int i(buffers_float.size() - 1); 0 <= i; --i) {
			buffers_float[i]->unmap();
		}
		for (int i(buffers_quat.size() - 1); 0 <= i; --i) {
			buffers_quat[i]->unmap();
		}
		for (int i(buffers_vec3.size() - 1); 0 <= i; --i) {
			buffers_vec3[i]->unmap();
		}
		for (int i(buffers_spring.size() - 1); 0 <= i; --i) {
			buffers_spring[i]->unmap();
		}
		for (int i(buffers_dintriangles.size() - 1); 0 <= i; --i) {
			buffers_dintriangles[i]->unmap();
		}
		for (int i(buffers_flvec4.size() - 1); 0 <= i; --i) {
			buffers_flvec4[i]->unmap();
		}
		for (int i(buffers_colgeo.size() - 1); 0 <= i; --i) {
			buffers_colgeo[i]->unmap();
		}
		_on_unmapped();
	}

	void terminate() {
		for (int i(buffers_int.size() - 1); 0 <= i; --i) {
			buffers_int[i]->destroy();
		}
		for (int i(buffers_float.size() - 1); 0 <= i; --i) {
			buffers_float[i]->destroy();
		}
		for (int i(buffers_quat.size() - 1); 0 <= i; --i) {
			buffers_quat[i]->destroy();
		}
		for (int i(buffers_vec3.size() - 1); 0 <= i; --i) {
			buffers_vec3[i]->destroy();
		}
		for (int i(buffers_spring.size() - 1); 0 <= i; --i) {
			buffers_spring[i]->destroy();
		}
		for (int i(buffers_dintriangles.size() - 1); 0 <= i; --i) {
			buffers_dintriangles[i]->destroy();
		}
		for (int i(buffers_flvec4.size() - 1); 0 <= i; --i) {
			buffers_flvec4[i]->destroy();
		}
		for (int i(buffers_colgeo.size() - 1); 0 <= i; --i) {
			buffers_colgeo[i]->destroy();
		}
	}

	bool was_changed() { return changed; }

	virtual void _on_mapped() { changed = false; }
	virtual void _on_unmapped() {}
	virtual void _on_resized(FlexUnit p_size) { changed = true; }
	virtual void _on_copied_unit(FlexUnit p_to, FlexUnit p_from) { changed = true; }

protected:
	virtual void resize_memory(FlexUnit p_size) {
		for (int i(buffers_int.size() - 1); 0 <= i; --i) {
			buffers_int[i]->resize(p_size);
		}
		for (int i(buffers_float.size() - 1); 0 <= i; --i) {
			buffers_float[i]->resize(p_size);
		}
		for (int i(buffers_quat.size() - 1); 0 <= i; --i) {
			buffers_quat[i]->resize(p_size);
		}
		for (int i(buffers_vec3.size() - 1); 0 <= i; --i) {
			buffers_vec3[i]->resize(p_size);
		}
		for (int i(buffers_spring.size() - 1); 0 <= i; --i) {
			buffers_spring[i]->resize(p_size);
		}
		for (int i(buffers_dintriangles.size() - 1); 0 <= i; --i) {
			buffers_dintriangles[i]->resize(p_size);
		}
		for (int i(buffers_flvec4.size() - 1); 0 <= i; --i) {
			buffers_flvec4[i]->resize(p_size);
		}
		for (int i(buffers_colgeo.size() - 1); 0 <= i; --i) {
			buffers_colgeo[i]->resize(p_size);
		}
		_on_resized(p_size);
	}
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from) {
		for (int i(buffers_int.size() - 1); 0 <= i; --i) {
			(*buffers_int[i])[p_to] = (*buffers_int[i])[p_from];
		}
		for (int i(buffers_float.size() - 1); 0 <= i; --i) {
			(*buffers_float[i])[p_to] = (*buffers_float[i])[p_from];
		}
		for (int i(buffers_quat.size() - 1); 0 <= i; --i) {
			(*buffers_quat[i])[p_to] = (*buffers_quat[i])[p_from];
		}
		for (int i(buffers_vec3.size() - 1); 0 <= i; --i) {
			(*buffers_vec3[i])[p_to] = (*buffers_vec3[i])[p_from];
		}
		for (int i(buffers_spring.size() - 1); 0 <= i; --i) {
			(*buffers_spring[i])[p_to] = (*buffers_spring[i])[p_from];
		}
		for (int i(buffers_dintriangles.size() - 1); 0 <= i; --i) {
			(*buffers_dintriangles[i])[p_to] = (*buffers_dintriangles[i])[p_from];
		}
		for (int i(buffers_flvec4.size() - 1); 0 <= i; --i) {
			(*buffers_flvec4[i])[p_to] = (*buffers_flvec4[i])[p_from];
		}
		for (int i(buffers_colgeo.size() - 1); 0 <= i; --i) {
			(*buffers_colgeo[i])[p_to] = (*buffers_colgeo[i])[p_from];
		}
		_on_copied_unit(p_to, p_from);
	}
};

class ParticlesMemory : public FlexBufferMemory {

	// particles: XYZ world position, W inverse mass
	// Phases: This is a flag that specify behaviour of particle like collision etc.. https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/flex/manual.html#phase
	// TODO remove normals
	FLEXBUFFERCLASS_4(ParticlesMemory, FlVector4, particles, Vector3, velocities, int, phases, FlVector4, normals);

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

class ActiveParticlesMemory : public FlexBufferMemory {

	FLEXBUFFERCLASS_1(ActiveParticlesMemory, FlexIndex, active_particles);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V

	void set_active_particle(const MemoryChunk *p_chunk, ActiveParticleIndex p_active_particle_index, ParticleBufferIndex p_particle_buffer_index);
};

class SpringMemory : public FlexBufferMemory {

	FLEXBUFFERCLASS_3(SpringMemory, Spring, springs, float, lengths, float, stiffness);

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

class DynamicTrianglesMemory : public FlexBufferMemory {

	FLEXBUFFERCLASS_1(DynamicTrianglesMemory, DynamicTriangle, triangles);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V
	///

	void set_triangle(const MemoryChunk *p_chunk, TriangleIndex p_triangle_index, const DynamicTriangle &p_triangle);
	const DynamicTriangle &get_triangle(const MemoryChunk *p_chunk, TriangleIndex p_triangle_index) const;
};

/// This represent primitive body
class GeometryMemory : public FlexBufferMemory {

	FLEXBUFFERCLASS_6(GeometryMemory, NvFlexCollisionGeometry, collision_shapes, FlVector4, positions, Quat, rotations, FlVector4, positions_prev, Quat, rotations_prev, int, flags);

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

class RawRigidsMemory : public FlexBufferMemory {

protected:
	FLEXBUFFERCLASS_5(RawRigidsMemory, float, stiffness, float, thresholds, float, creeps, Quat, rotation, Vector3, position);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V
	///

	void set_stiffness(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, float p_stiffness);
	float get_stiffness(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const;

	void set_threshold(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, float p_threshold);
	float get_threshold(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const;

	void set_creep(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, float p_creep);
	float get_creep(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const;

	void set_rotation(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, const Quat &p_rotation);
	const Quat &get_rotation(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const;

	void set_position(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, const Vector3 &p_position);
	const Vector3 &get_position(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const;
};

// This buffer is handled like this because Flex required to have one more element in buffers_offsets with value 0
// In this way I'm able to control buffer_offsets separately
class RigidsMemory : public RawRigidsMemory {

	friend class FlexSpace;

	Vector<FlexIndex> offsets;
	NvFlexVector<FlexIndex> buffer_offsets;

public:
	RigidsMemory(NvFlexLibrary *p_library) :
			RawRigidsMemory(p_library),
			buffer_offsets(p_library) {}

	virtual void _on_mapped();
	virtual void _on_unmapped();
	virtual void _on_resized(FlexUnit p_size);
	virtual void _on_copied_unit(FlexUnit p_to, FlexUnit p_from);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V
	///

	void set_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, RigidComponentIndex p_offset);
	RigidComponentIndex get_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const;

	void set_buffer_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index, RigidComponentBufferIndex p_offset);
	RigidComponentBufferIndex get_buffer_offset(const MemoryChunk *p_chunk, RigidIndex p_rigid_index) const;

	// Special function to set 0 on the first element of buffer offsets as specified by Flex
	void zeroed_first_buffer_offset();
};

/// This memory is used to store information for each rigid
class RigidsComponentsMemory : public FlexBufferMemory {

	FLEXBUFFERCLASS_2(RigidsComponentsMemory, FlexIndex, indices, Vector3, rests);
	//FLEXBUFFERCLASS_3(RigidsComponentsMemory, ParticleBufferIndex, indices, Vector3, rests, Vector3, normals);

	/// IMPORTANT
	/// These functions must be called only if the buffers are mapped
	/// |
	/// |
	/// V
	///

	void set_index(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index, ParticleBufferIndex p_particle_buffer_index);
	ParticleBufferIndex get_index(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index) const;

	void set_rest(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index, const Vector3 &p_rest);
	const Vector3 &get_rest(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index) const;

	//void set_normal(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index, const Vector3 &p_normal);
	//const Vector3 &get_normal(const MemoryChunk *p_chunk, RigidComponentIndex p_rigid_comp_index) const;
};

#endif // FLEX_MEMORY_H
