/*************************************************************************/
/*  flex_particle_physics_server.cpp                                     */
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

#include "flex_particle_physics_server.h"

#include "thirdparty/flex/include/NvFlex.h"

#include "string.h"

/**
	@author AndreaCatania
*/

#define MAXPARTICLES 10

#define DEVICE_ID 0
#define FS FlexParticlePhysicsServer::singleton

#define CreateThenReturnRID(owner, rid_data) \
    RID rid = owner.make_rid(rid_data);      \
    rid_data->__set_self(rid);               \
    rid_data->__set_physics_server(this);    \
    return rid;

// TODO use a class
NvFlexErrorSeverity error_severity = eNvFlexLogInfo; // contain last error severity
void ErrorCallback(NvFlexErrorSeverity severity, const char *msg, const char *file, int line) {
    print_error(String("Flex error file: ") + file + " - LINE: " + String::num(line) + " - MSG: " + msg);
    error_severity = severity;
}
bool has_error() {
    return error_severity == NvFlexErrorSeverity::eNvFlexLogError;
}

FlexBuffers::FlexBuffers() :
		// Allocation is managed automatically
        particles(FS->flex_lib),
		velocities(FS->flex_lib),
		phases(FS->flex_lib),
		active_particles(FS->flex_lib) {

	ERR_EXPLAIN("ERROR! Before initialize buffers you must initialize Flex library.");
	ERR_FAIL_COND(!FS->flex_lib);
}

void FlexBuffers::init() {}

void FlexBuffers::terminate() {
    particles.destroy();
    velocities.destroy();
    phases.destroy();
    active_particles.destroy();
}

void FlexBuffers::map() {
    particles.map(eNvFlexMapWait);
    velocities.map(eNvFlexMapWait);
    phases.map(eNvFlexMapWait);
    active_particles.map(eNvFlexMapWait);
    if (has_error()) {
        print_error("Has error");
    }
}

void FlexBuffers::unmap() {
    particles.unmap();
    velocities.unmap();
    phases.unmap();
    active_particles.unmap();
}

FlexParticlePhysicsServer *FlexParticlePhysicsServer::singleton = NULL;

RID FlexParticlePhysicsServer::body_create() {
    FlexParticleBody *particle_body = memnew(FlexParticleBody);
    CreateThenReturnRID(body_owner, particle_body);
}

void FlexParticlePhysicsServer::init() {

	// Init library
	ERR_FAIL_COND(flex_lib);

	NvFlexInitDesc desc;
    desc.deviceIndex = DEVICE_ID;
	desc.enableExtensions = true;
    desc.renderDevice = NULL;
    desc.renderContext = NULL;
    desc.computeContext = NULL;
	desc.computeType = eNvFlexCUDA;
    desc.runOnRenderContext = false;

    flex_lib = NvFlexInit(NV_FLEX_VERSION, ErrorCallback, &desc);
	ERR_FAIL_COND(!flex_lib);
    ERR_FAIL_COND(has_error());

	// Init solvert
	ERR_FAIL_COND(solver);

	NvFlexSolverDesc solver_desc;

    NvFlexSetSolverDescDefaults(&solver_desc);
    solver_desc.featureMode = eNvFlexFeatureModeDefault; // All modes enabled (Solid|Fluids) // TODO should be customizable
    solver_desc.maxParticles = MAXPARTICLES; // TODO should be customizable
    solver_desc.maxDiffuseParticles = MAXPARTICLES; // TODO should be customizable
    solver_desc.maxNeighborsPerParticle = 32; // TODO should be customizable
    solver_desc.maxContactsPerParticle = 10; // TODO should be customizable

    solver = NvFlexCreateSolver(flex_lib, &solver_desc);
    ERR_FAIL_COND(has_error());

	// Init buffers
	ERR_FAIL_COND(buffers);
	buffers = memnew(FlexBuffers);
    buffers->init();

    NvFlexParams params;
    // Initialize solver parameter
    NvFlexGetParams(solver, &params);
	params.gravity[0] = 0.0;
    params.gravity[1] = -10.0;
	params.gravity[2] = 0.0;
	params.radius = 0.1;
	params.numIterations = 3;
    params.maxSpeed = FLT_MAX;
    params.maxAcceleration = 100.0; // Gravity * 10 // TODO use a variable multiplication
    params.relaxationMode = eNvFlexRelaxationLocal;
    params.relaxationFactor = 1.0;
    params.solidPressure = 1.0;
	NvFlexSetParams(solver, &params);

    ERR_FAIL_COND(has_error());
}

void FlexParticlePhysicsServer::terminate() {

    ERR_FAIL_COND(!buffers);
    buffers->terminate();
	memdelete(buffers);
	buffers = NULL;

	ERR_FAIL_COND(!solver);
	NvFlexDestroySolver(solver);
	solver = NULL;

	ERR_FAIL_COND(!flex_lib);
	NvFlexShutdown(flex_lib);
	flex_lib = NULL;
}

void FlexParticlePhysicsServer::sync() {

    buffers->map();

    // Read operation
    if (active_particle_count > 0) {

        print_line("X: " + String::num_real(buffers->particles[0].x) + " Y: " + String::num_real(buffers->particles[0].y) + " Z: " + String::num_real(buffers->particles[0].z));
    }

    bool require_write = false;
    // Write operation
    if (!buffers->particles.size()) { // TODO just a test

        require_write = true;

        real_t mass = 2.0;
        buffers->particles.push_back(FlVector4(0.0, 0.0, 0.0, 1.0 / mass));
        buffers->velocities.push_back(FlVector3(0.0, 0.0, 0.0));
        const int group = 0;
        const int phase = NvFlexMakePhase(group, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter);
        buffers->phases.push_back(phase);
        buffers->active_particles.push_back(0); // index of particle
    }

    buffers->unmap();

    if (require_write) {

        NvFlexCopyDesc copy_desc;
        copy_desc.srcOffset = 0;
        copy_desc.dstOffset = 0;
        copy_desc.elementCount = buffers->particles.size();

        // Write buffer to GPU (command)
        NvFlexSetParticles(solver, buffers->particles.buffer, &copy_desc);
        NvFlexSetVelocities(solver, buffers->velocities.buffer, &copy_desc);
        NvFlexSetPhases(solver, buffers->phases.buffer, &copy_desc);
        NvFlexSetActive(solver, buffers->active_particles.buffer, &copy_desc);
        NvFlexSetActiveCount(solver, buffers->active_particles.size());
    }
}

void FlexParticlePhysicsServer::flush_queries() {}

void FlexParticlePhysicsServer::step(real_t p_delta_time) {

    active_particle_count = NvFlexGetActiveCount(solver);

    if (active_particle_count > 0) {

        // Step solver (command)
        const int substep = 1;
        const bool enable_timer = false; // Used for profiling
        NvFlexUpdateSolver(solver, p_delta_time, substep, enable_timer);

        // Write back to buffer (command)
        NvFlexCopyDesc copy_desc;
        copy_desc.srcOffset = 0;
        copy_desc.dstOffset = 0;
        copy_desc.elementCount = buffers->particles.size();

        NvFlexGetParticles(solver, buffers->particles.buffer, &copy_desc);
        NvFlexGetVelocities(solver, buffers->velocities.buffer, &copy_desc);
        NvFlexGetPhases(solver, buffers->phases.buffer, &copy_desc);
    }
}

FlexParticlePhysicsServer::FlexParticlePhysicsServer() :
		ParticlePhysicsServer(),
        flex_lib(NULL),
        solver(NULL),
        buffers(NULL),
        active_particle_count(0) {

	ERR_FAIL_COND(singleton);
	singleton = this;
}

FlexParticlePhysicsServer::~FlexParticlePhysicsServer() {
}
