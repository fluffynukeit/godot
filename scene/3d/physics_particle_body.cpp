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

    ClassDB::bind_method(D_METHOD("add_particle", "local_position", "mass"), &ParticleBody::add_particle);
    ClassDB::bind_method(D_METHOD("remove_particle", "particle_id"), &ParticleBody::remove_particle);

    ClassDB::bind_method(D_METHOD("_on_script_changed"), &ParticleBody::_on_script_changed);
    ClassDB::bind_method(D_METHOD("_commands_process_internal", "commands"), &ParticleBody::_commands_process_internal);

    BIND_VMETHOD(MethodInfo("_commands_process", PropertyInfo(Variant::OBJECT, "commands", PROPERTY_HINT_RESOURCE_TYPE, "ParticleBodyCommands")));
}

ParticleBody::ParticleBody() :
        ParticleObject(ParticlePhysicsServer::get_singleton()->body_create()) {
    connect(CoreStringNames::get_singleton()->script_changed, this, "_on_script_changed");
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
        } break;
        case NOTIFICATION_TRANSFORM_CHANGED: {

        } break;
        case NOTIFICATION_EXIT_WORLD: {
            ParticlePhysicsServer::get_singleton()->body_set_space(rid, RID());
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

void ParticleBody::_commands_process_internal(Object *p_cmds) {
    call("_commands_process", p_cmds);
}
