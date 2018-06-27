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

class ParticleBodyMeshInstance : public MeshInstance {
	GDCLASS(ParticleBodyMeshInstance, MeshInstance);

	enum RenderingUpdateApproach {
		RENDERING_UPDATE_APPROACH_NONE,
		RENDERING_UPDATE_APPROACH_PVP,
		RENDERING_UPDATE_APPROACH_SKELETON
	};

	ParticleBody *particle_body;
	Skeleton *skeleton;

	RenderingUpdateApproach rendering_approach;

	static void _bind_methods();
	virtual void _notification(int p_what);

public:
	ParticleBodyMeshInstance();

	_FORCE_INLINE_ Skeleton *get_skeleton() { return skeleton; }
	void update_mesh(ParticleBodyCommands *p_cmds);

	void update_mesh_pvparticles(ParticleBodyCommands *p_cmds);
	void update_mesh_skeleton(ParticleBodyCommands *p_cmds);

private:
	void prepare_mesh_for_rendering();
	void prepare_mesh_for_pvparticles();
	void prepare_mesh_skeleton_deformation();
};

#endif // PHYSICS_PARTICLE_BODY_MESH_INSTANCE_H
