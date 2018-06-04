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

    ClassDB::bind_method(D_METHOD("_commands_process_internal", "commands"), &ParticleBody::_commands_process_internal);

    BIND_VMETHOD(MethodInfo("_commands_process", PropertyInfo(Variant::OBJECT, "commands", PROPERTY_HINT_RESOURCE_TYPE, "ParticleBodyCommands")));

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "particle_shape", PROPERTY_HINT_RESOURCE_TYPE, "ParticleShape"), "set_particle_shape", "get_particle_shape");
}

ParticleBody::ParticleBody() :
        ParticleObject(ParticlePhysicsServer::get_singleton()->body_create()) {

    sphere_mesh.instance();
    sphere_mesh->set_radius(0.1);
    sphere_mesh->set_height(0.2);

    connect(CoreStringNames::get_singleton()->script_changed, this, "_on_script_changed");
}

void ParticleBody::set_particle_shape(Ref<ParticleShape> p_shape) {
    if (particle_shape == p_shape)
        return;

    if (particle_shape.is_valid())
        particle_shape->unregister_owner(this);

    particle_shape = p_shape;

    if (particle_shape.is_valid())
        particle_shape->register_owner(this);

    parse_resource();
}

Ref<ParticleShape> ParticleBody::get_particle_shape() const {
    return particle_shape;
}

void ParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {
    ParticlePhysicsServer::get_singleton()->body_add_particle(rid, p_local_position, p_mass);
}

void ParticleBody::remove_particle(int p_particle_id) {
    ParticlePhysicsServer::get_singleton()->body_remove_particle(rid, p_particle_id);
}

void ParticleBody::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            RID space = get_world()->get_particle_space();
            ParticlePhysicsServer::get_singleton()->body_set_space(rid, space);
            parse_resource();
        } break;
        case NOTIFICATION_TRANSFORM_CHANGED: {

        } break;
        case NOTIFICATION_EXIT_WORLD: {
            ParticlePhysicsServer::get_singleton()->body_set_space(rid, RID());
            parse_resource();
        } break;
    }
}

void ParticleBody::_on_script_changed() {
    if (get_script().is_null() || !has_method("_commands_process")) {
        ParticlePhysicsServer::get_singleton()->body_set_sync_callback(rid, NULL, "");
    } else {
        ParticlePhysicsServer::get_singleton()->body_set_sync_callback(rid, this, "_commands_process_internal");
    }
}

void ParticleBody::resource_changed(const RES &p_res) {
    parse_resource();
}

void ParticleBody::parse_resource() {

    const int particle_count = particle_shape.is_valid() ? particle_shape->get_particles().size() : 0;

    if (visual_instances.size() != particle_count) {

        if (visual_instances.size() > particle_count) {

            // If the particle count is less then visual instances size, free the last
            const int dif = visual_instances.size() - particle_count;
            for (int i = 0; i < dif; ++i) {

                const int p = particle_count - i - 1;
                VisualServer::get_singleton()->instance_set_scenario(visual_instances[p], RID());
                VS::get_singleton()->free(visual_instances[p]);
                remove_particle(p);
            }
            visual_instances.resize(particle_count);

        } else {

            if (get_world().is_null())
                return;

            // If the particle count is more then visual instances resize and create last
            const int dif = particle_count - visual_instances.size();
            visual_instances.resize(particle_count);
            for (int i = 0; i < dif; ++i) {

                const int p = particle_count - i - 1;
                visual_instances[p] = VisualServer::get_singleton()->instance_create();
                VisualServer::get_singleton()->instance_set_scenario(visual_instances[p], get_world()->get_scenario());
                VisualServer::get_singleton()->instance_set_base(visual_instances[p], sphere_mesh->get_rid());
                VisualServer::get_singleton()->instance_set_visible(visual_instances[p], true);
                VisualServer::get_singleton()->instance_set_layer_mask(visual_instances[p], 1);

                add_particle(particle_shape->get_particles()[p].relative_position, particle_shape->get_particles()[p].mass);
            }
        }
    }

    // for each particle set position to base
    Transform particle_relative_transf;
    for (int i = 0; i < particle_count; ++i) {

        particle_relative_transf.origin = particle_shape->get_particles()[i].relative_position;
        VisualServer::get_singleton()->instance_set_transform(visual_instances[i], get_global_transform() * particle_relative_transf);
    }
}

void ParticleBody::_commands_process_internal(Object *p_cmds) {

    ParticleBodyCommands *cmds(static_cast<ParticleBodyCommands *>(p_cmds));

    _process_visual_instances(cmds);
    call("_commands_process", p_cmds);
}

void ParticleBody::_process_visual_instances(ParticleBodyCommands *p_cmds) {

    const int particle_count = ParticlePhysicsServer::get_singleton()->body_get_particle_count(rid);
    ERR_FAIL_COND(visual_instances.size() != particle_count);

    Transform transf;
    for (int i = 0; i < particle_count; ++i) {

        transf.origin = p_cmds->get_particle_position(i);
        VisualServer::get_singleton()->instance_set_transform(visual_instances[i], transf);
    }
}
