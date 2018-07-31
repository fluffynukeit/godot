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
#include "flex_particle_body_constraint.h"
#include "flex_primitive_body.h"
#include "flex_primitive_shapes.h"
#include "print_string.h"

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
		contacts_buffers(NULL) {
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

	init_buffers();
	init_solver();

	contacts_buffers->resize(solver_max_particles);
	contacts_buffers->unmap();

	reset_params_to_defaults();
	CRASH_COND(has_error());
}

void FlexSpace::init_buffers() {
	CRASH_COND(particles_memory);
	CRASH_COND(particles_allocator);
	particles_memory = memnew(ParticlesMemory(flex_lib));
	particles_allocator = memnew(FlexMemoryAllocator(particles_memory, 2000, 1000, -1));
	particles_memory->unmap(); // *1

	CRASH_COND(active_particles_allocator);
	CRASH_COND(active_particles_memory);
	active_particles_memory = memnew(ActiveParticlesMemory(flex_lib));
	active_particles_allocator = memnew(FlexMemoryAllocator(active_particles_memory, particles_allocator->get_memory_size(), 1000, -1));
	active_particles_memory->unmap(); // *1

	active_particles_mchunk = active_particles_allocator->allocate_chunk(0);

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
	geometries_allocator = memnew(FlexMemoryAllocator(geometries_memory, 5, 5));
	geometries_memory->unmap(); // *1

	// *1: This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped
}

void FlexSpace::init_solver() {
	CRASH_COND(solver);
	NvFlexSolverDesc solver_desc;

	solver_max_particles = particles_allocator->get_memory_size();

	NvFlexSetSolverDescDefaults(&solver_desc);
	solver_desc.featureMode = eNvFlexFeatureModeDefault; // All modes enabled (Solid|Fluids) // TODO should be customizable
	solver_desc.maxParticles = solver_max_particles;
	solver_desc.maxDiffuseParticles = 0; // TODO should be customizable
	solver_desc.maxNeighborsPerParticle = 32; // TODO should be customizable
	solver_desc.maxContactsPerParticle = MAX_PERPARTICLE_CONTACT_COUNT;

	solver = NvFlexCreateSolver(flex_lib, &solver_desc);
	CRASH_COND(has_error());
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

void FlexSpace::sync() {

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

	///
	/// Stepping phase
	dispatch_callback_contacts();
	dispatch_callbacks();
	execute_delayed_commands();
	execute_geometries_commands();

	///
	/// Emit server sync
	ParticlePhysicsServer::get_singleton()->emit_signal("sync_end", get_self());

	///
	/// Unmap phase

	if (springs_memory->is_force_sanitization())
		springs_memory->notify_change();

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

		NvFlexParams params;
		NvFlexGetParams(solver, &params);
		terminate_solver();
		init_solver();
		NvFlexSetParams(solver, &params);

		contacts_buffers->resize(solver_max_particles);
	}

	contacts_buffers->unmap();

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

bool FlexSpace::can_commands_be_executed() const {
	return particles_memory->is_mapped();
}

void FlexSpace::add_particle_body(FlexParticleBody *p_body) {
	ERR_FAIL_COND(p_body->space);
	p_body->space = this;
	particle_bodies.push_back(p_body);

	p_body->changed_parameters = eChangedBodyParamALL;
	p_body->particles_mchunk = particles_allocator->allocate_chunk(0);
	p_body->springs_mchunk = springs_allocator->allocate_chunk(0);
	p_body->triangles_mchunk = triangles_allocator->allocate_chunk(0);
	p_body->inflatable_mchunk = inflatables_allocator->allocate_chunk(0);
	p_body->rigids_mchunk = rigids_allocator->allocate_chunk(0);
	p_body->rigids_components_mchunk = rigids_components_allocator->allocate_chunk(0);
}

void FlexSpace::remove_particle_body(FlexParticleBody *p_body) {

	// TODO think about the fact to transfer these data to a cache before clear it
	rigids_components_allocator->deallocate_chunk(p_body->rigids_components_mchunk);
	rigids_allocator->deallocate_chunk(p_body->rigids_mchunk);
	triangles_allocator->deallocate_chunk(p_body->triangles_mchunk);
	inflatables_allocator->deallocate_chunk(p_body->inflatable_mchunk);
	springs_allocator->deallocate_chunk(p_body->springs_mchunk);
	particles_allocator->deallocate_chunk(p_body->particles_mchunk);

	p_body->space = NULL;
	particle_bodies.erase(p_body);

	// TODO Show a warning and remove body constraint associated to this body
}

void FlexSpace::add_particle_body_constraint(FlexParticleBodyConstraint *p_constraint) {
	ERR_FAIL_COND(!p_constraint);
	p_constraint->space = this;
	constraints.push_back(p_constraint);

	p_constraint->springs_mchunk = springs_allocator->allocate_chunk(0);
}

void FlexSpace::remove_particle_body_constraint(FlexParticleBodyConstraint *p_constraint) {
	springs_allocator->deallocate_chunk(p_constraint->springs_mchunk);
	springs_memory->require_force_sanitization();

	p_constraint->space = NULL;

	constraints.erase(p_constraint);
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
	} else {
		return false;
	}

	return true;
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

void FlexSpace::dispatch_callback_contacts() {

	for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

		FlexParticleBody *particle_body = particle_bodies[i];
		if (!particle_body->is_monitorable())
			continue;

		for (int particle_buffer_index(particle_body->particles_mchunk->get_begin_index()); particle_buffer_index <= particle_body->particles_mchunk->get_end_index(); ++particle_buffer_index) {

			const int contact_index(contacts_buffers->indices[particle_buffer_index]);
			const uint32_t particle_contact_count(contacts_buffers->counts[contact_index]);

			if (!particle_contact_count)
				continue;

			const ParticleIndex particle_index(particle_body->particles_mchunk->get_chunk_index(particle_buffer_index));

			for (uint32_t c(0); c < particle_contact_count; ++c) {

				const FlVector4 &velocity_and_primitive(contacts_buffers->velocities_prim_indices[contact_index * MAX_PERPARTICLE_CONTACT_COUNT + c]);
				const FlVector4 &raw_normal(contacts_buffers->normals[contact_index * MAX_PERPARTICLE_CONTACT_COUNT + c]);

				Vector3 velocity(vec3_from_flvec4(velocity_and_primitive));
				Vector3 normal(vec3_from_flvec4(raw_normal));

				const int primitive_body_index(velocity_and_primitive.w);
				FlexPrimitiveBody *primitive_body = find_primitive_body(primitive_body_index);

				if (particle_body->is_monitoring_primitives_contacts())
					particle_body->dispatch_primitive_contact(primitive_body, particle_index, velocity, normal);

				if (primitive_body->is_monitoring_particles_contacts())
					primitive_body->dispatch_particle_contact(particle_body, particle_index, velocity, normal);
			}
		}
	}
}

void FlexSpace::dispatch_callbacks() {
	for (int i(particle_bodies.size() - 1); 0 <= i; --i) {
		particle_bodies[i]->dispatch_sync_callback();
	}
	for (int i(constraints.size() - 1); 0 <= i; --i) {
		constraints[i]->dispatch_sync_callback();
	}
	for (int i(primitive_bodies.size() - 1); 0 <= i; --i) {
		primitive_bodies[i]->dispatch_sync_callback();
	}
}

void FlexSpace::execute_delayed_commands() {

	int particles_count = 0;
	for (int body_index(particle_bodies.size() - 1); 0 <= body_index; --body_index) {

		FlexParticleBody *body = particle_bodies[body_index];

		if (body->delayed_commands.particles_to_remove.size()) {

			ParticlesMemorySweeper sweeper(this, body, particles_allocator, body->particles_mchunk, body->delayed_commands.particles_to_remove);
			sweeper.exec();
		}

		if (body->delayed_commands.springs_to_remove.size()) {

			SpringsMemorySweeper sweeper(body, springs_allocator, body->springs_mchunk, body->delayed_commands.springs_to_remove);
			sweeper.exec();
		}

		if (body->delayed_commands.triangles_to_remove.size()) {

			TrianglesMemorySweeper sweeper(body, triangles_allocator, body->triangles_mchunk, body->delayed_commands.triangles_to_remove);
			sweeper.exec();
		}

		if (body->delayed_commands.rigids_components_to_remove.size()) {

			RigidsComponentsMemorySweeper sweeper(rigids_components_allocator, body->rigids_components_mchunk, body->delayed_commands.rigids_components_to_remove, rigids_memory, body->rigids_mchunk);
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
			rigids_allocator->copy_chunk(swap_area, initial_chunk);
			rigids_allocator->copy_chunk(initial_chunk, lowest_chunk);
			rigids_allocator->copy_chunk(lowest_chunk, swap_area);
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
			geometries_memory->set_flags(body->geometry_mchunk, 0, (NvFlexMakeShapeFlagsWithChannels(body->get_shape()->get_type(), body->is_kinematic(), body->get_layer() << 24) | (body->is_area() ? eNvFlexShapeFlagTrigger : 0)));
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
					NvFlexSetNormals(solver, particles_memory->normals.buffer, &copy_desc);
				if (changed_params & eChangedBodyParamPhase)
					NvFlexSetPhases(solver, particles_memory->phases.buffer, &copy_desc);
			}

			body->clear_changed_params();
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

	if (triangles_memory->was_changed())
		NvFlexSetDynamicTriangles(solver, triangles_memory->triangles.buffer, NULL, triangles_allocator->get_last_used_index() + 1);

	if (inflatables_memory->was_changed())
		NvFlexSetInflatables(solver, inflatables_memory->start_triangle_indices.buffer, inflatables_memory->triangle_counts.buffer, inflatables_memory->rest_volumes.buffer, inflatables_memory->pressures.buffer, inflatables_memory->constraint_scales.buffer, inflatables_allocator->get_last_used_index() + 1);

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
		NvFlexGetParticles(solver, particles_memory->particles.buffer, &copy_desc);
		NvFlexGetVelocities(solver, particles_memory->velocities.buffer, &copy_desc);
		NvFlexGetNormals(solver, particles_memory->normals.buffer, &copy_desc);

		particle_bodies[i]->clear_delayed_commands();
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

	NvFlexGetContacts(solver, contacts_buffers->normals.buffer, contacts_buffers->velocities_prim_indices.buffer, contacts_buffers->indices.buffer, contacts_buffers->counts.buffer);
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

	const int pos = p_body->delayed_commands.particles_to_remove.find(chunk_index_old);
	if (0 <= pos)
		p_body->delayed_commands.particles_to_remove[pos] = chunk_index_new;
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
		if (p_index >= particle_bodies[i]->particles_mchunk->get_begin_index() && p_index <= particle_bodies[i]->particles_mchunk->get_end_index()) {
			return particle_bodies[i];
		}
	}
	return NULL;
}

FlexPrimitiveBody *FlexSpace::find_primitive_body(GeometryBufferIndex p_index) const {
	for (int i(primitive_bodies.size() - 1); 0 <= i; --i) {
		if (p_index == primitive_bodies[i]->geometry_mchunk->get_begin_index()) {
			return primitive_bodies[i];
		}
	}
	return NULL;
}

FlexMemorySweeperFast::FlexMemorySweeperFast(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove) :
		allocator(p_allocator),
		mchunk(r_mchunk),
		indices_to_remove(r_indices_to_remove) {}

void FlexMemorySweeperFast::exec() {

	FlexBufferIndex chunk_end_buffer_index(mchunk->get_end_index());
	const int rem_indices_count(indices_to_remove.size());

	for (int i = 0; i < rem_indices_count; ++i) {

		const FlexChunkIndex index_to_remove(indices_to_remove[i]);
		const FlexBufferIndex buffer_index_to_remove(mchunk->get_buffer_index(index_to_remove));

		on_element_removed(buffer_index_to_remove);
		if (chunk_end_buffer_index != buffer_index_to_remove) {
			allocator->get_memory()->copy(buffer_index_to_remove, 1, chunk_end_buffer_index);
			on_element_index_changed(chunk_end_buffer_index, buffer_index_to_remove);
		}

		const FlexChunkIndex old_chunk_index(mchunk->get_chunk_index(chunk_end_buffer_index));
		// Change the index from the next elements to remove
		if ((i + 1) < rem_indices_count)
			for (int b(i + 1); b < rem_indices_count; ++b) {
				if (old_chunk_index == indices_to_remove[b]) {
					indices_to_remove[b] = index_to_remove;
				}
			}

		--chunk_end_buffer_index;
	}
	allocator->resize_chunk(mchunk, chunk_end_buffer_index - mchunk->get_begin_index() + 1);
	indices_to_remove.clear(); // This clear is here to be sure that this vector not used anymore
}

ParticlesMemorySweeper::ParticlesMemorySweeper(FlexSpace *p_space, FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove) :
		FlexMemorySweeperFast(p_allocator, r_rigids_components_mchunk, r_indices_to_remove),
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

SpringsMemorySweeper::SpringsMemorySweeper(FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove) :
		FlexMemorySweeperFast(p_allocator, r_rigids_components_mchunk, r_indices_to_remove),
		body(p_body) {
}

void SpringsMemorySweeper::on_element_index_changed(FlexBufferIndex old_element_index, FlexBufferIndex new_element_index) {
	body->spring_index_changed(mchunk->get_chunk_index(old_element_index), mchunk->get_chunk_index(new_element_index));
}

TrianglesMemorySweeper::TrianglesMemorySweeper(FlexParticleBody *p_body, FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove) :
		FlexMemorySweeperFast(p_allocator, r_rigids_components_mchunk, r_indices_to_remove),
		body(p_body) {
}

void TrianglesMemorySweeper::on_element_removed(FlexBufferIndex on_element_removed) {
	body->reload_inflatables();
}

FlexMemorySweeperSlow::FlexMemorySweeperSlow(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove) :
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

RigidsComponentsMemorySweeper::RigidsComponentsMemorySweeper(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_components_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove, RigidsMemory *p_rigids_memory, MemoryChunk *&r_rigids_mchunk) :
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

RigidsMemorySweeper::RigidsMemorySweeper(FlexMemoryAllocator *p_allocator, MemoryChunk *&r_rigids_mchunk, Vector<FlexChunkIndex> &r_indices_to_remove, RigidsMemory *p_rigids_memory, FlexMemoryAllocator *p_rigids_components_allocator, RigidsComponentsMemory *p_rigids_components_memory, MemoryChunk *&r_rigids_components_mchunk) :
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
