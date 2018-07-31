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

ParticleClothVisualServerHandler::ParticleClothVisualServerHandler() {}

void ParticleClothVisualServerHandler::prepare(RID p_mesh, int p_surface, const Array &p_mesh_arrays) {
	clear();

	ERR_FAIL_COND(!p_mesh.is_valid());

	mesh = p_mesh;
	surface = p_surface;

	const uint32_t surface_format = VS::get_singleton()->mesh_surface_get_format(mesh, surface);
	const int surface_vertex_len = VS::get_singleton()->mesh_surface_get_array_len(mesh, p_surface);
	const int surface_index_len = VS::get_singleton()->mesh_surface_get_array_index_len(mesh, p_surface);
	uint32_t surface_offsets[VS::ARRAY_MAX];

	buffer = VS::get_singleton()->mesh_surface_get_array(mesh, surface);
	stride = VS::get_singleton()->mesh_surface_make_offsets_from_format(surface_format, surface_vertex_len, surface_index_len, surface_offsets);
	offset_vertices = surface_offsets[VS::ARRAY_VERTEX];
	offset_normal = surface_offsets[VS::ARRAY_NORMAL];

	mesh_indices = p_mesh_arrays[VS::ARRAY_INDEX];
}

void ParticleClothVisualServerHandler::clear() {

	if (mesh.is_valid()) {
		buffer.resize(0);
	}

	mesh = RID();
}

void ParticleClothVisualServerHandler::open() {
	write_buffer = buffer.write();
}

void ParticleClothVisualServerHandler::close() {
	write_buffer = PoolVector<uint8_t>::Write();
}

void ParticleClothVisualServerHandler::commit_changes() {
	VS::get_singleton()->mesh_surface_update_region(mesh, surface, 0, buffer);
}

void ParticleClothVisualServerHandler::set_vertex(int p_vertex, const void *p_vector3) {
	copymem(&write_buffer[p_vertex * stride + offset_vertices], p_vector3, sizeof(float) * 3);
}

void ParticleClothVisualServerHandler::set_normal(int p_vertex, const void *p_vector3) {
	copymem(&write_buffer[p_vertex * stride + offset_normal], p_vector3, sizeof(float) * 3);
}

void ParticleClothVisualServerHandler::set_aabb(const AABB &p_aabb) {
	VS::get_singleton()->mesh_set_custom_aabb(mesh, p_aabb);
}

void ParticleBodyMeshInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_draw_mesh_pvparticles"), &ParticleBodyMeshInstance::_draw_mesh_pvparticles);
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
				prepare_mesh_for_rendering();
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

			_clear_pvparticles_drawing();
		} break;
	}
}

ParticleBodyMeshInstance::ParticleBodyMeshInstance() :
		MeshInstance(),
		particle_body(NULL),
		skeleton(NULL),
		rendering_approach(RENDERING_UPDATE_APPROACH_NONE),
		visual_server_handler(NULL) {

	set_skeleton_path(NodePath());
	if (Engine::get_singleton()->is_editor_hint())
		set_notify_local_transform(true);
}

ParticleBodyMeshInstance::~ParticleBodyMeshInstance() {
	_clear_pvparticles_drawing();
}

void ParticleBodyMeshInstance::update_mesh(ParticleBodyCommands *p_cmds) {
	switch (rendering_approach) {
		case RENDERING_UPDATE_APPROACH_PVP:
			update_mesh_pvparticles(p_cmds);
			break;
		case RENDERING_UPDATE_APPROACH_SKELETON:
			update_mesh_skeleton(p_cmds);
			break;
	}
}

void ParticleBodyMeshInstance::update_mesh_pvparticles(ParticleBodyCommands *p_cmds) {

	PoolVector<int>::Read pb_indices_r = particle_body->get_particle_body_model()->get_dynamic_triangles_indices().read();
	visual_server_handler->open();

	PoolVector<int>::Read mesh_indices_r = visual_server_handler->get_mesh_indices().read();
	Vector3 v;
	for (int i(visual_server_handler->get_mesh_indices().size() - 1); 0 <= i; --i) {

		v = p_cmds->get_particle_position(pb_indices_r[i]);
		visual_server_handler->set_vertex(mesh_indices_r[i], reinterpret_cast<void *>(&v));
		v = p_cmds->get_particle_normal(pb_indices_r[i]) * -1;
		visual_server_handler->set_normal(mesh_indices_r[i], reinterpret_cast<void *>(&v));
	}

	visual_server_handler->set_aabb(p_cmds->get_aabb());

	visual_server_handler->close();
}

void ParticleBodyMeshInstance::_draw_mesh_pvparticles() {

	// The buffer is updated in the update_mesh_pvparticles
	visual_server_handler->commit_changes();
}

void ParticleBodyMeshInstance::update_mesh_skeleton(ParticleBodyCommands *p_cmds) {
	const int rigids_count = ParticlePhysicsServer::get_singleton()->body_get_rigid_count(particle_body->get_rid());
	const PoolVector<Vector3>::Read rigids_local_pos_r = particle_body->get_particle_body_model()->get_clusters_positions().read();

	for (int i = 0; i < rigids_count; ++i) {

		Transform t(Basis(p_cmds->get_rigid_rotation(i)), p_cmds->get_rigid_position(i));
		t.translate(rigids_local_pos_r[i] * -1);
		skeleton->set_bone_pose(i, t);
	}
}

void ParticleBodyMeshInstance::prepare_mesh_for_rendering() {

	if (Engine::get_singleton()->is_editor_hint())
		return;

	Ref<ParticleBodyModel> model = particle_body->get_particle_body_model();
	if (model.is_null())
		return;

	if (model->get_clusters_positions().size())
		prepare_mesh_skeleton_deformation();

	else if (model->get_dynamic_triangles_indices().size())
		prepare_mesh_for_pvparticles();
}

void ParticleBodyMeshInstance::prepare_mesh_for_pvparticles() {

	Ref<ParticleBodyModel> model = particle_body->get_particle_body_model();
	if (model.is_null())
		return;

	ERR_FAIL_COND(!model->get_dynamic_triangles_indices().size());

	ERR_FAIL_COND(!get_mesh()->get_surface_count());

	Ref<Material> material(get_surface_material(0));

	// Get current mesh array and create new mesh array with necessary flag for softbody
	Array surface_arrays = get_mesh()->surface_get_arrays(0);
	Array surface_blend_arrays = get_mesh()->surface_get_blend_shape_arrays(0);
	uint32_t surface_format = get_mesh()->surface_get_format(0);

	surface_format &= ~(Mesh::ARRAY_COMPRESS_VERTEX | Mesh::ARRAY_COMPRESS_NORMAL);
	surface_format |= Mesh::ARRAY_FLAG_USE_DYNAMIC_UPDATE;

	Ref<ArrayMesh> soft_mesh;
	soft_mesh.instance();
	soft_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface_arrays, surface_blend_arrays, surface_format);
	soft_mesh->surface_set_material(0, get_mesh()->surface_get_material(0));

	set_mesh(soft_mesh);

	set_surface_material(0, material);

	rendering_approach = RENDERING_UPDATE_APPROACH_PVP;

	ERR_FAIL_COND(visual_server_handler);
	visual_server_handler = memnew(ParticleClothVisualServerHandler);
	visual_server_handler->prepare(get_mesh()->get_rid(), 0, surface_arrays);

	/// Necessary in order to render the mesh correctly (Soft body nodes are in global space)
	call_deferred("set_as_toplevel", true);
	call_deferred("set_transform", Transform());

	VS::get_singleton()->connect("frame_pre_draw", this, "_draw_mesh_pvparticles");
}

void ParticleBodyMeshInstance::prepare_mesh_skeleton_deformation() {

	if (Engine::get_singleton()->is_editor_hint())
		return;

	if (get_mesh().is_null())
		return;

	Ref<ParticleBodyModel> model = particle_body->get_particle_body_model();
	if (model.is_null())
		return;

	ERR_FAIL_COND(!model->get_clusters_positions().size());
	ERR_FAIL_COND(get_mesh()->get_surface_count() != 1);

	Ref<Material> material(get_surface_material(0));

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
	new_mesh->surface_set_material(0, mesh->surface_get_material(0));

	set_mesh(new_mesh);

	set_surface_material(0, material);

	rendering_approach = RENDERING_UPDATE_APPROACH_SKELETON;

	_clear_pvparticles_drawing();
}

void ParticleBodyMeshInstance::_clear_pvparticles_drawing() {

	if (VS::get_singleton()->is_connected("frame_pre_draw", this, "_draw_mesh_pvparticles"))
		VS::get_singleton()->disconnect("frame_pre_draw", this, "_draw_mesh_pvparticles");

	if (!visual_server_handler)
		return;

	visual_server_handler->clear();
	memdelete(visual_server_handler);
	visual_server_handler = NULL;
}
