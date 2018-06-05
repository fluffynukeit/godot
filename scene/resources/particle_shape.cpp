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
    ERR_FAIL_COND_V(s_name.size() < 3, false);

    if ("particle" == s_name[0]) {
        const int particle_index = s_name[1].to_int();
        if ("mass" == s_name[2]) {
            particles[particle_index].mass = p_property;
        } else if ("position" == s_name[2]) {
            particles[particle_index].relative_position = p_property;
        } else if ("link_count" == s_name[2]) {
            particles[particle_index].link.resize(p_property);
            particles[particle_index].length.resize(p_property);
            particles[particle_index].stiffness.resize(p_property);
            _change_notify();
        } else if ("link" == s_name[2]) {

            ERR_FAIL_COND_V(s_name.size() < 5, false);
            const int link_index = s_name[3].to_int();
            if ("id" == s_name[4]) {

                particles[particle_index].link[link_index] = p_property;
            } else if ("length" == s_name[4]) {

                particles[particle_index].length[link_index] = p_property;
            } else if ("stiffness" == s_name[4]) {

                particles[particle_index].stiffness[link_index] = p_property;
            } else {
                return false;
            }
        } else {
            return false;
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
    ERR_FAIL_COND_V(s_name.size() < 3, false);

    if ("particle" == s_name[0]) {
        const int particle_index = s_name[1].to_int();
        if ("mass" == s_name[2]) {
            r_property = particles[particle_index].mass;
        } else if ("position" == s_name[2]) {
            r_property = particles[particle_index].relative_position;
        } else if ("link_count" == s_name[2]) {
            r_property = particles[particle_index].link.size();
        } else if ("link" == s_name[2]) {

            ERR_FAIL_COND_V(s_name.size() < 5, false);
            const int link_index = s_name[3].to_int();
            if ("id" == s_name[4]) {

                r_property = particles[particle_index].link[link_index];
            } else if ("length" == s_name[4]) {

                r_property = particles[particle_index].length[link_index];
            } else if ("stiffness" == s_name[4]) {

                r_property = particles[particle_index].stiffness[link_index];
            } else {
                return false;
            }
        } else {
            return false;
        }
        return true;
    }

    return false;
}

void ParticleShape::_get_property_list(List<PropertyInfo> *p_list) const {

    p_list->push_back(PropertyInfo(Variant::INT, "particle_count", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT));

    for (int i(0), s(particles.size()); i < s; ++i) {

        const String particle_id_s(String::num(i));
        p_list->push_back(PropertyInfo(Variant::INT, "particle/" + particle_id_s + "/mass"));
        p_list->push_back(PropertyInfo(Variant::VECTOR3, "particle/" + particle_id_s + "/position"));
        p_list->push_back(PropertyInfo(Variant::INT, "particle/" + particle_id_s + "/link_count"));

        for (int c(0), link_s(particles[i].link.size()); c < link_s; ++c) {

            const String link_id_s(String::num(c));
            p_list->push_back(PropertyInfo(Variant::INT, "particle/" + particle_id_s + "/link/" + link_id_s + "/id"));
            p_list->push_back(PropertyInfo(Variant::REAL, "particle/" + particle_id_s + "/link/" + link_id_s + "/length", PROPERTY_HINT_RANGE, "0,10,0.01"));
            p_list->push_back(PropertyInfo(Variant::REAL, "particle/" + particle_id_s + "/link/" + link_id_s + "/stiffness", PROPERTY_HINT_RANGE, "0,1,0.01"));
        }
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
