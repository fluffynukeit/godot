/*************************************************************************/
/*  compute_friction.cu                                                  */
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

struct GdFlexExtComputeFrictionCallback{
	NvFlexSolver *solver;

	int primitive_body_count;
	float* d_primitive_inv_transf;
	float* d_primitive_lvelocity;
	float* d_primitive_avelocity;
	float* d_primitive_aabbs;
	float* d_primitive_extent;


	GdFlexExtComputeFrictionCallback(NvFlexSolver* p_solver)
		: solver(p_solver),
		  primitive_body_count(0),
		  d_primitive_inv_transf(NULL),
		  d_primitive_lvelocity(NULL),
		  d_primitive_avelocity(NULL),
		  d_primitive_aabbs(NULL),
		  d_primitive_extent(NULL)
	{}

	~GdFlexExtComputeFrictionCallback(){
		free_all();
	}

	void free_all(){
		cudaFree(d_primitive_inv_transf);
		cudaFree(d_primitive_lvelocity);
		cudaFree(d_primitive_avelocity);
		cudaFree(d_primitive_aabbs);
		cudaFree(d_primitive_extent);

		d_primitive_inv_transf = NULL;
		d_primitive_lvelocity = NULL;
		d_primitive_avelocity = NULL;
		d_primitive_aabbs = NULL;
		d_primitive_extent = NULL;
	}
};

GdFlexExtComputeFrictionCallback *GdFlexExtCreateComputeFrictionCallback(NvFlexSolver *p_solver){
	return new GdFlexExtComputeFrictionCallback(p_solver);
}

void GdFlexExtDestroyComputeFrictionCallback(GdFlexExtComputeFrictionCallback* p_callback){
	delete p_callback;
}

struct GdTransform{
	Matrix33 basis;
	Vec3 origin;


	__device__ Vec3 xform(const Vec3& other) const {

		return Vec3(
					Dot3(basis.cols[0], other) + origin.x,
					Dot3(basis.cols[1], other) + origin.y,
					Dot3(basis.cols[2], other) + origin.z);
	}
};

__device__ bool compute_collision_box(const Vec3& particle_pos,
									  const GdTransform* p_primitive_inv_transf,
									  const Vec3* extent,
									  float radius,
									  float margin){


	Vec3 local_ppos = p_primitive_inv_transf->xform(particle_pos);

	Vec3 closest_point(local_ppos);

	closest_point.x = Min(closest_point.x, extent->x);
	closest_point.x = Max(closest_point.x, -extent->x);

	closest_point.y = Min(closest_point.y, extent->y);
	closest_point.y = Max(closest_point.y, -extent->y);

	closest_point.z = Min(closest_point.z, extent->z);
	closest_point.z = Max(closest_point.z, -extent->z);

	Vec3 collision_dist = local_ppos - closest_point;

	float squared_dist = collision_dist.x * collision_dist.x +
						 collision_dist.y * collision_dist.y +
						 collision_dist.z * collision_dist.z;


	if(squared_dist > radius * radius + margin){
		return false;
	}

	return true;
}

__device__ bool AABB_intersect(Vec3 aabb1_min,
							   Vec3 aabb1_max,
							   Vec3 aabb2_min,
							   Vec3 aabb2_max) {

	if(aabb1_min.x >= aabb2_max.x)
		return false;

	if(aabb1_max.x <= aabb2_min.x)
		return false;

	if(aabb1_min.y >= aabb2_max.y)
		return false;

	if(aabb1_max.y <= aabb2_min.y)
		return false;

	if(aabb1_min.z >= aabb2_max.z)
		return false;

	if(aabb1_max.z <= aabb2_min.z)
		return false;

	return true;
}

__global__ void ComputeFriction(float dt,
								Vec4* __restrict__ positions,
								Vec4* __restrict__ velocities,
								int p_primitive_body_count,
								const Vec3* p_primitive_aabbs,
								const GdTransform* p_primitive_inv_transf,
								const Vec3* p_extent ){

	const int i = blockIdx.x * blockDim.x + threadIdx.x;
	const int sorted_particle_index = i;

	const float margin = 0.005;
	const float radius = 0.1;
	const Vec3 radius_vec(radius, radius, radius);

	const Vec4 pos_4 = positions[sorted_particle_index];
	const Vec3 pos = Vec3(pos_4.x, pos_4.y, pos_4.z);

	Vec3 particle_aabb_begin = pos - radius_vec;
	Vec3 particle_aabb_end = pos + radius;

	const Vec4 vel_4 = velocities[sorted_particle_index];
	Vec3 velocity = Vec3(vel_4.x, vel_4.y, vel_4.z);

	for(int p = 0; p < p_primitive_body_count; p++ ){

		if( !AABB_intersect(particle_aabb_begin,
						   particle_aabb_end,
						   p_primitive_aabbs[p*2+0],
						   p_primitive_aabbs[p*2+0] + p_primitive_aabbs[p*2+1]) )
			continue;

		const bool collided = compute_collision_box(pos,
													p_primitive_inv_transf + p,
													p_extent + p,
													radius,
													margin);
		if( !collided )
			continue;

		// TODO take care of normal and compute friction force
		// Just a test
		velocity = Vec3(0, 0, 0);
	}

	velocities[sorted_particle_index] = Vec4(velocity.x, velocity.y, velocity.z, 0);
}

void ComputeFrictionCallback(NvFlexSolverCallbackParams p_params){

	GdFlexExtComputeFrictionCallback* callback = static_cast<GdFlexExtComputeFrictionCallback*>(p_params.userData);

	const int particle_count = p_params.numActive;
	const int kNumBlocks = (particle_count + kNumThreadsPerBlock - 1) / kNumThreadsPerBlock;

	ComputeFriction<<<kNumBlocks, kNumThreadsPerBlock>>>(p_params.dt,
													(Vec4*)p_params.particles,
													(Vec4*)p_params.velocities,
													callback->primitive_body_count,
													(Vec3*)callback->d_primitive_aabbs,
													(GdTransform*)callback->d_primitive_inv_transf,
													(Vec3*)callback->d_primitive_extent);
}

void GdFlexExtSetComputeFrictionCallback(GdFlexExtComputeFrictionCallback* p_callback,
										 int p_primitive_body_count,
										 float* p_primitive_inv_transform,
										 float* p_primitive_lvelocity,
										 float* p_primitive_avelocity,
										 float* p_primitive_aabbs,
										 float* p_primitive_extent){

	if( p_callback->primitive_body_count != p_primitive_body_count ){

		p_callback->primitive_body_count = p_primitive_body_count;

		p_callback->free_all();

		cudaMalloc(&p_callback->d_primitive_inv_transf,
				   sizeof(float) * p_primitive_body_count * 12); // Transform

		cudaMalloc(&p_callback->d_primitive_lvelocity,
				   sizeof(float) * p_primitive_body_count * 3); // Vector3

		cudaMalloc(&p_callback->d_primitive_avelocity,
				   sizeof(float) * p_primitive_body_count * 3); // Vector3

		cudaMalloc(&p_callback->d_primitive_aabbs,
				   sizeof(float) * p_primitive_body_count * 6); // AABB

		cudaMalloc(&p_callback->d_primitive_extent,
				   sizeof(float) * p_primitive_body_count * 3); // Vector3
	}

	if(!p_primitive_body_count)
		return;

	cudaMemcpy(p_callback->d_primitive_inv_transf,
			   p_primitive_inv_transform,
			   sizeof(float) * p_primitive_body_count * 12,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->d_primitive_lvelocity,
			   p_primitive_lvelocity,
			   sizeof(float) * p_primitive_body_count * 3,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->d_primitive_avelocity,
			   p_primitive_avelocity,
			   sizeof(float) * p_primitive_body_count * 3,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->d_primitive_aabbs,
			   p_primitive_aabbs,
			   sizeof(float) * p_primitive_body_count * 6,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->d_primitive_extent,
			   p_primitive_extent,
			   sizeof(float) * p_primitive_body_count * 3,
			   cudaMemcpyHostToDevice);

	NvFlexSolverCallback solver_callback;
	solver_callback.function = ComputeFrictionCallback;
	solver_callback.userData = p_callback;

	NvFlexRegisterSolverCallback(p_callback->solver,
								 solver_callback,
								 eNvFlexStageSubstepEnd);
}
