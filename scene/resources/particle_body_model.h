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

#ifndef PARTICLE_BODY_MODEL_H
#define PARTICLE_BODY_MODEL_H

#include "resource.h"

/// The particle model is a resource that doesn't have a specific RID that identify the shape in the ParticlePhysicsServer
/// Because it's not necessary
class ParticleBodyModel : public Resource {
	GDCLASS(ParticleBodyModel, Resource);
	OBJ_SAVE_TYPE(ParticleBodyModel);
	RES_BASE_EXTENSION("particle_body_model");

protected:
	static void _bind_methods();

private:
	PoolVector<Vector3> particles;
	PoolVector<real_t> masses;
	PoolVector<int> constraints_indexes; // pair of 2
	PoolVector<Vector2> constraints_info; // X = length, Y = stiffnes

	PoolVector<Vector3> clusters_positions; // Average position local space
	PoolVector<float> clusters_stiffness;
	PoolVector<int> clusters_offsets; // This represent the last id of a group of particles
	PoolVector<int> clusters_particle_indices; // Cluster index organized per offset

public:
	ParticleBodyModel();

	void set_particles(PoolVector<Vector3> p_particles);
	PoolVector<Vector3> get_particles() const { return particles; }
	PoolVector<Vector3> &get_particles_ref() { return particles; }

	void set_masses(PoolVector<real_t> p_mass);
	PoolVector<real_t> get_masses() { return masses; }
	PoolVector<real_t> &get_masses_ref() { return masses; }

	void set_constraints_indexes(const PoolVector<int> p_constraints_indexes);
	PoolVector<int> get_constraints_indexes() { return constraints_indexes; }
	PoolVector<int> &get_constraints_indexes_ref() { return constraints_indexes; }

	void set_constraints_info(const PoolVector<Vector2> p_constraints_info);
	PoolVector<Vector2> get_constraints_info() { return constraints_info; }
	PoolVector<Vector2> &get_constraints_info_ref() { return constraints_info; }

	void set_clusters_positions(const PoolVector<Vector3> p_clusters_positions);
	PoolVector<Vector3> get_clusters_positions() { return clusters_positions; }
	PoolVector<Vector3> &get_clusters_positions_ref() { return clusters_positions; }

	void set_clusters_stiffness(const PoolVector<float> p_clusters_stiffness);
	PoolVector<float> get_clusters_stiffness() { return clusters_stiffness; }
	PoolVector<float> &get_clusters_stiffness_ref() { return clusters_stiffness; }

	void set_clusters_offsets(const PoolVector<int> p_clusters_offsets);
	PoolVector<int> get_clusters_offsets() { return clusters_offsets; }
	PoolVector<int> &get_clusters_offsets_ref() { return clusters_offsets; }

	void set_clusters_particle_indices(const PoolVector<int> p_clusters_particle_indices);
	PoolVector<int> get_clusters_particle_indices() { return clusters_particle_indices; }
	PoolVector<int> &get_clusters_particle_indices_ref() { return clusters_particle_indices; }
};

#endif // PARTICLE_BODY_MODEL_H
