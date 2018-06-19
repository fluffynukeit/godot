/*************************************************************************/
/*  physics_particle_body_mesh_instance.h                                */
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

#include "physics_particle_body_mesh_instance.h"

#include "engine.h"
#include "scene/3d/physics_particle_body.h"
#include "scene/3d/skeleton.h"

void ParticleBodyMeshInstance::_bind_methods() {
}

void ParticleBodyMeshInstance::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {

			ERR_FAIL_COND(particle_body);

			if (!Engine::get_singleton()->is_editor_hint()) {

				set_as_toplevel(true);
				set_global_transform(Transform());
			}

			particle_body = Object::cast_to<ParticleBody>(get_parent());
			if (particle_body) {
				particle_body->set_particle_body_mesh(this);
				_reload_skeleton();
			}

		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {

			if (!Engine::get_singleton()->is_editor_hint())
				return;

			if (!particle_body)
				return;
			particle_body->set_global_transform(get_global_transform());
			set_notify_local_transform(false);
			set_transform(Transform());
			set_notify_local_transform(true);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (particle_body) {
				particle_body->set_particle_body_mesh(NULL);
			}
		} break;
	}
}

ParticleBodyMeshInstance::ParticleBodyMeshInstance() :
		MeshInstance(),
		particle_body(NULL),
		skeleton(NULL) {

	if (Engine::get_singleton()->is_editor_hint())
		set_notify_local_transform(true);
}

void ParticleBodyMeshInstance::_reload_skeleton() {

	if (Engine::get_singleton()->is_editor_hint())
		return;

	Ref<ParticleBodyModel> model = particle_body->get_particle_body_model();
	if (model.is_null())
		return;

	if (get_mesh().is_null())
		return;

	if (!model->get_particles().size())
		return;

	ERR_FAIL_COND(get_mesh()->get_surface_count() != 1);

	const int surface_id = 0;

	PoolVector<Vector3>::Read clusters_pos_r = model->get_clusters_positions().read();
	const int bone_count(model->get_clusters_positions().size());

	Array array_mesh = get_mesh()->surface_get_arrays(surface_id).duplicate();
	PoolVector<Vector3> vertices = array_mesh[VS::ARRAY_VERTEX];
	PoolVector<Vector3>::Read vertices_read = vertices.read();
	const int vertex_count(vertices.size());

	PoolVector<float> weights; // The weight value for vertex
	PoolVector<int> bone_indices; // The index of bone relative of vertex
	int max_weights_per_vertex = 0;

	ParticlePhysicsServer::get_singleton()->create_skeleton(clusters_pos_r.ptr(), bone_count, vertices_read.ptr(), vertex_count, &weights, &bone_indices, &max_weights_per_vertex);

	ERR_FAIL_COND(max_weights_per_vertex != VS::ARRAY_WEIGHTS_SIZE);

	if (!skeleton) {
		skeleton = memnew(Skeleton);
		add_child(skeleton);
		set_skeleton_path(skeleton->get_path());
	} else {
		skeleton->clear_bones();
	}

	for (int i(0); i < bone_count; ++i) {
		skeleton->add_bone("particle_" + String::num(i));
		skeleton->set_bone_disable_rest(i, true);
	}

	// Create skeleton using these info:
	array_mesh[VS::ARRAY_WEIGHTS] = weights;
	array_mesh[VS::ARRAY_BONES] = bone_indices;

	Ref<ArrayMesh> new_mesh;
	new_mesh.instance();
	new_mesh->add_surface_from_arrays(get_mesh()->surface_get_primitive_type(surface_id), array_mesh);

	set_mesh(new_mesh);
}
