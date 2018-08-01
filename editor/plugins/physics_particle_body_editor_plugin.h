/*************************************************************************/
/*  physics_particle_body_editor_plugin.h                                */
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

#ifndef PHYSICS_PARTICLE_BODY_EDITOR_PLUGIN_H
#define PHYSICS_PARTICLE_BODY_EDITOR_PLUGIN_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "scene/3d/physics_particle_body.h"
#include "scene/3d/physics_particle_body_mesh_instance.h"

class CheckBox;

class ParticleBodyEditor : public Control {

	GDCLASS(ParticleBodyEditor, Control);

	friend class PhysicsParticleBodyEditorPlugin;

	enum Menu {

		MENU_OPTION_CREATE_PARTICLE_SOFT_BODY,
		MENU_OPTION_CREATE_PARTICLE_RIGID_BODY,
		MENU_OPTION_CREATE_PARTICLE_CLOTH,
		MENU_OPTION_CREATE_PARTICLE_THREAD
	};

	struct {
		ConfirmationDialog *dialog;
		SpinBox *radius_input;
		SpinBox *global_stiffness_input;
		CheckBox *internal_sample_check;
		SpinBox *particle_spacing_input;
		SpinBox *sampling_input;
		SpinBox *clusterSpacing_input;
		SpinBox *clusterRadius_input;
		SpinBox *clusterStiffness_input;
		SpinBox *linkRadius_input;
		SpinBox *linkStiffness_input;
		SpinBox *plastic_threshold_input;
		SpinBox *plastic_creep_input;
	} soft_body_dialog;

	struct {
		ConfirmationDialog *dialog;
		SpinBox *radius_input;
		SpinBox *expand_input;
	} rigid_body_dialog;

	struct {
		ConfirmationDialog *dialog;
		SpinBox *stretch_stiffness_input;
		SpinBox *bend_stiffness_input;
		SpinBox *tether_stiffness_input;
		SpinBox *tether_give_input;
		SpinBox *pressure_input;
	} cloth_dialog;

	struct {
		ConfirmationDialog *dialog;
		SpinBox *particle_radius_input;
		SpinBox *extent_input;
		SpinBox *spacing_input;
		SpinBox *cluster_size;
		SpinBox *cluster_stiffness;
	} thread_dialog;

	ParticleBody *node;

	MenuButton *options;
	Button *show_gizmo_btn;

	SpinBox *inspector_mass_inp;
	RichTextLabel *inspector_particles_txt;

	void _menu_option(int p_option);
	void _create_soft_body();
	void _create_rigid_body();
	void _create_cloth();
	void _create_thread();

	void _toggle_show_hide_gizmo();

	void _mass_changed(real_t p_mass);

protected:
	void _node_removed(Node *p_node);
	static void _bind_methods();

public:
	ParticleBodyEditor();
	void edit(ParticleBody *p_body);
	void redraw();
};

class PhysicsParticleBodyEditorPlugin : public EditorPlugin {

	GDCLASS(PhysicsParticleBodyEditorPlugin, EditorPlugin);

	ParticleBodyEditor *particle_body_editor;
	EditorNode *editor;

public:
	virtual String get_name() const { return "PhysicsParticleBody"; }
	bool has_main_screen() const { return false; }
	virtual void edit(Object *p_object);
	virtual bool handles(Object *p_object) const;
	virtual void make_visible(bool p_visible);

	void redraw();
	bool is_editing(Node *p_other) const;

	PhysicsParticleBodyEditorPlugin(EditorNode *p_node);
	~PhysicsParticleBodyEditorPlugin();
};

#endif // PHYSICS_PARTICLE_BODY_EDITOR_PLUGIN_H
