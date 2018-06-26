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

#include "particle_body_model.h"

void ParticleBodyModel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_particles", "particles"), &ParticleBodyModel::set_particles);
	ClassDB::bind_method(D_METHOD("get_particles"), &ParticleBodyModel::get_particles);

	ClassDB::bind_method(D_METHOD("set_masses", "masses"), &ParticleBodyModel::set_masses);
	ClassDB::bind_method(D_METHOD("get_masses"), &ParticleBodyModel::get_masses);

	ClassDB::bind_method(D_METHOD("set_constraints_indexes", "constraints_indexes"), &ParticleBodyModel::set_constraints_indexes);
	ClassDB::bind_method(D_METHOD("get_constraints_indexes"), &ParticleBodyModel::get_constraints_indexes);

	ClassDB::bind_method(D_METHOD("set_constraints_info", "constraints_info"), &ParticleBodyModel::set_constraints_info);
	ClassDB::bind_method(D_METHOD("get_constraints_info"), &ParticleBodyModel::get_constraints_info);

	ClassDB::bind_method(D_METHOD("set_clusters_offsets", "clusters_offsets"), &ParticleBodyModel::set_clusters_offsets);
	ClassDB::bind_method(D_METHOD("get_clusters_offsets"), &ParticleBodyModel::get_clusters_offsets);

	ClassDB::bind_method(D_METHOD("set_clusters_positions", "clusters_position"), &ParticleBodyModel::set_clusters_positions);
	ClassDB::bind_method(D_METHOD("get_clusters_positions"), &ParticleBodyModel::get_clusters_positions);

	ClassDB::bind_method(D_METHOD("set_clusters_stiffness", "clusters_stiffness"), &ParticleBodyModel::set_clusters_stiffness);
	ClassDB::bind_method(D_METHOD("get_clusters_stiffness"), &ParticleBodyModel::get_clusters_stiffness);

	ClassDB::bind_method(D_METHOD("set_clusters_plastic_threshold", "clusters_plastic_threshold"), &ParticleBodyModel::set_clusters_plastic_threshold);
	ClassDB::bind_method(D_METHOD("get_clusters_plastic_threshold"), &ParticleBodyModel::get_clusters_plastic_threshold);

	ClassDB::bind_method(D_METHOD("set_clusters_plastic_creep", "clusters_plastic_creep"), &ParticleBodyModel::set_clusters_plastic_creep);
	ClassDB::bind_method(D_METHOD("get_clusters_plastic_creep"), &ParticleBodyModel::get_clusters_plastic_creep);

	ClassDB::bind_method(D_METHOD("set_clusters_particle_indices", "clusters_particle_indices"), &ParticleBodyModel::set_clusters_particle_indices);
	ClassDB::bind_method(D_METHOD("get_clusters_particle_indices"), &ParticleBodyModel::get_clusters_particle_indices);

	ClassDB::bind_method(D_METHOD("set_dynamic_triangles_indices", "dynamic_triangles_indices"), &ParticleBodyModel::set_dynamic_triangles_indices);
	ClassDB::bind_method(D_METHOD("get_dynamic_triangles_indices"), &ParticleBodyModel::get_dynamic_triangles_indices);

	ClassDB::bind_method(D_METHOD("set_want_inflatable", "want_inflatable"), &ParticleBodyModel::set_want_inflatable);
	ClassDB::bind_method(D_METHOD("get_want_inflatable"), &ParticleBodyModel::get_want_inflatable);

	ClassDB::bind_method(D_METHOD("set_rest_volume", "rest_volume"), &ParticleBodyModel::set_rest_volume);
	ClassDB::bind_method(D_METHOD("get_rest_volume"), &ParticleBodyModel::get_rest_volume);

	ClassDB::bind_method(D_METHOD("set_constraint_scale", "constraint_scale"), &ParticleBodyModel::set_constraint_scale);
	ClassDB::bind_method(D_METHOD("get_constraint_scale"), &ParticleBodyModel::get_constraint_scale);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR3_ARRAY, "particles"), "set_particles", "get_particles");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_REAL_ARRAY, "masses"), "set_masses", "get_masses");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "constraints_indexes"), "set_constraints_indexes", "get_constraints_indexes");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "constraints_info"), "set_constraints_info", "get_constraints_info");

	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR3_ARRAY, "clusters_positions"), "set_clusters_positions", "get_clusters_positions");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_REAL_ARRAY, "clusters_stiffness"), "set_clusters_stiffness", "get_clusters_stiffness");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_REAL_ARRAY, "clusters_plastic_threshold"), "set_clusters_plastic_threshold", "get_clusters_plastic_threshold");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_REAL_ARRAY, "clusters_plastic_creep"), "set_clusters_plastic_creep", "get_clusters_plastic_creep");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "clusters_offsets"), "set_clusters_offsets", "get_clusters_offsets");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "clusters_particle_indices"), "set_clusters_particle_indices", "get_clusters_particle_indices");

	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "dynamic_triangles_indices"), "set_dynamic_triangles_indices", "get_dynamic_triangles_indices");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "want_inflatable"), "set_want_inflatable", "get_want_inflatable");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "rest_volume"), "set_rest_volume", "get_rest_volume");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "constraint_scale"), "set_constraint_scale", "get_constraint_scale");
}

ParticleBodyModel::ParticleBodyModel() :
		want_inflatable(false) {}

void ParticleBodyModel::set_particles(PoolVector<Vector3> p_particles) {
	particles = p_particles;
}

void ParticleBodyModel::set_masses(PoolVector<real_t> p_mass) {
	masses = p_mass;
}

void ParticleBodyModel::set_constraints_indexes(const PoolVector<int> p_constraints_index) {
	constraints_indexes = p_constraints_index;
}

void ParticleBodyModel::set_constraints_info(const PoolVector<Vector2> p_constraints_info) {
	constraints_info = p_constraints_info;
}

void ParticleBodyModel::set_clusters_positions(const PoolVector<Vector3> p_clusters_position) {
	clusters_positions = p_clusters_position;
}

void ParticleBodyModel::set_clusters_stiffness(const PoolVector<float> p_clusters_stiffness) {
	clusters_stiffness = p_clusters_stiffness;
}

void ParticleBodyModel::set_clusters_plastic_threshold(const PoolVector<float> p_clusters_plastic_threshold) {
	clusters_plastic_threshold = p_clusters_plastic_threshold;
}

void ParticleBodyModel::set_clusters_plastic_creep(const PoolVector<float> p_clusters_plastic_creep) {
	clusters_plastic_creep = p_clusters_plastic_creep;
}

void ParticleBodyModel::set_clusters_offsets(const PoolVector<int> p_clusters_offsets) {
	clusters_offsets = p_clusters_offsets;
}

void ParticleBodyModel::set_clusters_particle_indices(const PoolVector<int> p_clusters_particle_indices) {
	clusters_particle_indices = p_clusters_particle_indices;
}

void ParticleBodyModel::set_dynamic_triangles_indices(const PoolVector<int> p_dynamic_triangles_indices) {
	dynamic_triangles_indices = p_dynamic_triangles_indices;
}

void ParticleBodyModel::set_want_inflatable(bool p_want) {
	want_inflatable = p_want;
}

void ParticleBodyModel::set_rest_volume(float p_rest_volume) {
	rest_volume = p_rest_volume;
}

void ParticleBodyModel::set_constraint_scale(float p_constraint_scale) {
	constraint_scale = p_constraint_scale;
}
