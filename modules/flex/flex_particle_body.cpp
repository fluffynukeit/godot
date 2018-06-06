/*************************************************************************/
/*  flex_particle_body.cpp                                               */
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

#include "flex_particle_body.h"
#include "flex_memory.h"
#include "flex_space.h"
#include "object.h"

FlexParticleBody::FlexParticleBody() :
        RIDFlex(),
        space(NULL),
        particles_mchunk(NULL),
        springs_mchunk(NULL),
        changed_parameters(0),
        group(1) {
    sync_callback.receiver = NULL;
}

void FlexParticleBody::set_sync_callback(Object *p_receiver, const StringName &p_method) {
    if (p_receiver) {
        ERR_FAIL_COND(!p_receiver->has_method(p_method));
    }
    sync_callback.receiver = p_receiver;
    sync_callback.method = p_method;
}

void FlexParticleBody::reset_changed_parameters() {
    changed_parameters = 0;
}

void FlexParticleBody::dispatch_sync_callback() {
    if (!sync_callback.receiver)
        return;
    static Variant::CallError error;
    const Variant *p = FlexParticlePhysicsServer::singleton->get_particle_body_commands_variant(this);
    sync_callback.receiver->call(sync_callback.method, &p, 1, error);
}

void FlexParticleBody::set_collision_group(uint32_t p_group) {
    group = eNvFlexPhaseGroupMask & p_group;
    changed_parameters |= eChangedParameterGroup;
}

uint32_t FlexParticleBody::get_collision_group() const {
    return group;
}

void FlexParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {

    delayed_commands.particle_to_add.push_back(ParticleToAdd(p_local_position, p_mass));
}

void FlexParticleBody::remove_particle(ParticleID p_particle) {
    ERR_FAIL_COND(!is_owner_of_particle(p_particle));
    delayed_commands.particle_to_remove.insert(p_particle);
}

void FlexParticleBody::add_spring(ParticleID p_particle_0, ParticleID p_particle_1, float p_length, float p_stiffness) {
    delayed_commands.springs_to_add.push_back(SpringToAdd(p_particle_0, p_particle_1, p_length, p_stiffness));
}

void FlexParticleBody::remove_spring(SpringID p_spring_id) {
    ERR_FAIL_COND(!is_owner_of_spring(p_spring_id));
    delayed_commands.springs_to_remove.insert(p_spring_id);
}

int FlexParticleBody::get_particle_count() const {
    return particles_mchunk ? particles_mchunk->get_size() : 0;
}

int FlexParticleBody::get_spring_count() const {
    return springs_mchunk ? springs_mchunk->get_size() : 0;
}

void FlexParticleBody::load_shape(Ref<ParticleShape> p_shape, const Transform &initial_transform) {

    { // Particle
        int active_p_count(get_particle_count());
        const int resource_p_count(p_shape->get_particles().size());

        if (active_p_count > resource_p_count) {

            // Remove last
            const int dif = active_p_count - resource_p_count;
            for (int i(0); i < dif; ++i) {
                remove_particle(active_p_count - i - 1);
            }

            active_p_count = resource_p_count;

        } else {

            // Add
            const int dif = resource_p_count - active_p_count;
            for (int i(0); i < dif; ++i) {
                const int p(resource_p_count - i - 1);
                add_particle(initial_transform.xform(p_shape->get_particles()[p].relative_position), p_shape->get_particles()[p].mass);
            }
        }

        for (int i(0); i < active_p_count; ++i) {
            reset_particle(i, initial_transform.xform(p_shape->get_particles()[i].relative_position), p_shape->get_particles()[i].mass);
        }
    }

    { // Spring
        int active_s_count(get_spring_count());
        int resource_s_count(p_shape->get_constraints().size());

        if (active_s_count > resource_s_count) {

            // Remove last
            const int dif = active_s_count - resource_s_count;
            for (int i(0); i < dif; ++i) {
                remove_spring(active_s_count - i - 1);
            }

            active_s_count = resource_s_count;

        } else {

            // Add
            const int dif = resource_s_count - active_s_count;
            for (int i(0); i < dif; ++i) {
                const int s(resource_s_count - i - 1);
                add_spring(p_shape->get_constraints()[s].particle_id_0, p_shape->get_constraints()[s].particle_id_1, p_shape->get_constraints()[s].length, p_shape->get_constraints()[s].stiffness);
            }
        }

        for (int i(0); i < active_s_count; ++i) {
            reset_spring(i, p_shape->get_constraints()[i].particle_id_0, p_shape->get_constraints()[i].particle_id_0, p_shape->get_constraints()[i].length, p_shape->get_constraints()[i].stiffness);
        }
    }
}

void FlexParticleBody::reset_particle(ParticleID p_particle_index, const Vector3 &p_position, real_t p_mass) {
    if (!particles_mchunk)
        return;
    space->get_particle_bodies_memory()->set_particle(particles_mchunk, p_particle_index, CreateParticle(p_position, p_mass));
    space->get_particle_bodies_memory()->set_velocity(particles_mchunk, p_particle_index, Vector3(0, 0, 0));
}

void FlexParticleBody::reset_spring(SpringID p_spring, ParticleID p_particle_0, ParticleID p_particle_1, float p_length, float p_stiffness) {
    if (!springs_mchunk)
        return;
    space->get_springs_memory()->set_spring(springs_mchunk, p_spring, Spring(springs_mchunk->get_begin_index() + p_particle_0, springs_mchunk->get_begin_index() + p_particle_1));
    space->get_springs_memory()->set_length(springs_mchunk, p_spring, p_length);
    space->get_springs_memory()->set_stiffness(springs_mchunk, p_spring, p_stiffness);
}

Vector3 FlexParticleBody::get_particle_position(ParticleID p_particle_index) const {
    if (!particles_mchunk)
        return return_err_vec3;
    const FlVector4 &p(space->get_particle_bodies_memory()->get_particle(particles_mchunk, p_particle_index));
    return gvec3_from_fvec4(p);
}

const Vector3 &FlexParticleBody::get_particle_velocity(ParticleID p_particle_index) const {
    if (!particles_mchunk)
        return return_err_vec3;
    return space->get_particle_bodies_memory()->get_velocity(particles_mchunk, p_particle_index);
}

void FlexParticleBody::set_particle_velocity(ParticleID p_particle_index, const Vector3 &p_velocity) {
    if (!particles_mchunk)
        return;
    return space->get_particle_bodies_memory()->set_velocity(particles_mchunk, p_particle_index, p_velocity);
}

bool FlexParticleBody::is_owner_of_particle(ParticleID p_particle) const {
    if (!particles_mchunk)
        return false;
    return (particles_mchunk && (particles_mchunk->get_begin_index() + p_particle) <= particles_mchunk->get_end_index());
}

bool FlexParticleBody::is_owner_of_spring(SpringID p_spring) const {
    if (!springs_mchunk)
        return false;
    return (springs_mchunk && (springs_mchunk->get_begin_index() + p_spring) <= springs_mchunk->get_end_index());
}

void FlexParticleBody::create_soft_body() {
    //NvFlexExtAsset *asset = NvFlexExtCreateSoftFromMesh();
    //asset->2
}

void FlexParticleBody::clear_commands() {
    delayed_commands.particle_to_add.clear();
    delayed_commands.springs_to_add.clear();
    delayed_commands.particle_to_remove.clear();
    delayed_commands.springs_to_remove.clear();
}
