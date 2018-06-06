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
    if ("particle_count" == p_name) {
        particles.resize(p_property);
        _change_notify();
        notify_change_to_owners();
        return true;
    } else if ("constraint_count" == p_name) {
        constraints.resize(p_property);
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
        } else {
            return false;
        }
        notify_change_to_owners();
        return true;

    } else if ("constraint" == s_name[0]) {

        const int constraint_index = s_name[1].to_int();
        if ("id_0" == s_name[2]) {
            constraints[constraint_index].particle_index_0 = p_property;
        } else if ("id_1" == s_name[2]) {
            constraints[constraint_index].particle_index_1 = p_property;
        } else if ("length" == s_name[2]) {
            constraints[constraint_index].length = p_property;
        } else if ("stiffness" == s_name[2]) {
            constraints[constraint_index].stiffness = p_property;
        } else {
            return false;
        }

        notify_change_to_owners();
        return true;
    }
}

bool ParticleShape::_get(const StringName &p_name, Variant &r_property) const {

    if ("particle_count" == p_name) {
        r_property = particles.size();
        return true;
    } else if ("constraint_count" == p_name) {
        r_property = constraints.size();
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
        } else {
            return false;
        }
        return true;

    } else if ("constraint" == s_name[0]) {

        const int constraint_index = s_name[1].to_int();
        if ("id_0" == s_name[2]) {
            r_property = constraints[constraint_index].particle_index_0;
        } else if ("id_1" == s_name[2]) {
            r_property = constraints[constraint_index].particle_index_1;
        } else if ("length" == s_name[2]) {
            r_property = constraints[constraint_index].length;
        } else if ("stiffness" == s_name[2]) {
            r_property = constraints[constraint_index].stiffness;
        } else {
            return false;
        }
        return true;
    }

    return false;
}

void ParticleShape::_get_property_list(List<PropertyInfo> *p_list) const {

    p_list->push_back(PropertyInfo(Variant::INT, "particle_count"));

    for (int i(0), s(particles.size()); i < s; ++i) {

        const String particle_id_s(String::num(i));
        p_list->push_back(PropertyInfo(Variant::REAL, "particle/" + particle_id_s + "/mass", PROPERTY_HINT_RANGE, "0,100,0.5"));
        p_list->push_back(PropertyInfo(Variant::VECTOR3, "particle/" + particle_id_s + "/position"));
    }

    p_list->push_back(PropertyInfo(Variant::INT, "constraint_count"));

    for (int i(0), s(constraints.size()); i < s; ++i) {

        const String constraint_id_s(String::num(i));
        p_list->push_back(PropertyInfo(Variant::INT, "constraint/" + constraint_id_s + "/id_0"));
        p_list->push_back(PropertyInfo(Variant::INT, "constraint/" + constraint_id_s + "/id_1"));
        p_list->push_back(PropertyInfo(Variant::REAL, "constraint/" + constraint_id_s + "/length", PROPERTY_HINT_RANGE, "0,10,0.01"));
        p_list->push_back(PropertyInfo(Variant::REAL, "constraint/" + constraint_id_s + "/stiffness", PROPERTY_HINT_RANGE, "0,1,0.01"));
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

void ParticleShape::set_constraints(const Vector<Constraint> &p_constraints) {
    constraints = p_constraints;
}

const Vector<ParticleShape::Constraint> &ParticleShape::get_constraints() const {
    return constraints;
}
