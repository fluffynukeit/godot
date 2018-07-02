/*************************************************************************/
/*  flex_primitive_body.cpp                                              */
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

#include "flex_primitive_body.h"

#include "core/object.h"
#include "flex_particle_body.h"
#include "flex_primitive_shapes.h"

FlexPrimitiveBody::FlexPrimitiveBody() :
		RIDFlex(),
		changed_parameters(0),
		geometry_mchunk(NULL),
		space(NULL),
		object_instance(NULL),
		shape(NULL),
		_is_kinematic(false),
		_is_area(false),
		layer(1),
		_is_monitoring_particles_contacts(false) {
}

FlexPrimitiveBody::~FlexPrimitiveBody() {
	if (shape) {
		shape->remove_owner(this);
	}
}

void FlexPrimitiveBody::set_space(FlexSpace *p_space) {
	space = p_space;
}

FlexSpace *FlexPrimitiveBody::get_space() const {
	return space;
}

void FlexPrimitiveBody::set_object_instance(Object *p_object) {
	object_instance = p_object;
}

void FlexPrimitiveBody::set_callback(ParticlePhysicsServer::ParticlePrimitiveBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method) {

	if (p_receiver) {
		ERR_FAIL_COND(!p_receiver->has_method(p_method));
	}

	switch (p_callback_type) {
		case ParticlePhysicsServer::PARTICLE_PRIMITIVE_BODY_CALLBACK_PARTICLECONTACT:
			particles_contact_callback.receiver = p_receiver;
			particles_contact_callback.method = p_method;
			break;
	}
}

void FlexPrimitiveBody::set_shape(FlexPrimitiveShape *p_shape) {
	if (shape) {
		shape->remove_owner(this);
	}
	shape = p_shape;
	changed_parameters |= eChangedPrimitiveBodyParamShape;
	if (shape) {
		shape->add_owner(this);
	}
}

FlexPrimitiveShape *FlexPrimitiveBody::get_shape() const {
	return shape;
}

void FlexPrimitiveBody::notify_shape_changed() {
	changed_parameters |= eChangedPrimitiveBodyParamShape;
}

void FlexPrimitiveBody::set_transform(const Transform &p_transf, bool p_is_teleport) {
	transf = p_transf;
	changed_parameters |= eChangedPrimitiveBodyParamTransform;
	if (!p_is_teleport)
		changed_parameters |= eChangedPrimitiveBodyParamTransformIsMotion;
}

void FlexPrimitiveBody::set_layer(uint32_t p_layer) {
	layer = p_layer & 0x7f; // Accept only the first 7 bit
	changed_parameters != eChangedPrimitiveBodyParamFlags;
}

void FlexPrimitiveBody::set_kinematic(bool p_kinematic) {
	_is_kinematic = p_kinematic;
	changed_parameters |= eChangedPrimitiveBodyParamFlags;
}

void FlexPrimitiveBody::set_area(bool p_area) {
	_is_area = p_area;
	changed_parameters |= eChangedPrimitiveBodyParamFlags;
}

void FlexPrimitiveBody::set_monitoring_particles_contacts(bool p_monitoring) {
	_is_monitoring_particles_contacts = p_monitoring;
}

void FlexPrimitiveBody::set_clean() {
	changed_parameters = 0;
}

void FlexPrimitiveBody::dispatch_particle_contact(FlexParticleBody *p_body, ParticleIndex p_particle_index, const Vector3 &p_velocity, const Vector3 &p_normal) {
	if (!particles_contact_callback.receiver)
		return;

	const Variant particle_body_object_instance(p_body->get_object_instance());
	const Variant particle((int)p_particle_index);
	const Variant velocity(p_velocity);
	const Variant normal(p_normal);

	const Variant *p[5] = { FlexParticlePhysicsServer::singleton->get_particle_body_commands_variant(p_body), &particle_body_object_instance, &particle, &velocity, &normal };

	static Variant::CallError error;
	particles_contact_callback.receiver->call(particles_contact_callback.method, p, 5, error);
}
