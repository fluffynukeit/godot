/*************************************************************************/
/*  flex_primitive_shapes.h                                              */
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

#include "flex_primitive_shapes.h"
#include "flex_primitive_body.h"

FlexPrimitiveShape::FlexPrimitiveShape() :
		RIDFlex() {
}

FlexPrimitiveShape::~FlexPrimitiveShape() {
	for (int i(owners.size() - 1); 0 <= i; --i) {
		owners[i]->set_shape(NULL);
	}
}

FlexPrimitiveBoxShape::FlexPrimitiveBoxShape() :
		FlexPrimitiveShape(),
		extends(1.0, 1.0, 1.0) {}

void FlexPrimitiveShape::add_owner(FlexPrimitiveBody *p_owner) {
	owners.push_back(p_owner);
}

void FlexPrimitiveShape::remove_owner(FlexPrimitiveBody *p_owner) {
	owners.erase(p_owner);
}

void FlexPrimitiveShape::notify_change() {
	for (int i(owners.size() - 1); 0 <= i; --i) {
		owners[i]->notify_shape_changed();
	}
}

void FlexPrimitiveBoxShape::get_shape(NvFlexCollisionGeometry *r_shape) const {
	r_shape->box.halfExtents[0] = extends.x;
	r_shape->box.halfExtents[1] = extends.y;
	r_shape->box.halfExtents[2] = extends.z;
}

void FlexPrimitiveBoxShape::set_data(const Variant &p_data) {
	set_extends(p_data);
}

Variant FlexPrimitiveBoxShape::get_data() const {
	return extends;
}

void FlexPrimitiveBoxShape::set_extends(const Vector3 &p_extends) {
	extends = p_extends;
}
