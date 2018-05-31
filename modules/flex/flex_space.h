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

class ParticleBodyBuffer : public FlexMemory {
    int buffers[10];

public:
    virtual void resize(int p_size);
    virtual void shift_back(int p_from, int p_to, int p_shift);
    virtual void set_data(int p_pos, int p_data);
};

struct FlexBuffers {
    // TODO this is just an initial test, implement a better memory handling in order to avoid brute force update
    NvFlexVector<FlVector4> particles; // XYZ world position, W inverse mass
    NvFlexVector<FlVector3> velocities;
    NvFlexVector<int> phases; // This is a flag that specify behaviour of particle like collision etc.. https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/flex/manual.html#phase
    NvFlexVector<int> active_particles;

    FlexBuffers(NvFlexLibrary *p_flex_lib);
};

class FlexSpace : public RIDFlex {
    friend class FlexBuffers;

    NvFlexLibrary *flex_lib;
    NvFlexSolver *solver;
    FlexBuffers *buffers;
    int active_particle_count;

    FlexMemoryAllocator buf;

public:
    FlexSpace();
    ~FlexSpace();

    void init();
    void terminate();
    void sync();
    void step(real_t p_delta_time);

    void terminate_buffers(FlexBuffers *p_buffers);
    void map_buffers(FlexBuffers *p_buffers);
    void unmap_buffers(FlexBuffers *p_buffers);
};

#endif // FLEX_SPACE_H
