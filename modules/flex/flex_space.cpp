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
#include "print_string.h"

#define MAXPARTICLES 1000

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
        particle_bodies_memory(NULL) {
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
    particle_bodies_allocator = memnew(FlexMemoryAllocator(particle_bodies_memory, ((FlexUnit)(MAXPARTICLES / 3))));
    particle_bodies_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

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

    dispatch_callbacks();
    execute_delayed_commands();

    particle_bodies_memory->unmap();

    commands_write_to_gpu();
}

void FlexSpace::step(real_t p_delta_time) {

    // Step solver (command)
    const int substep = 1;
    const bool enable_timer = false; // Used for profiling
    NvFlexUpdateSolver(solver, p_delta_time, substep, enable_timer);

    commands_read_from_gpu();
}

void FlexSpace::dispatch_callbacks() {
    for (int i(particle_bodies.size() - 1); 0 <= i; --i) {
        particle_bodies[i]->dispatch_sync_callback();
    }
}

void FlexSpace::execute_delayed_commands() {
    for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

        FlexParticleBody *body = particle_bodies[i];
        if (body->delayed_commands.particle_to_add.size()) {

            // Allocate memory
            if (body->memory_chunk) {
                // Resize memory chunk
                particle_bodies_allocator->resize_chunk(body->memory_chunk, body->memory_chunk->get_size() + body->delayed_commands.particle_to_add.size());
            } else {
                // Allocate new one
                body->memory_chunk = particle_bodies_allocator->allocate_chunk(body->delayed_commands.particle_to_add.size());
            }

            // Write on memory
            ERR_FAIL_COND(!body->memory_chunk);
            for (int p(body->delayed_commands.particle_to_add.size() - 1); 0 <= p; --p) {

                particle_bodies_memory->set_particle(body->memory_chunk, p, body->delayed_commands.particle_to_add[p].particle);
                particle_bodies_memory->set_velocity(body->memory_chunk, p, FlVector3());
                // TODO add here all parameter correctly
                const int group = 0;
                const int phase = NvFlexMakePhase(group, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter);
                particle_bodies_memory->set_phase(body->memory_chunk, p, phase);
                particle_bodies_memory->set_active_particle(body->memory_chunk, p);
            }
        }

        if (body->delayed_commands.particle_to_remove.size()) {

            // Remove particles
            if (!body->memory_chunk)
                continue;

            ParticleID last_id(body->memory_chunk->get_end_index());
            for (Set<ParticleID>::Element *e = body->delayed_commands.particle_to_remove.front(); e; e = e->next()) {
                particle_bodies_memory->copy(body->memory_chunk, last_id, /*id_to_remove*/ e->get());
                --last_id;
            }
            FlexUnit new_size = body->memory_chunk->get_size() - body->delayed_commands.particle_to_remove.size();
            particle_bodies_allocator->resize_chunk(body->memory_chunk, new_size);
        }
    }
}

void FlexSpace::commands_write_to_gpu() {
    NvFlexCopyDesc copy_desc;
    for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

        FlexParticleBody *body = particle_bodies[i];
        if (!body->memory_chunk)
            continue;

        copy_desc.srcOffset = body->memory_chunk->get_begin_index();
        copy_desc.dstOffset = body->memory_chunk->get_begin_index();
        copy_desc.elementCount = body->memory_chunk->get_size();

        // TODO write only necessary (part of buffer or just skip an entire buffer if not necessary)
        NvFlexSetParticles(solver, particle_bodies_memory->particles.buffer, &copy_desc);
        NvFlexSetVelocities(solver, particle_bodies_memory->velocities.buffer, &copy_desc);
        NvFlexSetPhases(solver, particle_bodies_memory->phases.buffer, &copy_desc);
        NvFlexSetActive(solver, particle_bodies_memory->active_particles.buffer, &copy_desc);
        NvFlexSetActiveCount(solver, particle_bodies_memory->active_particles.size());
    }
}

void FlexSpace::commands_read_from_gpu() {
    NvFlexCopyDesc copy_desc;
    for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

        if (!particle_bodies[i]->memory_chunk)
            continue;

        // Write back to buffer (command)
        copy_desc.srcOffset = particle_bodies[i]->memory_chunk->get_begin_index();
        copy_desc.dstOffset = particle_bodies[i]->memory_chunk->get_begin_index();
        copy_desc.elementCount = particle_bodies[i]->memory_chunk->get_size();

        // TODO read only necessary (part of buffer or just skip an entire buffer if not necessary)
        NvFlexGetParticles(solver, particle_bodies_memory->particles.buffer, &copy_desc);
        NvFlexGetVelocities(solver, particle_bodies_memory->velocities.buffer, &copy_desc);
        NvFlexGetPhases(solver, particle_bodies_memory->phases.buffer, &copy_desc);

        particle_bodies[i]->clear_commands();
    }
}

void FlexSpace::add_particle_body(FlexParticleBody *p_body) {
    ERR_FAIL_COND(p_body->space);
    p_body->space = this;
    particle_bodies.push_back(p_body);
}

void FlexSpace::remove_particle_body(FlexParticleBody *p_body) {
    // TODO implement
    print_error("remove_particle_body NOT YET IMPLEMENTED! ");
    //p_body->space = NULL;
    //particle_bodies.erase(p_body);
}
