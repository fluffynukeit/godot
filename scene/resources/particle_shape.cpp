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
	ClassDB::bind_method(D_METHOD("set_particles", "particles"), &ParticleShape::set_particles);
	ClassDB::bind_method(D_METHOD("get_particles"), &ParticleShape::get_particles);

	ClassDB::bind_method(D_METHOD("set_masses", "masses"), &ParticleShape::set_masses);
	ClassDB::bind_method(D_METHOD("get_masses"), &ParticleShape::get_masses);

	ClassDB::bind_method(D_METHOD("set_constraints_indexes", "constraints_indexes"), &ParticleShape::set_constraints_indexes);
	ClassDB::bind_method(D_METHOD("get_constraints_indexes"), &ParticleShape::get_constraints_indexes);

	ClassDB::bind_method(D_METHOD("set_constraints_info", "constraints_info"), &ParticleShape::set_constraints_info);
	ClassDB::bind_method(D_METHOD("get_constraints_info"), &ParticleShape::get_constraints_info);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR3_ARRAY, "particles"), "set_particles", "get_particles");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_REAL_ARRAY, "masses"), "set_masses", "get_masses");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "constraints_indexes"), "set_constraints_indexes", "get_constraints_indexes");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "constraints_info"), "set_constraints_info", "get_constraints_info");
}

ParticleShape::ParticleShape() {}

void ParticleShape::set_particles(PoolVector<Vector3> p_particles) {
	particles = p_particles;
}

void ParticleShape::set_masses(PoolVector<real_t> p_mass) {
	masses = p_mass;
}

void ParticleShape::set_constraints_indexes(const PoolVector<int> p_constraints_index) {
	constraints_indexes = p_constraints_index;
}

void ParticleShape::set_constraints_info(const PoolVector<Vector2> p_constraints_info) {
	constraints_info = p_constraints_info;
}
