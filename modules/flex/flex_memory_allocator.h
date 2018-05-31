/*************************************************************************/
/*  flex_memory_allocator.h                                              */
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

#ifndef FLEX_MEMORY_ALLOCATOR_H
#define FLEX_MEMORY_ALLOCATOR_H

#include "vector.h"

class FlexMemory {
public:
    virtual void resize(int p_size) = 0;
    // Move bits by shifting in the memory
    // p_shift could be only negative
    virtual void shift_back(int p_from, int p_to, int p_shift) = 0;
    virtual void set_data(int p_pos, int set_data) = 0;
};

struct Stack {
    friend class FlexMemoryAllocator;

private:
    int begin_index;
    int end_index;
    bool is_free;
    int size;

    Stack() :
            begin_index(0),
            end_index(0),
            is_free(true),
            size(0) {}

    Stack(int p_begin_index, int p_end_index, bool p_is_free) :
            begin_index(p_begin_index),
            end_index(p_end_index),
            is_free(p_is_free),
            size(p_end_index - p_begin_index + 1) {
    }

    Stack(const Stack &other) :
            begin_index(other.begin_index),
            end_index(other.end_index),
            is_free(other.is_free),
            size(other.size) {}

    void set(int p_begin_index, int p_end_index, bool p_is_free) {
        begin_index = p_begin_index;
        end_index = p_end_index;
        is_free = p_is_free;
        size = p_end_index - p_begin_index + 1;
    }
};

class FlexMemoryAllocator {

    Vector<Stack *> memory_table;
    int memory_size;
    struct {
        int occupied_memory;
        int biggest_stack_size;
    } cache;

    FlexMemory *memory;

public:
    FlexMemoryAllocator(FlexMemory *p_memory, int p_size);
    bool resize(int p_size);
    void trim(bool want_update_cache = true); // Remove free stack between used memory

    // Allocate memory, return null if no more space available
    Stack *allocate(int p_size);

#define deallocateStack(flex_memory, p_stack) \
    flex_memory.__deallocate(p_stack);        \
    p_stack = NULL

    /// IMPORTANT Don't call it directly, Use macro deallocateStack
    void __deallocate(Stack *p_stack);

    // TODO Just a test
    void set_data(Stack *p_stack, int p_data);

private:
    bool redux_memory(int p_size);
    void update_cache();

    Stack *insert_stack(int p_pos = -1);
    void remove_stack(int p_pos);
};

#endif // FLEX_MEMORY_ALLOCATOR_H
