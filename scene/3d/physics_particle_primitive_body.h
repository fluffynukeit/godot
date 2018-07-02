/*************************************************************************/
/*  physics_particle_primitive_body.h                                    */
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

#ifndef PARTICLE_PRIMITIVE_BODY_H
#define PARTICLE_PRIMITIVE_BODY_H

#include "physics_particle_object.h"

#include "scene/resources/shape.h"

class ParticlePrimitiveBody : public ParticleObject {

	GDCLASS(ParticlePrimitiveBody, ParticleObject);

	Ref<Shape> shape;

	uint32_t collision_layer;

	// DEBUG
	Node *debug_shape;

protected:
	static void _bind_methods();
	virtual void _notification(int p_what);

public:
	ParticlePrimitiveBody();
	virtual ~ParticlePrimitiveBody();

	void move(const Transform &p_transform);

	void set_shape(const Ref<Shape> &p_shape);
	Ref<Shape> get_shape() const;

	void set_kinematic(bool p_kinematic);
	bool is_kinematic() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_layer_bit(int p_bit, bool p_value);
	bool get_collision_layer_bit(int p_bit) const;

	void set_monitoring_particles(bool p_monitoring);
	bool is_monitoring_particles() const;

protected:
	virtual void on_particle_contact(Object *p_particle_body_commands, Object *p_particle_body, int p_particle_index, Vector3 p_velocity, Vector3 p_normal);

private:
	void _create_debug_shape();
	void resource_changed(RES res);
};

class ParticlePrimitiveArea : public ParticlePrimitiveBody {
	GDCLASS(ParticlePrimitiveArea, ParticlePrimitiveBody);

public:
	ParticlePrimitiveArea();
};

#endif // PARTICLE_PRIMITIVE_BODY_H
