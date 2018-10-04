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

/*
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
}*/

// ------------------------
// Collision verifier

struct GdFlexExtComputeAABBCallback{
	NvFlexSolver *solver;

	int particle_body_count;
	float* aabbs;

	// Device memory
	int* d_last_pindex_particle_body; // Size of particle_body_count
	float* d_aabbs; // Size of particle_body_count * size(Vec3)


	GdFlexExtComputeAABBCallback(NvFlexSolver* p_solver)
		: solver(p_solver),
		  particle_body_count(0),
		  aabbs(NULL),
		  d_last_pindex_particle_body(NULL),
		  d_aabbs(NULL)
	{}

	~GdFlexExtComputeAABBCallback(){

		if(d_last_pindex_particle_body)
			cudaFree(d_last_pindex_particle_body);

		if(d_aabbs)
			cudaFree(d_aabbs);
	}
};

GdFlexExtComputeAABBCallback *GdFlexExtCreateComputeAABBCallback(NvFlexSolver *p_solver){
	return new GdFlexExtComputeAABBCallback(p_solver);
}

void GdFlexExtDestroyComputeAABBCallback(GdFlexExtComputeAABBCallback* p_callback){
	delete p_callback;
}

__global__ void ComputeAABB(const Vec4* __restrict__ positions, Vec4* __restrict__ velocities, const int* __restrict__ p_sorted_to_original_map, int p_particle_body_count, int* p_last_pindex_particle_body, Vector3* p_aabbs){

	const int i = blockIdx.x * blockDim.x + threadIdx.x;
	const int sorted_particle_index = i;
	const int particle_index = p_sorted_to_original_map[sorted_particle_index];

	velocities[sorted_particle_index] = Vec4(0,0,0,0);

	// Search the particle body owner
	int particle_body_index = -1;
	for( int y = 0; y < p_particle_body_count; ++y ){
		if( particle_index <= p_last_pindex_particle_body[y] ){
			particle_body_index = y;
			break;
		}
	}

	if( particle_body_index == -1 )
		return;

	const Vector3 other_vector = Vector3(positions[sorted_particle_index].x, positions[sorted_particle_index].y, positions[sorted_particle_index].z);

	Vector3 begin = ((Vector3*)p_aabbs)[particle_body_index * 2 + 0];
	Vector3 end = begin + ((Vector3*)p_aabbs)[particle_body_index * 2 + 1];

	if (other_vector.x < begin.x)
		begin.x = other_vector.x;
	if (other_vector.y < begin.y)
		begin.y = other_vector.y;
	if (other_vector.z < begin.z)
		begin.z = other_vector.z;

	if (other_vector.x > end.x)
		end.x = other_vector.x;
	if (other_vector.y > end.y)
		end.y = other_vector.y;
	if (other_vector.z > end.z)
		end.z = other_vector.z;

	((Vector3*)p_aabbs)[particle_body_index*2+0] = begin;
	((Vector3*)p_aabbs)[particle_body_index*2+1] = end - begin;

	//((Vector3*)p_aabbs)[particle_body_index*2+0] = Vector3(-99999,-99999,-99999);
	//((Vector3*)p_aabbs)[particle_body_index*2+1] = Vector3(99999,99999,99999);
}

void ComputeAABBCallback(NvFlexSolverCallbackParams p_params){

	GdFlexExtComputeAABBCallback* callback = static_cast<GdFlexExtComputeAABBCallback*>(p_params.userData);

	const int particle_count = p_params.numActive;
	const int kNumBlocks = (particle_count+kNumThreadsPerBlock-1)/kNumThreadsPerBlock;

	ComputeAABB<<<kNumBlocks, kNumThreadsPerBlock>>>((Vec4*)p_params.particles,
													 (Vec4*)p_params.velocities,
													 p_params.sortedToOriginalMap,
													 callback->particle_body_count,
													 callback->d_last_pindex_particle_body,
													 (Vector3*)callback->d_aabbs);

	cudaMemcpy(callback->aabbs, callback->d_aabbs, sizeof(float) * 2 * 3 * callback->particle_body_count, cudaMemcpyDeviceToHost);
	//callback->aabbs[0] = -99999;
	callback->aabbs = NULL;
}

void GdFlexExtSetComputeAABBCallback(GdFlexExtComputeAABBCallback* p_callback, int p_particle_body_count, int* p_last_pindex_particle_body, float* p_aabbs){

	p_callback->aabbs = p_aabbs;

	if( p_callback->particle_body_count != p_particle_body_count ){

		p_callback->particle_body_count = p_particle_body_count;

		if( p_callback->d_last_pindex_particle_body )
			cudaFree(p_callback->d_last_pindex_particle_body);

		if( p_callback->d_aabbs )
			cudaFree(p_callback->d_aabbs);

		cudaMalloc(&p_callback->d_last_pindex_particle_body, sizeof(int)*p_particle_body_count);
		cudaMalloc(&p_callback->d_aabbs, sizeof(float) * 2 * 3 * p_particle_body_count);
	}

	cudaMemcpyAsync(p_callback->d_last_pindex_particle_body, p_last_pindex_particle_body, sizeof(int)*p_particle_body_count, cudaMemcpyHostToDevice);
	cudaMemcpyAsync(p_callback->d_aabbs, p_aabbs, sizeof(float) * 2 * 3 * p_particle_body_count, cudaMemcpyHostToDevice);

	p_aabbs[0] = -99999;

	NvFlexSolverCallback solver_callback;
	solver_callback.function = ComputeAABBCallback;
	solver_callback.userData = p_callback;

	NvFlexRegisterSolverCallback(p_callback->solver, solver_callback, eNvFlexStageUpdateEnd);
}
