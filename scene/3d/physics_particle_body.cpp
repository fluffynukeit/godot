/*************************************************************************/
/*  physics_particle_body.cpp                                            */
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
 * @author AndreaCatania
 */

#include "physics_particle_body.h"
#include "core_string_names.h"

void ParticleObject::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_rid"), &ParticleBody::get_rid);
}

ParticleObject::ParticleObject(RID p_rid) :
        rid(p_rid) {
}

void ParticleBody::_bind_methods() {

    ClassDB::bind_method(D_METHOD("set_particle_shape", "particle_shape"), &ParticleBody::set_particle_shape);
    ClassDB::bind_method(D_METHOD("get_particle_shape"), &ParticleBody::get_particle_shape);

    ClassDB::bind_method(D_METHOD("add_particle", "local_position", "mass"), &ParticleBody::add_particle);
    ClassDB::bind_method(D_METHOD("remove_particle", "particle_id"), &ParticleBody::remove_particle);

    ClassDB::bind_method(D_METHOD("_on_script_changed"), &ParticleBody::_on_script_changed);
    ClassDB::bind_method(D_METHOD("resource_changed", "resource"), &ParticleBody::resource_changed);

    ClassDB::bind_method(D_METHOD("commands_process_internal", "commands"), &ParticleBody::commands_process_internal);

    BIND_VMETHOD(MethodInfo("_commands_process", PropertyInfo(Variant::OBJECT, "commands", PROPERTY_HINT_RESOURCE_TYPE, "ParticleBodyCommands")));

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "particle_shape", PROPERTY_HINT_RESOURCE_TYPE, "ParticleShape"), "set_particle_shape", "get_particle_shape");

    ADD_SIGNAL(MethodInfo("resource_loaded"));
}

ParticleBody::ParticleBody() :
        ParticleObject(ParticlePhysicsServer::get_singleton()->body_create()),
        reset_particles_to_base_shape(true) {

    debug_particle_mesh.instance();
    debug_particle_mesh->set_radius(0.1);
    debug_particle_mesh->set_height(0.2);

    connect(CoreStringNames::get_singleton()->script_changed, this, "_on_script_changed");

    set_notify_transform(true);
}

void ParticleBody::set_particle_shape(Ref<ParticleShape> p_shape) {
    if (particle_shape == p_shape)
        return;

    if (particle_shape.is_valid())
        particle_shape->unregister_owner(this);

    particle_shape = p_shape;

    if (particle_shape.is_valid())
        particle_shape->register_owner(this);

    resource_changed(particle_shape);
}

Ref<ParticleShape> ParticleBody::get_particle_shape() const {
    return particle_shape;
}

void ParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {
    ParticlePhysicsServer::get_singleton()->body_add_particle(rid, p_local_position, p_mass);
}

void ParticleBody::remove_particle(int p_particle_index) {
    ParticlePhysicsServer::get_singleton()->body_remove_particle(rid, p_particle_index);
}

void ParticleBody::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            ParticlePhysicsServer::get_singleton()->body_set_space(rid, get_world()->get_particle_space());
            resource_changed(particle_shape);
        } break;
        case NOTIFICATION_TRANSFORM_CHANGED: {

            reset_debug_particle_positions();
        } break;
        case NOTIFICATION_EXIT_WORLD: {
            ParticlePhysicsServer::get_singleton()->body_set_space(rid, RID());
            initialize_debug_resource();
        } break;
    }
}

void ParticleBody::_on_script_changed() {
    ParticlePhysicsServer::get_singleton()->body_set_sync_callback(rid, this, "commands_process_internal");
}

void ParticleBody::resource_changed(const RES &p_res) {
    Ref<ParticleShape> particle_shape = p_res;
    if (particle_shape.is_valid()) {
        reset_particles_to_base_shape = true;
        initialize_debug_resource();
    }
}

void ParticleBody::commands_process_internal(Object *p_cmds) {
    ParticleBodyCommands *cmds(static_cast<ParticleBodyCommands *>(p_cmds));
    reset_particles(cmds);
    update_debug_visual_instances(cmds);

    if (!get_script().is_null() && has_method("_commands_process")) {
        call("_commands_process", p_cmds);
    }
}

void ParticleBody::reset_particles(ParticleBodyCommands *p_cmds) {

    if (!reset_particles_to_base_shape)
        return;

    reset_particles_to_base_shape = false;

    int active_p_count(ParticlePhysicsServer::get_singleton()->body_get_particle_count(rid));
    const int resource_p_count(particle_shape->get_particles().size());

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
            add_particle(get_global_transform().xform(particle_shape->get_particles()[p].relative_position), particle_shape->get_particles()[p].mass);
        }
    }

    for (int i(0); i < active_p_count; ++i) {
        p_cmds->reset_particle(i, get_global_transform().xform(particle_shape->get_particles()[i].relative_position), particle_shape->get_particles()[i].mass);
    }

    emit_signal("resource_loaded");
}

void ParticleBody::initialize_debug_resource() {

    const int particle_count = particle_shape.is_valid() ? particle_shape->get_particles().size() : 0;
    resize_debug_particle_visual_instance(particle_count);
    reset_debug_particle_positions();
}

void ParticleBody::update_debug_visual_instances(ParticleBodyCommands *p_cmds) {

    const int particle_count = ParticlePhysicsServer::get_singleton()->body_get_particle_count(rid);
    resize_debug_particle_visual_instance(ParticlePhysicsServer::get_singleton()->body_get_particle_count(rid));

    Transform transf;
    for (int i = 0; i < particle_count; ++i) {

        transf.origin = p_cmds->get_particle_position(i);
        VisualServer::get_singleton()->instance_set_transform(debug_particle_visual_instances[i], transf);
    }
}

void ParticleBody::resize_debug_particle_visual_instance(int new_size) {

    if (debug_particle_visual_instances.size() == new_size)
        return;

    if (debug_particle_visual_instances.size() > new_size) {

        // If the particle count is less then visual instances size, free the last
        const int dif = debug_particle_visual_instances.size() - new_size;
        for (int i = 0; i < dif; ++i) {

            const int p = debug_particle_visual_instances.size() - i - 1;
            VisualServer::get_singleton()->instance_set_scenario(debug_particle_visual_instances[p], RID());
            VS::get_singleton()->free(debug_particle_visual_instances[p]);
        }
        debug_particle_visual_instances.resize(new_size);
    } else {

        if (!is_inside_world())
            return;

        // If the particle count is more then visual instances resize and create last
        const int dif = new_size - debug_particle_visual_instances.size();
        debug_particle_visual_instances.resize(new_size);
        for (int i = 0; i < dif; ++i) {

            const int p = new_size - i - 1;
            debug_particle_visual_instances[p] = VisualServer::get_singleton()->instance_create();
            VisualServer::get_singleton()->instance_set_scenario(debug_particle_visual_instances[p], get_world()->get_scenario());
            VisualServer::get_singleton()->instance_set_base(debug_particle_visual_instances[p], debug_particle_mesh->get_rid());
            VisualServer::get_singleton()->instance_set_visible(debug_particle_visual_instances[p], true);
            VisualServer::get_singleton()->instance_set_layer_mask(debug_particle_visual_instances[p], 1);
        }
    }
}

void ParticleBody::reset_debug_particle_positions() {

    if (debug_particle_visual_instances.size() == particle_shape->get_particles().size()) {

        Transform particle_relative_transf;
        for (int i = 0; i < debug_particle_visual_instances.size(); ++i) {

            particle_relative_transf.origin = particle_shape->get_particles()[i].relative_position;
            VisualServer::get_singleton()->instance_set_transform(debug_particle_visual_instances[i], get_global_transform() * particle_relative_transf);
        }
    }
}
