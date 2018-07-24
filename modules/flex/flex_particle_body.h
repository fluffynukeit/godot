/*************************************************************************/
/*  flex_particle_body.h                                                 */
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

#ifndef FLEX_PARTICLE_BODY_H
#define FLEX_PARTICLE_BODY_H

#include "flex_particle_physics_server.h"
#include "flex_utility.h"
#include "rid_flex.h"

class FlexParticleBody;
class FlexSpace;
class Object;
class MemoryChunk;
class FlexPrimitiveBody;

enum ChangedBodyParameter {
	eChangedBodyParamParticleJustAdded = 1 << 0,
	eChangedBodyParamPositionMass = 1 << 1,
	eChangedBodyParamVelocity = 1 << 2,
	eChangedBodyParamPhase = 1 << 3,
	eChangedBodyParamInflatable = 1 << 4,

	eChangedBodyParamALL = eChangedBodyParamParticleJustAdded | eChangedBodyParamPositionMass | eChangedBodyParamVelocity | eChangedBodyParamPhase | eChangedBodyParamInflatable
};

/// This class represent a group of particles that are constrained each other and form a body.
/// This body can be rigid or soft.
///
/// It's possible to add and remove particles, since the particles internally can change its buffer position and then its ID
/// if you need to track a specific particle over time you can create a reference, when you add it.
/// The reference is an ID that don't change during time, and with it is possible to get the current ID of particle and then interrogate it.
///
/// [COMMAND] All functions marked with this label are commands and will be executed in the next tick.
class FlexParticleBody : public RIDFlex {

	friend class FlexSpace;
	friend class FlexParticleBodyCommands;

	FlexCallBackData sync_callback;
	FlexCallBackData particle_index_changed_callback;
	FlexCallBackData spring_index_changed_callback;
	FlexCallBackData primitive_contact_callback;

	struct {
		Vector<ParticleIndex> particles_to_remove;
		Vector<SpringIndex> springs_to_remove;
		Vector<TriangleIndex> triangles_to_remove;
		Vector<RigidIndex> rigids_to_remove;
		// rigid component to add
		Vector<RigidComponentIndex> rigids_components_to_remove;
	} delayed_commands;

	uint32_t changed_parameters;

	MemoryChunk *particles_mchunk;
	MemoryChunk *springs_mchunk;
	MemoryChunk *triangles_mchunk;
	MemoryChunk *inflatable_mchunk;
	MemoryChunk *rigids_mchunk;
	MemoryChunk *rigids_components_mchunk;

	FlexSpace *space;
	Object *object_instance;
	uint32_t collision_group;
	uint32_t collision_flags;
	uint32_t collision_primitive_mask;
	float rest_volume;
	float pressure; // 1 mean rest_volume
	float constraint_scale;

	bool _is_monitorable;
	bool _is_monitoring_primitives_contacts;

public:
	FlexParticleBody();

	_FORCE_INLINE_ FlexSpace *get_space() { return space; }

	void set_object_instance(Object *p_object);
	_FORCE_INLINE_ Object *get_object_instance() const { return object_instance; }

	void set_callback(ParticlePhysicsServer::ParticleBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method);

	_FORCE_INLINE_ uint32_t get_changed_parameters() const { return changed_parameters; }

	void set_collision_group(uint32_t p_group);
	uint32_t get_collision_group() const;

	uint32_t get_collision_flag_bit(ParticlePhysicsServer::ParticleCollisionFlag p_flag) const;
	void set_collision_flag(ParticlePhysicsServer::ParticleCollisionFlag p_flag, bool active);
	bool get_collision_flag(ParticlePhysicsServer::ParticleCollisionFlag p_flag) const;

	// Accept only first 7 bit
	void set_collision_primitive_mask(uint32_t p_primitive_mask);
	uint32_t get_collision_primitive_mask() const;

	void set_rest_volume(float p_rest_volume);
	float get_rest_volume() const;

	void set_pressure(float p_pressure);
	float get_pressure() const;

	void set_constraint_scale(float p_constraint_scale);
	float get_constraint_scale() const;

	void set_monitorable(bool p_monitorable);
	_FORCE_INLINE_ bool is_monitorable() const { return _is_monitorable; }

	void set_monitoring_primitives_contacts(bool p_monitoring);
	_FORCE_INLINE_ bool is_monitoring_primitives_contacts() const { return _is_monitoring_primitives_contacts; }

	void remove_particle(ParticleIndex p_particle);
	void remove_spring(SpringIndex p_spring_index);
	void remove_triangle(const TriangleIndex p_triangle_index);
	void remove_rigid(RigidIndex p_rigid_index);
	void remove_rigid_component(RigidComponentIndex p_rigid_component_index);

	int get_particle_count() const;
	int get_spring_count() const;
	int get_triangle_count() const;
	int get_rigid_count() const;

	// CMD
	void reset_spring(SpringIndex p_spring, ParticleIndex p_particle_0, ParticleIndex p_particle_1, float p_length, float p_stiffness);

	void set_particle_position_mass(ParticleIndex p_particle, const Vector3 &p_position, real_t p_mass);

	void set_particle_position(ParticleIndex p_particle, const Vector3 &p_position);
	Vector3 get_particle_position(ParticleIndex p_particle) const;

	void set_particle_mass(ParticleIndex p_particle, real_t p_mass);
	float get_particle_mass(ParticleIndex p_particle) const;

	const Vector3 &get_particle_velocity(ParticleIndex p_particle) const;
	void set_particle_velocity(ParticleIndex p_particle, const Vector3 &p_velocity);

	const DynamicTriangle &get_triangle(TriangleIndex p_triangle_index) const;

	const Vector3 &get_rigid_position(RigidIndex p_rigid_index) const;
	const Quat &get_rigid_rotation(RigidIndex p_rigid_index) const;

	void reload_rigids_COM();
	void reload_rigid_COM(RigidIndex p_rigid);
	// ~CMD

	bool is_owner_of_particle(ParticleIndex p_particle) const;
	bool is_owner_of_spring(SpringIndex p_spring) const;
	bool is_owner_of_triangle(TriangleIndex p_rigid) const;
	bool is_owner_of_rigid(RigidIndex p_triangle) const;
	bool is_owner_of_rigid_component(RigidComponentIndex p_rigid_component) const;

	// Internals
	void clear_changed_params();
	void clear_delayed_commands();
	void dispatch_sync_callback();
	void particle_index_changed(ParticleIndex p_old_particle_index, ParticleIndex p_new_particle_index);
	void spring_index_changed(SpringIndex p_old_spring_index, SpringIndex p_new_spring_index);
	void dispatch_primitive_contact(FlexPrimitiveBody *p_primitive, ParticleIndex p_particle_index, const Vector3 &p_velocity, const Vector3 &p_normal);
	void reload_inflatables();
};

#endif // FLEX_PARTICLE_BODY_H
