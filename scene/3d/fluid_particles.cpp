/*************************************************************************/
/*  fluid_particles.cpp                                                  */
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

#include "fluid_particles.h"
#include "scene/resources/world.h"

void FluidParticles::_bind_methods() {
}

void FluidParticles::_notification(int p_what) {

	if (p_what == NOTIFICATION_ENTER_WORLD) {

		VisualServer::get_singleton()->fluid_particles_set_radius(
				fluid_particles,
				ParticlePhysicsServer::get_singleton()->space_get_particle_radius(
						get_world()->get_particle_space()));
	}
}

AABB FluidParticles::get_aabb() const {
	// TODO implement it properly
	return AABB(Vector3(-9999, -9999, -9999), Vector3(999999, 999999, 999999));
}

PoolVector<Face3> FluidParticles::get_faces(uint32_t p_usage_flags) const {
	return PoolVector<Face3>();
}

FluidParticles::FluidParticles() :
		GeometryInstance() {
	fluid_particles = VisualServer::get_singleton()->fluid_particles_create();
	set_base(fluid_particles);

	Vector<Vector3> positions;
	positions.push_back(Vector3(0, 0, 0));
	positions.push_back(Vector3(1, 0, 0));
	positions.push_back(Vector3(0, 1, 0));
	positions.push_back(Vector3(0, 0, 1));

	VisualServer::get_singleton()->fluid_particles_pre_allocate_memory(
			fluid_particles,
			10);

	VisualServer::get_singleton()->fluid_particles_set_positions(
			fluid_particles,
			(float *)positions.ptrw(),
			3,
			positions.size());
}

FluidParticles::~FluidParticles() {
	VS::get_singleton()->free(fluid_particles);
}
