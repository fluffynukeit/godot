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
#include "flex_primitive_shapes.h"

FlexPrimitiveBody::FlexPrimitiveBody() :
		RIDFlex(),
		changed_parameters(0),
		geometry_mchunk(NULL),
		space(NULL),
		shape(NULL),
		_is_kinematic(false),
		_is_area(false),
		layer(1),
		_is_monitoring_particles(false) {
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

void FlexPrimitiveBody::set_clean() {
	changed_parameters = 0;
}

void FlexPrimitiveBody::set_monitoring_particles(bool p_monitoring) {
	_is_monitoring_particles = p_monitoring;
}
