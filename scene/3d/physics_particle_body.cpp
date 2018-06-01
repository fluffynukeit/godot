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

void ParticleObject::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_rid"), &ParticleBody::get_rid);
}

ParticleObject::ParticleObject(RID p_rid) :
        rid(p_rid) {
}

void ParticleBody::_bind_methods() {
    ClassDB::bind_method(D_METHOD("add_particle", "local_position", "mass"), &ParticleBody::add_particle);
}

ParticleBody::ParticleBody() :
        ParticleObject(ParticlePhysicsServer::get_singleton()->body_create()) {
}

void ParticleBody::add_particle(const Vector3 &p_local_position, real_t p_mass) {
    ParticlePhysicsServer::get_singleton()->body_add_particle(rid, p_local_position, p_mass);
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
