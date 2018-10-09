/*************************************************************************/
/*  flex_primitive_body.h                                                */
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

#ifndef FLEX_PRIMITIVE_BODY_H
#define FLEX_PRIMITIVE_BODY_H

#include "flex_space.h"
#include "rid_flex.h"

#include "servers/particle_physics_server.h"

class FlexPrimitiveShape;

enum ChangedPrimitiveBodyParameter {
	eChangedPrimitiveBodyParamShape = 1 << 0,
	eChangedPrimitiveBodyParamTransform = 1 << 1,
	eChangedPrimitiveBodyParamTransformIsMotion = 1 << 2, // This is applied in addition to the Transform to tell that change is a movement and not a teleport
	eChangedPrimitiveBodyParamFlags = 1 << 3,

	eChangedPrimitiveBodyParamAll = eChangedPrimitiveBodyParamShape | eChangedPrimitiveBodyParamTransform | eChangedPrimitiveBodyParamFlags
};

class FlexPrimitiveBody : public RIDFlex {

	friend class FlexSpace;

	FlexCallBackData sync_callback;
	FlexCallBackData particles_contact_callback;

	uint32_t changed_parameters;
	MemoryChunk *geometry_mchunk;

	FlexSpace *space;
	Object *object_instance;
	FlexPrimitiveShape *shape;

	AABB aabb;
	Transform transf;

	// allow only first 7 bit (max 7 channel)
	uint32_t layer;
	bool _is_kinematic; // if false is static
	bool _is_area;

	bool _is_monitoring_particles_contacts;

public:
	FlexPrimitiveBody();
	~FlexPrimitiveBody();

	void set_space(FlexSpace *p_space);
	FlexSpace *get_space() const;

	void set_object_instance(Object *p_object);
	_FORCE_INLINE_ Object *get_object_instance() const { return object_instance; }

	void set_callback(ParticlePhysicsServer::ParticlePrimitiveBodyCallback p_callback_type, Object *p_receiver, const StringName &p_method);

	void set_shape(FlexPrimitiveShape *p_shape);
	FlexPrimitiveShape *get_shape() const;
	void notify_shape_changed();

	void update_aabb();
	const AABB &get_aabb() { return aabb; }
	void set_transform(const Transform &p_transf, bool p_is_teleport);

	// Accept only the first 7 bit
	void set_layer(uint32_t p_layer);
	uint32_t get_layer() const { return layer; }

	void set_kinematic(bool p_kinematic);
	bool is_kinematic() const { return _is_kinematic; }

	void set_area(bool p_area);
	bool is_area() const { return _is_area; }

	void set_monitoring_particles_contacts(bool p_monitoring);
	_FORCE_INLINE_ bool is_monitoring_particles_contacts() const { return _is_monitoring_particles_contacts; }

	// Internals
	void set_clean();
	void dispatch_particle_contact(FlexParticleBody *p_body, ParticleIndex p_particle_index, const Vector3 &p_velocity, const Vector3 &p_normal);
	void dispatch_sync_callback();
};

#endif // FLEX_PRIMITIVE_BODY_H
