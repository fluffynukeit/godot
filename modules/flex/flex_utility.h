/*************************************************************************/
/*  flex_utility.h                                                        */
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

#ifndef FLEX_UTILITY_H
#define FLEX_UTILITY_H

// Change the name used intenally by flex in order to avoid colliding with Godot
#define Vector2 FlVector2
#define Vec2 FlVec2
#define Vector3 FlVector3
#define Vec3 FlVec3
#define Vector4 FlVector4
#define Vec4 FlVec4
#define Quat FlQuat
#define Plane FlPlane
#define Transform FlTransform

#include "thirdparty/flex/core/maths.h"

#undef Vector2
#undef Vec2
#undef Vector3
#undef Vec3
#undef Vector4
#undef Vec4
#undef Quat
#undef Plane
#undef Transform

#include "core/string_db.h"
#include "flex_memory_allocator.h"
#include "math_defs.h"
#include "vector3.h"

typedef int ParticleIndex; // Particle index relative to body, can change during time
typedef int ParticleBufferIndex; // Particle global index, can change during time
typedef int SpringIndex; // Spring index relative to body, can change during time
typedef int SpringBufferIndex; // Spring index global index, can change during time

#define CreateParticle(position, mass) \
    FlVector4(position.x, position.y, position.z, mass ? (1 / mass) : 0)

#define gvec3_from_fvec4(particle) \
    Vector3(particle[0], particle[1], particle[2])

struct Spring {
    ParticleBufferIndex index0;
    ParticleBufferIndex index1;

    Spring() :
            index0(0),
            index1(0) {}

    Spring(ParticleBufferIndex p_index0, ParticleBufferIndex p_index1) :
            index0(p_index0),
            index1(p_index1) {}
};

struct FlexCallBackData {
    Object *receiver;
    StringName method;

    FlexCallBackData() :
            receiver(NULL) {}
};

#define make_memory_index(p_chunk, p_index)         \
    int index = p_chunk->get_buffer_index(p_index); \
    ERR_FAIL_COND(index > p_chunk->get_end_index());

#define make_memory_index_V(p_chunk, p_index, ret)  \
    int index = p_chunk->get_buffer_index(p_index); \
    ERR_FAIL_COND_V(index > p_chunk->get_end_index(), ret);

static FlVector4 return_err_flvec4(0, 0, 0, 0);
static Vector3 return_err_vec3(0, 0, 0);
static Spring return_err_spring(-1, -1);

#endif // FLEX_UTILITY_H
