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

class FlexPrimitiveShape;

enum ChangedPrimitiveBodyParameter {
	eChangedPrimitiveBodyParamShape = 1 << 0,
	eChangedPrimitiveBodyParamTransform = 1 << 1,
	eChangedPrimitiveBodyParamFlags = 1 << 2,

	eChangedPrimitiveBodyParamAll = eChangedPrimitiveBodyParamShape | eChangedPrimitiveBodyParamTransform | eChangedPrimitiveBodyParamFlags
};

class FlexPrimitiveBody : public RIDFlex {

	friend class FlexSpace;

	uint32_t changed_parameters;
	MemoryChunk *geometry_mchunk;

	FlexSpace *space;
	FlexPrimitiveShape *shape;

	bool kinematic; // if false is static

public:
	FlexPrimitiveBody();
	~FlexPrimitiveBody();

	void set_space(FlexSpace *p_space);
	FlexSpace *get_space() const;

	void set_shape(FlexPrimitiveShape *p_shape);
	FlexPrimitiveShape *get_shape() const;
	void notify_shape_changed();

	void set_kinematic(bool p_kinematic);
	bool is_kinematic() const { return kinematic; }

private:
	void set_clean();
};

#endif // FLEX_PRIMITIVE_BODY_H
