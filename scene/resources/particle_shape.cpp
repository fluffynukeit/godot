/*************************************************************************/
/*  shape.h                                                              */
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

#include "particle_shape.h"

void ParticleShape::_bind_methods() {
}

bool ParticleShape::_set(const StringName &p_name, const Variant &p_property) {
    if (p_name == "particle_count") {
        particles.resize(p_property);
        _change_notify();
        notify_change_to_owners();
        return true;
    }

    Vector<String> s_name = String(p_name).split("/");
    ERR_FAIL_COND_V(s_name.size() != 3, false);

    if ("particle" == s_name[0]) {
        const int particle_index = s_name[1].to_int();
        if ("mass" == s_name[2]) {
            particles[particle_index].mass = p_property;
        } else if ("position" == s_name[2]) {
            particles[particle_index].relative_position = p_property;
        }
        notify_change_to_owners();
        return true;
    }

    return false;
}

bool ParticleShape::_get(const StringName &p_name, Variant &r_property) const {
    if (p_name == "particle_count") {
        r_property = particles.size();
        return true;
    }

    Vector<String> s_name = String(p_name).split("/");
    ERR_FAIL_COND_V(s_name.size() != 3, false);

    if ("particle" == s_name[0]) {
        const int particle_index = s_name[1].to_int();
        if ("mass" == s_name[2]) {
            r_property = particles[particle_index].mass;
        } else if ("position" == s_name[2]) {
            r_property = particles[particle_index].relative_position;
        }
        return true;
    }

    return false;
}

void ParticleShape::_get_property_list(List<PropertyInfo> *p_list) const {

    p_list->push_back(PropertyInfo(Variant::INT, "particle_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));

    for (int i(0), s(particles.size()); i < s; ++i) {
        p_list->push_back(PropertyInfo(Variant::INT, "particle/" + String::num(i) + "/mass"));
        p_list->push_back(PropertyInfo(Variant::VECTOR3, "particle/" + String::num(i) + "/position"));
    }
}

ParticleShape::ParticleShape() {
}

void ParticleShape::set_particles(Vector<Particle> &p_particles) {
    particles = p_particles;
}

const Vector<ParticleShape::Particle> &ParticleShape::get_particles() const {
    return particles;
}
