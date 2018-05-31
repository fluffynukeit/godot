/*************************************************************************/
/*  flex_space.h                                                         */
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

#ifndef FLEX_SPACE_H
#define FLEX_SPACE_H

#include "rid_flex.h"

#include "flex_maths.h"
#include "thirdparty/flex/include/NvFlexExt.h"

#include "flex_memory_allocator.h"
#include "math_defs.h"

class NvFlexLibrary;
class NvFlexSolver;

class ParticleBodiesMemory : public FlexMemory {
public:
    NvFlexVector<FlVector4> particles; // XYZ world position, W inverse mass
    NvFlexVector<FlVector3> velocities;
    NvFlexVector<int> phases; // This is a flag that specify behaviour of particle like collision etc.. https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/flex/manual.html#phase
    NvFlexVector<int> active_particles; // TODO this function can't stay here, should be handled outside this buffer.

    ParticleBodiesMemory(NvFlexLibrary *p_flex_lib);

    virtual void reserve(int p_size);
    virtual void shift_back(int p_from, int p_to, int p_shift);

    void map();
    void unmap();
    void terminate();

    void set_particle(const Stack *p_stack, int p_particle_index, FlVector4 p_particle);
    void set_velocity(const Stack *p_stack, int p_particle_index, FlVector3 p_velocity);
    void set_phase(const Stack *p_stack, int p_particle_index, int p_phase);
    void set_active_particles(const Stack *p_stack, int p_particle_index, int p_index);
};

class FlexSpace : public RIDFlex {
    friend class FlexBuffers;

    NvFlexLibrary *flex_lib;
    NvFlexSolver *solver;
    FlexMemoryAllocator *particle_bodies_allocator;
    ParticleBodiesMemory *particle_bodies_memory;
    int active_particle_count;

    Stack *test_stack;

public:
    FlexSpace();
    ~FlexSpace();

    void init();
    void terminate();
    void sync();
    void step(real_t p_delta_time);
};

#endif // FLEX_SPACE_H
