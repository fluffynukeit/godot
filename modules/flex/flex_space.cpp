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

#include "core/os/semaphore.h"
#include "core/os/thread.h"
#include "core/print_string.h"
#include "flex_memory.h"
#include "flex_particle_body.h"
#include "flex_particle_body_constraint.h"
#include "flex_primitive_body.h"
#include "flex_primitive_shapes.h"
#include "godot_flex_ext.h"
#include "profiler.h"

#define DEVICE_ID 0
//#define COLLISION_CHECK_MT

// TODO use a class
NvFlexErrorSeverity error_severity = eNvFlexLogInfo; // contain last error severity
void ErrorCallback(NvFlexErrorSeverity severity, const char *msg, const char *file, int line) {
	print_error(String("Flex error file: ") + file + " - LINE: " + String::num(line) + " - MSG: " + msg);
	error_severity = severity;
}
bool has_error() {
	return error_severity == eNvFlexLogError;
}

#ifdef COLLISION_CHECK_MT
void threadmanager_dispatch_cb_contacts(void *p_u_d);
#endif

FlexSpace::FlexSpace() :
		RIDFlex(),
		collision_check_thread1_td(false, this, 0, 0),
		flex_lib(NULL),
		solver(NULL),
		solver_max_particles(0),
		particles_allocator(NULL),
		particles_memory(NULL),
		active_particles_allocator(NULL),
		active_particles_memory(NULL),
		active_particles_mchunk(NULL),
		springs_allocator(NULL),
		springs_memory(NULL),
		inflatables_allocator(NULL),
		inflatables_memory(NULL),
		triangles_allocator(NULL),
		triangles_memory(NULL),
		rigids_allocator(NULL),
		rigids_memory(NULL),
		rigids_components_allocator(NULL),
		rigids_components_memory(NULL),
		geometries_allocator(NULL),
		geometries_memory(NULL),
		contacts_buffers(NULL),
		are_updated_primitive_bodies_cf(false),
		compute_aabb_callback(NULL),
		compute_friction_callback(NULL),
		tearing_max_splits(100),
		tearing_max_spring_checks(200),
		force_buffer_write(false),
		particle_radius(0.0) {

#ifdef COLLISION_CHECK_MT
	collision_check_thread1_td.semaphore = Semaphore::create();
	collision_check_thread1 = Thread::create(
			threadmanager_dispatch_cb_contacts,
			&collision_check_thread1_td);
#endif

	init();
}

FlexSpace::~FlexSpace() {
	terminate();

#ifdef COLLISION_CHECK_MT
	collision_check_thread1_td.stop = true;
	collision_check_thread1_td.semaphore->post();
	Thread::wait_to_finish(collision_check_thread1);
	memdelete(collision_check_thread1_td.semaphore);
	memdelete(collision_check_thread1);
#endif
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

	init_buffers();
	init_solver();

	contacts_buffers->resize(solver_max_particles);
	contacts_buffers->unmap();

	reset_params_to_defaults();
	CRASH_COND(has_error());
}

NvFlexLibrary *FlexSpace::get_flex_library() {
	return flex_lib;
}

void FlexSpace::init_buffers() {
	CRASH_COND(particles_memory);
	CRASH_COND(particles_allocator);
	particles_memory = memnew(ParticlesMemory(flex_lib));
	particles_allocator = memnew(FlexMemoryAllocator(
			particles_memory,
			10000,
			5000,
			-1));
	particles_allocator->register_resizechunk_callback(
			this,
			&FlexSpace::update_particle_buffer_index);

	particles_memory->unmap(); // *1

	CRASH_COND(active_particles_allocator);
	CRASH_COND(active_particles_memory);
	active_particles_memory = memnew(ActiveParticlesMemory(flex_lib));
	active_particles_allocator = memnew(FlexMemoryAllocator(active_particles_memory, particles_allocator->get_memory_size(), 1000, -1));
	active_particles_memory->unmap(); // *1

	active_particles_mchunk = active_particles_allocator->allocate_chunk(0, this);

	CRASH_COND(contacts_buffers);
	contacts_buffers = memnew(ContactsBuffers(flex_lib)); // This is resized when the solver is initialized

	CRASH_COND(springs_allocator);
	CRASH_COND(springs_memory);
	springs_memory = memnew(SpringMemory(flex_lib));
	springs_allocator = memnew(FlexMemoryAllocator(springs_memory, 2000, 500));
	springs_memory->unmap(); // *1

	CRASH_COND(triangles_allocator);
	CRASH_COND(triangles_memory);
	triangles_memory = memnew(DynamicTrianglesMemory(flex_lib));
	triangles_allocator = memnew(FlexMemoryAllocator(triangles_memory, 200, 100));
	triangles_memory->unmap(); // *1

	CRASH_COND(inflatables_allocator);
	CRASH_COND(inflatables_memory);
	inflatables_memory = memnew(InflatablesMemory(flex_lib));
	inflatables_allocator = memnew(FlexMemoryAllocator(inflatables_memory, 5, 5));
	inflatables_memory->unmap(); // *1

	CRASH_COND(rigids_allocator);
	CRASH_COND(rigids_memory);
	rigids_memory = memnew(RigidsMemory(flex_lib));
	rigids_allocator = memnew(FlexMemoryAllocator(rigids_memory, 500, 200));
	rigids_memory->unmap(); // *1

	CRASH_COND(rigids_components_allocator);
	CRASH_COND(rigids_components_memory);
	rigids_components_memory = memnew(RigidsComponentsMemory(flex_lib));
	rigids_components_allocator = memnew(FlexMemoryAllocator(rigids_components_memory, 500, 300));
	rigids_components_memory->unmap(); // *1

	CRASH_COND(geometries_allocator);
	CRASH_COND(geometries_memory);
	geometries_memory = memnew(GeometryMemory(flex_lib));
	geometries_allocator = memnew(FlexMemoryAllocator(geometries_memory, 20, 5));
	geometries_memory->unmap(); // *1

	// *1: This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped
}

void FlexSpace::init_solver() {
	CRASH_COND(solver);

	//if (compute_aabb_callback) {
	//	GdFlexExtDestroyComputeAABBCallback(compute_aabb_callback);
	//	compute_aabb_callback = NULL;
	//}

	if (compute_friction_callback) {
		GdFlexExtDestroyComputeFrictionCallback(compute_friction_callback);
		compute_friction_callback = NULL;
	}

	NvFlexSolverDesc solver_desc;

	solver_max_particles = particles_allocator->get_memory_size();

	NvFlexSetSolverDescDefaults(&solver_desc);
	solver_desc.featureMode = eNvFlexFeatureModeDefault; // TODO should be customizable
	solver_desc.maxParticles = solver_max_particles;
	solver_desc.maxDiffuseParticles = 0; // TODO should be customizable
	solver_desc.maxNeighborsPerParticle = 96; // TODO should be customizable
	solver_desc.maxContactsPerParticle = MAX_PERPARTICLE_CONTACT_COUNT;

	solver = NvFlexCreateSolver(flex_lib, &solver_desc);
	CRASH_COND(has_error());

	//compute_aabb_callback = GdFlexExtCreateComputeAABBCallback(solver);
	compute_friction_callback = GdFlexExtCreateComputeFrictionCallback(solver);

	force_buffer_write = true;
}

void FlexSpace::terminate() {

	if (particles_memory) {
		particles_memory->terminate();
		memdelete(particles_memory);
		particles_memory = NULL;
	}

	if (particles_allocator) {
		memdelete(particles_allocator);
		particles_allocator = NULL;
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

	if (triangles_memory) {
		triangles_memory->terminate();
		memdelete(triangles_memory);
		triangles_memory = NULL;
	}

	if (triangles_allocator) {
		memdelete(triangles_allocator);
		triangles_allocator = NULL;
	}

	if (inflatables_memory) {
		inflatables_memory->terminate();
		memdelete(inflatables_memory);
		inflatables_memory = NULL;
	}

	if (inflatables_allocator) {
		memdelete(inflatables_allocator);
		inflatables_allocator = NULL;
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

	if (contacts_buffers) {
		contacts_buffers->terminate();
		memdelete(contacts_buffers);
		contacts_buffers = NULL;
	}

	terminate_solver();

	if (flex_lib) {
		NvFlexShutdown(flex_lib);
		flex_lib = NULL;
	}
}

void FlexSpace::terminate_solver() {
	if (solver) {
		NvFlexDestroySolver(solver);
		solver = NULL;
	}
}

void FlexSpace::update_particle_buffer_index(
		void *data,
		void *owner,
		int p_old_begin_index,
		int p_old_size,
		int p_new_begin_index,
		int p_new_size) {

	if (!p_old_size)
		return;

	if (!p_new_size)
		return;

	FlexSpace *space = static_cast<FlexSpace *>(data);
	FlexParticleBody *particle_body = static_cast<FlexParticleBody *>(owner);

	const int shift = p_new_begin_index - p_old_begin_index;

	for (
			SpringIndex i(particle_body->get_spring_count() - 1);
			0 <= i;
			--i) {

		Spring s = space->get_springs_memory()->get_spring(
				particle_body->springs_mchunk,
				i);

		s.index0 += shift;
		s.index1 += shift;

		space->get_springs_memory()->set_spring(
				particle_body->springs_mchunk,
				i,
				s);
	}

	for (
			TriangleIndex i(particle_body->get_triangle_count() - 1);
			0 <= i;
			--i) {

		DynamicTriangle t = space->get_triangles_memory()->get_triangle(
				particle_body->triangles_mchunk,
				i);

		t.index0 += shift;
		t.index1 += shift;
		t.index2 += shift;

		space->get_triangles_memory()->set_triangle(
				particle_body->triangles_mchunk,
				i,
				t);
	}

	for (
			RigidComponentIndex i(particle_body->rigids_components_mchunk->get_size() - 1);
			0 <= i;
			--i) {

		ParticleBufferIndex p = space->get_rigids_components_memory()->get_index(
				particle_body->rigids_components_mchunk,
				i);

		p += shift;

		space->get_rigids_components_memory()->set_index(
				particle_body->rigids_components_mchunk,
				i,
				p);
	}

	space->is_active_particles_buffer_dirty = true;
}

void FlexSpace::sync() {}

void FlexSpace::_sync() {

	{
		PROFILE("flex_server_mapping")

		///
		/// Map phase
		particles_memory->map();
		active_particles_memory->map();
		springs_memory->map();
		triangles_memory->map();
		inflatables_memory->map();
		rigids_memory->map();
		rigids_components_memory->map();
		geometries_memory->map();
		contacts_buffers->map();
	}

	///
	/// Stepping phase
	dispatch_callback_contacts();
	dispatch_callbacks();
	execute_delayed_commands();
	execute_geometries_commands();

	///
	/// Emit server sync
	ParticlePhysicsServer::get_singleton()->emit_signal("sync_end", get_self());

	// Call this just before the unmap to doesn't reset the AABB
	// computed on previous step
	set_custom_flex_callback();

	///
	/// Unmap phase
	NvFlexSolver *old_solver = NULL;
	{
		PROFILE("flex_server_unmapping")
		particles_memory->unmap();

		active_particles_memory->unmap();

		if (springs_memory->was_changed())
			springs_allocator->sanitize(); // *1
		springs_memory->unmap();

		rebuild_inflatables_indices();
		triangles_memory->unmap();
		inflatables_memory->unmap();

		rigids_memory->unmap();
		rigids_components_memory->unmap();

		if (geometries_memory->was_changed())
			geometries_allocator->sanitize(); // *1
		geometries_memory->unmap();

		// *1: The memory must be consecutive to correctly write it on GPU

		if (particles_allocator->get_memory_size() != solver_max_particles) {

			PROFILE("flex_server_solver_recreation")

			NvFlexParams params;
			NvFlexGetParams(solver, &params);
			old_solver = solver;
			solver = NULL;
			init_solver();
			NvFlexSetParams(solver, &params);

			contacts_buffers->resize(solver_max_particles);
		}

		contacts_buffers->unmap();
	}

	///
	/// Write phase
	commands_write_buffer();

	/// Destroy old solver
	if (old_solver)
		NvFlexDestroySolver(old_solver);
}

void FlexSpace::step(real_t p_delta_time, bool enable_timer) {

	// I'm forced to move the synchronization here to be sure that all
	// nodes notifications (like transform change) are already executed
	_sync();

	// Step solver (command)
	const int substep = 1;
	NvFlexUpdateSolver(solver, p_delta_time, substep, enable_timer);

	commands_read_buffer();
}

bool FlexSpace::can_commands_be_executed() const {
	return particles_memory->is_mapped();
}

void FlexSpace::add_particle_body(FlexParticleBody *p_body) {
	ERR_FAIL_COND(p_body->space);

	p_body->space = this;
	const int particle_body_id = particle_bodies.size();
	particle_bodies.push_back(p_body);
	//particle_bodies_pindices.resize((particle_body_id + 1) * 2);
	//particle_bodies_aabb.resize(particle_body_id + 1);

	p_body->id = particle_body_id;
	p_body->changed_parameters = eChangedBodyParamALL;
	p_body->particles_mchunk = particles_allocator->allocate_chunk(0, p_body);
	p_body->springs_mchunk = springs_allocator->allocate_chunk(0, p_body);
	p_body->triangles_mchunk = triangles_allocator->allocate_chunk(0, p_body);
	p_body->inflatable_mchunk = inflatables_allocator->allocate_chunk(0, p_body);
	p_body->rigids_mchunk = rigids_allocator->allocate_chunk(0, p_body);
	p_body->rigids_components_mchunk = rigids_components_allocator->allocate_chunk(0, p_body);

	update_particle_body_tearing_state(p_body);
}

void FlexSpace::remove_particle_body(FlexParticleBody *p_body) {

	// TODO think about the fact to transfer these data to a cache before clear it
	rigids_components_allocator->deallocate_chunk(p_body->rigids_components_mchunk);
	rigids_allocator->deallocate_chunk(p_body->rigids_mchunk);
	triangles_allocator->deallocate_chunk(p_body->triangles_mchunk);
	inflatables_allocator->deallocate_chunk(p_body->inflatable_mchunk);
	springs_allocator->deallocate_chunk(p_body->springs_mchunk);
	particles_allocator->deallocate_chunk(p_body->particles_mchunk);

	rigids_components_memory->notify_change();
	rigids_memory->notify_change();
	triangles_memory->notify_change();
	inflatables_memory->notify_change();
	springs_memory->notify_change();

	p_body->space = NULL;
	p_body->id = -1;
	auto it = std::find(particle_bodies.begin(), particle_bodies.end(), p_body);
	if (it != particle_bodies.end())
		particle_bodies.erase(it, it + 1);

	//particle_bodies_pindices.resize(particle_bodies_pindices.size() - 2);
	//particle_bodies_aabb.resize(particle_bodies_aabb.size() - 1);

	// Rebuild ids
	for (int i = 0; i < particle_bodies.size(); ++i) {
		particle_bodies[i]->id = i;
	}

	update_particle_body_tearing_state(p_body);

	// TODO Show a warning and remove body constraint associated to this body
}

void FlexSpace::update_particle_body_tearing_state(FlexParticleBody *p_body) {

	auto it = std::find(
			particle_bodies_tearing.begin(),
			particle_bodies_tearing.end(),
			p_body);

	if (it != particle_bodies_tearing.end()) {
		// Found
		if (!p_body->is_tearing_active() || this != p_body->space) {
			particle_bodies_tearing.erase(it, it + 1);
		}
	} else {
		// Not found
		if (p_body->is_tearing_active() && this == p_body->space) {
			particle_bodies_tearing.push_back(p_body);
		}
	}
}

int FlexSpace::get_particle_count() const {
	return active_particles_mchunk->get_size();
}

void FlexSpace::add_particle_body_constraint(FlexParticleBodyConstraint *p_constraint) {
	ERR_FAIL_COND(!p_constraint);
	p_constraint->space = this;
	constraints.push_back(p_constraint);

	p_constraint->springs_mchunk = springs_allocator->allocate_chunk(0, p_constraint);
}

void FlexSpace::remove_particle_body_constraint(FlexParticleBodyConstraint *p_constraint) {
	springs_allocator->deallocate_chunk(p_constraint->springs_mchunk);
	springs_memory->notify_change();

	p_constraint->space = NULL;

	auto it = std::find(constraints.begin(), constraints.end(), p_constraint);
	if (it != constraints.end())
		constraints.erase(it, it + 1);
}

void FlexSpace::add_primitive_body(FlexPrimitiveBody *p_body) {
	ERR_FAIL_COND(p_body->space);
	p_body->space = this;
	p_body->changed_parameters = eChangedPrimitiveBodyParamAll;
	p_body->geometry_mchunk = geometries_allocator->allocate_chunk(0, p_body);
	primitive_bodies.push_back(p_body);

	update_custom_friction_primitive_body(p_body);
}

void FlexSpace::remove_primitive_body(FlexPrimitiveBody *p_body) {
	ERR_FAIL_COND(p_body->space != this);

	geometries_allocator->deallocate_chunk(p_body->geometry_mchunk);
	geometries_memory->notify_change();

	p_body->space = NULL;

	auto itpb = std::find(primitive_bodies.begin(), primitive_bodies.end(), p_body);
	if (itpb != primitive_bodies.end())
		primitive_bodies.erase(itpb, itpb + 1);

	update_custom_friction_primitive_body(p_body);
}

int FlexSpace::get_primitive_body_count() const {
	return primitive_bodies.size();
}

bool FlexSpace::set_param(const StringName &p_name, const Variant &p_property) {
	NvFlexParams params;
	NvFlexGetParams(solver, &params);

	if (FlexParticlePhysicsServer::singleton->solver_param_numIterations == p_name) {

		params.numIterations = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_gravity == p_name) {

		(*((Vector3 *)params.gravity)) = ((Vector3)p_property);
	} else if (FlexParticlePhysicsServer::singleton->solver_param_radius == p_name) {

		params.radius = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_solidRestDistance == p_name) {

		params.solidRestDistance = p_property;
		particle_radius = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_fluidRestDistance == p_name) {

		params.fluidRestDistance = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_dynamicFriction == p_name) {

		params.dynamicFriction = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_staticFriction == p_name) {

		params.staticFriction = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_particleFriction == p_name) {

		params.particleFriction = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_restitution == p_name) {

		params.restitution = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_adhesion == p_name) {

		params.adhesion = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_sleepThreshold == p_name) {

		params.sleepThreshold = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_maxSpeed == p_name) {

		params.maxSpeed = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_maxAcceleration == p_name) {

		params.maxAcceleration = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_shockPropagation == p_name) {

		params.shockPropagation = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_dissipation == p_name) {

		params.dissipation = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_damping == p_name) {

		params.damping = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_wind == p_name) {

		(*((Vector3 *)params.wind)) = ((Vector3)p_property);
	} else if (FlexParticlePhysicsServer::singleton->solver_param_drag == p_name) {

		params.drag = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_lift == p_name) {

		params.lift = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_cohesion == p_name) {

		params.cohesion = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_surfaceTension == p_name) {

		params.surfaceTension = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_viscosity == p_name) {

		params.viscosity = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_vorticityConfinement == p_name) {

		params.vorticityConfinement = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_anisotropyScale == p_name) {

		params.anisotropyScale = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_anisotropyMin == p_name) {

		params.anisotropyMin = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_anisotropyMax == p_name) {

		params.anisotropyMax = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_smoothing == p_name) {

		params.smoothing = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_solidPressure == p_name) {

		params.solidPressure = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_freeSurfaceDrag == p_name) {

		params.freeSurfaceDrag = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_buoyancy == p_name) {

		params.buoyancy = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseThreshold == p_name) {

		params.diffuseThreshold = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseBuoyancy == p_name) {

		params.diffuseBuoyancy = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseDrag == p_name) {

		params.diffuseDrag = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseBallistic == p_name) {

		params.diffuseBallistic = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseLifetime == p_name) {

		params.diffuseLifetime = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_collisionDistance == p_name) {

		params.collisionDistance = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_particleCollisionMargin == p_name) {

		params.particleCollisionMargin = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_shapeCollisionMargin == p_name) {

		params.shapeCollisionMargin = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_relaxationMode == p_name) {

		params.relaxationMode = "global" == p_property ? eNvFlexRelaxationGlobal : eNvFlexRelaxationLocal;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_relaxationFactor == p_name) {

		params.relaxationFactor = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_tearing_max_splits == p_name) {

		tearing_max_splits = p_property;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_tearing_max_spring_checks == p_name) {

		tearing_max_spring_checks = p_property;
	} else {
		return false;
	}

	_is_using_default_params = false;
	NvFlexSetParams(solver, &params);
	return true;
}

bool FlexSpace::get_param(const StringName &p_name, Variant &r_property) const {
	NvFlexParams params;
	NvFlexGetParams(solver, &params);

	if (FlexParticlePhysicsServer::singleton->solver_param_numIterations == p_name) {

		r_property = params.numIterations;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_gravity == p_name) {

		r_property = (*((Vector3 *)params.gravity));
	} else if (FlexParticlePhysicsServer::singleton->solver_param_solidRestDistance == p_name) {

		r_property = params.solidRestDistance;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_fluidRestDistance == p_name) {

		r_property = params.fluidRestDistance;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_dynamicFriction == p_name) {

		r_property = params.dynamicFriction;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_staticFriction == p_name) {

		r_property = params.staticFriction;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_particleFriction == p_name) {

		r_property = params.particleFriction;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_restitution == p_name) {

		r_property = params.restitution;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_adhesion == p_name) {

		r_property = params.adhesion;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_sleepThreshold == p_name) {

		r_property = params.sleepThreshold;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_maxSpeed == p_name) {

		r_property = params.maxSpeed;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_maxAcceleration == p_name) {

		r_property = params.maxAcceleration;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_shockPropagation == p_name) {

		r_property = params.shockPropagation;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_dissipation == p_name) {

		r_property = params.dissipation;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_damping == p_name) {

		r_property = params.damping;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_wind == p_name) {

		r_property = (*((Vector3 *)params.wind));
	} else if (FlexParticlePhysicsServer::singleton->solver_param_drag == p_name) {

		r_property = params.drag;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_lift == p_name) {

		r_property = params.lift;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_cohesion == p_name) {

		r_property = params.cohesion;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_surfaceTension == p_name) {

		r_property = params.surfaceTension;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_viscosity == p_name) {

		r_property = params.viscosity;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_vorticityConfinement == p_name) {

		r_property = params.vorticityConfinement;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_anisotropyScale == p_name) {

		r_property = params.anisotropyScale;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_anisotropyMin == p_name) {

		r_property = params.anisotropyMin;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_anisotropyMax == p_name) {

		r_property = params.anisotropyMax;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_smoothing == p_name) {

		r_property = params.smoothing;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_solidPressure == p_name) {

		r_property = params.solidPressure;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_freeSurfaceDrag == p_name) {

		r_property = params.freeSurfaceDrag;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_buoyancy == p_name) {

		r_property = params.buoyancy;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseThreshold == p_name) {

		r_property = params.diffuseThreshold;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseBuoyancy == p_name) {

		r_property = params.diffuseBuoyancy;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseDrag == p_name) {

		r_property = params.diffuseDrag;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseBallistic == p_name) {

		r_property = params.diffuseBallistic;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_diffuseLifetime == p_name) {

		r_property = params.diffuseLifetime;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_collisionDistance == p_name) {

		r_property = params.collisionDistance;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_particleCollisionMargin == p_name) {

		r_property = params.particleCollisionMargin;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_shapeCollisionMargin == p_name) {

		r_property = params.shapeCollisionMargin;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_relaxationMode == p_name) {

		r_property = eNvFlexRelaxationGlobal == params.relaxationMode ? "global" : "local";
	} else if (FlexParticlePhysicsServer::singleton->solver_param_relaxationFactor == p_name) {

		r_property = params.relaxationFactor;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_tearing_max_splits == p_name) {

		r_property = tearing_max_splits;
	} else if (FlexParticlePhysicsServer::singleton->solver_param_tearing_max_spring_checks == p_name) {

		r_property = tearing_max_spring_checks;
	} else {
		return false;
	}

	return true;
}

real_t FlexSpace::get_particle_radius() const {
	return particle_radius;
}

void FlexSpace::reset_params_to_defaults() {
	Map<StringName, Variant> defaults;
	FlexParticlePhysicsServer::singleton->space_get_params_defaults(&defaults);
	for (Map<StringName, Variant>::Element *e = defaults.front(); e; e = e->next()) {
		set_param(e->key(), e->get());
	}
	_is_using_default_params = true;
}

bool FlexSpace::is_using_default_params() const {
	return _is_using_default_params;
}

void FlexSpace::set_custom_flex_callback() {

	PROFILE("flex_server_set_custom_flex_callback")
	///
	/// AABB
	///

	//const int particle_body_count = particle_bodies.size();
	//
	//// TODO this should happens only when a particle is added / or removed
	//for (int i = 0; i < particle_body_count; ++i) {
	//
	//	const FlexParticleBody *pb = particle_bodies[i];
	//
	//	// Rebuilding this each time allow me to not bother too much on
	//	// particle addition / removal that each function can do
	//	particle_bodies_pindices.write[i * 2 + 0] =
	//			pb->particles_mchunk->get_begin_index();
	//
	//	particle_bodies_pindices.write[i * 2 + 1] =
	//			pb->particles_mchunk->get_buffer_index(
	//					pb->get_particle_count());
	//
	//	particle_bodies_aabb.write[i].set_position(
	//			pb->get_particle_position(0));
	//
	//	particle_bodies_aabb.write[i].set_size(Vector3());
	//}
	//
	//GdFlexExtSetComputeAABBCallback(
	//		compute_aabb_callback,
	//		particle_body_count,
	//		particle_bodies_pindices.ptrw(),
	//		(float *)particle_bodies_aabb.ptrw());

	///
	/// FRICTION
	///

	if (!are_updated_primitive_bodies_cf) {

		GdFlexExtUpdateComputeFrictionPrimitives(
				compute_friction_callback,
				primitive_bodies_cf_prev_transform.size(),
				(const float *)&primitive_bodies_cf_prev_transform[0],
				(const float *)&primitive_bodies_cf_prev_inv_transform[0],
				(const float *)&primitive_bodies_cf_curr_inv_transform[0],
				(const float *)&primitive_bodies_cf_motion[0],
				(const float *)&primitive_bodies_cf_extent[0],
				&primitive_bodies_cf_friction[0],
				&primitive_bodies_cf_friction_2_threshold[0],
				&primitive_bodies_cf_layers[0],
				0.0035 /*Margin*/);
		are_updated_primitive_bodies_cf = true;
	}

	GdFlexExtUpdateComputeFrictionParticles(
			compute_friction_callback,
			particles_memory->particles.size(),
			(const float *)particles_memory->particles.mappedPtr,
			get_particle_radius());

	GdFlexExtSetComputeFrictionCallback(
			compute_friction_callback);
}

void thread_dispatch_cb_contacts(FlexSpace *p_space, int start, int end) {

	for (int i(start); i < end; ++i) {

		FlexParticleBody *particle_body = p_space->particle_bodies[i];
		if (!particle_body->is_monitorable())
			continue;

		FlexBufferIndex end_index =
				particle_body->particles_mchunk->get_buffer_index(
						particle_body->get_particle_count());

		for (int particle_buffer_index =
						particle_body->particles_mchunk->get_begin_index();
				particle_buffer_index <= end_index;
				++particle_buffer_index) {

			const int contact_index(p_space->contacts_buffers->indices[particle_buffer_index]);
			const uint32_t particle_contact_count(p_space->contacts_buffers->counts[contact_index]);

			if (!particle_contact_count)
				continue;

			const ParticleIndex particle_index(particle_body->particles_mchunk->get_chunk_index(particle_buffer_index));

			for (uint32_t c(0); c < particle_contact_count; ++c) {

				const FlVector4 &velocity_and_primitive(p_space->contacts_buffers->velocities_prim_indices[contact_index * MAX_PERPARTICLE_CONTACT_COUNT + c]);
				const FlVector4 &raw_normal(p_space->contacts_buffers->normals[contact_index * MAX_PERPARTICLE_CONTACT_COUNT + c]);

				Vector3 velocity(vec3_from_flvec4(velocity_and_primitive));
				Vector3 normal(vec3_from_flvec4(raw_normal));

				const int primitive_body_index(velocity_and_primitive.w);
				FlexPrimitiveBody *primitive_body = p_space->find_primitive_body(primitive_body_index);

				if (!primitive_body->is_monitoring_particles_contacts())
					continue;

				if (particle_body->is_monitoring_primitives_contacts())
					particle_body->dispatch_primitive_contact(primitive_body, particle_index, velocity, normal);

				if (primitive_body->is_monitoring_particles_contacts())
					primitive_body->dispatch_particle_contact(particle_body, particle_index, velocity, normal);
			}
		}
	}
}

#ifdef COLLISION_CHECK_MT
void threadmanager_dispatch_cb_contacts(void *p_user_data) {

	ThreadData *p_td = static_cast<ThreadData *>(p_user_data);

	while (true) {

		Error e = p_td->semaphore->wait();
		CRASH_COND(e != OK);

		if (p_td->stop) {
			return;
		}

		thread_dispatch_cb_contacts(
				p_td->space,
				p_td->start,
				p_td->end);

		p_td->done = true;
	}
}
#endif

void FlexSpace::dispatch_callback_contacts() {

	PROFILE("flex_server_dispatch_callback_contacts")

#ifdef COLLISION_CHECK_MT
	const int size(particle_bodies.size());
	const int size_per_thread = size / 2.0;

	collision_check_thread1_td.done = false;
	collision_check_thread1_td.end = size_per_thread;
	Error e = collision_check_thread1_td.semaphore->post();
	CRASH_COND(e != OK);

	thread_dispatch_cb_contacts(this, size_per_thread, size);

	// Wait for thread
	while (!collision_check_thread1_td.done) {
	}
#else

	thread_dispatch_cb_contacts(this, 0, particle_bodies.size());
#endif
}

void FlexSpace::dispatch_callbacks() {
	{
		PROFILE("flex_server_dispatch_callbacks_particle_bodies")
		for (int i(particle_bodies.size() - 1); 0 <= i; --i) {
			particle_bodies[i]->dispatch_sync_callback();
		}
	}
	{

		PROFILE("flex_server_dispatch_callbacks_constraints")
		for (int i(constraints.size() - 1); 0 <= i; --i) {
			constraints[i]->dispatch_sync_callback();
		}
	}
	{

		PROFILE("flex_server_dispatch_callbacks_primitives")
		for (int i(primitive_bodies.size() - 1); 0 <= i; --i) {
			primitive_bodies[i]->dispatch_sync_callback();
		}
	}
}

void FlexSpace::execute_delayed_commands() {

	PROFILE("flex_server_execute_delayed_commands")
	int particles_count = 0;
	for (
			int body_index(particle_bodies.size() - 1);
			0 <= body_index;
			--body_index) {

		FlexParticleBody *body = particle_bodies[body_index];

		if (body->delayed_commands.particles_to_unactive.size()) {

			int particle_count = body->get_particle_count();

			body->set_particle_count(
					particle_count -
					body->delayed_commands.particles_to_unactive.size());

			ParticlesMemorySweeper sweeper(
					this,
					body,
					particles_allocator,
					body->particles_mchunk,
					body->delayed_commands.particles_to_unactive,
					false,
					body->particles_mchunk->get_buffer_index(particle_count - 1));
			sweeper.exec();
		}

		if (body->delayed_commands.particles_to_remove.size()) {

			int particle_count = body->get_particle_count();

			body->set_particle_count(
					particle_count -
					body->delayed_commands.particles_to_remove.size());

			ParticlesMemorySweeper sweeper(
					this,
					body,
					particles_allocator,
					body->particles_mchunk,
					body->delayed_commands.particles_to_remove,
					true,
					body->particles_mchunk->get_buffer_index(particle_count - 1));
			sweeper.exec();
		}

		if (body->delayed_commands.springs_to_remove.size()) {

			SpringsMemorySweeper sweeper(
					body,
					springs_allocator,
					body->springs_mchunk,
					body->delayed_commands.springs_to_remove);
			sweeper.exec();
		}

		if (body->delayed_commands.triangles_to_remove.size()) {

			TrianglesMemorySweeper sweeper(
					body,
					triangles_allocator,
					body->triangles_mchunk,
					body->delayed_commands.triangles_to_remove);
			sweeper.exec();
		}

		if (body->delayed_commands.rigids_components_to_remove.size()) {

			RigidsComponentsMemorySweeper sweeper(
					rigids_components_allocator,
					body->rigids_components_mchunk,
					body->delayed_commands.rigids_components_to_remove,
					rigids_memory,
					body->rigids_mchunk);

			sweeper.exec();
			body->reload_rigids_COM();

			// Check if there are particles to remove
			int previous_offset(0);
			for (int i(0); i < body->rigids_mchunk->get_size(); ++i) {
				if (previous_offset == rigids_memory->get_offset(body->rigids_mchunk, i)) {
					body->remove_rigid(i);
				} else {
					previous_offset = rigids_memory->get_offset(body->rigids_mchunk, i);
				}
			}
		}

		if (body->delayed_commands.rigids_to_remove.size()) {

			RigidsMemorySweeper sweeper(rigids_allocator, body->rigids_mchunk, body->delayed_commands.rigids_to_remove, rigids_memory, rigids_components_allocator, rigids_components_memory, body->rigids_components_mchunk);
			sweeper.exec();
		}

		// Apply changed properties
		const uint32_t body_changed_parameters = body->get_changed_parameters();
		if (body_changed_parameters != 0) {
			for (int i(body->get_particle_count() - 1); 0 <= i; --i) {

				if (body_changed_parameters & eChangedBodyParamPhase) {
					particles_memory->set_phase(body->particles_mchunk, i, NvFlexMakePhaseWithChannels(body->collision_group, body->collision_flags, body->collision_primitive_mask));
				}
			}

			if (body_changed_parameters & eChangedBodyParamInflatable && body->inflatable_mchunk->get_size()) {

				if (body->get_rest_volume()) {
					body->space->inflatables_allocator->resize_chunk(body->inflatable_mchunk, 1);
					inflatables_memory->set_rest_volume(body->inflatable_mchunk, 0, body->rest_volume);
					inflatables_memory->set_pressure(body->inflatable_mchunk, 0, body->pressure);
					inflatables_memory->set_constraint_scale(body->inflatable_mchunk, 0, body->constraint_scale);
				} else {
					body->space->inflatables_allocator->resize_chunk(body->inflatable_mchunk, 0);
				}
			}
		}

		particles_count += body->get_particle_count();
	}

	for (
			int constraint_index(constraints.size() - 1);
			0 <= constraint_index;
			--constraint_index) {

		FlexParticleBodyConstraint *constraint = constraints[constraint_index];

		if (constraint->delayed_commands.springs_to_remove.size()) {

			FlexMemorySweeperFast sweeper(
					springs_allocator,
					constraint->springs_mchunk,
					constraint->delayed_commands.springs_to_remove,
					true);

			sweeper.exec();
			springs_memory->notify_change();
		}
	}

	particles_count += execute_tearing();

	if (active_particles_mchunk->get_size() != particles_count)
		is_active_particles_buffer_dirty = true;

	if (is_active_particles_buffer_dirty) {

		is_active_particles_buffer_dirty = false;

		active_particles_allocator->resize_chunk(active_particles_mchunk, particles_count, false);
		ERR_FAIL_COND(active_particles_mchunk->get_begin_index() != 0);

		int active_particle_index(0);
		for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

			FlexParticleBody *body = particle_bodies[i];

			for (int p(0); p < body->get_particle_count(); ++p) {

				active_particles_memory->set_active_particle(
						active_particles_mchunk,
						active_particle_index,
						body->particles_mchunk->get_buffer_index(p));

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

	// 2. Step make buffer offsets
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

	MemoryChunk *swap_area = rigids_allocator->allocate_chunk(1, NULL);

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
			rigids_allocator->copy_chunk(swap_area, initial_chunk);
			rigids_allocator->copy_chunk(initial_chunk, lowest_chunk);
			rigids_allocator->copy_chunk(lowest_chunk, swap_area);
		}
	}

	rigids_allocator->deallocate_chunk(swap_area);

	rigids_memory->zeroed_first_buffer_offset();
}

void FlexSpace::execute_geometries_commands() {

	PROFILE("flex_server_execute_geometries_commands")
	///
	/// CUSTOM FRICTION PRIMITIVE UPDATE
	////////////////////////////////////

	for (int i = 0; i < primitive_bodies_cf.size(); ++i) {

		FlexPrimitiveBody *pb = primitive_bodies_cf[i];

		if (!(pb->changed_parameters & eChangedPrimitiveBodyParamTransform))
			continue;

		if (pb->changed_parameters & eChangedPrimitiveBodyParamTransformIsMotion) {

			// Is motion

			primitive_bodies_cf_prev_transform[i] =
					primitive_bodies_cf_curr_transform[i];

			primitive_bodies_cf_prev_inv_transform[i] =
					primitive_bodies_cf_curr_inv_transform[i];

			primitive_bodies_cf_curr_transform[i] = pb->get_transform();
			primitive_bodies_cf_curr_inv_transform[i] = pb->get_transform().inverse();

			primitive_bodies_cf_motion[i] =
					primitive_bodies_cf_prev_inv_transform[i] *
					primitive_bodies_cf_curr_transform[i];

		} else {

			// Is Teleport
			primitive_bodies_cf_curr_transform[i] = pb->get_transform();
			primitive_bodies_cf_curr_inv_transform[i] = pb->get_transform().inverse();

			primitive_bodies_cf_prev_transform[i] =
					primitive_bodies_cf_curr_transform[i];

			primitive_bodies_cf_prev_inv_transform[i] =
					primitive_bodies_cf_curr_inv_transform[i];

			primitive_bodies_cf_motion[i] = Transform();
		}
		if (pb->use_custom_friction)
			are_updated_primitive_bodies_cf = false;
	}

	///
	/// Primitive body process
	////////////////////////////////////

	for (int i(primitive_bodies.size() - 1); 0 <= i; --i) {

		FlexPrimitiveBody *body = primitive_bodies[i];

		if (!body->get_shape()) {
			// Remove geometry if has memory chunk
			if (body->geometry_mchunk->get_size()) {
				geometries_allocator->resize_chunk(body->geometry_mchunk, 0);
				geometries_memory->notify_change();
			}
			continue;
		}

		// Add or update geometry

		if (body->changed_parameters == 0)
			continue; // Nothing to update

		if (!body->geometry_mchunk->get_size()) {
			geometries_allocator->resize_chunk(body->geometry_mchunk, 1);
			geometries_memory->set_self(body->geometry_mchunk, 0, body);
			body->changed_parameters = eChangedPrimitiveBodyParamAll;
		}

		if (body->changed_parameters & eChangedPrimitiveBodyParamShape) {
			NvFlexCollisionGeometry geometry;
			body->get_shape()->get_shape(this, body->get_scale(), &geometry);
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
			body->update_aabb();
		}

		if (body->changed_parameters & eChangedPrimitiveBodyParamFlags) {
			//shift layer by 23 to match: NvFlexPhase
			geometries_memory->set_flags(body->geometry_mchunk, 0, (NvFlexMakeShapeFlagsWithChannels(body->get_shape()->get_type(), body->is_kinematic(), body->get_layer() << 24) | (body->is_area() ? eNvFlexShapeFlagTrigger : 0)));
		}

		body->set_clean();
	}
}

// This function return true immediately when the has_top and has_bottom are both  true
// Also it use a triangle as starting point to avoid to check all triangles of mesh
bool can_split_particle(
		FlexParticleBody *pb,
		const ParticleIndex p_particle,
		ParticlePhysicsServer::Triangle &p_triangle,
		const uint32_t hash_check,
		const Vector3 &split_plane,
		const real_t w,
		bool &r_has_top,
		bool &r_has_bottom) {

	const FlVector4 fl_centroid =
			(pb->get_particle(p_triangle.a) +
					pb->get_particle(p_triangle.b) +
					pb->get_particle(p_triangle.c)) /
			3.0;

	const Vector3 centroid(vec3_from_flvec4(fl_centroid));

	if (split_plane.dot(centroid) > w) {
		// Top
		r_has_top = true;
	} else {
		// Bottom
		r_has_bottom = true;
	}

	if (r_has_top && r_has_bottom)
		return true;

	p_triangle.hash_check = hash_check;

	// Otherwise check adjacent triangle
	for (int e(2); 0 <= e; --e) {

		if (0 > p_triangle.edges[e].adjacent_triangle_index)
			continue;

		ParticlePhysicsServer::Triangle &other_tri(
				pb->get_tearing_data()->triangles.write[p_triangle.edges[e].adjacent_triangle_index]);

		if (hash_check == other_tri.hash_check)
			continue; // Already checked by this run

		if (!other_tri.contains(p_particle))
			continue;

		if (can_split_particle(
					pb,
					p_particle,
					other_tri,
					hash_check,
					split_plane,
					w,
					r_has_top,
					r_has_bottom))
			return true;
	}

	return false;
}

// This function understand if the triangle is on top or bottom side of the split plane
// Also it doesn't check all triangles but only the near triangles.
void get_near_triangles(
		FlexParticleBody *pb,
		const ParticleIndex p_particle,
		ParticlePhysicsServer::Triangle &p_triangle,
		const uint32_t hash_check,
		const Vector3 &split_plane,
		const real_t w,
		std::vector<int> &adjacent_triangles) {

	const FlVector4 fl_centroid =
			(pb->get_particle(p_triangle.a) +
					pb->get_particle(p_triangle.b) +
					pb->get_particle(p_triangle.c)) /
			3.0;

	const Vector3 centroid(vec3_from_flvec4(fl_centroid));

	if (split_plane.dot(centroid) > w) {
		// Top
		pb->get_tearing_data()->sides[p_triangle.self_id] = true;
	} else {
		// Bottom
		pb->get_tearing_data()->sides[p_triangle.self_id] = false;
	}

	adjacent_triangles.push_back(p_triangle.self_id);
	p_triangle.hash_check = hash_check;

	// Otherwise check adjacent triangle
	for (int e(2); 0 <= e; --e) {

		if (0 > p_triangle.edges[e].adjacent_triangle_index)
			continue;

		ParticlePhysicsServer::Triangle &other_tri(
				pb->get_tearing_data()->triangles.write[p_triangle.edges[e].adjacent_triangle_index]);

		if (hash_check == other_tri.hash_check)
			continue; // Already checked by this run

		if (!other_tri.contains(p_particle))
			continue;

		get_near_triangles(
				pb,
				p_particle,
				other_tri,
				hash_check,
				split_plane,
				w,
				adjacent_triangles);
	}
}

int FlexSpace::execute_tearing() {

	int added_particles(0);
	_tearing_splits.resize(tearing_max_splits);

	std::vector<int> adjacent_triangles;

	for (int i(particle_bodies_tearing.size() - 1); 0 <= i; --i) {
		FlexParticleBody *pb = particle_bodies_tearing[i];

		int split_count = 0;

		std::vector<ForceTearing> &force_tearings(pb->get_force_tearings());

		/// Use another cycle to avoid too much checks and also to
		/// not delay the cut with delayed check
		for (int x(force_tearings.size() - 1); 0 <= x; --x) {

			const ParticleIndex particle_to_split = force_tearings[x].particle_to_split;
			const Vector3 &split_plane(force_tearings[x].split_plane);

			const FlVector4 pts_pos = pb->get_particle(particle_to_split);

			const real_t w(
					split_plane.dot(
							vec3_from_flvec4(pts_pos)));

			// Find involved triangle
			int involved_triangle_id(-1);
			for (int t(pb->tearing_data->triangles.size() - 1); 0 <= t; --t) {

				const ParticlePhysicsServer::Triangle &tri =
						pb->tearing_data->triangles[t];

				if (tri.contains(particle_to_split)) {
					involved_triangle_id = t;
					break;
				}
			}

			if (0 > involved_triangle_id) {
				ERR_PRINTS("0 > involved_triangle_id, this may be a bug. Particle to split: " + String::num_int64(particle_to_split));
				continue;
			}

			ParticlePhysicsServer::Triangle &involved_triangle =
					pb->tearing_data->triangles.write[involved_triangle_id];

			bool has_top(false);
			bool has_bottom(false);

			// Continue only if has_top and has_bottom are true
			if (!can_split_particle(
						pb,
						particle_to_split,
						involved_triangle,
						Math::rand(),
						split_plane,
						w,
						has_top,
						has_bottom))
				continue;

			// Perform the split
			const int split_index = split_count++;

			_tearing_splits[split_index].particle_to_split = particle_to_split;
			_tearing_splits[split_index].involved_triangle_id = involved_triangle_id;
			_tearing_splits[split_index].w = w;
			_tearing_splits[split_index].split_plane = split_plane;
		}

		force_tearings.clear();

		/// Check the prings to see if tearing should occur

		const int spring_count(pb->get_spring_count());
		int spring_index(pb->tearing_data->check_stopped + 1);
		int pb_max_spring_checks = tearing_max_spring_checks > spring_count ? spring_count : tearing_max_spring_checks;

		for (
				int check_index(0);
				pb_max_spring_checks > check_index && tearing_max_splits > split_count;
				++check_index, ++spring_index) {

			// Reset
			if (spring_count <= spring_index)
				spring_index = 0;

			const Spring &s = springs_memory->get_spring(
					pb->springs_mchunk,
					spring_index);

			const FlVector4 &p0 = particles_memory->get_particle(s.index0);
			if (0 == p0.w)
				continue;

			const FlVector4 &p1 = particles_memory->get_particle(s.index1);
			if (0 == p1.w)
				continue;

			const real_t extension_2 = extract_position((p1 - p0)).length_squared();
			const real_t rest_length_2 = pb->tearing_data->spring_rest_lengths_2[spring_index];

			if (extension_2 < rest_length_2 * pb->tearing_max_extension)
				continue;

			const ParticleBufferIndex particle_buffer_to_split =
					Math::random(0.0, 1.0) > 0.5 ? s.index0 : s.index1;

			const ParticleIndex particle_to_split =
					pb->particles_mchunk->get_chunk_index(
							particle_buffer_to_split);

			// Split the same spring and same triangle one time per frame
			bool split_in_progress = false;
			for (int x(0); x < split_count; ++x) {
				if (_tearing_splits[x].particle_to_split == particle_to_split) {
					split_in_progress = true;
					break;
				}
			}

			if (split_in_progress)
				continue;

			// Find involved triangle
			int involved_triangle_id(-1);
			for (int t(pb->tearing_data->triangles.size() - 1); 0 <= t; --t) {

				const ParticlePhysicsServer::Triangle &tri =
						pb->tearing_data->triangles[t];

				if (tri.contains(particle_to_split)) {
					involved_triangle_id = t;
					break;
				}
			}

			ERR_FAIL_COND_V(0 > involved_triangle_id, 0); // Impossible

			ParticlePhysicsServer::Triangle &involved_triangle =
					pb->tearing_data->triangles.write[involved_triangle_id];

			// This section understand in which side the triangle is

			const FlVector4 n(p1 - p0);
			Vector3 split_plane = vec3_from_flvec4(n);
			split_plane.normalize();

			const FlVector4 pts_pos =
					get_particles_memory()->get_particle(
							particle_buffer_to_split);

			const real_t w(
					split_plane.dot(
							vec3_from_flvec4(pts_pos)));

			bool has_top(false);
			bool has_bottom(false);

			// Continue only if has_top and has_bottom are true
			if (!can_split_particle(
						pb,
						particle_to_split,
						involved_triangle,
						Math::rand(),
						split_plane,
						w,
						has_top,
						has_bottom))
				continue;

			// Perform the split
			const int split_index = split_count++;

			_tearing_splits[split_index].particle_to_split = particle_to_split;
			_tearing_splits[split_index].involved_triangle_id = involved_triangle_id;
			_tearing_splits[split_index].w = w;
			_tearing_splits[split_index].split_plane = split_plane;
		}

		pb->tearing_data->check_stopped = spring_index;
		pb->tearing_data->splits.resize(split_count);

		if (!split_count)
			continue;

		// This avoid too much reallocation
		pb->add_unactive_particles(split_count);
		added_particles += split_count;

		for (
				int split_index(0);
				split_index < split_count;
				++split_index) {

			const ParticleIndex added_particle = pb->add_particles(1);
			const ParticleBufferIndex added_particle_buffer =
					pb->particles_mchunk->get_buffer_index(added_particle);

			const ParticleBufferIndex particle_buffer_to_split(
					pb->particles_mchunk->get_buffer_index(
							_tearing_splits[split_index].particle_to_split));

			// Duplicate particle
			pb->copy_particle(
					added_particle,
					_tearing_splits[split_index].particle_to_split);

			const int hash_check(Math::rand());

			adjacent_triangles.clear();
			get_near_triangles(
					pb,
					_tearing_splits[split_index].particle_to_split,
					pb->tearing_data->triangles.write[_tearing_splits[split_index].involved_triangle_id],
					hash_check,
					_tearing_splits[split_index].split_plane,
					_tearing_splits[split_index].w,
					adjacent_triangles);

			ERR_FAIL_COND_V(!adjacent_triangles.size(), 0);

			// Update the spring
			for (int g(adjacent_triangles.size() - 1); 0 <= g; --g) {

				const int t(adjacent_triangles[g]);
				ParticlePhysicsServer::Triangle &triangle(pb->tearing_data->triangles.write[t]);

				if (pb->tearing_data->sides[t]) {
					// Is on top side
				} else {

					// Is on bottom side

					// Update spring
					for (int e(2); 0 <= e; --e) {

						bool update_adjacent = false;
						if (
								0 <= triangle.edges[e].adjacent_triangle_index &&
								pb->tearing_data->triangles[triangle.edges[e].adjacent_triangle_index].hash_check == hash_check && // To be sure that this triangle is near
								pb->tearing_data->sides[triangle.edges[e].adjacent_triangle_index]) {

							// Split phase

							SpringIndex index;
							// This edge is attached with a triangle that is on
							// the other side of the split plane
							if (0 <= triangle.edges[e].bending_spring_index) {

								// Convert the bending spring to streaching spring
								// bending is no more required, so reuse it

								index = triangle.edges[e].bending_spring_index;
								triangle.edges[e].bending_spring_index = -1;

								// Reset bending spring index on the adjacent triangle
								pb->tearing_data->triangles.write[triangle.edges[e].adjacent_triangle_index].edges[triangle.edges[e].adjacent_edge_index].bending_spring_index = -1;

								pb->copy_spring(
										index,
										triangle.springs[e]);

								pb->tearing_data->spring_rest_lengths_2.write[index] =
										pb->tearing_data->spring_rest_lengths_2[triangle.springs[e]];

								triangle.springs[e] = index;
							}
							update_adjacent = true;
						}

						// Update particle ID
						Spring spring = pb->get_spring_indices(triangle.springs[e]);

						if (particle_buffer_to_split == spring.index0) {
							spring.index0 = added_particle_buffer;
						} else if (particle_buffer_to_split == spring.index1) {
							spring.index1 = added_particle_buffer;
						}

						pb->set_spring_indices(
								triangle.springs[e],
								spring);

						// Update bending spring
						if (0 <= triangle.edges[e].bending_spring_index) {

							Spring spring_bend = pb->get_spring_indices(
									triangle.edges[e].bending_spring_index);

							if (particle_buffer_to_split == spring_bend.index0) {
								spring_bend.index0 = added_particle_buffer;
							} else if (particle_buffer_to_split == spring_bend.index1) {
								spring_bend.index1 = added_particle_buffer;
							}

							pb->set_spring_indices(
									triangle.edges[e].bending_spring_index,
									spring_bend);
						}

						if (update_adjacent) {

							const int other_spring_id(pb->tearing_data->triangles.write[triangle.edges[e].adjacent_triangle_index].springs[triangle.edges[e].adjacent_edge_index]);

							const Spring &other_spring(pb->get_spring_indices(other_spring_id));

							// Crossed check
							if (
									(other_spring.index0 != spring.index0 && other_spring.index1 != spring.index1) &&
									(other_spring.index1 != spring.index0 && other_spring.index0 != spring.index1)) {

								// The two triangles are no more attached

								ParticlePhysicsServer::Edge &other_triangle_edge(
										pb->tearing_data->triangles.write[triangle.edges[e].adjacent_triangle_index].edges[triangle.edges[e].adjacent_edge_index]);

								other_triangle_edge.adjacent_edge_index = -1;
								other_triangle_edge.adjacent_triangle_index = -1;

								triangle.edges[e].adjacent_edge_index = -1;
								triangle.edges[e].adjacent_triangle_index = -1;
							}
						}
					}

					// Update triangle

					DynamicTriangle dynamic_tri = pb->get_triangle(t);
					if (dynamic_tri.index0 == particle_buffer_to_split) {

						dynamic_tri.index0 = added_particle_buffer;
						triangle.a = added_particle;

					} else if (dynamic_tri.index1 == particle_buffer_to_split) {

						dynamic_tri.index1 = added_particle_buffer;
						triangle.b = added_particle;

					} else {

						dynamic_tri.index2 = added_particle_buffer;
						triangle.c = added_particle;
					}

					pb->set_triangle(t, dynamic_tri);
				}
			}

			pb->tearing_data->splits.write[split_index].previous_p_index =
					_tearing_splits[split_index].particle_to_split;

			pb->tearing_data->splits.write[split_index].new_p_index =
					added_particle;
		}
	}

	return added_particles;
}

void FlexSpace::commands_write_buffer() {

	PROFILE("flex_server_commands_write_buffer")

	NvFlexCopyDesc copy_desc;

	if (force_buffer_write) {
		NvFlexSetParticles(solver, particles_memory->particles.buffer, NULL);
		NvFlexSetVelocities(solver, particles_memory->velocities.buffer, NULL);
		NvFlexSetNormals(solver, particles_memory->normals.buffer, NULL);
		NvFlexSetPhases(solver, particles_memory->phases.buffer, NULL);
	} else {

		for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

			FlexParticleBody *body = particle_bodies[i];
			if (!body->particles_mchunk)
				continue;

			const uint32_t changed_params(
					body->get_changed_parameters());

			if (changed_params != 0) {
				copy_desc.srcOffset = body->particles_mchunk->get_begin_index();
				copy_desc.dstOffset = body->particles_mchunk->get_begin_index();
				copy_desc.elementCount = body->particles_mchunk->get_size();

				if (changed_params & eChangedBodyParamParticleJustAdded) {
					NvFlexSetParticles(solver, particles_memory->particles.buffer, &copy_desc);
					NvFlexSetVelocities(solver, particles_memory->velocities.buffer, &copy_desc);
					NvFlexSetNormals(solver, particles_memory->normals.buffer, &copy_desc);
					NvFlexSetPhases(solver, particles_memory->phases.buffer, &copy_desc);
				} else {
					if (changed_params & eChangedBodyParamPositionMass)
						NvFlexSetParticles(solver, particles_memory->particles.buffer, &copy_desc);
					if (changed_params & eChangedBodyParamVelocity)
						NvFlexSetVelocities(solver, particles_memory->velocities.buffer, &copy_desc);
					if (changed_params & eChangedBodyParamNormal)
						NvFlexSetNormals(
								solver,
								particles_memory->normals.buffer,
								&copy_desc);
					if (changed_params & (eChangedBodyParamPhase | eChangedBodyParamPhaseSingle))
						NvFlexSetPhases(
								solver,
								particles_memory->phases.buffer,
								&copy_desc);
					//if(changed_params & eChangedBodyRestParticles)
					//	NvFlexSetRestParticles(solver, )
				}

				body->clear_changed_params();
			}
		}
	}

	if (force_buffer_write || active_particles_memory->was_changed()) {
		copy_desc.srcOffset = 0;
		copy_desc.dstOffset = 0;
		copy_desc.elementCount = active_particles_mchunk->get_size();

		NvFlexSetActive(solver, active_particles_memory->active_particles.buffer, &copy_desc);
		NvFlexSetActiveCount(solver, active_particles_mchunk->get_size());
	}

	if (force_buffer_write || springs_memory->was_changed())
		NvFlexSetSprings(solver, springs_memory->springs.buffer, springs_memory->lengths.buffer, springs_memory->stiffness.buffer, springs_allocator->get_last_used_index() + 1);

	if (force_buffer_write || triangles_memory->was_changed())
		NvFlexSetDynamicTriangles(
				solver,
				triangles_memory->triangles.buffer,
				NULL,
				triangles_allocator->get_last_used_index() + 1);

	if (force_buffer_write || inflatables_memory->was_changed())
		NvFlexSetInflatables(solver, inflatables_memory->start_triangle_indices.buffer, inflatables_memory->triangle_counts.buffer, inflatables_memory->rest_volumes.buffer, inflatables_memory->pressures.buffer, inflatables_memory->constraint_scales.buffer, inflatables_allocator->get_last_used_index() + 1);

	if (force_buffer_write || rigids_memory->was_changed())
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

	if (force_buffer_write || geometries_memory->was_changed())
		NvFlexSetShapes(
				solver,
				geometries_memory->collision_shapes.buffer,
				geometries_memory->positions.buffer,
				geometries_memory->rotations.buffer,
				geometries_memory->positions_prev.buffer,
				geometries_memory->rotations_prev.buffer,
				geometries_memory->flags.buffer,
				geometries_allocator->get_last_used_index() + 1);

	active_particles_memory->changes_synced();
	springs_memory->changes_synced();
	inflatables_memory->changes_synced();
	triangles_memory->changes_synced();
	rigids_memory->changes_synced();
	rigids_components_memory->changes_synced();
	geometries_memory->changes_synced();
	force_buffer_write = false;
}

void FlexSpace::commands_read_buffer() {

	PROFILE("flex_server_step_read_buffer")

	NvFlexGetParticles(solver, particles_memory->particles.buffer, NULL);
	NvFlexGetVelocities(solver, particles_memory->velocities.buffer, NULL);
	NvFlexGetNormals(solver, particles_memory->normals.buffer, NULL);

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

	NvFlexGetContacts(
			solver,
			contacts_buffers->normals.buffer,
			contacts_buffers->velocities_prim_indices.buffer,
			contacts_buffers->indices.buffer,
			contacts_buffers->counts.buffer);
}

void FlexSpace::on_particle_removed(FlexParticleBody *p_body, ParticleBufferIndex p_index) {
	ERR_FAIL_COND(p_body->space != this);

	// Find and remove springs associated to the particle to remove
	for (int spring_index(p_body->springs_mchunk->get_size() - 1); 0 <= spring_index; --spring_index) {
		const Spring &spring = springs_memory->get_spring(p_body->springs_mchunk, spring_index);
		if (spring.index0 == p_index || spring.index1 == p_index) {
			p_body->remove_spring(spring_index);
		}
	}

	// Find and remove triangles associated to the particle to remove
	for (int triangle_index(p_body->triangles_mchunk->get_size() - 1); 0 <= triangle_index; --triangle_index) {
		const DynamicTriangle &triangle = triangles_memory->get_triangle(p_body->triangles_mchunk, triangle_index);
		if (triangle.index0 == p_index || triangle.index1 == p_index || triangle.index2 == p_index) {
			p_body->remove_triangle(triangle_index);
		}
	}

	// Remove rigid components associated to this body
	for (RigidComponentIndex i(p_body->rigids_components_mchunk->get_size() - 1); 0 <= i; --i) {

		if (p_index == rigids_components_memory->get_index(p_body->rigids_components_mchunk, i)) {
			p_body->remove_rigid_component(i);
		}
	}
}

void FlexSpace::on_particle_index_changed(FlexParticleBody *p_body, ParticleBufferIndex p_index_old, ParticleBufferIndex p_index_new) {
	ERR_FAIL_COND(p_body->space != this);

	// Change springs index
	for (int i(p_body->springs_mchunk->get_size() - 1); 0 <= i; --i) {

		const Spring &spring(springs_memory->get_spring(p_body->springs_mchunk, i));
		if (spring.index0 == p_index_old) {

			springs_memory->set_spring(p_body->springs_mchunk, i, Spring(p_index_new, spring.index1));
		} else if (spring.index1 == p_index_old) {

			springs_memory->set_spring(p_body->springs_mchunk, i, Spring(spring.index0, p_index_new));
		}
	}

	// Change triangle index
	for (int i(p_body->triangles_mchunk->get_size() - 1); 0 <= i; --i) {

		const DynamicTriangle &triangle = triangles_memory->get_triangle(p_body->triangles_mchunk, i);
		if (triangle.index0 == p_index_old) {

			triangles_memory->set_triangle(p_body->triangles_mchunk, i, DynamicTriangle(p_index_new, triangle.index1, triangle.index2));
		} else if (triangle.index1 == p_index_old) {

			triangles_memory->set_triangle(p_body->triangles_mchunk, i, DynamicTriangle(triangle.index0, p_index_new, triangle.index2));
		} else if (triangle.index2 == p_index_old) {

			triangles_memory->set_triangle(p_body->triangles_mchunk, i, DynamicTriangle(triangle.index0, triangle.index1, p_index_new));
		}
	}

	// Change rigid index
	for (int i(p_body->rigids_components_mchunk->get_size() - 1); 0 <= i; --i) {

		ParticleBufferIndex buffer_index = rigids_components_memory->get_index(p_body->rigids_components_mchunk, i);
		if (p_index_old == buffer_index) {

			rigids_components_memory->set_index(p_body->rigids_components_mchunk, i, p_index_new);
		}
	}

	// Update id even in the commands
	const int chunk_index_old(p_body->particles_mchunk->get_chunk_index(p_index_old));
	const int chunk_index_new(p_body->particles_mchunk->get_chunk_index(p_index_new));

	{
		auto it = std::find(
				p_body->delayed_commands.particles_to_remove.begin(),
				p_body->delayed_commands.particles_to_remove.end(),
				chunk_index_old);
		if (it != p_body->delayed_commands.particles_to_remove.end()) {
			const int pos = it - p_body->delayed_commands.particles_to_remove.begin();
			p_body->delayed_commands.particles_to_remove[pos] = chunk_index_new;
		}
	}

	{
		auto it = std::find(
				p_body->delayed_commands.particles_to_unactive.begin(),
				p_body->delayed_commands.particles_to_unactive.end(),
				chunk_index_old);
		if (it != p_body->delayed_commands.particles_to_unactive.end()) {
			const int pos = it - p_body->delayed_commands.particles_to_unactive.begin();
			p_body->delayed_commands.particles_to_unactive[pos] = chunk_index_new;
		}
	}
}

void FlexSpace::rebuild_inflatables_indices() {
	if (!triangles_memory->was_changed()) {
		if (inflatables_memory->was_changed()) {
			inflatables_allocator->sanitize();
		}
		return;
	}
	triangles_allocator->sanitize();
	inflatables_allocator->sanitize();

	for (int body_i(particle_bodies.size() - 1); 0 <= body_i; --body_i) {
		FlexParticleBody *body = particle_bodies[body_i];

		if (!body->inflatable_mchunk->get_size())
			continue;

		inflatables_memory->set_start_triangle_index(body->inflatable_mchunk, 0, body->triangles_mchunk->get_begin_index());
		inflatables_memory->set_triangle_count(body->inflatable_mchunk, 0, body->triangles_mchunk->get_size());
	}
}

FlexParticleBody *FlexSpace::find_particle_body(ParticleBufferIndex p_index) const {
	for (int i(particle_bodies.size() - 1); 0 <= i; --i) {
		if (
				p_index >= particle_bodies[i]->particles_mchunk->get_begin_index() &&
				p_index <= particle_bodies[i]->particles_mchunk->get_end_index()) {
			return particle_bodies[i];
		}
	}
	return NULL;
}

FlexPrimitiveBody *FlexSpace::find_primitive_body(GeometryBufferIndex p_index) const {
	return geometries_memory->get_self(p_index);
}

void FlexSpace::update_custom_friction_primitive_body(
		FlexPrimitiveBody *p_body) {

	if (!p_body->space && 0 <= p_body->_custom_friction_id) {

		// Remove phase

		ERR_FAIL_COND(primitive_bodies_cf[p_body->_custom_friction_id] != p_body);

		// Swap (to prevent reindex everything)
		const int new_size(primitive_bodies_cf.size() - 1);

		const int new_id(p_body->_custom_friction_id);
		const int old_id(new_size);

		primitive_bodies_cf[old_id]->_custom_friction_id = new_id;

		primitive_bodies_cf[new_id] = primitive_bodies_cf[old_id];
		primitive_bodies_cf.resize(new_size);

		primitive_bodies_cf_prev_transform.resize(new_size);

		primitive_bodies_cf_prev_inv_transform.resize(new_size);

		primitive_bodies_cf_curr_transform[new_id] =
				primitive_bodies_cf_curr_transform[old_id];
		primitive_bodies_cf_curr_transform.resize(new_size);

		primitive_bodies_cf_curr_inv_transform[new_id] =
				primitive_bodies_cf_curr_inv_transform[old_id];
		primitive_bodies_cf_curr_inv_transform.resize(new_size);

		primitive_bodies_cf_motion.resize(new_size);

		primitive_bodies_cf_extent[new_id] =
				primitive_bodies_cf_extent[old_id];
		primitive_bodies_cf_extent.resize(new_size);

		primitive_bodies_cf_friction[new_id] =
				primitive_bodies_cf_friction[old_id];
		primitive_bodies_cf_friction.resize(new_size);

		primitive_bodies_cf_friction_2_threshold[new_id] =
				primitive_bodies_cf_friction_2_threshold[old_id];
		primitive_bodies_cf_friction_2_threshold.resize(new_size);

		primitive_bodies_cf_layers[new_id] =
				primitive_bodies_cf_layers[old_id];
		primitive_bodies_cf_layers.resize(new_size);

		p_body->_custom_friction_id = -1;

	} else {

		if (!p_body->is_using_custom_friction())
			return;

		ERR_FAIL_COND(p_body->space != this);

		int id = p_body->_custom_friction_id;

		if (0 > id) {

			// Add phase

			id = primitive_bodies_cf.size();
			const int new_size(id + 1);

			primitive_bodies_cf.resize(new_size);
			primitive_bodies_cf_prev_transform.resize(new_size);
			primitive_bodies_cf_prev_inv_transform.resize(new_size);
			primitive_bodies_cf_curr_transform.resize(new_size);
			primitive_bodies_cf_curr_inv_transform.resize(new_size);
			primitive_bodies_cf_motion.resize(new_size);
			primitive_bodies_cf_extent.resize(new_size);
			primitive_bodies_cf_friction.resize(new_size);
			primitive_bodies_cf_friction_2_threshold.resize(new_size);
			primitive_bodies_cf_layers.resize(new_size);

			p_body->_custom_friction_id = id;
		} else {

			ERR_FAIL_COND(primitive_bodies_cf[id] != p_body);
		}

		// Update phase

		primitive_bodies_cf[id] = p_body;

		primitive_bodies_cf_curr_transform[id] = p_body->get_transform();

		primitive_bodies_cf_curr_inv_transform[id] =
				p_body->get_transform().inverse();

		primitive_bodies_cf_extent[id] =
				p_body->get_shape() && p_body->get_shape()->get_type() == eNvFlexShapeBox ?
						static_cast<FlexPrimitiveBoxShape *>(p_body->get_shape())->get_extends() :
						Vector3();

		primitive_bodies_cf_friction[id] = p_body->get_custom_friction();
		primitive_bodies_cf_friction_2_threshold[id] =
				p_body->get_custom_friction_threshold() *
				p_body->get_custom_friction_threshold();

		primitive_bodies_cf_layers[id] = p_body->get_layer();
	}
}

FlexMemorySweeperFast::FlexMemorySweeperFast(
		FlexMemoryAllocator *p_allocator,
		MemoryChunk *&r_mchunk,
		std::vector<FlexChunkIndex> &r_indices_to_remove,
		bool p_reallocate_memory,
		FlexBufferIndex p_custom_chunk_end_buffer_index) :
		allocator(p_allocator),
		mchunk(r_mchunk),
		indices_to_remove(r_indices_to_remove),
		reallocate_memory(p_reallocate_memory),
		custom_chunk_end_buffer_index(p_custom_chunk_end_buffer_index) {}

void FlexMemorySweeperFast::exec() {

	FlexBufferIndex chunk_end_buffer_index(
			-1 == custom_chunk_end_buffer_index ?
					mchunk->get_end_index() :
					custom_chunk_end_buffer_index);

	const int rem_indices_count(indices_to_remove.size());

	for (int i = 0; i < rem_indices_count; ++i) {

		const FlexChunkIndex index_to_remove(indices_to_remove[i]);
		const FlexBufferIndex buffer_index_to_remove(
				mchunk->get_buffer_index(index_to_remove));

		on_element_removed(buffer_index_to_remove);
		if (chunk_end_buffer_index != buffer_index_to_remove) {

			allocator->get_memory()->copy(
					buffer_index_to_remove,
					1,
					chunk_end_buffer_index);

			on_element_index_changed(
					chunk_end_buffer_index,
					buffer_index_to_remove);
		}

		const FlexChunkIndex old_chunk_index(
				mchunk->get_chunk_index(chunk_end_buffer_index));

		// Change the index from the next elements to remove
		if ((i + 1) < rem_indices_count)
			for (int b(i + 1); b < rem_indices_count; ++b) {
				if (old_chunk_index == indices_to_remove[b]) {
					indices_to_remove[b] = index_to_remove;
				}
			}

		--chunk_end_buffer_index;
	}

	if (reallocate_memory)
		allocator->resize_chunk(
				mchunk,
				chunk_end_buffer_index - mchunk->get_begin_index() + 1);

	indices_to_remove.clear(); // This clear is here to be sure that this vector not used anymore
}

ParticlesMemorySweeper::ParticlesMemorySweeper(
		FlexSpace *p_space,
		FlexParticleBody *p_body,
		FlexMemoryAllocator *p_allocator,
		MemoryChunk *&r_rigids_components_mchunk,
		std::vector<FlexChunkIndex> &r_indices_to_remove,
		bool p_reallocate_memory,
		FlexBufferIndex p_custom_chunk_end_buffer_index) :
		FlexMemorySweeperFast(
				p_allocator,
				r_rigids_components_mchunk,
				r_indices_to_remove,
				p_reallocate_memory,
				p_custom_chunk_end_buffer_index),
		space(p_space),
		body(p_body) {
}

void ParticlesMemorySweeper::on_element_removed(FlexBufferIndex on_element_removed) {
	space->on_particle_removed(body, on_element_removed);
}

void ParticlesMemorySweeper::on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index) {
	space->on_particle_index_changed(body, old_element_index, new_element_index);
	body->particle_index_changed(mchunk->get_chunk_index(old_element_index), mchunk->get_chunk_index(new_element_index));
}

SpringsMemorySweeper::SpringsMemorySweeper(FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, std::vector<FlexChunkIndex> &r_indices_to_remove) :
		FlexMemorySweeperFast(
				p_allocator,
				r_rigids_components_mchunk,
				r_indices_to_remove,
				true),
		body(p_body) {
}

void SpringsMemorySweeper::on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index) {
	body->spring_index_changed(mchunk->get_chunk_index(old_element_index), mchunk->get_chunk_index(new_element_index));
}

TrianglesMemorySweeper::TrianglesMemorySweeper(FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, std::vector<FlexChunkIndex> &r_indices_to_remove) :
		FlexMemorySweeperFast(
				p_allocator,
				r_rigids_components_mchunk,
				r_indices_to_remove,
				true),
		body(p_body) {
}

void TrianglesMemorySweeper::on_element_removed(FlexBufferIndex on_element_removed) {
	body->reload_inflatables();
}

FlexMemorySweeperSlow::FlexMemorySweeperSlow(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_mchunk, std::vector<FlexChunkIndex> &r_indices_to_remove) :
		allocator(p_allocator),
		mchunk(r_mchunk),
		indices_to_remove(r_indices_to_remove) {}

void FlexMemorySweeperSlow::exec() {

	FlexBufferIndex chunk_end_index(mchunk->get_end_index());
	const int rem_indices_count(indices_to_remove.size());

	for (int i = 0; i < rem_indices_count; ++i) {

		const FlexChunkIndex index_to_remove(indices_to_remove[i]);
		const FlexBufferIndex buffer_index_to_remove(mchunk->get_buffer_index(index_to_remove));

		on_element_remove(index_to_remove);

		int sub_chunk_size(chunk_end_index - (buffer_index_to_remove + 1) + 1);
		allocator->get_memory()->copy(buffer_index_to_remove, sub_chunk_size, buffer_index_to_remove + 1);

		on_element_removed(index_to_remove);

		// Change the index from the next elements to remove
		if ((i + 1) < rem_indices_count)
			for (int b(i + 1); b < rem_indices_count; ++b) {
				if (index_to_remove <= indices_to_remove[b]) {
					indices_to_remove[b] -= 1;
				}
			}

		--chunk_end_index;
	}
	allocator->resize_chunk(mchunk, chunk_end_index - mchunk->get_begin_index() + 1);
	indices_to_remove.clear(); // This clear is here to be sure that this vector not used anymore
}

RigidsComponentsMemorySweeper::RigidsComponentsMemorySweeper(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, std::vector<FlexChunkIndex> &r_indices_to_remove, RigidsMemory *p_rigids_memory, MemoryChunk *&r_rigids_mchunk) :
		FlexMemorySweeperSlow(p_allocator, r_rigids_components_mchunk, r_indices_to_remove),
		rigids_memory(p_rigids_memory),
		rigids_mchunk(r_rigids_mchunk) {}

void RigidsComponentsMemorySweeper::on_element_removed(RigidComponentIndex p_removed_index) {
	// Update offset in rigid body
	for (int i(rigids_mchunk->get_size() - 1); 0 <= i; --i) {
		RigidComponentIndex offset(rigids_memory->get_offset(rigids_mchunk, i));
		if (offset > p_removed_index) {
			rigids_memory->set_offset(rigids_mchunk, i, offset - 1);
		} else {
			break;
		}
	}
}

RigidsMemorySweeper::RigidsMemorySweeper(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_mchunk, std::vector<FlexChunkIndex> &r_indices_to_remove, RigidsMemory *p_rigids_memory, FlexMemoryAllocator *p_rigids_components_allocator, RigidsComponentsMemory *p_rigids_components_memory, MemoryChunk *&r_rigids_components_mchunk) :
		FlexMemorySweeperSlow(p_allocator, r_rigids_mchunk, r_indices_to_remove),
		rigids_memory(p_rigids_memory),
		rigids_components_allocator(p_rigids_components_allocator),
		rigids_components_memory(p_rigids_components_memory),
		rigids_components_mchunk(r_rigids_components_mchunk) {}

void RigidsMemorySweeper::on_element_remove(RigidIndex p_removed_index) {
	RigidComponentIndex rigids_start_index = p_removed_index == 0 ? RigidComponentIndex(0) : rigids_memory->get_offset(mchunk, p_removed_index - 1);
	RigidComponentIndex next_rigid_index = rigids_memory->get_offset(mchunk, p_removed_index);
	rigid_particle_index_count = next_rigid_index - rigids_start_index;
}

void RigidsMemorySweeper::on_element_removed(RigidIndex p_removed_index) {

	RigidComponentIndex rigids_start_index = p_removed_index == 0 ? RigidComponentIndex(0) : rigids_memory->get_offset(mchunk, p_removed_index - 1);
	RigidComponentIndex next_rigid_index = rigids_start_index + rigid_particle_index_count;

	// Recreate offset
	rigids_memory->set_offset(mchunk, p_removed_index, next_rigid_index);

	// Remove all indices of rigid
	int sub_chunk_size(rigids_components_mchunk->get_end_index() - next_rigid_index + 1);
	rigids_components_memory->copy(rigids_components_mchunk->get_buffer_index(rigids_start_index), sub_chunk_size, rigids_components_mchunk->get_buffer_index(next_rigid_index));

	rigids_components_allocator->resize_chunk(rigids_components_mchunk, rigids_start_index + sub_chunk_size);
}
