/*************************************************************************/
/*  flex_space.cpp                                                       */
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

#include "flex_space.h"

#include "thirdparty/flex/include/NvFlex.h"

#include "flex_memory.h"
#include "flex_particle_body.h"
#include "flex_primitive_body.h"
#include "flex_primitive_shapes.h"
#include "print_string.h"

// TODO make this customizable in settings
#define MAXPARTICLES 5000
// TODO this must be dynamic
#define MAXGEOMETRIES 10

#define DEVICE_ID 0

// TODO use a class
NvFlexErrorSeverity error_severity = eNvFlexLogInfo; // contain last error severity
void ErrorCallback(NvFlexErrorSeverity severity, const char *msg, const char *file, int line) {
	print_error(String("Flex error file: ") + file + " - LINE: " + String::num(line) + " - MSG: " + msg);
	error_severity = severity;
}
bool has_error() {
	return error_severity == eNvFlexLogError;
}

FlexSpace::FlexSpace() :
		RIDFlex(),
		flex_lib(NULL),
		solver(NULL),
		particle_bodies_allocator(NULL),
		particle_bodies_memory(NULL),
		active_particles_allocator(NULL),
		active_particles_memory(NULL),
		active_particles_mchunk(NULL),
		springs_allocator(NULL),
		springs_memory(NULL),
		rigids_allocator(NULL),
		rigids_memory(NULL),
		rigids_components_allocator(NULL),
		rigids_components_memory(NULL),
		geometries_allocator(NULL),
		geometries_memory(NULL),
		particle_radius(0.1) {
	init();
}

FlexSpace::~FlexSpace() {
	terminate();
}

void FlexSpace::init() {

	// Init library
	CRASH_COND(flex_lib);

	NvFlexInitDesc desc;
	desc.deviceIndex = DEVICE_ID;
	desc.enableExtensions = true;
	desc.renderDevice = NULL;
	desc.renderContext = NULL;
	desc.computeContext = NULL;
	desc.computeType = eNvFlexCUDA;
	desc.runOnRenderContext = false;

	flex_lib = NvFlexInit(NV_FLEX_VERSION, ErrorCallback, &desc);
	CRASH_COND(!flex_lib);
	CRASH_COND(has_error());

	// Init solvert
	CRASH_COND(solver);

	NvFlexSolverDesc solver_desc;

	NvFlexSetSolverDescDefaults(&solver_desc);
	solver_desc.featureMode = eNvFlexFeatureModeDefault; // All modes enabled (Solid|Fluids) // TODO should be customizable
	solver_desc.maxParticles = MAXPARTICLES; // TODO should be customizable
	solver_desc.maxDiffuseParticles = MAXPARTICLES; // TODO should be customizable
	solver_desc.maxNeighborsPerParticle = 32; // TODO should be customizable
	solver_desc.maxContactsPerParticle = 10; // TODO should be customizable

	solver = NvFlexCreateSolver(flex_lib, &solver_desc);
	CRASH_COND(has_error());

	// Init buffers
	CRASH_COND(particle_bodies_memory);
	CRASH_COND(particle_bodies_allocator);
	particle_bodies_memory = memnew(ParticleBodiesMemory(flex_lib));
	particle_bodies_allocator = memnew(FlexMemoryAllocator(particle_bodies_memory, MAXPARTICLES)); // TODO must be dynamic
	particle_bodies_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

	CRASH_COND(active_particles_allocator);
	CRASH_COND(active_particles_memory);
	active_particles_memory = memnew(ActiveParticlesMemory(flex_lib));
	active_particles_allocator = memnew(FlexMemoryAllocator(active_particles_memory, MAXPARTICLES)); // TODO must be dynamic
	active_particles_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

	active_particles_mchunk = active_particles_allocator->allocate_chunk(0);

	CRASH_COND(springs_allocator);
	CRASH_COND(springs_memory);
	springs_memory = memnew(SpringMemory(flex_lib));
	springs_allocator = memnew(FlexMemoryAllocator(springs_memory, ((FlexUnit)(MAXPARTICLES * 100)))); // TODO must be dynamic
	springs_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

	CRASH_COND(rigids_allocator);
	CRASH_COND(rigids_memory);
	rigids_memory = memnew(RigidsMemory(flex_lib));
	rigids_allocator = memnew(FlexMemoryAllocator(rigids_memory, ((FlexUnit)(MAXPARTICLES * 100)))); // TODO must be dynamic
	rigids_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

	CRASH_COND(rigids_components_allocator);
	CRASH_COND(rigids_components_memory);
	rigids_components_memory = memnew(RigidsComponentsMemory(flex_lib));
	rigids_components_allocator = memnew(FlexMemoryAllocator(rigids_components_memory, ((FlexUnit)(MAXPARTICLES * 100)))); // TODO must be dynamic
	rigids_components_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

	CRASH_COND(geometries_allocator);
	CRASH_COND(geometries_memory);
	geometries_memory = memnew(GeometryMemory(flex_lib));
	geometries_allocator = memnew(FlexMemoryAllocator(geometries_memory, MAXGEOMETRIES)); // TODO must be dynamic
	geometries_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

	NvFlexParams params;
	// Initialize solver parameter
	NvFlexGetParams(solver, &params);
	params.gravity[0] = 0.0;
	params.gravity[1] = -10.0;
	params.gravity[2] = 0.0;
	params.radius = particle_radius;
	params.solidRestDistance = params.radius * 0.9;
	params.fluidRestDistance = params.radius * 0.5;
	params.numIterations = 3;
	params.maxSpeed = FLT_MAX;
	params.maxAcceleration = Vector3(params.gravity[0], params.gravity[1], params.gravity[2]).length() * 10.0;
	params.relaxationMode = eNvFlexRelaxationLocal;
	params.relaxationFactor = 1.0;
	params.solidPressure = 1.0;
	params.collisionDistance = MAX(params.solidRestDistance, params.fluidRestDistance) * 0.5;
	params.shapeCollisionMargin = params.collisionDistance * 0.5;
	params.dynamicFriction = 0.1;
	params.staticFriction = 0.1;
	params.particleFriction = 0.1;
	NvFlexSetParams(solver, &params);

	CRASH_COND(has_error());
}

void FlexSpace::terminate() {

	if (particle_bodies_memory) {
		particle_bodies_memory->terminate();
		memdelete(particle_bodies_memory);
		particle_bodies_memory = NULL;
	}

	if (particle_bodies_allocator) {
		memdelete(particle_bodies_allocator);
		particle_bodies_allocator = NULL;
	}

	if (active_particles_memory) {
		active_particles_allocator->deallocate_chunk(active_particles_mchunk);
		active_particles_memory->terminate();
		memdelete(active_particles_memory);
		active_particles_memory = NULL;
	}

	if (active_particles_allocator) {
		memdelete(active_particles_allocator);
		active_particles_allocator = NULL;
	}

	if (springs_memory) {
		springs_memory->terminate();
		memdelete(springs_memory);
		springs_memory = NULL;
	}

	if (springs_allocator) {
		memdelete(springs_allocator);
		springs_allocator = NULL;
	}

	if (rigids_memory) {
		rigids_memory->terminate();
		memdelete(rigids_memory);
		rigids_memory = NULL;
	}

	if (rigids_allocator) {
		memdelete(rigids_allocator);
		rigids_allocator = NULL;
	}

	if (rigids_components_memory) {
		rigids_components_memory->terminate();
		memdelete(rigids_components_memory);
		rigids_components_memory = NULL;
	}

	if (rigids_components_allocator) {
		memdelete(rigids_components_allocator);
		rigids_components_allocator = NULL;
	}

	if (geometries_memory) {
		geometries_memory->terminate();
		memdelete(geometries_memory);
		geometries_memory = NULL;
	}

	if (geometries_allocator) {
		memdelete(geometries_allocator);
		geometries_allocator = NULL;
	}

	if (solver) {
		NvFlexDestroySolver(solver);
		solver = NULL;
	}

	if (flex_lib) {
		NvFlexShutdown(flex_lib);
		flex_lib = NULL;
	}
}

void FlexSpace::sync() {

	ChunkInteger a(1);
	BufferInteger b = 1;
	//// FINISH TEST

	///
	/// Map phase
	particle_bodies_memory->map();
	active_particles_memory->map();
	springs_memory->map();
	rigids_memory->map();
	rigids_components_memory->map();
	geometries_memory->map();

	///
	/// Stepping phase
	dispatch_callbacks();
	execute_delayed_commands();
	execute_geometries_commands();

	///
	/// Unmap phase
	particle_bodies_memory->unmap();

	active_particles_memory->unmap();

	if (springs_memory->was_changed())
		springs_allocator->sanitize(); // The memory must be consecutive to be sent to GPU
	springs_memory->unmap();

	rigids_memory->unmap();
	rigids_components_memory->unmap();

	if (geometries_memory->was_changed())
		geometries_allocator->sanitize(); // The memory must be consecutive to be sent to GPU
	geometries_memory->unmap();

	///
	/// Write phase
	commands_write_buffer();
}

void FlexSpace::step(real_t p_delta_time) {

	// Step solver (command)
	const int substep = 1;
	const bool enable_timer = false; // Used for profiling
	NvFlexUpdateSolver(solver, p_delta_time, substep, enable_timer);

	commands_read_buffer();
}

void FlexSpace::add_particle_body(FlexParticleBody *p_body) {
	ERR_FAIL_COND(p_body->space);
	p_body->space = this;
	particle_bodies.push_back(p_body);
}

void FlexSpace::remove_particle_body(FlexParticleBody *p_body) {

	if (p_body->particles_mchunk)
		particle_bodies_allocator->deallocate_chunk(p_body->particles_mchunk);
	if (p_body->springs_mchunk)
		springs_allocator->deallocate_chunk(p_body->springs_mchunk);
	p_body->space = NULL;
	particle_bodies.erase(p_body);
}

void FlexSpace::add_primitive_body(FlexPrimitiveBody *p_body) {
	ERR_FAIL_COND(p_body->space);
	p_body->space = this;
	p_body->changed_parameters = eChangedPrimitiveBodyParamAll;
	primitive_bodies.push_back(p_body);
}

void FlexSpace::remove_primitive_body(FlexPrimitiveBody *p_body) {
	ERR_FAIL_COND(p_body->space != this);

	if (p_body->geometry_mchunk)
		geometry_chunks_to_deallocate.push_back(p_body->geometry_mchunk);

	p_body->space = NULL;
	p_body->geometry_mchunk = NULL;
	primitive_bodies.erase(p_body);
}

void FlexSpace::dispatch_callbacks() {
	for (int i(particle_bodies.size() - 1); 0 <= i; --i) {
		particle_bodies[i]->dispatch_sync_callback();
	}
}

void FlexSpace::execute_delayed_commands() {

	int particles_count = 0;
	for (int body_index(particle_bodies.size() - 1); 0 <= body_index; --body_index) {

		FlexParticleBody *body = particle_bodies[body_index];
		if (body->delayed_commands.particle_to_add.size()) {

			int previous_size = 0;

			// Allocate memory for particles
			if (body->particles_mchunk) {
				previous_size = body->particles_mchunk->get_size();
				// Resize existing memory chunk
				particle_bodies_allocator->resize_chunk(body->particles_mchunk, previous_size + body->delayed_commands.particle_to_add.size());
			} else {
				// Allocate new one
				body->particles_mchunk = particle_bodies_allocator->allocate_chunk(body->delayed_commands.particle_to_add.size());
			}

			// Write on memory
			for (int p(body->delayed_commands.particle_to_add.size() - 1); 0 <= p; --p) {

				particle_bodies_memory->set_particle(body->particles_mchunk, previous_size + p, body->delayed_commands.particle_to_add[p].particle);
				particle_bodies_memory->set_velocity(body->particles_mchunk, previous_size + p, Vector3());
				particle_bodies_memory->set_phase(body->particles_mchunk, previous_size + p, NvFlexMakePhase(body->get_collision_group(), 0));
			}
		}

		if (body->delayed_commands.springs_to_add.size()) {

			int previous_size = 0;

			// Allocate memory for springs
			if (body->springs_mchunk) {
				previous_size = body->springs_mchunk->get_size();
				// Resize existing memory chunk
				springs_allocator->resize_chunk(body->springs_mchunk, previous_size + body->delayed_commands.springs_to_add.size());
			} else {
				// Allocate new chunk
				body->springs_mchunk = springs_allocator->allocate_chunk(body->delayed_commands.springs_to_add.size());
			}

			// Write on memory
			for (int s(body->delayed_commands.springs_to_add.size() - 1); 0 <= s; --s) {
				const SpringToAdd &sta(body->delayed_commands.springs_to_add[s]);
				body->reset_spring(previous_size + s, sta.particle_0, sta.particle_1, sta.length, sta.stiffness);
			}
		}

		if (body->delayed_commands.rigids_to_add.size()) {

			ERR_FAIL_COND(!body->particles_mchunk);

			int index_count = 0;
			int previous_size = 0;

			if (body->rigids_mchunk) {
				previous_size = body->rigids_mchunk->get_size();
				rigids_allocator->resize_chunk(body->rigids_mchunk, previous_size + body->delayed_commands.rigids_to_add.size());
			} else {
				body->rigids_mchunk = rigids_allocator->allocate_chunk(body->delayed_commands.rigids_to_add.size());
			}

			for (int r(body->delayed_commands.rigids_to_add.size() - 1); 0 <= r; --r) {

				rigids_memory->set_position(body->rigids_mchunk, previous_size + r, body->delayed_commands.rigids_to_add[r].global_transform.origin);
				rigids_memory->set_rotation(body->rigids_mchunk, previous_size + r, body->delayed_commands.rigids_to_add[r].global_transform.basis.get_quat());
				rigids_memory->set_stiffness(body->rigids_mchunk, previous_size + r, body->delayed_commands.rigids_to_add[r].stiffness);
				rigids_memory->set_threshold(body->rigids_mchunk, previous_size + r, body->delayed_commands.rigids_to_add[r].plastic_threshold);
				rigids_memory->set_creep(body->rigids_mchunk, previous_size + r, body->delayed_commands.rigids_to_add[r].plastic_creep);

				index_count += body->delayed_commands.rigids_to_add[r].indices.size();
			}

			// Insert indices

			previous_size = 0;
			if (body->rigids_components_mchunk) {
				previous_size = body->rigids_components_mchunk->get_size();
				rigids_components_allocator->resize_chunk(body->rigids_components_mchunk, previous_size + index_count);
			} else {
				body->rigids_components_mchunk = rigids_components_allocator->allocate_chunk(index_count);
			}

			RigidComponentIndex rigid_comp_index(previous_size == 0 ? 0 : rigids_memory->get_offset(body->rigids_mchunk, previous_size - 1));
			for (int r(0); r < body->delayed_commands.rigids_to_add.size(); ++r) {

				rigids_memory->set_offset(body->rigids_mchunk, previous_size + r, rigid_comp_index + body->delayed_commands.rigids_to_add[r].indices.size());

				// Allocate components
				PoolVector<int>::Read indices_r = body->delayed_commands.rigids_to_add[r].indices.read();
				PoolVector<Vector3>::Read rests_r = body->delayed_commands.rigids_to_add[r].rests.read();

				for (int rigid_p_index(body->delayed_commands.rigids_to_add[r].indices.size() - 1); 0 <= rigid_p_index; --rigid_p_index) {
					rigids_components_memory->set_index(body->rigids_components_mchunk, rigid_comp_index + rigid_p_index, body->particles_mchunk->get_buffer_index(indices_r[rigid_p_index]));
					rigids_components_memory->set_rest(body->rigids_components_mchunk, rigid_comp_index + rigid_p_index, rests_r[rigid_p_index]);
					//rigids_components_memory->set_normal(body->rigids_components_mchunk, rigid_comp_index + rigid_p_index, rests_r[rigid_p_index].normalized() * -1);
				}
				rigid_comp_index += body->delayed_commands.rigids_to_add[r].indices.size();
			}
		}

		if (body->delayed_commands.particle_to_remove.size() && body->particles_mchunk) {

			// Remove particles

			ParticleBufferIndex last_buffer_index(body->particles_mchunk->get_end_index());
			for (int i(0); i < body->delayed_commands.particle_to_remove.size(); ++i) {

				// Copy the values of last index to the index to remove (lose order)
				const ParticleBufferIndex buffer_index_to_remove(body->particles_mchunk->get_buffer_index(body->delayed_commands.particle_to_remove[i]));

				on_particle_removed(body, buffer_index_to_remove);
				if (last_buffer_index != buffer_index_to_remove) {
					particle_bodies_memory->copy(last_buffer_index, 1, buffer_index_to_remove);
					on_particle_index_changed(body, last_buffer_index, buffer_index_to_remove);
					body->particle_index_changed(body->particles_mchunk->get_chunk_index(last_buffer_index), body->delayed_commands.particle_to_remove[i]);
				}

				--last_buffer_index;
			}
			const FlexUnit new_size = body->particles_mchunk->get_size() - body->delayed_commands.particle_to_remove.size();
			particle_bodies_allocator->resize_chunk(body->particles_mchunk, new_size);
		}

		if (body->delayed_commands.springs_to_remove.size() && body->springs_mchunk) {

			// Remove springs

			SpringIndex last_buffer_index(body->springs_mchunk->get_end_index());
			for (Set<SpringIndex>::Element *e = body->delayed_commands.springs_to_remove.front(); e; e = e->next()) {

				// Copy the values of last ID to the ID to remove (lose order)
				const SpringBufferIndex buffer_index_to_remove(body->springs_mchunk->get_buffer_index(e->get()));
				springs_memory->copy(last_buffer_index, 1, buffer_index_to_remove);
				body->spring_index_changed(body->springs_mchunk->get_chunk_index(last_buffer_index), e->get());
				--last_buffer_index;
			}
			const FlexUnit new_size = body->springs_mchunk->get_size() - body->delayed_commands.springs_to_remove.size();
			springs_allocator->resize_chunk(body->springs_mchunk, new_size);
		}

		if (body->delayed_commands.rigids_components_to_remove.size() && body->rigids_components_mchunk) {

			const int comp_to_rem_size(body->delayed_commands.rigids_components_to_remove.size());

			FlexUnit chunk_size(body->rigids_components_mchunk->get_size());
			for (int i = 0; i < comp_to_rem_size; ++i) {

				// Shift left memory data from the component to remove +1
				--chunk_size;
				const RigidComponentIndex component_to_remove_index(body->delayed_commands.rigids_components_to_remove[i]);
				const RigidComponentBufferIndex component_to_remove_buffer_index(body->rigids_components_mchunk->get_buffer_index(component_to_remove_index));
				rigids_components_memory->copy(component_to_remove_buffer_index + 1, chunk_size, component_to_remove_buffer_index);

				// Update offset in rigid body
				for (RigidIndex i(body->rigids_mchunk->get_size() - 1); 0 <= i; --i) {
					RigidComponentIndex offset(rigids_memory->get_offset(body->rigids_mchunk, i));
					if (offset >= component_to_remove_index) {
						rigids_memory->set_offset(body->rigids_mchunk, i, offset - 1);
					} else {
						break;
					}
				}

				// Change the index from the next elements to remove
				if ((i + 1) < comp_to_rem_size)
					for (int b(i + 1); b < comp_to_rem_size; ++b) {
						if (component_to_remove_index <= body->delayed_commands.rigids_components_to_remove[b]) {
							body->delayed_commands.rigids_components_to_remove[b] -= 1;
						}
					}
			}

			rigids_components_allocator->resize_chunk(body->rigids_components_mchunk, chunk_size);
		}

		// Apply changed properties
		const uint32_t body_changed_parameters = body->get_changed_parameters();
		if (body_changed_parameters != 0) {
			for (int i(body->get_particle_count() - 1); 0 <= i; --i) {
				if (body_changed_parameters & eChangedBodyParamPhase) {
					particle_bodies_memory->set_phase(body->particles_mchunk, i, NvFlexMakePhaseWithChannels(body->collision_group, body->collision_flags, body->collision_primitive_mask));
				}
			}
		}

		particles_count += body->particles_mchunk ? body->particles_mchunk->get_size() : 0;
	}

	if (active_particles_mchunk->get_size() != particles_count) {

		active_particles_allocator->resize_chunk(active_particles_mchunk, particles_count);

		int active_particle_index(0);
		for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

			FlexParticleBody *body = particle_bodies[i];

			for (int p(0); p < body->particles_mchunk->get_size(); ++p) {
				active_particles_memory->set_active_particle(active_particles_mchunk, active_particle_index, body->particles_mchunk->get_buffer_index(p));
				++active_particle_index;
			}
		}
	}

	if (rigids_memory->was_changed())
		rebuild_rigids_offsets();
}

void FlexSpace::rebuild_rigids_offsets() {

	// Flex require a buffer of offsets that points to the buffer of indices
	// For this reason when the rigid memory change all the ID should be recreated in order to syn them

	// 1. Step trim
	rigids_allocator->sanitize();
	rigids_components_allocator->sanitize();

	// 2. Step get buffer offsets
	for (int body_i(particle_bodies.size() - 1); 0 <= body_i; --body_i) {
		FlexParticleBody *body = particle_bodies[body_i];

		if (!body->rigids_mchunk)
			continue;

		for (int rigid_i(body->rigids_mchunk->get_size() - 1); 0 <= rigid_i; --rigid_i) {

			rigids_memory->set_buffer_offset(body->rigids_mchunk, rigid_i, body->rigids_components_mchunk->get_buffer_index(rigids_memory->get_offset(body->rigids_mchunk, rigid_i)));
		}
	}

	// 3. Step sorting
	// Inverse Heap Sort
	const int chunks_size(rigids_allocator->get_chunk_count());

	MemoryChunk *swap_area = rigids_allocator->allocate_chunk(1);

	for (int chunk_i(0); chunk_i < chunks_size; ++chunk_i) {

		MemoryChunk *initial_chunk = rigids_allocator->get_chunk(chunk_i);

		if (initial_chunk->get_is_free())
			break; // End reached

		MemoryChunk *lowest_chunk = initial_chunk;
		int lowest_val = rigids_memory->get_buffer_offset(initial_chunk, 0);

		for (int chunk_x(chunk_i + 1); chunk_x < chunks_size; ++chunk_x) {

			MemoryChunk *other_chunk = rigids_allocator->get_chunk(chunk_x);

			if (other_chunk->get_is_free())
				break; // End reached

			int other_val = rigids_memory->get_buffer_offset(initial_chunk, 0);

			if (lowest_val > other_val) {
				lowest_val = other_val;
				lowest_chunk = other_chunk;
			}
		}

		if (lowest_chunk != initial_chunk) {
			rigids_allocator->copy_chunk(initial_chunk, swap_area);
			rigids_allocator->copy_chunk(lowest_chunk, initial_chunk);
			rigids_allocator->copy_chunk(swap_area, lowest_chunk);
		}
	}

	rigids_allocator->deallocate_chunk(swap_area);

	rigids_memory->zeroed_first_buffer_offset();
}

void FlexSpace::execute_geometries_commands() {
	for (int i(primitive_bodies.size() - 1); 0 <= i; --i) {

		FlexPrimitiveBody *body = primitive_bodies[i];

		if (!body->get_shape()) {
			// Remove geometry if has memory chunk
			if (body->geometry_mchunk) {
				geometry_chunks_to_deallocate.push_back(body->geometry_mchunk);
				body->geometry_mchunk = NULL;
			}
			continue;
		}

		// Add or update geometry

		if (body->changed_parameters == 0)
			continue; // Nothing to update

		if (!body->geometry_mchunk) {
			body->geometry_mchunk = geometries_allocator->allocate_chunk(1);
			body->changed_parameters = eChangedPrimitiveBodyParamAll;
		}

		if (body->changed_parameters & eChangedPrimitiveBodyParamShape) {
			NvFlexCollisionGeometry geometry;
			body->get_shape()->get_shape(&geometry);
			geometries_memory->set_shape(body->geometry_mchunk, 0, geometry);
			body->changed_parameters |= eChangedPrimitiveBodyParamFlags;
		}

		if (body->changed_parameters & eChangedPrimitiveBodyParamTransform) {

			Basis basis = body->transf.basis;
			if (body->get_shape()->need_alignment()) {
				basis *= body->get_shape()->get_alignment_basis();
			}

			if (body->changed_parameters & eChangedPrimitiveBodyParamTransformIsMotion) {
				geometries_memory->set_position_prev(body->geometry_mchunk, 0, geometries_memory->get_position(body->geometry_mchunk, 0));
				geometries_memory->set_rotation_prev(body->geometry_mchunk, 0, geometries_memory->get_rotation(body->geometry_mchunk, 0));
			} else {
				geometries_memory->set_position_prev(body->geometry_mchunk, 0, flvec4_from_vec3(body->transf.origin));
				geometries_memory->set_rotation_prev(body->geometry_mchunk, 0, basis.get_quat());
			}

			geometries_memory->set_position(body->geometry_mchunk, 0, flvec4_from_vec3(body->transf.origin));
			geometries_memory->set_rotation(body->geometry_mchunk, 0, basis.get_quat());
		}

		if (body->changed_parameters & eChangedPrimitiveBodyParamFlags) {
			//shift layer by 23 to match: NvFlexPhase
			uint32_t flag = NvFlexMakeShapeFlagsWithChannels(body->get_shape()->get_type(), body->is_kinematic(), body->get_layer() << 24);
			geometries_memory->set_flags(body->geometry_mchunk, 0, NvFlexMakeShapeFlagsWithChannels(body->get_shape()->get_type(), body->is_kinematic(), body->get_layer() << 24));
		}

		body->set_clean();
	}

	for (int i(geometry_chunks_to_deallocate.size() - 1); 0 <= i; --i) {
		geometries_allocator->deallocate_chunk(geometry_chunks_to_deallocate[i]);
	}
	geometry_chunks_to_deallocate.clear();
}

void FlexSpace::commands_write_buffer() {
	NvFlexCopyDesc copy_desc;
	for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

		FlexParticleBody *body = particle_bodies[i];
		if (!body->particles_mchunk)
			continue;

		const uint32_t changed_params(body->get_changed_parameters());
		if (changed_params != 0) {
			copy_desc.srcOffset = body->particles_mchunk->get_begin_index();
			copy_desc.dstOffset = body->particles_mchunk->get_begin_index();
			copy_desc.elementCount = body->particles_mchunk->get_size();

			if (changed_params & eChangedBodyParamPositionMass)
				NvFlexSetParticles(solver, particle_bodies_memory->particles.buffer, &copy_desc);
			if (changed_params & eChangedBodyParamVelocity)
				NvFlexSetVelocities(solver, particle_bodies_memory->velocities.buffer, &copy_desc);
			if (changed_params & eChangedBodyParamPhase)
				NvFlexSetPhases(solver, particle_bodies_memory->phases.buffer, &copy_desc);

			body->set_clean();
		}
	}

	if (active_particles_memory->was_changed()) {
		copy_desc.srcOffset = 0;
		copy_desc.dstOffset = 0;
		copy_desc.elementCount = active_particles_mchunk->get_size();

		NvFlexSetActive(solver, active_particles_memory->active_particles.buffer, &copy_desc);
		NvFlexSetActiveCount(solver, active_particles_mchunk->get_size());
	}

	if (springs_memory->was_changed())
		NvFlexSetSprings(solver, springs_memory->springs.buffer, springs_memory->lengths.buffer, springs_memory->stiffness.buffer, springs_allocator->get_last_used_index() + 1);

	if (rigids_memory->was_changed())
		NvFlexSetRigids(
				solver,
				rigids_memory->buffer_offsets.buffer,
				rigids_components_memory->indices.buffer,
				rigids_components_memory->rests.buffer,
				NULL, //rigids_components_memory->normals.buffer,
				rigids_memory->stiffness.buffer,
				rigids_memory->thresholds.buffer,
				rigids_memory->creeps.buffer,
				rigids_memory->rotation.buffer,
				rigids_memory->position.buffer,
				rigids_allocator->get_last_used_index() + 1,
				rigids_components_allocator->get_last_used_index() + 1);

	// TODO remove this
	int a = rigids_allocator->get_last_used_index() + 1;
	int b = rigids_components_allocator->get_last_used_index() + 1;

	if (geometries_memory->was_changed())
		NvFlexSetShapes(solver, geometries_memory->collision_shapes.buffer, geometries_memory->positions.buffer, geometries_memory->rotations.buffer, geometries_memory->positions_prev.buffer, geometries_memory->rotations_prev.buffer, geometries_memory->flags.buffer, geometries_allocator->get_last_used_index() + 1);
}

void FlexSpace::commands_read_buffer() {
	NvFlexCopyDesc copy_desc;
	for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

		if (!particle_bodies[i]->particles_mchunk)
			continue;

		// Write back to buffer (command)
		copy_desc.srcOffset = particle_bodies[i]->particles_mchunk->get_begin_index();
		copy_desc.dstOffset = particle_bodies[i]->particles_mchunk->get_begin_index();
		copy_desc.elementCount = particle_bodies[i]->particles_mchunk->get_size();

		// TODO read only necessary (part of buffer or just skip an entire buffer if not necessary)
		NvFlexGetParticles(solver, particle_bodies_memory->particles.buffer, &copy_desc);
		NvFlexGetVelocities(solver, particle_bodies_memory->velocities.buffer, &copy_desc);
		NvFlexGetNormals(solver, particle_bodies_memory->normals.buffer, &copy_desc);

		particle_bodies[i]->clear_commands();
	}

	// Read rigids
	NvFlexGetRigids(
			solver,
			NULL,
			NULL,
			NULL,
			NULL, //rigids_components_memory->normals.buffer,
			NULL,
			NULL,
			NULL,
			rigids_memory->rotation.buffer,
			rigids_memory->position.buffer);
}

void FlexSpace::on_particle_removed(FlexParticleBody *p_body, ParticleBufferIndex p_index) {
	// Find and remove springs associated to the particle to remove
	if (p_body->springs_mchunk) {
		// Find all springs associated to removed particle and put in the remove list
		for (int spring_index(p_body->springs_mchunk->get_size() - 1); 0 <= spring_index; --spring_index) {
			const Spring &spring = springs_memory->get_spring(p_body->springs_mchunk, spring_index);
			if (spring.index0 == p_index || spring.index1 == p_index) {
				p_body->remove_spring(spring_index);
			}
		}
	}

	// Remove rigid components associated to this body
	if (p_body->rigids_components_mchunk) {
		for (RigidComponentIndex i(p_body->rigids_components_mchunk->get_size() - 1); 0 <= i; --i) {

			if (p_index == rigids_components_memory->get_index(p_body->rigids_components_mchunk, i)) {
				p_body->remove_rigid_component(i);
			}
		}
	}
}

void FlexSpace::on_particle_index_changed(FlexParticleBody *p_body, ParticleBufferIndex p_index_old, ParticleBufferIndex p_index_new) {

	// Change springs index
	if (p_body->springs_mchunk) {
		for (int i(p_body->springs_mchunk->get_size() - 1); 0 <= i; --i) {

			const Spring &spring(springs_memory->get_spring(p_body->springs_mchunk, i));
			if (spring.index0 == p_index_old) {

				springs_memory->set_spring(p_body->springs_mchunk, i, Spring(p_index_new, spring.index1));
			} else if (spring.index1 == p_index_old) {

				springs_memory->set_spring(p_body->springs_mchunk, i, Spring(spring.index0, p_index_new));
			}
		}
	}

	// Change rigid index
	if (p_body->rigids_components_mchunk) {
		for (int i(p_body->rigids_components_mchunk->get_size() - 1); 0 <= i; --i) {

			ParticleBufferIndex buffer_index = rigids_components_memory->get_index(p_body->rigids_components_mchunk, i);
			if (p_index_old == buffer_index) {

				rigids_components_memory->set_index(p_body->rigids_components_mchunk, i, p_index_new);
			}
		}
	}

	// Update id even in the commands
	const int chunk_index_old(p_body->particles_mchunk->get_chunk_index(p_index_old));
	const int chunk_index_new(p_body->particles_mchunk->get_chunk_index(p_index_new));

	const int pos = p_body->delayed_commands.particle_to_remove.find(chunk_index_old);
	if (0 <= pos)
		p_body->delayed_commands.particle_to_remove[pos] = chunk_index_new;
}
