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

#include "print_string.h"

#define MAXPARTICLES 10

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

ParticleBodiesMemory::ParticleBodiesMemory(NvFlexLibrary *p_flex_lib) :
        particles(p_flex_lib),
        velocities(p_flex_lib),
        phases(p_flex_lib),
        active_particles(p_flex_lib) {
}

void ParticleBodiesMemory::reserve(int p_size) {
    particles.resize(p_size);
    velocities.resize(p_size);
    phases.resize(p_size);
    active_particles.resize(p_size);
}

void ParticleBodiesMemory::shift_back(int p_from, int p_to, int p_shift) {
    for (int i(p_from); i <= p_to; ++i) {
        particles[i - p_shift] = particles[i];
        velocities[i - p_shift] = velocities[i];
        phases[i - p_shift] = phases[i];
    }
}

void ParticleBodiesMemory::map() {
    particles.map(eNvFlexMapWait);
    velocities.map(eNvFlexMapWait);
    phases.map(eNvFlexMapWait);
    active_particles.map(eNvFlexMapWait);
}

void ParticleBodiesMemory::unmap() {
    particles.unmap();
    velocities.unmap();
    phases.unmap();
    active_particles.unmap();
}

void ParticleBodiesMemory::terminate() {
    particles.destroy();
    velocities.destroy();
    phases.destroy();
    active_particles.destroy();
}

void ParticleBodiesMemory::set_particle(const Stack *p_stack, int p_particle_index, FlVector4 p_particle) {
    particles[p_stack->get_begin_index() + p_particle_index] = p_particle;
}

void ParticleBodiesMemory::set_velocity(const Stack *p_stack, int p_particle_index, FlVector3 p_velocity) {
    velocities[p_stack->get_begin_index() + p_particle_index] = p_velocity;
}

void ParticleBodiesMemory::set_phase(const Stack *p_stack, int p_particle_index, int p_phase) {
    phases[p_stack->get_begin_index() + p_particle_index] = p_phase;
}

void ParticleBodiesMemory::set_active_particles(const Stack *p_stack, int p_particle_index, int p_index) {
    active_particles[p_stack->get_begin_index() + p_particle_index] = p_index;
}

FlexSpace::FlexSpace() :
        RIDFlex(),
        flex_lib(NULL),
        solver(NULL),
        particle_bodies_allocator(NULL),
        particle_bodies_memory(NULL),
        active_particle_count(0) {
    init();
}

FlexSpace::~FlexSpace() {
    terminate();
}

void FlexSpace::init() {

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
    ERR_FAIL_COND(particle_bodies_memory);
    ERR_FAIL_COND(particle_bodies_allocator);
    particle_bodies_memory = memnew(ParticleBodiesMemory(flex_lib));
    particle_bodies_allocator = memnew(FlexMemoryAllocator(particle_bodies_memory, 0));

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

void FlexSpace::terminate() {

    ERR_FAIL_COND(!particle_bodies_memory);
    particle_bodies_memory->terminate();
    memdelete(particle_bodies_memory);
    particle_bodies_memory = NULL;

    ERR_FAIL_COND(!particle_bodies_allocator);
    memdelete(particle_bodies_allocator);
    particle_bodies_allocator = NULL;

    ERR_FAIL_COND(!solver);
    NvFlexDestroySolver(solver);
    solver = NULL;

    ERR_FAIL_COND(!flex_lib);
    NvFlexShutdown(flex_lib);
    flex_lib = NULL;
}

void FlexSpace::sync() {

    particle_bodies_memory->map();

    // Read operation
    if (active_particle_count > 0) {

        print_line("X: " + String::num_real(particle_bodies_memory->particles[0].x) + " Y: " + String::num_real(particle_bodies_memory->particles[0].y) + " Z: " + String::num_real(particle_bodies_memory->particles[0].z));
    }

    bool require_write = false;
    // Write operation
    if (!particle_bodies_memory->particles.size()) { // TODO just a test

        // TODO remove, just a test
        particle_bodies_allocator->reserve(1); // Set max memory to 1
        test_stack = particle_bodies_allocator->allocate(1); // allocate for 1 buffer

        require_write = true;

        real_t mass = 2.0;
        particle_bodies_memory->set_particle(test_stack, 0, FlVector4(0.0, 0.0, 0.0, 1.0 / mass));
        particle_bodies_memory->set_velocity(test_stack, 0, FlVector3(0.0, 0.0, 0.0));
        const int group = 0;
        const int phase = NvFlexMakePhase(group, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter);
        particle_bodies_memory->set_phase(test_stack, 0, phase);
        particle_bodies_memory->set_active_particles(test_stack, 0, 0);
    }

    particle_bodies_memory->unmap();

    if (require_write) {

        NvFlexCopyDesc copy_desc;
        copy_desc.srcOffset = 0;
        copy_desc.dstOffset = 0;
        copy_desc.elementCount = particle_bodies_memory->particles.size();

        // Write buffer to GPU (command)
        NvFlexSetParticles(solver, particle_bodies_memory->particles.buffer, &copy_desc);
        NvFlexSetVelocities(solver, particle_bodies_memory->velocities.buffer, &copy_desc);
        NvFlexSetPhases(solver, particle_bodies_memory->phases.buffer, &copy_desc);
        NvFlexSetActive(solver, particle_bodies_memory->active_particles.buffer, &copy_desc);
        NvFlexSetActiveCount(solver, particle_bodies_memory->active_particles.size());
    }
}

void FlexSpace::step(real_t p_delta_time) {

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
        copy_desc.elementCount = active_particle_count;

        NvFlexGetParticles(solver, particle_bodies_memory->particles.buffer, &copy_desc);
        NvFlexGetVelocities(solver, particle_bodies_memory->velocities.buffer, &copy_desc);
        NvFlexGetPhases(solver, particle_bodies_memory->phases.buffer, &copy_desc);
    }
}
