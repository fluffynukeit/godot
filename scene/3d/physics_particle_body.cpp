/*************************************************************************/
/*  physics_particle_body.cpp                                            */
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

#include "physics_particle_body.h"

#include "core_string_names.h"
#include "scene/3d/physics_particle_body_mesh_instance.h"
#include "scene/3d/skeleton.h"

void ParticleBody::_bind_methods() {

	ClassDB::bind_method(D_METHOD("get_particle_body_mesh"), &ParticleBody::get_particle_body_mesh);

	ClassDB::bind_method(D_METHOD("set_particle_body_model", "model"), &ParticleBody::set_particle_body_model);
	ClassDB::bind_method(D_METHOD("get_particle_body_model"), &ParticleBody::get_particle_body_model);

	ClassDB::bind_method(D_METHOD("set_pressure", "pressure"), &ParticleBody::set_pressure);
	ClassDB::bind_method(D_METHOD("get_pressure"), &ParticleBody::get_pressure);

	ClassDB::bind_method(D_METHOD("remove_particle", "particle_index"), &ParticleBody::remove_particle);
	ClassDB::bind_method(D_METHOD("remove_rigid", "rigid_index"), &ParticleBody::remove_rigid);

	ClassDB::bind_method(D_METHOD("set_collision_group", "layer"), &ParticleBody::set_collision_group);
	ClassDB::bind_method(D_METHOD("get_collision_group"), &ParticleBody::get_collision_group);

	ClassDB::bind_method(D_METHOD("set_collision_flag_self_collide", "active"), &ParticleBody::set_collision_flag_self_collide);
	ClassDB::bind_method(D_METHOD("get_collision_flag_self_collide"), &ParticleBody::get_collision_flag_self_collide);

	ClassDB::bind_method(D_METHOD("set_collision_flag_self_collide_filter", "active"), &ParticleBody::set_collision_flag_self_collide_filter);
	ClassDB::bind_method(D_METHOD("get_collision_flag_self_collide_filter"), &ParticleBody::get_collision_flag_self_collide_filter);

	ClassDB::bind_method(D_METHOD("set_collision_flag_fluid", "active"), &ParticleBody::set_collision_flag_fluid);
	ClassDB::bind_method(D_METHOD("get_collision_flag_fluid"), &ParticleBody::get_collision_flag_fluid);

	ClassDB::bind_method(D_METHOD("set_collision_primitive_mask", "mask"), &ParticleBody::set_collision_primitive_mask);
	ClassDB::bind_method(D_METHOD("get_collision_primitive_mask"), &ParticleBody::get_collision_primitive_mask);

	ClassDB::bind_method(D_METHOD("get_particle_count"), &ParticleBody::get_particle_count);
	ClassDB::bind_method(D_METHOD("get_spring_count"), &ParticleBody::get_spring_count);
	ClassDB::bind_method(D_METHOD("get_rigid_count"), &ParticleBody::get_rigid_count);

	ClassDB::bind_method(D_METHOD("resource_changed", "resource"), &ParticleBody::resource_changed);

	ClassDB::bind_method(D_METHOD("commands_process_internal", "commands"), &ParticleBody::commands_process_internal);

	ClassDB::bind_method(D_METHOD("_on_script_changed"), &ParticleBody::_on_script_changed);

	// Virtual methods
	BIND_VMETHOD(MethodInfo("_commands_process", PropertyInfo(Variant::OBJECT, "commands", PROPERTY_HINT_RESOURCE_TYPE, "ParticleBodyCommands")));
	BIND_VMETHOD(MethodInfo("_on_particle_index_change", PropertyInfo(Variant::OBJECT, "old_index"), PropertyInfo(Variant::OBJECT, "new_index")));
	BIND_VMETHOD(MethodInfo("_on_spring_index_change", PropertyInfo(Variant::OBJECT, "old_index"), PropertyInfo(Variant::OBJECT, "new_index")));

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "particle_body_model", PROPERTY_HINT_RESOURCE_TYPE, "ParticleBodyModel"), "set_particle_body_model", "get_particle_body_model");

	ADD_PROPERTY(PropertyInfo(Variant::REAL, "pressure"), "set_pressure", "get_pressure");

	ADD_GROUP("Collision", "collision_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_group", PROPERTY_HINT_RANGE, "0,21,1"), "set_collision_group", "get_collision_group");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collision_flag_self_collide"), "set_collision_flag_self_collide", "get_collision_flag_self_collide");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collision_flag_self_collide_filter"), "set_collision_flag_self_collide_filter", "get_collision_flag_self_collide_filter");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "collision_flag_fluid"), "set_collision_flag_fluid", "get_collision_flag_fluid");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_primitive_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_collision_primitive_mask", "get_collision_primitive_mask");

	ADD_SIGNAL(MethodInfo("resource_loaded"));
}

ParticleBody::ParticleBody() :
		ParticleObject(ParticlePhysicsServer::get_singleton()->body_create()),
		reload_particle_model(true),
		particle_body_mesh(NULL) {

	set_notify_transform(true);

	connect(CoreStringNames::get_singleton()->script_changed, this, "_on_script_changed");
}

ParticleBody::~ParticleBody() {

	if (particle_body_model.is_valid())
		particle_body_model->unregister_owner(this);

	ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SYNC, NULL, "");
	ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED, NULL, "");
	ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED, NULL, "");
}

void ParticleBody::set_particle_body_mesh(ParticleBodyMeshInstance *p_mesh) {
	ERR_FAIL_COND(particle_body_mesh);
	particle_body_mesh = p_mesh;
}

void ParticleBody::set_particle_body_model(Ref<ParticleBodyModel> p_model) {
	if (particle_body_model == p_model)
		return;

	if (particle_body_model.is_valid())
		particle_body_model->unregister_owner(this);

	particle_body_model = p_model;

	if (particle_body_model.is_valid())
		particle_body_model->register_owner(this);

	resource_changed(particle_body_model);
}

Ref<ParticleBodyModel> ParticleBody::get_particle_body_model() const {
	return particle_body_model;
}

void ParticleBody::remove_particle(int p_particle_index) {
	ParticlePhysicsServer::get_singleton()->body_remove_particle(rid, p_particle_index);
}

void ParticleBody::remove_rigid(int p_rigid_index) {
	ParticlePhysicsServer::get_singleton()->body_remove_rigid(rid, p_rigid_index);
}

void ParticleBody::set_collision_group(uint32_t p_group) {
	ParticlePhysicsServer::get_singleton()->body_set_collision_group(rid, p_group);
}

uint32_t ParticleBody::get_collision_group() const {
	return ParticlePhysicsServer::get_singleton()->body_get_collision_group(rid);
}

void ParticleBody::set_collision_flag_self_collide(bool p_active) {
	ParticlePhysicsServer::get_singleton()->body_set_collision_flag(rid, ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE, p_active);
}

bool ParticleBody::get_collision_flag_self_collide() const {
	return ParticlePhysicsServer::get_singleton()->body_get_collision_flag(rid, ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE);
}

void ParticleBody::set_collision_flag_self_collide_filter(bool p_active) {
	ParticlePhysicsServer::get_singleton()->body_set_collision_flag(rid, ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE_FILTER, p_active);
}

bool ParticleBody::get_collision_flag_self_collide_filter() const {
	return ParticlePhysicsServer::get_singleton()->body_get_collision_flag(rid, ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_SELF_COLLIDE_FILTER);
}

void ParticleBody::set_collision_flag_fluid(bool p_active) {
	ParticlePhysicsServer::get_singleton()->body_set_collision_flag(rid, ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_FLUID, p_active);
}

bool ParticleBody::get_collision_flag_fluid() const {
	return ParticlePhysicsServer::get_singleton()->body_get_collision_flag(rid, ParticlePhysicsServer::PARTICLE_COLLISION_FLAG_FLUID);
}

void ParticleBody::set_collision_primitive_mask(uint32_t p_mask) {
	ParticlePhysicsServer::get_singleton()->body_set_collision_primitive_mask(rid, p_mask);
}

uint32_t ParticleBody::get_collision_primitive_mask() const {
	return ParticlePhysicsServer::get_singleton()->body_get_collision_primitive_mask(rid);
}

void ParticleBody::set_pressure(real_t p_pressure) {
	ParticlePhysicsServer::get_singleton()->body_set_pressure(rid, p_pressure);
}

real_t ParticleBody::get_pressure() const {
	return ParticlePhysicsServer::get_singleton()->body_get_pressure(rid);
}

int ParticleBody::get_particle_count() const {
	return ParticlePhysicsServer::get_singleton()->body_get_particle_count(rid);
}

int ParticleBody::get_spring_count() const {
	return ParticlePhysicsServer::get_singleton()->body_get_spring_count(rid);
}

int ParticleBody::get_rigid_count() const {
	return ParticlePhysicsServer::get_singleton()->body_get_rigid_count(rid);
}

void ParticleBody::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_WORLD: {
			ParticlePhysicsServer::get_singleton()->body_set_space(rid, get_world()->get_particle_space());
			ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SYNC, this, "commands_process_internal");
			resource_changed(particle_body_model);

		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {

			debug_reset_particle_positions();

		} break;
		case NOTIFICATION_EXIT_WORLD: {
			ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SYNC, NULL, "");
			ParticlePhysicsServer::get_singleton()->body_set_space(rid, RID());
			debug_initialize_resource();
		} break;
	}
}

void ParticleBody::resource_changed(const RES &p_res) {
	if (particle_body_model == p_res) {
		_on_model_change();
	}
}

void ParticleBody::commands_process_internal(Object *p_cmds) {

	ParticleBodyCommands *cmds(static_cast<ParticleBodyCommands *>(p_cmds));

	if (reload_particle_model) {
		reload_particle_model = false;
		cmds->load_model(particle_body_model, get_global_transform());
		emit_signal("resource_loaded");
	}

	if (particle_body_mesh)
		particle_body_mesh->update_mesh(cmds);
	debug_update(cmds);

	if (!get_script().is_null() && has_method("_commands_process")) {
		call("_commands_process", p_cmds);
	}
}

void ParticleBody::_on_script_changed() {
	if (has_method("_on_particle_index_change")) {
		ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED, this, "_on_particle_index_change");
	} else {
		ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_PARTICLEINDEXCHANGED, NULL, "");
	}

	if (has_method("_on_spring_index_change")) {
		ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED, this, "_on_spring_index_change");
	} else {
		ParticlePhysicsServer::get_singleton()->body_set_callback(rid, ParticlePhysicsServer::PARTICLE_BODY_CALLBACK_SPRINGINDEXCHANGED, NULL, "");
	}
}

void ParticleBody::_on_model_change() {
	reload_particle_model = true;
	debug_initialize_resource();
}

void ParticleBody::debug_initialize_resource() {

	if (!is_inside_tree() || !get_tree()->is_debugging_collisions_hint())
		return;

	debug_particle_mesh.instance();
	debug_particle_mesh->set_radius(0.05);
	debug_particle_mesh->set_height(0.1);
	debug_particle_mesh->set_radial_segments(8);
	debug_particle_mesh->set_rings(8);

	const int particle_count = particle_body_model.is_valid() ? particle_body_model->get_particles_ref().size() : 0;
	debug_resize_particle_visual_instance(particle_count);
	debug_reset_particle_positions();
}

void ParticleBody::debug_resize_particle_visual_instance(int new_size) {

	if (debug_particles_mesh.size() == new_size)
		return;

	if (debug_particles_mesh.size() > new_size) {

		// If the particle count is less then visual instances size, free the last
		const int dif = debug_particles_mesh.size() - new_size;
		for (int i = 0; i < dif; ++i) {

			const int p = debug_particles_mesh.size() - i - 1;
			debug_particles_mesh[p]->queue_delete();
			debug_particles_mesh[p] = NULL;
		}
		debug_particles_mesh.resize(new_size);
	} else {

		if (!is_inside_world())
			return;

		// If the particle count is more then visual instances resize and create last
		const int dif = new_size - debug_particles_mesh.size();
		debug_particles_mesh.resize(new_size);
		for (int i = 0; i < dif; ++i) {

			const int p = new_size - i - 1;
			debug_particles_mesh[p] = memnew(MeshInstance);
			debug_particles_mesh[p]->set_as_toplevel(true);
			debug_particles_mesh[p]->set_material_override(get_tree()->get_debug_collision_material());
			debug_particles_mesh[p]->set_mesh(debug_particle_mesh);
			add_child(debug_particles_mesh[p]);
		}
	}
}

void ParticleBody::debug_update(ParticleBodyCommands *p_cmds) {

	if (!get_tree()->is_debugging_collisions_hint())
		return;

	const int particle_count = ParticlePhysicsServer::get_singleton()->body_get_particle_count(rid);
	debug_resize_particle_visual_instance(ParticlePhysicsServer::get_singleton()->body_get_particle_count(rid));

	Transform transf;
	for (int i = 0; i < particle_count; ++i) {

		transf.origin = p_cmds->get_particle_position(i);
		debug_particles_mesh[i]->set_global_transform(transf);
	}
}

void ParticleBody::debug_reset_particle_positions() {

	if (!get_tree()->is_debugging_collisions_hint())
		return;

	if (particle_body_model.is_null())
		return;

	if (debug_particles_mesh.size() == particle_body_model->get_particles_ref().size()) {

		Transform particle_relative_transf;
		for (int i = 0; i < debug_particles_mesh.size(); ++i) {

			particle_relative_transf.origin = particle_body_model->get_particles_ref()[i];
			debug_particles_mesh[i]->set_global_transform(get_global_transform() * particle_relative_transf);
		}
	}
}
