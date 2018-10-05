/*************************************************************************/
/*  collision_verifier.cu                                                */
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

/**
 * This is writtein in Cuda C
 */

#include <cuda.h>
#include <cuda_runtime_api.h>

#include "thirdparty/flex/include/NvFlex.h"
#include "thirdparty/flex/core/maths.h"

static const int kNumThreadsPerBlock = 256;

struct GdFlexExtCollisionVerifierCallback{
	NvFlexSolver* solver;

	GdFlexExtCollisionVerifierCallback(NvFlexSolver* p_solver)
		: solver(p_solver)
	{}
};

GdFlexExtCollisionVerifierCallback* GdFlexExtCreateCollisionVerifierCallback(NvFlexSolver* p_solver){
	return new GdFlexExtCollisionVerifierCallback(p_solver);
}

void GdFlexExtDestroyCollisionVerifierCallback(GdFlexExtCollisionVerifierCallback* p_callback){
	return delete p_callback;
}

__global__ void CollisionVerify(int numParticles, const Vec4* __restrict__ positions, Vec4* __restrict__ velocities, float dt){

	const int i = blockIdx.x*blockDim.x + threadIdx.x;
	const int particle_index = i;



}

void CollisionVerifierCallback(NvFlexSolverCallbackParams p_params){
	const int particle_count = p_params.numActive;

	const int kNumBlocks = (particle_count+kNumThreadsPerBlock-1)/kNumThreadsPerBlock;

	CollisionVerify<<<kNumBlocks, kNumThreadsPerBlock>>>(p_params.numActive, (Vec4*)p_params.particles, (Vec4*)p_params.velocities, p_params.dt);
}

void GdFlexExtSetCollisionVerifierCallback(GdFlexExtCollisionVerifierCallback *p_callback){
	NvFlexSolverCallback callback;
	callback.function = CollisionVerifierCallback;
	NvFlexRegisterSolverCallback(p_callback->solver, callback, eNvFlexStageUpdateEnd);
}
