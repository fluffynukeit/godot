/*************************************************************************/
/*  flex_particle_body.cpp                                               */
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
    @author AndreaCatania
*/

#include "flex_particle_body.h"
#include "core/object.h"
#include "flex_memory.h"
#include "flex_space.h"

FlexParticleBody::FlexParticleBody() :
		RIDFlex(),
		id(-1),
		space(NULL),
		object_instance(NULL),
		particles_mchunk(NULL),
		springs_mchunk(NULL),
		triangles_mchunk(NULL),
		inflatable_mchunk(NULL),
		rigids_mchunk(NULL),
		rigids_components_mchunk(NULL),
		changed_parameters(0),
		collision_group(0),
		collision_flags(0),
		collision_primitive_mask(eNvFlexPhaseShapeChannel0),
		rest_volume(0),
		pressure(1),
		constraint_scale(0),
		particle_count(0),
		_is_monitorable(false),
		_is_monitoring_primitives_contacts(false),
		tearing_active(false),
		tearing_max_extension(2.0),
		tearing_data(NULL) {
	sync_callback.receiver = NULL;
}

void FlexParticleBody::set_object_instance(Object *p_object) {
	object_instance = p_object;
}

void FlexParticleBody::set_callback(ParticlePhysicsServer::ParticleBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method) {

	if (p_receiver) {
		ERR_FAIL_COND(!p_receiver->has_method(p_method));
	}

	switch (p_callback_type) {
		case ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SYNC:
			sync_callback.receiver = p_receiver;
			sync_callback.method = p_method;
			break;
		case ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED:
			particle_index_changed_callback.receiver = p_receiver;
			particle_index_changed_callback.method = p_method;
			break;
		case ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED:
			spring_index_changed_callback.receiver = p_receiver;
			spring_index_changed_callback.method = p_method;
			break;
		case ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_PRIMITIVECONTACT:
			primitive_contact_callback.receiver = p_receiver;
			primitive_contact_callback.method = p_method;
			break;
	}
}

void FlexParticleBody::set_collision_group(uint32_t p_group) {
	if (p_group >= (2 ^ 20))
		return;
	collision_group = p_group;
	changed_parameters |= eChangedBodyParamPhase;
}

uint32_t FlexParticleBody::get_collision_group() const {
	return collision_group;
}

uint32_t FlexParticleBody::get_collision_flag_bit(ParticlePhysicsServer::ParticleCollisionFlag p_flag) const {
	switch (p_flag) {
		case ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE:
			return eNvFlexPhaseSelfCollide;
		case ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE_FILTER:
			return eNvFlexPhaseSelfCollideFilter;
		case ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_FLUID:
			return eNvFlexPhaseFluid;
	}
	return 0;
}

void FlexParticleBody::set_collision_flag(ParticlePhysicsServer::ParticleCollisionFlag p_flag, bool p_active) {

	if (p_active) {
		collision_flags |= get_collision_flag_bit(p_flag);
	} else {
		collision_flags &= ~get_collision_flag_bit(p_flag);
	}
	changed_parameters |= eChangedBodyParamPhase;
}

bool FlexParticleBody::get_collision_flag(ParticlePhysicsServer::ParticleCollisionFlag p_flag) const {
	return collision_flags & get_collision_flag_bit(p_flag);
}

void FlexParticleBody::set_collision_primitive_mask(uint32_t p_primitive_mask) {
	collision_primitive_mask = eNvFlexPhaseShapeChannelMask & (p_primitive_mask << 24);
	changed_parameters |= eChangedBodyParamPhase;
}

uint32_t FlexParticleBody::get_collision_primitive_mask() const {
	return collision_primitive_mask >> 24;
}

void FlexParticleBody::set_rest_volume(float p_rest_volume) {
	rest_volume = p_rest_volume;
	changed_parameters |= eChangedBodyParamInflatable;
}

float FlexParticleBody::get_rest_volume() const {
	return rest_volume;
}

void FlexParticleBody::set_pressure(float p_pressure) {
	pressure = p_pressure;
	changed_parameters |= eChangedBodyParamInflatable;
}

float FlexParticleBody::get_pressure() const {
	return pressure;
}

void FlexParticleBody::set_constraint_scale(float p_constraint_scale) {
	constraint_scale = p_constraint_scale;
	changed_parameters |= eChangedBodyParamInflatable;
}

float FlexParticleBody::get_constraint_scale() const {
	return constraint_scale;
}

void FlexParticleBody::set_monitorable(bool p_monitorable) {
	_is_monitorable = p_monitorable;
}

void FlexParticleBody::set_monitoring_primitives_contacts(bool p_monitoring) {
	_is_monitoring_primitives_contacts = p_monitoring;
}

void FlexParticleBody::unactive_particle(ParticleIndex p_particle) {
	ERR_FAIL_COND(!is_owner_of_particle(p_particle));
	if (-1 == delayed_commands.particles_to_unactive.find(p_particle))
		delayed_commands.particles_to_unactive.push_back(p_particle);
}

void FlexParticleBody::remove_particle(ParticleIndex p_particle) {
	ERR_FAIL_COND(!is_owner_of_particle(p_particle));
	if (-1 == delayed_commands.particles_to_remove.find(p_particle))
		delayed_commands.particles_to_remove.push_back(p_particle);
}

void FlexParticleBody::remove_spring(SpringIndex p_spring_index) {
	ERR_FAIL_COND(!is_owner_of_spring(p_spring_index));
	if (-1 == delayed_commands.springs_to_remove.find(p_spring_index))
		delayed_commands.springs_to_remove.push_back(p_spring_index);
}

void FlexParticleBody::remove_triangle(const TriangleIndex p_triangle_index) {
	ERR_FAIL_COND(!is_owner_of_triangle(p_triangle_index));
	if (-1 == delayed_commands.triangles_to_remove.find(p_triangle_index))
		delayed_commands.triangles_to_remove.push_back(p_triangle_index);
}

void FlexParticleBody::remove_rigid(RigidIndex p_rigid_index) {
	ERR_FAIL_COND(!is_owner_of_rigid(p_rigid_index));
	if (-1 == delayed_commands.rigids_to_remove.find(p_rigid_index))
		delayed_commands.rigids_to_remove.push_back(p_rigid_index);
}

void FlexParticleBody::remove_rigid_component(RigidComponentIndex p_rigid_component_index) {
	ERR_FAIL_COND(!is_owner_of_rigid_component(p_rigid_component_index));
	if (-1 == delayed_commands.rigids_components_to_remove.find(p_rigid_component_index))
		delayed_commands.rigids_components_to_remove.push_back(p_rigid_component_index);
}

int FlexParticleBody::get_spring_count() const {
	return springs_mchunk ? springs_mchunk->get_size() : 0;
}

int FlexParticleBody::get_triangle_count() const {
	return triangles_mchunk ? triangles_mchunk->get_size() : 0;
}

int FlexParticleBody::get_rigid_count() const {
	return rigids_mchunk ? rigids_mchunk->get_size() : 0;
}

void FlexParticleBody::set_particle_count(int p_particle_count) {
	int memory_size = particles_mchunk ? particles_mchunk->get_size() : 0;
	if (memory_size < p_particle_count) {
		particle_count = memory_size;
		ERR_FAIL();
	} else {
		particle_count = p_particle_count;
	}
}

int FlexParticleBody::get_particle_count() const {
	return particle_count;
}

void FlexParticleBody::set_tearing_active(bool p_active) {
	tearing_active = p_active;

	if (p_active) {
		if (!tearing_data)
			tearing_data = std::make_shared<ParticlePhysicsServer::TearingData>();
	} else {
		tearing_data.reset();
	}

	if (space) {
		space->update_particle_body_tearing_state(this);
	}
}

bool FlexParticleBody::is_tearing_active() const {
	return tearing_active;
}

void FlexParticleBody::add_unactive_particles(int p_particle_count) {
	ERR_FAIL_COND(p_particle_count < 1);

	const int previous_size = get_particle_count();
	const int new_size = previous_size + p_particle_count;

	space->get_particles_allocator()->resize_chunk(
			particles_mchunk,
			new_size);
}

ParticleIndex FlexParticleBody::add_particles(int p_particle_count) {
	ERR_FAIL_COND_V(p_particle_count < 1, -1);

	const int previous_size = get_particle_count();
	const int new_size = previous_size + p_particle_count;

	if (new_size > particles_mchunk->get_size()) {
		// Require resize
		space->get_particles_allocator()->resize_chunk(
				particles_mchunk,
				new_size);
	}

	set_particle_count(new_size);

	return previous_size;
}

void FlexParticleBody::copy_particle(ParticleIndex p_to, ParticleIndex p_from) {
	set_particle(p_to, get_particle(p_from));
	set_particle_velocity(p_to, get_particle_velocity(p_from));
	set_particle_normal(p_to, get_particle_normal(p_from));
}

void FlexParticleBody::set_particle(ParticleIndex p_particle_index, const FlVector4 &p_particle) {
	space->get_particles_memory()->set_particle(
			particles_mchunk,
			p_particle_index,
			p_particle);
}

const FlVector4 &FlexParticleBody::get_particle(ParticleIndex p_particle_index) const {
	return space->get_particles_memory()->get_particle(
			particles_mchunk,
			p_particle_index);
}

void FlexParticleBody::set_particle_position_mass(ParticleIndex p_particle_index, const Vector3 &p_position, real_t p_mass) {
	space->get_particles_memory()->set_particle(particles_mchunk, p_particle_index, make_particle(p_position, p_mass));
	changed_parameters |= eChangedBodyParamPositionMass;
}

Vector3 FlexParticleBody::get_particle_position(ParticleIndex p_particle_index) const {
	const FlVector4 &p(space->get_particles_memory()->get_particle(particles_mchunk, p_particle_index));
	return extract_position(p);
}

void FlexParticleBody::set_particle_position(ParticleIndex p_particle_index, const Vector3 &p_position) {
	space->get_particles_memory()->set_particle(particles_mchunk, p_particle_index, make_particle(p_position, get_particle_mass(p_particle_index)));
	changed_parameters |= eChangedBodyParamPositionMass;
}

void FlexParticleBody::set_particle_mass(ParticleIndex p_particle_index, real_t p_mass) {
	space->get_particles_memory()->set_particle(particles_mchunk, p_particle_index, make_particle(get_particle_position(p_particle_index), p_mass));
	changed_parameters |= eChangedBodyParamPositionMass;
}

float FlexParticleBody::get_particle_mass(ParticleIndex p_particle_index) const {
	return extract_mass(space->get_particles_memory()->get_particle(particles_mchunk, p_particle_index));
}

float FlexParticleBody::get_particle_inv_mass(ParticleIndex p_particle_index) const {
	return extract_inverse_mass(space->get_particles_memory()->get_particle(particles_mchunk, p_particle_index));
}

const Vector3 &FlexParticleBody::get_particle_velocity(ParticleIndex p_particle_index) const {
	return space->get_particles_memory()->get_velocity(particles_mchunk, p_particle_index);
}

void FlexParticleBody::set_particle_velocity(ParticleIndex p_particle_index, const Vector3 &p_velocity) {
	space->get_particles_memory()->set_velocity(particles_mchunk, p_particle_index, p_velocity);
	changed_parameters |= eChangedBodyParamVelocity;
}

const FlVector4 &FlexParticleBody::get_particle_normal(ParticleIndex p_particle_index) const {
	return space->get_particles_memory()->get_normal(particles_mchunk, p_particle_index);
}

void FlexParticleBody::set_particle_normal(ParticleIndex p_particle_index, const Vector3 &p_normal) {
	space->get_particles_memory()->set_normal(particles_mchunk, p_particle_index, flvec4_from_vec3(p_normal));
	changed_parameters |= eChangedBodyParamNormal;
}

void FlexParticleBody::set_particle_normal(ParticleIndex p_particle_index, const FlVector4 &p_normal) {
	space->get_particles_memory()->set_normal(particles_mchunk, p_particle_index, p_normal);
	changed_parameters |= eChangedBodyParamNormal;
}

void FlexParticleBody::set_triangle(
		TriangleIndex p_triangle_index,
		const DynamicTriangle &p_triangle) {

	space->get_triangles_memory()->set_triangle(
			triangles_mchunk,
			p_triangle_index,
			p_triangle);
}
const DynamicTriangle &FlexParticleBody::get_triangle(TriangleIndex p_triangle_index) const {
	return space->get_triangles_memory()->get_triangle(triangles_mchunk, p_triangle_index);
}

const Vector3 &FlexParticleBody::get_rigid_position(RigidIndex p_rigid_index) const {
	return space->get_rigids_memory()->get_position(rigids_mchunk, p_rigid_index);
}

const Quat &FlexParticleBody::get_rigid_rotation(RigidIndex p_rigid_index) const {
	return space->get_rigids_memory()->get_rotation(rigids_mchunk, p_rigid_index);
}

void FlexParticleBody::reload_rigids_COM() {
	for (int i(rigids_mchunk->get_size() - 1); 0 <= i; --i) {
		reload_rigid_COM(i);
	}
}

void FlexParticleBody::reload_rigid_COM(RigidIndex p_rigid) {

	// TODO This class calculates the new COM from current position of particle
	// and this is not correct because I will set a deformed rest and not the correct one
	// Improve this class by using the rest position already set in the buffer
	ERR_PRINT("reload_rigid_COM is used but must be improved!");

	// calculate ne center of mass
	const RigidComponentIndex start_offset = p_rigid == 0 ? RigidComponentIndex(0) : space->get_rigids_memory()->get_offset(rigids_mchunk, p_rigid - 1);
	const RigidComponentIndex end_offset = space->get_rigids_memory()->get_offset(rigids_mchunk, p_rigid);

	const int size(end_offset - start_offset);

	if (!size)
		return;

	// In global space
	Vector3 center;
	for (RigidComponentIndex i(start_offset); i < end_offset; ++i) {
		ParticleBufferIndex particle = space->get_rigids_components_memory()->get_index(rigids_components_mchunk, i);
		const FlVector4 &particle_data(space->get_particles_memory()->get_particle(particles_mchunk, particles_mchunk->get_chunk_index(particle)));
		center += extract_position(particle_data);
	}
	center /= (float)size;

	// calculate rests
	for (RigidComponentIndex i(start_offset); i < end_offset; ++i) {
		ParticleBufferIndex particle = space->get_rigids_components_memory()->get_index(rigids_components_mchunk, i);
		const FlVector4 &particle_data(space->get_particles_memory()->get_particle(particles_mchunk, particles_mchunk->get_chunk_index(particle)));

		space->get_rigids_components_memory()->set_rest(rigids_components_mchunk, i, extract_position(particle_data) - center);
	}

	// Assign new position
	space->get_rigids_memory()->set_position(rigids_mchunk, p_rigid, center);
}

void FlexParticleBody::set_spring(
		SpringIndex p_index,
		ParticleIndex p_particle_0,
		ParticleIndex p_particle_1,
		float p_length,
		float p_stiffness) {

	ERR_FAIL_COND(!is_owner_of_spring(p_index));

	space->get_springs_memory()->set_spring(
			springs_mchunk,
			p_index,
			Spring(
					particles_mchunk->get_buffer_index(p_particle_0),
					particles_mchunk->get_buffer_index(p_particle_1)));

	space->get_springs_memory()->set_length(
			springs_mchunk,
			p_index,
			p_length);

	space->get_springs_memory()->set_stiffness(
			springs_mchunk,
			p_index,
			p_stiffness);
}

const Spring &FlexParticleBody::get_spring_indices(SpringIndex spring_index) const {
	return space->get_springs_memory()->get_spring(
			springs_mchunk,
			spring_index);
}

void FlexParticleBody::set_spring_indices(SpringIndex spring_index, const Spring &p_spring) {
	space->get_springs_memory()->set_spring(
			springs_mchunk,
			spring_index,
			p_spring);
}

SpringIndex FlexParticleBody::add_spring(
		ParticleIndex p_particle_0,
		ParticleIndex p_particle_1,
		float p_length,
		float p_stiffness) {

	const SpringIndex new_index = add_spring();
	set_spring(
			new_index,
			p_particle_0,
			p_particle_1,
			p_length,
			p_stiffness);
	return new_index;
}

SpringIndex FlexParticleBody::add_spring() {
	const int previous_size = get_spring_count();
	space->get_springs_allocator()->resize_chunk(springs_mchunk, previous_size + 1);
	return previous_size;
}

void FlexParticleBody::copy_spring(SpringIndex p_to, SpringIndex p_from) {
	const Spring &s = get_spring_indices(p_from);
	const real_t length = space->get_springs_memory()->get_length(springs_mchunk, p_from);
	const real_t stiffness = space->get_springs_memory()->get_stiffness(springs_mchunk, p_from);

	space->get_springs_memory()->set_spring(
			springs_mchunk,
			p_to,
			Spring(
					s.index0,
					s.index1));

	space->get_springs_memory()->set_length(
			springs_mchunk,
			p_to,
			length);

	space->get_springs_memory()->set_stiffness(
			springs_mchunk,
			p_to,
			stiffness);
}

SpringIndex FlexParticleBody::duplicate_spring(SpringIndex p_other_spring) {
	SpringIndex index = add_spring();
	copy_spring(index, p_other_spring);
	return index;
}

real_t FlexParticleBody::get_spring_extension(SpringIndex spring_index) const {
	const Spring &s = get_spring_indices(spring_index);

	const FlVector4 &p0 = space->get_particles_memory()->get_particle(s.index0);
	const FlVector4 &p1 = space->get_particles_memory()->get_particle(s.index1);

	return extract_position((p1 - p0)).length();
}

bool FlexParticleBody::is_owner_of_particle(ParticleIndex p_particle) const {
	return particles_mchunk->get_buffer_index(p_particle) <= particles_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_spring(SpringIndex p_spring) const {
	return springs_mchunk->get_buffer_index(p_spring) <= springs_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_triangle(TriangleIndex p_triangle) const {
	return triangles_mchunk->get_buffer_index(p_triangle) <= triangles_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_rigid(RigidIndex p_rigid) const {
	return rigids_mchunk->get_buffer_index(p_rigid) <= rigids_mchunk->get_end_index();
}

bool FlexParticleBody::is_owner_of_rigid_component(RigidComponentIndex p_rigid_component) const {
	return rigids_components_mchunk->get_buffer_index(p_rigid_component) <= rigids_components_mchunk->get_end_index();
}

void FlexParticleBody::clear_changed_params() {
	changed_parameters = 0;
}

void FlexParticleBody::dispatch_sync_callback() {
	if (!sync_callback.receiver)
		return;
	static Variant::CallError error;
	const Variant *p = FlexParticlePhysicsServer::singleton->get_particle_body_commands_variant(this);
	sync_callback.receiver->call(sync_callback.method, &p, 1, error);
}

void FlexParticleBody::particle_index_changed(ParticleIndex p_old_particle_index, ParticleIndex p_new_particle_index) {
	if (!particle_index_changed_callback.receiver)
		return;
	particle_index_changed_callback.receiver->call(particle_index_changed_callback.method, (int)p_old_particle_index, (int)p_new_particle_index);
}

void FlexParticleBody::spring_index_changed(SpringIndex p_old_spring_index, SpringIndex p_new_spring_index) {
	if (!spring_index_changed_callback.receiver)
		return;
	spring_index_changed_callback.receiver->call(spring_index_changed_callback.method, (int)p_old_spring_index, (int)p_new_spring_index);
}

void FlexParticleBody::dispatch_primitive_contact(FlexPrimitiveBody *p_primitive, ParticleIndex p_particle_index, const Vector3 &p_velocity, const Vector3 &p_normal) {
	if (!primitive_contact_callback.receiver)
		return;

	const Variant primitive_body_object_instance(p_primitive->get_object_instance());
	const Variant particle((int)p_particle_index);
	const Variant velocity(p_velocity);
	const Variant normal(p_normal);

	const Variant *p[5] = { FlexParticlePhysicsServer::singleton->get_particle_body_commands_variant(this), &primitive_body_object_instance, &particle, &velocity, &normal };

	static Variant::CallError error;
	primitive_contact_callback.receiver->call(primitive_contact_callback.method, p, 5, error);
}

void FlexParticleBody::reload_inflatables() {
	if (!inflatable_mchunk->get_size())
		return;

	space->get_inflatables_memory()->set_start_triangle_index(inflatable_mchunk, 0, triangles_mchunk->get_begin_index());
	space->get_inflatables_memory()->set_triangle_count(inflatable_mchunk, 0, get_triangle_count());
}
