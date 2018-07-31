/*************************************************************************/
/*  flex_particle_body_constraint.h                                      */
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

#include "flex_particle_body_constraint.h"

#include "flex_particle_physics_server.h"

FlexParticleBodyConstraint::FlexParticleBodyConstraint(FlexParticleBody *p_body0, FlexParticleBody *p_body1) :
		body0(p_body0),
		body1(p_body1),
		space(NULL) {

	sync_callback.receiver = NULL;
}

FlexParticleBodyConstraint::~FlexParticleBodyConstraint() {
	if (!space)
		return;

	space->remove_particle_body_constraint(this);
}

void FlexParticleBodyConstraint::set_callback(Object *p_receiver, const StringName &p_method) {
	sync_callback.receiver = p_receiver;
	sync_callback.method = p_method;
}

int FlexParticleBodyConstraint::get_spring_count() const {
	return springs_mchunk ? springs_mchunk->get_size() : 0;
}

bool FlexParticleBodyConstraint::is_owner_of_spring(SpringIndex p_spring) const {
	return springs_mchunk->get_buffer_index(p_spring) <= springs_mchunk->get_end_index();
}

void FlexParticleBodyConstraint::remove_spring(SpringIndex p_spring_index) {
	ERR_FAIL_COND(!is_owner_of_spring(p_spring_index));
	if (-1 == delayed_commands.springs_to_remove.find(p_spring_index))
		delayed_commands.springs_to_remove.push_back(p_spring_index);
}

void FlexParticleBodyConstraint::dispatch_sync_callback() {
	if (!sync_callback.receiver)
		return;
	static Variant::CallError error;
	const Variant *p = FlexParticlePhysicsServer::singleton->get_particle_body_constraint_commands_variant(this);
	sync_callback.receiver->call(sync_callback.method, &p, 1, error);
}

void FlexParticleBodyConstraint::clear_delayed_commands() {
	//delayed_commands.springs_to_remove.clear();
}
