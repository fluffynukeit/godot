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

	// Previous frame primitive transform
	float* primitive_prev_transfs;
	float* primitive_inv_prev_transfs;

	// Current invers transform
	float* primitive_inv_curr_transfs;

	// Motion that primitive did to reach current transform
	float* primitive_motion_transfs;

	//float* primitive_aabbs;

	float* primitive_extent;
	float* primitive_frictions;
	float* primitive_friction_2_thresholds;
	uint32_t* primitive_layers;
	float primitive_margin;

	int particle_count;
	float* prev_particles_position_mass;

	float particle_radius;


	GdFlexExtComputeFrictionCallback(NvFlexSolver* p_solver)
		: solver(p_solver),
		  primitive_body_count(0),
		  primitive_prev_transfs(NULL),
		  primitive_inv_prev_transfs(NULL),
		  primitive_inv_curr_transfs(NULL),
		  primitive_motion_transfs(NULL),
		  //primitive_aabbs(NULL),
		  primitive_extent(NULL),
		  primitive_frictions(NULL),
		  primitive_friction_2_thresholds(NULL),
		  primitive_layers(NULL),
		  primitive_margin(0.01),
		  particle_count(0),
		  prev_particles_position_mass(NULL),
		  particle_radius(0.1)
	{}

	~GdFlexExtComputeFrictionCallback(){
		free_primitives();
		free_particles();
	}

	void free_primitives(){
		cudaFree(primitive_prev_transfs);
		cudaFree(primitive_inv_prev_transfs);
		cudaFree(primitive_inv_curr_transfs);
		cudaFree(primitive_motion_transfs);
		//cudaFree(primitive_aabbs);
		cudaFree(primitive_extent);
		cudaFree(primitive_frictions);
		cudaFree(primitive_friction_2_thresholds);
		cudaFree(primitive_layers);

		primitive_prev_transfs = NULL;
		primitive_inv_prev_transfs = NULL;
		primitive_inv_curr_transfs = NULL;
		primitive_motion_transfs = NULL;
		//primitive_aabbs = NULL;
		primitive_extent = NULL;
		primitive_frictions = NULL;
		primitive_friction_2_thresholds = NULL;
		primitive_layers = NULL;

		primitive_body_count = 0;
	}

	void free_particles(){
		cudaFree(prev_particles_position_mass);
		prev_particles_position_mass = NULL;

		particle_count = 0;
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

	/// The last component of vector is ignored
	__device__ Vec3 xform(const Vec4& other) const {

		return Vec3(
					Dot3(basis.cols[0], other) + origin.x,
					Dot3(basis.cols[1], other) + origin.y,
					Dot3(basis.cols[2], other) + origin.z);
	}

	__device__ void translate(const Vec3& vec){
		origin.x += Dot3(basis.cols[0], vec);
		origin.y += Dot3(basis.cols[1], vec);
		origin.z += Dot3(basis.cols[2], vec);
	}

	__device__ GdTransform translated(const Vec3& vec) const {
		GdTransform t = *this;
		t.translate(vec);
		return t;
	}

	__device__ void operator*=(const GdTransform &p_transform) {

		origin = xform(p_transform.origin);
		basis = Multiply(basis, p_transform.basis);
	}

	__device__ GdTransform operator*(const GdTransform &p_transform) const {

		GdTransform t = *this;
		t *= p_transform;
		return t;
	}
};

__device__ float get_sphere_penetration(
		const Vec3 &box_half_extent,
		const Vec3 &sphere_rel_pos,
		Vec3 &r_closest_point,
		Vec3 &r_normal ){

	//project the center of the sphere on the closest face of the box
	float face_dist = box_half_extent.x - sphere_rel_pos.x;
	float min_dist = face_dist;
	r_closest_point = sphere_rel_pos;
	r_closest_point.x = box_half_extent.x;
	r_normal.x = 1.0;
	r_normal.y = 0.0;
	r_normal.z = 0.0;

	face_dist = box_half_extent.x + sphere_rel_pos.x;
	if (face_dist < min_dist){
		min_dist = face_dist;
		r_closest_point = sphere_rel_pos;
		r_closest_point.x = -box_half_extent.x;
		r_normal.x = -1.0;
		r_normal.y = 0.0;
		r_normal.z = 0.0;
	}

	face_dist = box_half_extent.y - sphere_rel_pos.y;
	if (face_dist < min_dist)
	{
		min_dist = face_dist;
		r_closest_point = sphere_rel_pos;
		r_closest_point.y = box_half_extent.y;
		r_normal.x = 0.0;
		r_normal.y = 1.0;
		r_normal.z = 0.0;
	}

	face_dist = box_half_extent.y + sphere_rel_pos.y;
	if (face_dist < min_dist)
	{
		min_dist = face_dist;
		r_closest_point = sphere_rel_pos;
		r_closest_point.y = -box_half_extent.y;
		r_normal.x = 0.0;
		r_normal.y = -1.0;
		r_normal.z = 0.0;
	}

	face_dist = box_half_extent.z - sphere_rel_pos.z;
	if (face_dist < min_dist)
	{
		min_dist = face_dist;
		r_closest_point = sphere_rel_pos;
		r_closest_point.z = box_half_extent.z;
		r_normal.x = 0.0;
		r_normal.y = 0.0;
		r_normal.z = 1.0;
	}

	face_dist = box_half_extent.z + sphere_rel_pos.z;
	if (face_dist < min_dist)
	{
		min_dist = face_dist;
		r_closest_point = sphere_rel_pos;
		r_closest_point.z = -box_half_extent.z;
		r_normal.x = 0.0;
		r_normal.y = 0.0;
		r_normal.z = -1.0;
	}

	return min_dist;
}

__device__ float length_2(const Vec3 &vec){

	return
			vec.x * vec.x +
			vec.y * vec.y +
			vec.z * vec.z;
}

__device__ bool compute_collision_box(const Vec4& particle_pos,
									  const GdTransform* p_primitive_inv_transf,
									  const Vec3* extent,
									  float radius,
									  float margin,
									  float *r_penetration_dist = NULL,
									  Vec3 *r_normal = NULL,
									  Vec3 *r_closest_point_on_box = NULL){


	Vec3 local_ppos = p_primitive_inv_transf->xform(particle_pos);

	Vec3 closest_point(local_ppos);

	closest_point.x = Min(closest_point.x, extent->x);
	closest_point.x = Max(closest_point.x, -extent->x);

	closest_point.y = Min(closest_point.y, extent->y);
	closest_point.y = Max(closest_point.y, -extent->y);

	closest_point.z = Min(closest_point.z, extent->z);
	closest_point.z = Max(closest_point.z, -extent->z);

	Vec3 collision_point = local_ppos - closest_point;

	const float squared_dist = length_2(collision_point);

	const bool collision = squared_dist < radius * radius + margin;

	if(collision){

		if(r_penetration_dist && r_normal && r_closest_point_on_box){

			Vec3 normal(0, 0, 0);
			if(squared_dist <= FLT_EPSILON){
				// When the distance is lower than epsilon the sphere center is
				// inside the box
				(*r_penetration_dist) =
						-get_sphere_penetration(
							*extent,
							local_ppos,
							closest_point,
							normal);
				normal *= -1;

			}else{
				float dist = sqrt(squared_dist);
				(*r_penetration_dist) = dist - radius;

				if (dist != 0) {
					normal = collision_point / dist;
				}
			}

			(*r_closest_point_on_box) = closest_point;
			(*r_normal) = normal;
		}

	}

	return collision;
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

__device__ Vec3 get_contact_velocity(
		float inverse_dt,
		const Vec4 &f,
		const Vec4 &t,
		float friction){

	const float alpha = 1 - friction;

	return Vec3(
			(alpha * (t.x - f.x)) * inverse_dt,
			(alpha * (t.y - f.y)) * inverse_dt,
			(alpha * (t.z - f.z)) * inverse_dt);
}

__device__ Vec4 linear_interpolation(
		const Vec4 &f,
		const Vec4 &t,
		float alpha){

	return Vec4(
			f.x + (alpha * (t.x - f.x)),
			f.y + (alpha * (t.y - f.y)),
			f.z + (alpha * (t.z - f.z)),
			f.w);
}

__global__ void ComputeFriction(
		const float p_inverse_dt,
		const int *p_sorted_to_original,
		Vec4* __restrict__ r_particle_positions,
		Vec4* __restrict__ r_particle_velocities,
		const int* p_particle_phases,
		const int p_primitive_body_count,
		const GdTransform *p_primitive_prev_transfs,
		const GdTransform *p_primitive_inv_prev_transfs,
		const GdTransform *p_primitive_inv_curr_transfs,
		const GdTransform *p_primitive_motion_transfs,
		//const Vec3 *p_primitive_aabbs,
		const Vec3 *p_primitive_extents,
		const float *p_primitive_frictions,
		const float *p_primitive_friction_2_thresholds,
		const uint32_t* p_primitive_layers,
		const float p_primitive_margin,
		const Vec4* p_prev_particles_position_mass,
		const float p_particle_radius ){

	const int i = blockIdx.x * blockDim.x + threadIdx.x;
	const int sorted_particle_index = i;

	const int original_particle_index = p_sorted_to_original[sorted_particle_index];

	const Vec4 curr_particle_pos = r_particle_positions[sorted_particle_index];
	const int phase = p_particle_phases[sorted_particle_index];
	const int particle_layer = phase >> 24;

	const Vec4 prev_particle_pos = p_prev_particles_position_mass[original_particle_index];

	//TEST_AABB
	//const Vec3 radius_vec(radius, radius, radius);
	//const Vec3 current_p_pos_vec3 = Vec3(current_p_pos.x, current_p_pos.y, current_p_pos.z);
	//
	//Vec3 particle_aabb_begin = current_p_pos - radius_vec;
	//Vec3 particle_aabb_end = current_p_pos + radius;

	for(int p = 0; p < p_primitive_body_count; p++ ){

		const float friction = p_primitive_frictions[p];
		const float friction_threshold = p_primitive_friction_2_thresholds[p];
		const uint32_t layer = p_primitive_layers[p];

		if(!(layer & particle_layer))
			continue;

		//if( !AABB_intersect(
		//		particle_aabb_begin,
		//		particle_aabb_end,
		//		p_primitive_aabbs[p*2+0],
		//		p_primitive_aabbs[p*2+0] + p_primitive_aabbs[p*2+1]) )
		//	continue;

		/// NOTE
		/// To make the calculation more lightweight the two cases when
		/// the particle enter or exit are not properly handled.
		/// This mean that when the particle exit the surface this algorithm
		/// doesn't compute the friction.
		/// However this is not a big problem thanks to high frame rate

		/// Step 1
		/// Check if the particle collide or is escaped from the rough surface



		if( !compute_collision_box(
					curr_particle_pos,
					p_primitive_inv_curr_transfs + p,
					p_primitive_extents + p,
					p_particle_radius,
					p_primitive_margin) )
			continue;

		/// Step 2
		/// Check if previously the particle was colliding
		///
		/// This can't be cached due to the fact that the particles ID
		/// can change frame by frame. (More work should be required)

		Vec3 normal;
		Vec3 closest_point_on_box;
		float penetration;

		if( !compute_collision_box(
					prev_particle_pos,
					p_primitive_inv_prev_transfs + p,
					p_primitive_extents + p,
					p_particle_radius,
					p_primitive_margin,
					&penetration,
					&normal,
					&closest_point_on_box) )
			continue;

		/// Step 3
		/// Compute motion of primitive body with particle
		/// attached to the safe position to prevent overlapping

		Vec3 safe_relative_prev_particle_pos =
				closest_point_on_box + normal * p_particle_radius;
				//closest_point_on_box + normal * (p_particle_radius + p_primitive_margin);

		GdTransform full_friction_particle_trs =
				p_primitive_prev_transfs[p].translated(safe_relative_prev_particle_pos) *
				p_primitive_motion_transfs[p];

		/// Step 4
		/// Calculate new position and velocity according to
		/// static and and dynamic friction

		Vec3 delta_movement(
					curr_particle_pos.x - full_friction_particle_trs.origin.x,
					curr_particle_pos.y - full_friction_particle_trs.origin.y,
					curr_particle_pos.z - full_friction_particle_trs.origin.z);

		float alpha = 0; // Static friction

		if(friction_threshold < length_2(delta_movement))
			alpha = 1 - friction; // Dynamic friction

		delta_movement *= alpha;

		const Vec4 new_p_pos(full_friction_particle_trs.origin + delta_movement, curr_particle_pos.w);

		r_particle_positions[sorted_particle_index] = new_p_pos;

		r_particle_velocities[sorted_particle_index] =
				(new_p_pos - prev_particle_pos) * p_inverse_dt;
		// last (W) is not used but reset it
		r_particle_velocities[sorted_particle_index][3] = 0;

		// TODO in case of multi contact this is no more correct
		// For this reason allow only one contact
		return;
	}
}

void ComputeFrictionCallback(NvFlexSolverCallbackParams p_params){

	GdFlexExtComputeFrictionCallback* callback =
			static_cast<GdFlexExtComputeFrictionCallback*>(p_params.userData);

	const float inverse_dt = 1.0 / p_params.dt;
	const int particle_count = p_params.numActive;
	const int kNumBlocks = (particle_count + kNumThreadsPerBlock - 1) / kNumThreadsPerBlock;

	ComputeFriction<<<kNumBlocks, kNumThreadsPerBlock>>>(
			inverse_dt,
			p_params.sortedToOriginalMap,
			(Vec4*)p_params.particles,
			(Vec4*)p_params.velocities,
			p_params.phases,
			callback->primitive_body_count,
			(GdTransform*)callback->primitive_prev_transfs,
			(GdTransform*)callback->primitive_inv_prev_transfs,
			(GdTransform*)callback->primitive_inv_curr_transfs,
			(GdTransform*)callback->primitive_motion_transfs,
			//(Vec3*)callback->primitive_aabbs,
			(Vec3*)callback->primitive_extent,
			callback->primitive_frictions,
			callback->primitive_friction_2_thresholds,
			callback->primitive_layers,
			callback->primitive_margin,
			(Vec4*)callback->prev_particles_position_mass,
			callback->particle_radius);
}

void GdFlexExtSetComputeFrictionCallback(
		GdFlexExtComputeFrictionCallback* p_callback,
		const int p_primitive_body_count,
		const float *p_primitive_prev_transfs,
		const float *p_primitive_inv_prev_transfs,
		const float *p_primitive_inv_curr_transfs,
		const float *p_primitive_motions,
		//const float *p_primitive_aabbs,
		const float *p_primitive_extents,
		const float *p_primitive_frictions,
		const float *p_primitive_friction_2_thresholds,
		const uint32_t * p_primitive_layers,
		const float p_primitive_margin,
		const int p_particle_count,
		const float *p_prev_particles_position_mass,
		const float p_particle_radius){

	if( p_callback->primitive_body_count != p_primitive_body_count ){

		p_callback->free_primitives();

		p_callback->primitive_body_count = p_primitive_body_count;

		if(p_primitive_body_count){
			cudaMalloc(&p_callback->primitive_prev_transfs,
					   sizeof(float) * p_primitive_body_count * 12); // Transform

			cudaMalloc(&p_callback->primitive_inv_prev_transfs,
					   sizeof(float) * p_primitive_body_count * 12); // Transform

			cudaMalloc(&p_callback->primitive_inv_curr_transfs,
					   sizeof(float) * p_primitive_body_count * 12); // Transform

			cudaMalloc(&p_callback->primitive_motion_transfs,
					   sizeof(float) * p_primitive_body_count * 12); // Transform

			//cudaMalloc(&p_callback->primitive_aabbs,
			//		   sizeof(float) * p_primitive_body_count * 6); // AABB

			cudaMalloc(&p_callback->primitive_extent,
					   sizeof(float) * p_primitive_body_count * 3); // Vector3

			cudaMalloc(&p_callback->primitive_frictions,
					   sizeof(float) * p_primitive_body_count * 1); // Float

			cudaMalloc(&p_callback->primitive_friction_2_thresholds,
					   sizeof(float) * p_primitive_body_count * 1); // Float

			cudaMalloc(&p_callback->primitive_layers,
					   sizeof(uint32_t) * p_primitive_body_count * 1); // unsigned int
		}
	}

	if(p_callback->particle_count != p_particle_count){

		p_callback->free_particles();
		p_callback->particle_count = p_particle_count;

		if(p_particle_count){
			cudaMalloc(
					&p_callback->prev_particles_position_mass,
					sizeof(float) * p_particle_count * 4 ); // Vector4
		}
	}

	if(!p_primitive_body_count)
		return;

	if(!p_particle_count)
		return;

	cudaMemcpy(p_callback->primitive_prev_transfs,
			   p_primitive_prev_transfs,
			   sizeof(float) * p_primitive_body_count * 12,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->primitive_inv_prev_transfs,
			   p_primitive_inv_prev_transfs,
			   sizeof(float) * p_primitive_body_count * 12,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->primitive_inv_curr_transfs,
			   p_primitive_inv_curr_transfs,
			   sizeof(float) * p_primitive_body_count * 12,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->primitive_motion_transfs,
			   p_primitive_motions,
			   sizeof(float) * p_primitive_body_count * 12,
			   cudaMemcpyHostToDevice);

	//cudaMemcpy(p_callback->d_primitive_aabbs,
	//		   p_primitive_aabbs,
	//		   sizeof(float) * p_primitive_body_count * 6,
	//		   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->primitive_extent,
			   p_primitive_extents,
			   sizeof(float) * p_primitive_body_count * 3,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->primitive_frictions,
			   p_primitive_frictions,
			   sizeof(float) * p_primitive_body_count * 1,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->primitive_friction_2_thresholds,
			   p_primitive_friction_2_thresholds,
			   sizeof(float) * p_primitive_body_count * 1,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->primitive_layers,
			   p_primitive_layers,
			   sizeof(uint32_t) * p_primitive_body_count * 1,
			   cudaMemcpyHostToDevice);

	cudaMemcpy(p_callback->prev_particles_position_mass,
			   p_prev_particles_position_mass,
			   sizeof(float) * p_particle_count * 4,
			   cudaMemcpyHostToDevice);


	p_callback->primitive_margin = p_primitive_margin;
	p_callback->particle_radius = p_particle_radius;

	NvFlexSolverCallback solver_callback;
	solver_callback.function = ComputeFrictionCallback;
	solver_callback.userData = p_callback;

	NvFlexRegisterSolverCallback(p_callback->solver,
								 solver_callback,
								 eNvFlexStageSubstepEnd);
}
