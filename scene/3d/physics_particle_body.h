/*************************************************************************/
/*  physics_particle_body.h                                              */
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

#ifndef PARTICLE_BODY_H
#define PARTICLE_BODY_H

#include "physics_particle_object.h"
#include "scene/resources/particle_body_model.h"
#include "scene/resources/primitive_meshes.h"
#include "spatial.h"

class MeshInstance;
class ParticleBodyMeshInstance;

class ParticleBody : public ParticleObject {
	GDCLASS(ParticleBody, ParticleObject);

	bool reset_particles_to_base_shape;
	ParticleBodyMeshInstance *particle_body_mesh;
	Ref<ParticleBodyModel> particle_body_model;

	Vector3 COM_global_position;

	MeshInstance *debug_COM_particles_mesh;
	Vector<MeshInstance *> debug_particles_mesh;
	Ref<SphereMesh> debug_particle_mesh;

protected:
	static void _bind_methods();

public:
	ParticleBody();
	virtual ~ParticleBody();

	void set_particle_body_mesh(ParticleBodyMeshInstance *p_mesh);
	ParticleBodyMeshInstance *get_particle_body_mesh() const { return particle_body_mesh; }

	void set_particle_body_model(Ref<ParticleBodyModel> p_model);
	Ref<ParticleBodyModel> get_particle_body_model() const;

	void add_particle(const Vector3 &p_local_position, real_t p_mass);
	void remove_particle(int p_particle_index);

	void set_collision_group(uint32_t p_layer);
	uint32_t get_collision_group() const;

	void set_collision_flag_self_collide(bool p_active);
	bool get_collision_flag_self_collide() const;

	void set_collision_flag_self_collide_filter(bool p_active);
	bool get_collision_flag_self_collide_filter() const;

	void set_collision_flag_fluid(bool p_active);
	bool get_collision_flag_fluid() const;

	void set_collision_primitive_mask(uint32_t p_mask);
	uint32_t get_collision_primitive_mask() const;

protected:
	void _notification(int p_what);
	void resource_changed(const RES &p_res);

	void commands_process_internal(Object *p_cmds);

private:
	void _on_script_changed();
	void _on_model_change();
	void body_mesh_skeleton_update(ParticleBodyCommands *p_cmds);

	void debug_initialize_resource();
	void debug_resize_particle_visual_instance(int new_size);
	void debug_update(ParticleBodyCommands *p_cmds);
	void debug_reset_particle_positions();
};

#endif // PARTICLE_BODY_H
