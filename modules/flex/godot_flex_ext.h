/*************************************************************************/
/*  godot_flex_ext.cpp                                                   */
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

#ifndef GODOT_FLEX_EXT_H
#define GODOT_FLEX_EXT_H

#include "NvFlex.h"

struct GdFlexExtCollisionVerifierCallback;

/**
 * Add a task that is executed at the end of NvFlexUpdateSolver.
 * It set a boolean variable depending if the particle body has a collision or not
 *
 * @param[in] solver A valid solver created with NvFlexCreateSolver()
 * @return
 */
GdFlexExtCollisionVerifierCallback *GdFlexExtCreateCollisionVerifierCallback(NvFlexSolver *solver);

/**
 * Destroy the GdFlexExtCollisionVerifierCallback callback
 *
 * @param p_callback pointer to callback structure
 */
void GdFlexExtDestroyCollisionVerifierCallback(GdFlexExtCollisionVerifierCallback *p_callback);

/**
 * Set the callback that verify the collisions
 *
 * @param p_callback pointer to callback structure
 */
void GdFlexExtSetCollisionVerifierCallback(GdFlexExtCollisionVerifierCallback *p_callback);

struct GdFlexExtComputeAABBCallback;

/**
 * Craete callback struct that halds info about Compute AABB task
 * @param p_solver valid solver
 * @return struct callback
 */
GdFlexExtComputeAABBCallback *GdFlexExtCreateComputeAABBCallback(NvFlexSolver *p_solver);

/**
 * Destroy the callback
 *
 * @param p_callback valid pointer to the callback created by GdFlexExtCreateComputeAABBCallback
 */
void GdFlexExtDestroyComputeAABBCallback(GdFlexExtComputeAABBCallback *p_callback);

/**
 * Set the callback that compute particle body AABB
 *
 * @param p_callback a valid pointer created by GdFlexExtCreateCollisionVerifierCallback
 * @param p_particle_body_count The number of particle body
 * @param p_last_pindex_particle_body The Array (size = p_particle_body_count) of last particle index for each body
 * @param p_aabbs The Array (size = p_particle_body_count) with initial AABB
 */
void GdFlexExtSetComputeAABBCallback(GdFlexExtComputeAABBCallback *p_callback, int p_particle_body_count, int *p_last_pindex_particle_body, float *p_aabbs);

/**
 * Callback struct used to hold compute friction callback
 */
struct GdFlexExtComputeFrictionCallback;

GdFlexExtComputeFrictionCallback *GdFlexExtCreateComputeFrictionCallback(NvFlexSolver *p_solver);

void GdFlexExtDestroyComputeFrictionCallback(GdFlexExtComputeFrictionCallback *p_callback);

void ComputeFrictionCallback(NvFlexSolverCallbackParams p_params);

void GdFlexExtSetComputeFrictionCallback(GdFlexExtComputeFrictionCallback *p_callback, int p_primitive_body_count, float *p_primitive_transform, float *p_primitive_lvelocity, float *p_primitive_avelocity, float *p_primitive_aabbs, float *p_primitive_extent);

#endif // GODOT_FLEX_EXT_H
