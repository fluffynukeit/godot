/*************************************************************************/
/*  flex_particle_body.h                                                 */
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

#ifndef FLEX_PARTICLE_BODY_H
#define FLEX_PARTICLE_BODY_H

#include "flex_particle_physics_server.h"
#include "flex_utility.h"
#include "rid_flex.h"
#include "string_db.h"

class FlexParticleBody;
class FlexSpace;
class Object;

struct ParticleToAdd {
    FlVector4 particle;
    bool want_reference;

    ParticleToAdd() {}

    ParticleToAdd(const Vector3 &p_position, real_t p_mass) {
        particle = CreateParticle(p_position, p_mass);
        want_reference = false;
    }
};

struct SpringToAdd {

    ParticleID particle_0;
    ParticleID particle_1;
    float length;
    float stiffness;

    SpringToAdd() :
            particle_0(-1),
            particle_1(-1),
            length(0),
            stiffness(0) {}

    SpringToAdd(ParticleID p_particle_0, ParticleID p_particle_1, float p_length, float p_stiffness) :
            particle_0(p_particle_0),
            particle_1(p_particle_1),
            length(p_length),
            stiffness(p_stiffness) {
    }
};

enum ChangedParameter {
    eChangedParameterGroup = 1 << 0
};

/// This class represent a group of particles that are constrained each other and form a body.
/// This body can be rigid or soft.
///
/// It's possible to add and remove particles, since the particles internally can change its buffer position and then its ID
/// if you need to track a specific particle over time you can create a reference, when you add it.
/// The reference is an ID that don't change during time, and with it is possible to get the current ID of particle and then interrogate it.
///
/// [COMMAND] All functions marked with this label are commands and will be executed in the next tick.
class FlexParticleBody : public RIDFlex {

    friend class FlexSpace;
    friend class FlexParticleBodyCommands;

    struct {
        Object *receiver; // Use pointer directly to speed up the process, but it's a bit risky
        StringName method;
    } sync_callback;

    struct {
        Vector<ParticleToAdd> particle_to_add;
        Vector<SpringToAdd> springs_to_add;
        Set<ParticleID> particle_to_remove;
        Set<SpringID> springs_to_remove;
    } delayed_commands;

    uint32_t changed_parameters;

    MemoryChunk *particles_mchunk;
    MemoryChunk *springs_mchunk;

    FlexSpace *space;
    uint32_t group;

public:
    FlexParticleBody();

    _FORCE_INLINE_ FlexSpace *get_space() { return space; }

    /// IMPORTANT Remember to remove it if Object will be destroyed
    void set_sync_callback(Object *p_receiver, const StringName &p_method);

    void reset_changed_parameters();
    _FORCE_INLINE_ uint32_t get_changed_parameters() const { return changed_parameters; }

    void set_collision_group(uint32_t p_layer);
    uint32_t get_collision_group() const;

    void add_particle(const Vector3 &p_local_position, real_t p_mass);
    void remove_particle(ParticleID p_particle);

    void add_spring(ParticleID p_particle_0, ParticleID p_particle_1, float p_length, float p_stiffness);
    void remove_spring(SpringID p_spring_id);

    int get_particle_count() const;
    int get_spring_count() const;

    // CMD
    void load_shape(Ref<ParticleShape> p_shape, const Transform &initial_transform);

    void reset_particle(ParticleID p_particle, const Vector3 &p_position, real_t p_mass);
    void reset_spring(SpringID p_spring, ParticleID p_particle_0, ParticleID p_particle_1, float p_length, float p_stiffness);

    Vector3 get_particle_position(ParticleID p_particle) const;

    const Vector3 &get_particle_velocity(ParticleID p_particle) const;
    void set_particle_velocity(ParticleID p_particle, const Vector3 &p_velocity);
    // ~CMD

    bool is_owner_of_particle(ParticleID p_particle) const;
    bool is_owner_of_spring(SpringID p_spring) const;

    // This should be on physics server
    void create_soft_body();

private:
    void dispatch_sync_callback();
    void clear_commands();
};

#endif // FLEX_PARTICLE_BODY_H
