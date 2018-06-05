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
        particle_bodies_memory(NULL),
        spring_allocator(NULL),
        spring_memory(NULL) {
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

    ERR_FAIL_COND(spring_allocator);
    ERR_FAIL_COND(spring_memory);
    spring_memory = memnew(SpringMemory(flex_lib));
    spring_allocator = memnew(FlexMemoryAllocator(spring_memory, ((FlexUnit)(MAXPARTICLES * 2))));
    spring_memory->unmap(); // This is mandatory because the FlexMemoryAllocator when resize the memory will leave the buffers mapped

    NvFlexParams params;
    // Initialize solver parameter
    NvFlexGetParams(solver, &params);
    params.gravity[0] = 0.0;
    params.gravity[1] = -10.0;
    params.gravity[2] = 0.0;
    params.radius = 0.1;
    params.solidRestDistance = params.radius * 0.9;
    params.fluidRestDistance = params.radius * 0.5;
    params.numIterations = 3;
    params.maxSpeed = FLT_MAX;
    params.maxAcceleration = Vector3(params.gravity[0], params.gravity[1], params.gravity[2]).length() * 10.0;
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
    spring_memory->map();

    dispatch_callbacks();
    execute_delayed_commands();

    particle_bodies_memory->unmap();

    if (spring_memory->was_changed())
        spring_allocator->sanitize(); // This buffer should be compact when the GPU has to use it
    spring_memory->unmap();

    commands_write_buffer();
}

void FlexSpace::step(real_t p_delta_time) {

    // Step solver (command)
    const int substep = 1;
    const bool enable_timer = false; // Used for profiling
    NvFlexUpdateSolver(solver, p_delta_time, substep, enable_timer);

    commands_read_buffer();
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
            if (body->particles_mchunk) {
                // Resize memory chunk
                particle_bodies_allocator->resize_chunk(body->particles_mchunk, body->particles_mchunk->get_size() + body->delayed_commands.particle_to_add.size());
            } else {
                // Allocate new one
                body->particles_mchunk = particle_bodies_allocator->allocate_chunk(body->delayed_commands.particle_to_add.size());
            }

            // Write on memory
            ERR_FAIL_COND(!body->particles_mchunk);
            for (int p(body->delayed_commands.particle_to_add.size() - 1); 0 <= p; --p) {

                particle_bodies_memory->set_particle(body->particles_mchunk, p, body->delayed_commands.particle_to_add[p].particle);
                particle_bodies_memory->set_velocity(body->particles_mchunk, p, Vector3());
                // TODO add here all parameter correctly
                const int group = 0;
                const int phase = NvFlexMakePhase(group, eNvFlexPhaseSelfCollide);
                particle_bodies_memory->set_phase(body->particles_mchunk, p, phase);
                particle_bodies_memory->set_active_particle(body->particles_mchunk, p);
            }
        }

        if (body->delayed_commands.particle_to_remove.size()) {

            // Remove particles
            if (!body->particles_mchunk)
                continue;

            ParticleID last_particle_index(body->particles_mchunk->get_end_index());
            for (Set<ParticleID>::Element *e = body->delayed_commands.particle_to_remove.front(); e; e = e->next()) {

                // Copy the values of last ID to the ID to remove (lose order)
                const int id_to_remove(body->particles_mchunk->get_begin_index() + e->get());
                particle_bodies_memory->copy(body->particles_mchunk->get_begin_index() + last_particle_index, 1, id_to_remove);
                --last_particle_index;

                if (!body->springs_mchunk)
                    continue;

                // Removes all springs of removed particle
                SpringID last_spring_index(body->springs_mchunk->get_end_index());
                for (int spring_id(body->springs_mchunk->get_size() - 1); 0 <= spring_id; --spring_id) {
                    const Spring &spring = spring_memory->get_spring(body->springs_mchunk, spring_id);
                    if (spring.id0 == id_to_remove || spring.id1 == id_to_remove) {
                        spring_memory->copy(last_spring_index, 1, body->springs_mchunk->get_begin_index() + spring_id);
                        --last_spring_index;
                    }
                }
                spring_allocator->resize_chunk(body->springs_mchunk, last_spring_index + 1);
            }
            FlexUnit new_size = body->particles_mchunk->get_size() - body->delayed_commands.particle_to_remove.size();
            particle_bodies_allocator->resize_chunk(body->particles_mchunk, new_size);
        }
    }
}

void FlexSpace::commands_write_buffer() {
    NvFlexCopyDesc copy_desc;
    for (int i(particle_bodies.size() - 1); 0 <= i; --i) {

        FlexParticleBody *body = particle_bodies[i];
        if (!body->particles_mchunk)
            continue;

        copy_desc.srcOffset = body->particles_mchunk->get_begin_index();
        copy_desc.dstOffset = body->particles_mchunk->get_begin_index();
        copy_desc.elementCount = body->particles_mchunk->get_size();

        // TODO write only necessary (part of buffer or just skip an entire buffer if not necessary)
        NvFlexSetParticles(solver, particle_bodies_memory->particles.buffer, &copy_desc);
        NvFlexSetVelocities(solver, particle_bodies_memory->velocities.buffer, &copy_desc);
        NvFlexSetPhases(solver, particle_bodies_memory->phases.buffer, &copy_desc);
        NvFlexSetActive(solver, particle_bodies_memory->active_particles.buffer, &copy_desc);
        NvFlexSetActiveCount(solver, particle_bodies_memory->active_particles.size());
    }

    if (spring_memory->was_changed())
        NvFlexSetSprings(solver, spring_memory->springs.buffer, spring_memory->lengths.buffer, spring_memory->stiffness.buffer, spring_allocator->get_last_used_index() + 1);
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
