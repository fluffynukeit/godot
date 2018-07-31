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

#ifndef FLEX_PARTICLE_BODY_CONSTRAINT_H
#define FLEX_PARTICLE_BODY_CONSTRAINT_H

#include "flex_memory.h"
#include "flex_utility.h"
#include "rid_flex.h"

class FlexParticleBody;
class FlexSpace;

class FlexParticleBodyConstraint : public RIDFlex {

	friend class FlexSpace;
	friend class FlexParticleBodyConstraintCommands;

	struct {
		Vector<SpringIndex> springs_to_remove;
	} delayed_commands;

	FlexCallBackData sync_callback;
	FlexSpace *space;

	MemoryChunk *springs_mchunk;

	FlexParticleBody *body0;
	FlexParticleBody *body1;

public:
	FlexParticleBodyConstraint(FlexParticleBody *p_body0, FlexParticleBody *p_body1);
	virtual ~FlexParticleBodyConstraint();

	void set_callback(Object *p_receiver, const StringName &p_method);
	_FORCE_INLINE_ FlexSpace *get_space() {
		return space;
	}

	int get_spring_count() const;
	bool is_owner_of_spring(SpringIndex p_spring) const;

	void remove_spring(SpringIndex p_spring_index);

private:
	void dispatch_sync_callback();
	void clear_delayed_commands();
};

#endif // FLEX_PARTICLE_BODY_CONSTRAINT_H
