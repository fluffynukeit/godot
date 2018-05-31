/*************************************************************************/
/*  flex_memory_allocator.cpp                                            */
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

#include "flex_memory_allocator.h"
#include "print_string.h"

FlexMemoryAllocator::FlexMemoryAllocator(FlexMemory *p_memory, int p_size) :
        memory(p_memory),
        memory_size(0) {

    cache.occupied_memory = 0;
    cache.biggest_stack_size = 0;
    reserve(p_size);
}

FlexMemoryAllocator::~FlexMemoryAllocator() {
    // Delete all Stack
    for (int i(memory_table.size() - 1); i <= 0; --i) {
        delete memory_table[i];
    }
}

bool FlexMemoryAllocator::reserve(int p_size) {
    if (memory_size == p_size)
        return true;

    if (memory_size > p_size) {

        ERR_FAIL_COND_V(cache.occupied_memory > p_size, false);

        if (!redux_memory(p_size)) { // Try to redux size without trim
            trim(false);
            ERR_FAIL_COND_V(redux_memory(p_size), false);
        }
    } else {
        // just increase memory space

        if (memory_table.size() <= 0) {
            insert_stack()->set(memory_size, p_size - 1, true);
        } else {
            Stack *last_stack = memory_table[memory_table.size() - 1];
            if (last_stack->is_free) {
                last_stack->set(last_stack->begin_index, p_size - 1, true);
            } else {
                insert_stack()->set(last_stack->end_index + 1, p_size - 1, true);
            }
        }
        memory_size = p_size;
        memory->reserve(p_size);
    }

    update_cache();
    return true;
}

void FlexMemoryAllocator::trim(bool want_update_cache) {

    int msize(memory_table.size());
    for (int i(0); i < msize; ++i) {
        if (!memory_table[i]->is_free)
            continue;

        const int next_i = i + 1;
        if (next_i >= msize)
            break; // End of memory table

        if (memory_table[next_i]->is_free) {

            // MERGE
            memory_table[i]->set(memory_table[i]->begin_index, memory_table[next_i]->end_index, true);
            remove_stack(next_i);
            msize = memory_table.size();
            --i; // This is required in order to refetch this stack.
        } else {

            // SWAP
            const Stack i_stack_cpy(*memory_table[i]);
            const Stack next_i_stack_cpy(*memory_table[next_i]);

            // TODO remove
            Stack *a = memory_table[i];
            Stack *b = memory_table[next_i];

            memory_table[next_i]->set(i_stack_cpy.begin_index, i_stack_cpy.begin_index + next_i_stack_cpy.size - 1, false);
            memory_table[i]->set(memory_table[next_i]->end_index + 1, next_i_stack_cpy.end_index, true);

            Stack *i_stack = memory_table[i];
            memory_table[i] = memory_table[next_i];
            memory_table[next_i] = i_stack;

            memory->shift_back(next_i_stack_cpy.begin_index, next_i_stack_cpy.end_index, /*shift <- */ i_stack_cpy.size);
        }
    }

    if (want_update_cache)
        update_cache();
}

Stack *FlexMemoryAllocator::allocate(int p_size) {
    bool space_available = false;
    if (p_size <= cache.biggest_stack_size) {
        space_available = true;
    } else {
        const int total_space = memory_size - cache.occupied_memory;
        if (total_space >= p_size) {
            // Space is available but require trim
            space_available = true;
            trim(false);
        }
    }

    if (!space_available) {
        print_error("No space available in this memory!");
        return NULL;
    }

    const int size(memory_table.size());
    for (int i = 0; i < size; ++i) {
        if (!memory_table[i]->is_free || memory_table[i]->size < p_size)
            continue;

        if (memory_table[i]->size != p_size) {
            // Perform split of current stack
            insert_stack(i + 1)->set(memory_table[i]->begin_index + p_size, memory_table[i]->end_index, true);
        }
        memory_table[i]->set(memory_table[i]->begin_index, memory_table[i]->begin_index + p_size - 1, false);
        cache.occupied_memory += memory_table[i]->size;
        update_cache();
        return memory_table[i];
    }

    print_error("No space available in this memory!");
    return NULL;
}

void FlexMemoryAllocator::__deallocate(Stack *p_stack) {
    p_stack->is_free = true;

    cache.occupied_memory -= p_stack->size;
    // update cache
    if (cache.biggest_stack_size < p_stack->size) {
        cache.biggest_stack_size = p_stack->size;
    }
}

bool FlexMemoryAllocator::redux_memory(int p_size) {
    if (memory_table.size() > 0) {
        const int last_stack_index = memory_table.size() - 1;
        Stack *last_stack = memory_table[last_stack_index];
        if (last_stack->is_free && last_stack->begin_index >= p_size) {

            // The last free stack fits in the new size
            if (last_stack->begin_index == p_size) {
                // When the last stack begin is equal to the new size, no more space available
                remove_stack(last_stack_index);
            } else {
                // When the last stack begin is less then new size, space is available
                last_stack->set(last_stack->begin_index, p_size - 1, true);
            }
            memory_size = p_size;
            memory->reserve(p_size);
            return true;
        }
        return false;
    } else {
        return false;
    }
}

void FlexMemoryAllocator::update_cache() {

    // Get biggest stack size
    cache.biggest_stack_size = 0;
    for (int i = memory_table.size() - 1; i >= 0; --i) {
        if (memory_table[i]->is_free && cache.biggest_stack_size < memory_table[i]->size) {
            cache.biggest_stack_size = memory_table[i]->size;
        }
    }
}

Stack *FlexMemoryAllocator::insert_stack(int p_pos) {
    Stack *stack = new Stack;
    if (p_pos == -1)
        memory_table.push_back(stack);
    else
        memory_table.insert(p_pos, stack);
    return stack;
}

void FlexMemoryAllocator::remove_stack(int p_pos) {
    delete memory_table[p_pos];
    memory_table.remove(p_pos);
}
