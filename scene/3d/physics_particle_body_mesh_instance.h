/*************************************************************************/
/*  physics_particle_body_mesh_instance.cpp                              */
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
 * @author AndreaCatania
 */

#include "scene/3d/mesh_instance.h"

#ifndef PHYSICS_PARTICLE_BODY_MESH_INSTANCE_H
#define PHYSICS_PARTICLE_BODY_MESH_INSTANCE_H

class ParticleBody;
class ParticleBodyCommands;
class Skeleton;

class ParticleClothVisualServerHandler {

	friend class ParticleBodyMeshInstance;

	RID mesh;
	int surface;
	PoolVector<uint8_t> buffer;
	uint32_t stride;
	uint32_t offset_vertices;
	uint32_t offset_normal;

	PoolVector<int> mesh_indices;

	PoolVector<uint8_t>::Write write_buffer;

private:
	ParticleClothVisualServerHandler();
	bool is_ready() { return mesh.is_valid(); }
	void prepare(RID p_mesh_rid, int p_surface, const Array &p_mesh_arrays);
	void clear();
	void open();
	void close();
	void commit_changes();

public:
	void set_vertex(int p_vertex_id, const void *p_vector3);
	void set_normal(int p_vertex_id, const void *p_vector3);
	void set_aabb(const AABB &p_aabb);

	PoolVector<int> get_mesh_indices() const { return mesh_indices; }
};

class ParticleBodyMeshInstance : public MeshInstance {
	GDCLASS(ParticleBodyMeshInstance, MeshInstance);

	enum RenderingUpdateApproach {
		RENDERING_UPDATE_APPROACH_NONE,
		RENDERING_UPDATE_APPROACH_PVP,
		RENDERING_UPDATE_APPROACH_SKELETON
	};

	ParticleClothVisualServerHandler *visual_server_handler;

	bool look_y_previous_cluster;
	real_t weight_fall_off;
	real_t weight_max_distance;

	ParticleBody *particle_body;
	Skeleton *skeleton;

	RenderingUpdateApproach rendering_approach;

	static void _bind_methods();
	virtual void _notification(int p_what);

public:
	ParticleBodyMeshInstance();
	virtual ~ParticleBodyMeshInstance();

	void set_look_y_previous_cluster(bool keep);
	bool get_look_y_previous_cluster() const { return look_y_previous_cluster; }

	void set_weight_fall_off(real_t p_weight_fall_off);
	real_t get_weight_fall_off() const { return weight_fall_off; }

	void set_weight_max_distance(real_t p_weight_max_distance);
	real_t get_weight_max_distance() const { return weight_max_distance; }

	_FORCE_INLINE_ Skeleton *get_skeleton() { return skeleton; }

	void update_mesh(ParticleBodyCommands *p_cmds);

	void update_mesh_pvparticles(ParticleBodyCommands *p_cmds);
	void _draw_mesh_pvparticles();
	void update_mesh_skeleton(ParticleBodyCommands *p_cmds);

private:
	void prepare_mesh_for_rendering();
	void prepare_mesh_for_pvparticles();
	void prepare_mesh_skeleton_deformation();
	void _clear_pvparticles_drawing();
};

#endif // PHYSICS_PARTICLE_BODY_MESH_INSTANCE_H
