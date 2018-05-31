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
    virtual void reserve(int p_size) = 0;
    /// Move bits by shifting in the memory
    /// p_shift could be only negative
    virtual void shift_back(int p_from, int p_to, int p_shift) = 0;
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

public:
    _FORCE_INLINE_ int get_begin_index() const { return begin_index; }
};

/// This class is responsible for memory management.
/// It's possible to allocate and deallocate memory.
/// When you allocate the memory it returns a pointer to an object Stack that represent the portion of memory allocated
///
/// I've created this class with the idea to have a unique class that is able to handle all kind of buffer structure
/// For this reason this class doesn't perform any real memory allocation.
/// This task is performed on FlexMemory object.
/// FlexMemory is an interface that each buffer should implement.
/// As you can see neither FlexMemoryAllocator nor FlexMemory define how to set data in the buffer,
/// infact this should be implemented by the child of FlexMemory
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
    ~FlexMemoryAllocator();
    bool reserve(int p_size);
    void trim(bool want_update_cache = true); // Remove free stack between used memory

    // Allocate memory, return null if no more space available
    Stack *allocate(int p_size);

#define deallocateStack(flex_memory, p_stack) \
    flex_memory.__deallocate(p_stack);        \
    p_stack = NULL

    /// IMPORTANT Don't call it directly, Use macro deallocateStack
    void __deallocate(Stack *p_stack);

private:
    bool redux_memory(int p_size);
    void update_cache();

    Stack *insert_stack(int p_pos = -1);
    void remove_stack(int p_pos);
};

#endif // FLEX_MEMORY_ALLOCATOR_H
