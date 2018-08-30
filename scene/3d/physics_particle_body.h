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

	bool update_spatial_transform;

	bool reload_particle_model;
	ParticleBodyMeshInstance *particle_body_mesh;
	Ref<ParticleBodyModel> particle_body_model;

	Vector<MeshInstance *> debug_particles_mesh;
	Ref<SphereMesh> debug_particle_mesh;

#ifdef TOOLS_ENABLED
public:
	bool draw_gizmo;
	Vector<int> selected_particles;

private:
#endif

protected:
	static void _bind_methods();

public:
	ParticleBody();
	virtual ~ParticleBody();

	virtual String get_configuration_warning() const;

	void set_particle_body_mesh(ParticleBodyMeshInstance *p_mesh);
	ParticleBodyMeshInstance *get_particle_body_mesh() const { return particle_body_mesh; }

	void set_particle_body_model(Ref<ParticleBodyModel> p_model);
	Ref<ParticleBodyModel> get_particle_body_model() const;

	void set_update_spatial_transform(bool p_update);
	bool get_update_spatial_transform() const;

	void remove_particle(int p_particle_index);
	void remove_rigid(int p_rigid_index);

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

	void set_collision_primitive_mask_bit(int p_bit, bool p_value);
	bool get_collision_primitive_mask_bit(int p_bit) const;

	void set_monitorable(bool p_monitorable);
	bool is_monitorable() const;

	void set_monitoring_primitives_contacts(bool p_monitoring);
	bool is_monitoring_primitives_contacts() const;

	// Utility:

	int get_particle_count() const;
	int get_spring_count() const;
	int get_rigid_count() const;

protected:
	void _notification(int p_what);
	void resource_changed(RES p_res);

	void commands_process_internal(Object *p_cmds);
	void update_transform(ParticleBodyCommands *p_cmds);
	void on_primitive_contact(Object *p_cmds, Object *p_primitive_object, int p_particle_index, Vector3 p_velocity, Vector3 p_normal);

private:
	void _on_script_changed();

	void debug_initialize_resource();
	void debug_resize_particle_visual_instance(int new_size);
	void debug_update(ParticleBodyCommands *p_cmds);
	void debug_reset_particle_positions();
};

class SoftParticleBody : public ParticleBody {
	GDCLASS(SoftParticleBody, ParticleBody);

	real_t radius;
	real_t global_stiffness;
	bool internal_sample;
	real_t particle_spacing;
	real_t sampling;
	real_t cluster_spacing;
	real_t cluster_radius;
	real_t cluster_stiffness;
	real_t link_radius;
	real_t link_stiffness;
	real_t plastic_threshold;
	real_t plastic_creep;

protected:
	static void _bind_methods();

public:
	SoftParticleBody();

	void set_radius(real_t p_value);
	real_t get_radius() const;
	void set_global_stiffness(real_t p_value);
	real_t get_global_stiffness() const;
	void set_internal_sample(bool p_value);
	bool get_internal_sample() const;
	void set_particle_spacing(real_t p_value);
	real_t get_particle_spacing() const;
	void set_sampling(real_t p_value);
	real_t get_sampling() const;
	void set_cluster_spacing(real_t p_value);
	real_t get_cluster_spacing() const;
	void set_cluster_radius(real_t p_value);
	real_t get_cluster_radius() const;
	void set_cluster_stiffness(real_t p_value);
	real_t get_cluster_stiffness() const;
	void set_link_radius(real_t p_value);
	real_t get_link_radius() const;
	void set_link_stiffness(real_t p_value);
	real_t get_link_stiffness() const;
	void set_plastic_threshold(real_t p_value);
	real_t get_plastic_threshold() const;
	void set_plastic_creep(real_t p_value);
	real_t get_plastic_creep() const;
};

class RigidParticleBody : public ParticleBody {
	GDCLASS(RigidParticleBody, ParticleBody);

	real_t radius;
	real_t expand;

protected:
	static void _bind_methods();

public:
	RigidParticleBody();

	void set_radius(real_t p_value);
	real_t get_radius() const;

	void set_expand(real_t p_value);
	real_t get_expand() const;
};

class ClothParticleBody : public ParticleBody {
	GDCLASS(ClothParticleBody, ParticleBody);

protected:
	static void _bind_methods();

	real_t stretch_stiffness;
	real_t bend_stiffness;
	real_t tether_stiffness;
	real_t tether_give;
	real_t rest_pressure;

public:
	ClothParticleBody();

	void set_stretch_stiffness(real_t p_value);
	real_t get_stretch_stiffness() const;

	void set_bend_stiffness(real_t p_value);
	real_t get_bend_stiffness() const;

	void set_tether_stiffness(real_t p_value);
	real_t get_tether_stiffness() const;

	void set_tether_give(real_t p_value);
	real_t get_tether_give() const;

	void set_rest_pressure(real_t p_value);
	real_t get_rest_pressure() const;

	void set_pressure(real_t p_pressure);
	real_t get_pressure() const;
};

#endif // PARTICLE_BODY_H
