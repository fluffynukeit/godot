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

#include "core/math/quat.h"
#include "core/math/transform.h"
#include "core/math/vector3.h"
#include "core/string_db.h"
#include "math_defs.h"

typedef int FlexUnit;

#define define_flex_integer(clazz_name)                                    \
	struct clazz_name {                                                    \
		FlexUnit value;                                                    \
																		   \
		clazz_name() {}                                                    \
																		   \
		clazz_name(const clazz_name &p_other) :                            \
				value(p_other.value) {}                                    \
																		   \
		clazz_name(FlexUnit p_value) :                                     \
				value(p_value) {}                                          \
																		   \
		void operator=(const FlexUnit p_value) { value = p_value; }        \
		void operator=(const clazz_name &p_test) { value = p_test.value; } \
																		   \
		bool operator==(clazz_name p_other) {                              \
			return operator==(p_other.value);                              \
		}                                                                  \
		bool operator!=(clazz_name p_other) {                              \
			return operator!=(p_other.value);                              \
		}                                                                  \
																		   \
		bool operator<(clazz_name p_other) {                               \
			return operator<(p_other.value);                               \
		}                                                                  \
		bool operator>(clazz_name p_other) {                               \
			return operator>(p_other.value);                               \
		}                                                                  \
																		   \
		bool operator<=(clazz_name p_other) {                              \
			return operator<=(p_other.value);                              \
		}                                                                  \
		bool operator>=(clazz_name p_other) {                              \
			return operator>=(p_other.value);                              \
		}                                                                  \
																		   \
		operator int() const { return value; }                             \
																		   \
		clazz_name operator+(FlexUnit p_value) {                           \
			clazz_name tmp(*this);                                         \
			tmp.value += p_value;                                          \
			return tmp;                                                    \
		}                                                                  \
		clazz_name operator-(FlexUnit p_value) {                           \
			clazz_name tmp(*this);                                         \
			tmp.value -= p_value;                                          \
			return tmp;                                                    \
		}                                                                  \
																		   \
		void operator+=(FlexUnit p_value) {                                \
			value += p_value;                                              \
		}                                                                  \
																		   \
		void operator-=(FlexUnit p_value) {                                \
			value -= p_value;                                              \
		}                                                                  \
																		   \
		bool operator==(FlexUnit p_value) {                                \
			return value == p_value;                                       \
		}                                                                  \
																		   \
		bool operator!=(FlexUnit p_value) {                                \
			return !(operator==(p_value));                                 \
		}                                                                  \
																		   \
		bool operator<(FlexUnit p_value) {                                 \
			return value < p_value;                                        \
		}                                                                  \
		bool operator>(FlexUnit p_value) {                                 \
			return value > p_value;                                        \
		}                                                                  \
																		   \
		bool operator<=(FlexUnit p_value) {                                \
			return value <= p_value;                                       \
		}                                                                  \
		bool operator>=(FlexUnit p_value) {                                \
			return value >= p_value;                                       \
		}                                                                  \
																		   \
		clazz_name operator+(clazz_name p_other) {                         \
			return operator+(p_other.value);                               \
		}                                                                  \
		clazz_name operator-(clazz_name p_other) {                         \
			return operator-(p_other.value);                               \
		}                                                                  \
																		   \
		clazz_name &operator++() {                                         \
			++value;                                                       \
			return *this;                                                  \
		}                                                                  \
		clazz_name operator++(FlexUnit) {                                  \
			clazz_name tmp(*this);                                         \
			operator++();                                                  \
			return tmp;                                                    \
		}                                                                  \
																		   \
		clazz_name &operator--() {                                         \
			--value;                                                       \
			return *this;                                                  \
		}                                                                  \
		clazz_name operator--(FlexUnit) {                                  \
			clazz_name tmp(*this);                                         \
			operator--();                                                  \
			return tmp;                                                    \
		}                                                                  \
	}

define_flex_integer(FlexChunkIndex);
define_flex_integer(FlexBufferIndex);

typedef FlexChunkIndex ParticleIndex; // Particle index relative to the memory chunk, can change during time
typedef FlexBufferIndex ParticleBufferIndex; // Particle global index, can change during time
typedef FlexChunkIndex RigidIndex; // Rigid index relative to the memory chunk, can change during time
typedef FlexBufferIndex RigidBufferIndex; // Rigid global index, can change during time
typedef FlexChunkIndex RigidComponentIndex; // Rigid component index relative to the memory chunk, can change during time
typedef FlexBufferIndex RigidComponentBufferIndex; // Rigid component global index, can change during time
typedef FlexChunkIndex ActiveParticleIndex; // Active Particle index relative to the memory chunk, can change during time
typedef FlexBufferIndex ActiveParticleBufferIndex; // Active Particle global index, can change during time
typedef FlexChunkIndex SpringIndex; // Spring index relative to the memory chunk, can change during time
typedef FlexBufferIndex SpringBufferIndex; // Spring global index, can change during time
typedef FlexChunkIndex TriangleIndex; // Triangle index relative to the memory chunk, can change during time
typedef FlexBufferIndex TriangleBufferIndex; // Triangle global index, can change during time
typedef FlexChunkIndex InflatableIndex; // Inflatable index relative to the memory chunk, can change during time
typedef FlexBufferIndex InflatableBufferIndex; // Inflatable global index, can change during time
typedef FlexChunkIndex GeometryIndex; // Geometry index relative to the memory chunk, can change during time
typedef FlexBufferIndex GeometryBufferIndex; // Geometry global index, cavec3_from_flvec4time

typedef int FlexIndex;

#define invert_mass(inverse_mass) \
	inverse_mass ? 1 / inverse_mass : 0

#define vec3_from_flvec4(vec4) \
	Vector3(vec4.x, vec4.y, vec4.z)

#define make_particle(position, mass) \
	FlVector4(position.x, position.y, position.z, invert_mass(mass))

#define extract_position(particle) \
	vec3_from_flvec4(particle)

#define extract_inverse_mass(particle) \
	particle.w

#define extract_mass(particle) \
	invert_mass(extract_inverse_mass(particle))

#define flvec4_from_vec3(vec3) \
	FlVector4(vec3.x, vec3.y, vec3.z, 0)

struct Spring {
	ParticleBufferIndex index0;
	ParticleBufferIndex index1;

	Spring() {}

	Spring(ParticleBufferIndex p_index0, ParticleBufferIndex p_index1) :
			index0(p_index0),
			index1(p_index1) {}
};

struct DynamicTriangle {
	ParticleBufferIndex index0;
	ParticleBufferIndex index1;
	ParticleBufferIndex index2;

	DynamicTriangle() {}

	DynamicTriangle(ParticleBufferIndex p_index0, ParticleBufferIndex p_index1, ParticleBufferIndex p_index2) :
			index0(p_index0),
			index1(p_index1),
			index2(p_index2) {}
};

struct FlexCallBackData {
	Object *receiver;
	StringName method;

	FlexCallBackData() :
			receiver(NULL) {}
};

static const FlVector4 return_err_flvec4(0, 0, 0, 0);
static const Vector3 return_err_vec3(0, 0, 0);
static const Spring return_err_spring(-1, -1);
static const DynamicTriangle return_err_triangle(-1, -1, -1);
static const Quat return_err_quat;

#endif // FLEX_UTILITY_H
