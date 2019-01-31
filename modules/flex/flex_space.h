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

struct NvFlexLibrary;
struct NvFlexSolver;
class FlexParticleBody;
class FlexParticleBodyConstraint;
class FlexPrimitiveBody;
class FlexMemoryAllocator;
class ParticlesMemory;
class ActiveParticlesMemory;
class SpringMemory;
class InflatablesMemory;
class DynamicTrianglesMemory;
class RigidsMemory;
class RigidsComponentsMemory;
class GeometryMemory;
class ContactsBuffers;
class GdFlexExtComputeAABBCallback;
class GdFlexExtComputeFrictionCallback;

struct TearingSplit {
	ParticleIndex particle_to_split;
	int involved_triangle_id;
	real_t w;
	Vector3 split_plane;
};

class FlexSpace : public RIDFlex {

	friend class FlexBuffers;
	friend class FlexParticleBodyCommands;
	friend class FlexParticleBodyConstraintCommands;

	NvFlexLibrary *flex_lib;
	NvFlexSolver *solver;
	int solver_max_particles;

	FlexMemoryAllocator *particles_allocator;
	ParticlesMemory *particles_memory;

	FlexMemoryAllocator *active_particles_allocator;
	ActiveParticlesMemory *active_particles_memory;
	MemoryChunk *active_particles_mchunk;

	ContactsBuffers *contacts_buffers;

	FlexMemoryAllocator *springs_allocator;
	SpringMemory *springs_memory;

	FlexMemoryAllocator *inflatables_allocator;
	InflatablesMemory *inflatables_memory;

	FlexMemoryAllocator *triangles_allocator;
	DynamicTrianglesMemory *triangles_memory;

	FlexMemoryAllocator *rigids_allocator;
	RigidsMemory *rigids_memory;

	FlexMemoryAllocator *rigids_components_allocator;
	RigidsComponentsMemory *rigids_components_memory;

	FlexMemoryAllocator *geometries_allocator;
	GeometryMemory *geometries_memory;

	Vector<FlexParticleBody *> particle_bodies;
	Vector<FlexParticleBody *> particle_bodies_tearing;
	Vector<TearingSplit> _tearing_splits;
	int tearing_max_splits;
	int tearing_max_spring_checks;

	Vector<FlexParticleBodyConstraint *> constraints;

	Vector<FlexPrimitiveBody *> primitive_bodies;
	Vector<FlexPrimitiveBody *> primitive_bodies_contact_monitoring;

	Vector<FlexPrimitiveBody *> primitive_bodies_cf;
	Vector<Transform> primitive_bodies_cf_prev_transform;
	Vector<Transform> primitive_bodies_cf_prev_inv_transform;
	Vector<Transform> primitive_bodies_cf_curr_transform;
	Vector<Transform> primitive_bodies_cf_curr_inv_transform;
	Vector<Transform> primitive_bodies_cf_motion;
	Vector<Vector3> primitive_bodies_cf_extent;
	Vector<real_t> primitive_bodies_cf_friction;
	Vector<real_t> primitive_bodies_cf_friction_2_threshold;
	Vector<uint32_t> primitive_bodies_cf_layers;

	bool _is_using_default_params;

	/// Custom kernels

	// Array size particle_bodies.size() * 2 where the first element is the starting index and the second the last particle index
	Vector<int> particle_bodies_pindices;
	Vector<AABB> particle_bodies_aabb;
	GdFlexExtComputeAABBCallback *compute_aabb_callback;

	GdFlexExtComputeFrictionCallback *compute_friction_callback;

	bool force_buffer_write;
	float particle_radius;

	bool is_active_particles_buffer_dirty;

public:
	FlexSpace();
	~FlexSpace();

	void init();
	NvFlexLibrary *get_flex_library();
	NvFlexSolver *get_solver() { return solver; }

private:
	void init_buffers();
	void init_solver();

public:
	void terminate();

private:
	void terminate_solver();

	static void update_particle_buffer_index(
			void *data,
			void *owner,
			int p_old_begin_index,
			int p_old_size,
			int p_new_begin_index,
			int p_new_size);

public:
	void sync();
	void _sync();
	void step(real_t p_delta_time, bool enable_timer);

	int get_solver_max_particles() const { return solver_max_particles; }

	_FORCE_INLINE_ FlexMemoryAllocator *get_particles_allocator() { return particles_allocator; }
	_FORCE_INLINE_ ParticlesMemory *get_particles_memory() { return particles_memory; }
	_FORCE_INLINE_ FlexMemoryAllocator *get_springs_allocator() { return springs_allocator; }
	_FORCE_INLINE_ SpringMemory *get_springs_memory() { return springs_memory; }
	_FORCE_INLINE_ InflatablesMemory *get_inflatables_memory() { return inflatables_memory; }
	_FORCE_INLINE_ DynamicTrianglesMemory *get_triangles_memory() { return triangles_memory; }
	_FORCE_INLINE_ RigidsMemory *get_rigids_memory() { return rigids_memory; }
	_FORCE_INLINE_ RigidsComponentsMemory *get_rigids_components_memory() { return rigids_components_memory; }

	bool can_commands_be_executed() const;

	void add_particle_body(FlexParticleBody *p_body);
	void remove_particle_body(FlexParticleBody *p_body);
	void update_particle_body_tearing_state(FlexParticleBody *p_body);
	int get_particle_count() const;

	void add_particle_body_constraint(FlexParticleBodyConstraint *p_constraint);
	void remove_particle_body_constraint(FlexParticleBodyConstraint *p_constraint);

	void add_primitive_body(FlexPrimitiveBody *p_body);
	void remove_primitive_body(FlexPrimitiveBody *p_body);
	void primitive_body_sync_cmonitoring(FlexPrimitiveBody *p_body);
	int get_primitive_body_count() const;

	bool set_param(const StringName &p_name, const Variant &p_property);
	bool get_param(const StringName &p_name, Variant &r_property) const;
	real_t get_particle_radius() const;
	void reset_params_to_defaults();
	bool is_using_default_params() const;

	// internals
	void set_custom_flex_callback();
	void dispatch_callback_contacts();
	void dispatch_callbacks();
	void execute_delayed_commands();
	void rebuild_rigids_offsets();
	void execute_geometries_commands();
	int execute_tearing();

	void commands_write_buffer();
	void commands_read_buffer();

	void on_particle_removed(FlexParticleBody *p_body, ParticleBufferIndex p_index);
	void on_particle_index_changed(FlexParticleBody *p_body, ParticleBufferIndex p_index_old, ParticleBufferIndex p_index_new);

	void rebuild_inflatables_indices();

	FlexParticleBody *find_particle_body(ParticleBufferIndex p_index) const;
	FlexPrimitiveBody *find_primitive_body(GeometryBufferIndex p_index, bool p_contact_monitoring_only) const;

	void update_custom_friction_primitive_body(
			FlexPrimitiveBody *p_body);
};

class FlexMemorySweeper : public FlexMemoryModificator {
};

// Change index of last index
class FlexMemorySweeperFast : public FlexMemorySweeper {
protected:
	FlexMemoryAllocator *allocator;
	MemoryChunk *&mchunk;
	Vector<FlexChunkIndex> &indices_to_remove;
	const bool reallocate_memory;
	const FlexBufferIndex custom_chunk_end_buffer_index;

public:
	FlexMemorySweeperFast(
			FlexMemoryAllocator *p_allocator,
			MemoryChunk *&r_rigids_components_mchunk,
			Vector<FlexChunkIndex> &r_indices_to_remove,
			bool p_reallocate_memory,
			FlexBufferIndex p_custom_chunk_end_buffer_index = -1);

	virtual void on_element_removed(FlexBufferIndex on_element_removed) {} // Just after removal
	virtual void on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index) {}

	virtual void exec();
};

class ParticlesMemorySweeper : public FlexMemorySweeperFast {
	FlexSpace *space;
	FlexParticleBody *body;

public:
	ParticlesMemorySweeper(
			FlexSpace *p_space,
			FlexParticleBody *p_body,
			FlexMemoryAllocator *p_allocator,
			MemoryChunk *&r_rigids_components_mchunk,
			Vector<FlexChunkIndex> &r_indices_to_remove,
			bool p_reallocate_memory,
			FlexBufferIndex p_custom_chunk_end_buffer_index = -1);

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

	virtual void on_element_removed(FlexBufferIndex on_element_removed);
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
