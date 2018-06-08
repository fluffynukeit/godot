/*************************************************************************/
/*  physics_particle_primitive_body.cpp                                  */
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

#include "physics_particle_primitive_body.h"

void ParticlePrimitiveBody::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_shape", "shape"), &ParticlePrimitiveBody::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &ParticlePrimitiveBody::get_shape);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "shape", PROPERTY_HINT_RESOURCE_TYPE, "Shape"), "set_shape", "get_shape");
}

ParticlePrimitiveBody::ParticlePrimitiveBody() :
		ParticleObject(ParticlePhysicsServer::get_singleton()->primitive_body_create()) {
}

void ParticlePrimitiveBody::set_shape(const Ref<Shape> &p_shape) {

	if (!shape.is_null())
		shape->unregister_owner(this);
	shape = p_shape;
	if (!shape.is_null())
		shape->register_owner(this);
	update_gizmo();
	update_configuration_warning();
}

Ref<Shape> ParticlePrimitiveBody::get_shape() const {
	return shape;
}
