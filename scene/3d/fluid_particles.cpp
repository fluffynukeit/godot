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
#include "core/engine.h"
#include "scene/3d/physics_particle_body.h"
#include "scene/resources/world.h"

void FluidParticles::_bind_methods() {

	ClassDB::bind_method(
			D_METHOD("update_data", "cmds"),
			&FluidParticles::update_data);

	ClassDB::bind_method(
			D_METHOD("set_drop_thickness_factor", "factor"),
			&FluidParticles::set_drop_thickness_factor);
	ClassDB::bind_method(
			D_METHOD("get_drop_thickness_factor"),
			&FluidParticles::get_drop_thickness_factor);

	ADD_PROPERTY(
			PropertyInfo(
					Variant::REAL,
					"drop_thickness_factor",
					PROPERTY_HINT_RANGE,
					"0,1,0.0001"),
			"set_drop_thickness_factor",
			"get_drop_thickness_factor");
}

void FluidParticles::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {

			ERR_FAIL_COND(particle_body);

			if (!Engine::get_singleton()->is_editor_hint()) {

				set_as_toplevel(true);
				set_global_transform(Transform());
			}

			if (particle_body) {
				particle_body->disconnect("commands_process", this, "update_data");
			}

			particle_body = Object::cast_to<ParticleBody>(get_parent());

			if (particle_body) {
				particle_body->connect("commands_process", this, "update_data");
			}

		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {

			if (!Engine::get_singleton()->is_editor_hint())
				return;

			if (!particle_body)
				return;

			particle_body->set_global_transform(get_global_transform());
			set_notify_local_transform(false);
			set_transform(Transform());
			set_notify_local_transform(true);

		} break;
		case NOTIFICATION_EXIT_TREE: {
			particle_body = NULL;
		} break;
	}
}

AABB FluidParticles::get_aabb() const {
	return AABB();
}

PoolVector<Face3> FluidParticles::get_faces(uint32_t p_usage_flags) const {
	return PoolVector<Face3>();
}

FluidParticles::FluidParticles() :
		GeometryInstance(),
		particle_body(NULL),
		drop_thickness_factor(0.01) {

	fluid_particles = VisualServer::get_singleton()->fluid_particles_create();
	set_base(fluid_particles);
}

FluidParticles::~FluidParticles() {
	VS::get_singleton()->free(fluid_particles);
}

void FluidParticles::set_drop_thickness_factor(real_t p_factor) {

	drop_thickness_factor = p_factor;

	VisualServer::get_singleton()->fluid_particles_set_drop_thickness_factor(
			fluid_particles,
			drop_thickness_factor);
}

void FluidParticles::update_data(Object *p_cmds) {

	ParticleBodyCommands *cmds = cast_to<ParticleBodyCommands>(p_cmds);

	static const AABB aabb(Vector3(-9999, -9999, -9999), Vector3(19998, 19998, 19998));
	//const AABB aabb = cmds->get_aabb();
	const float *pbuffer = cmds->get_particle_buffer();
	const float *vbuffer = cmds->get_particle_velocities_buffer();
	const int count = cmds->get_particle_count();

	VisualServer::get_singleton()->fluid_particles_set_aabb(
			fluid_particles,
			aabb);

	// TODO please move this to proper method, copy this here is wrong.
	// See how ParticleBody update its mesh
	VisualServer::get_singleton()->fluid_particles_set_data(
			fluid_particles,
			cmds->get_particle_buffer_stride(),
			pbuffer,
			vbuffer,
			count);

	if (get_world().is_valid())
		VisualServer::get_singleton()->fluid_particles_set_radius(
				fluid_particles,
				ParticlePhysicsServer::get_singleton()->space_get_particle_radius(
						get_world()->get_particle_space()));
}
