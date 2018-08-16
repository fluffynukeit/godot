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

MemoryChunk FlexMemoryAllocator::zero_memory_chunk(0, -1, false);

void FlexMemory::copy(FlexBufferIndex p_to_begin_index, FlexUnit p_size, FlexBufferIndex p_from_begin_index) {
	for (int i(0); i < p_size; ++i) {
		copy_unit(p_to_begin_index + i, p_from_begin_index + i);
	}
}

FlexMemoryAllocator::FlexMemoryAllocator(FlexMemory *p_memory, FlexUnit p_size, FlexUnit p_reallocation_extra_size, FlexUnit p_max_memory_size) :
		memory(p_memory),
		memory_size(0),
		reallocation_extra_size(p_reallocation_extra_size),
		max_memory_size(p_max_memory_size) {

	cache.occupied_memory = 0;
	cache.biggest_free_chunk_size = 0;
	resize_memory(p_size);
}

FlexMemoryAllocator::~FlexMemoryAllocator() {
	// Delete all chunk
	for (FlexUnit i(memory_table.size() - 1); 0 <= i; --i) {
		delete memory_table[i];
	}
}

bool FlexMemoryAllocator::resize_memory(FlexUnit p_size) {
	if (memory_size == p_size)
		return true;

	if (max_memory_size > -1 && p_size > max_memory_size) {
		ERR_PRINT("Memory limit reached");
		return false;
	}

	if (memory_size > p_size) {

		ERR_FAIL_COND_V(cache.occupied_memory > p_size, false);

		if (!redux_memory(p_size)) { // Try to redux size without trim
			sanitize(false);
			ERR_FAIL_COND_V(!redux_memory(p_size), false);
		}
	} else {
		// just increase memory space

		if (memory_table.size() <= 0) {
			create_chunk()->set(memory_size, p_size - 1, true);
		} else {
			MemoryChunk *last_chunk = memory_table[memory_table.size() - 1];
			if (last_chunk->is_free) {
				last_chunk->set(last_chunk->begin_index, p_size - 1, true);
			} else {
				create_chunk()->set(last_chunk->end_index + 1, p_size - 1, true);
			}
		}
	}
	memory_size = p_size;
	memory->resize_memory(p_size);

	find_biggest_chunk_size();
	return true;
}

void FlexMemoryAllocator::sanitize(bool p_want_update_cache, bool p_trim) {

	FlexUnit msize(memory_table.size());
	for (FlexUnit i(0); i < msize; ++i) {
		if (!memory_table[i]->is_free)
			continue;

		const FlexUnit next_i = i + 1;
		if (next_i >= msize)
			break; // End of memory table

		if (memory_table[next_i]->is_free) {

			// MERGE
			memory_table[i]->set(memory_table[i]->begin_index, memory_table[next_i]->end_index, true);
			delete_chunk(next_i);
			msize = memory_table.size();
			--i; // This is required in order to refetch this chunk.
		} else {

			if (!p_trim)
				continue;

			// SWAP
			const MemoryChunk i_chunk_cpy(*memory_table[i]);
			const MemoryChunk next_i_chunk_cpy(*memory_table[next_i]);

			memory_table[next_i]->set(i_chunk_cpy.begin_index, i_chunk_cpy.begin_index + next_i_chunk_cpy.size - 1, false);
			memory_table[i]->set(memory_table[next_i]->end_index + 1, next_i_chunk_cpy.end_index, true);

			MemoryChunk *i_chunk = memory_table[i];
			memory_table.write[i] = memory_table[next_i];
			memory_table.write[next_i] = i_chunk;

			// Shift back data, even if the chunks collides it will work because the copy is performed incrementally (in the opposite way of the swap [free space] <- [data])
			memory->copy(memory_table[i]->begin_index, next_i_chunk_cpy.size, next_i_chunk_cpy.begin_index);
		}
	}

	if (p_want_update_cache)
		find_biggest_chunk_size();
}

MemoryChunk *FlexMemoryAllocator::allocate_chunk(FlexUnit p_size) {

	if (p_size == 898)
		int a = 0;

	if (0 >= p_size)
		return &zero_memory_chunk;

	bool space_available = false;
	if (p_size <= cache.biggest_free_chunk_size) {
		space_available = true;
	} else {
		const int total_space = memory_size - cache.occupied_memory;
		if (total_space >= p_size) {
			// Space is available but require trim
			space_available = true;
			sanitize(false);
		}
	}

	if (!space_available) {
		if (reallocation_extra_size > -1 && resize_memory(memory_size + p_size + reallocation_extra_size)) {
			return allocate_chunk(p_size);
		} else {
			ERR_PRINT("No space available in this memory!");
			return NULL;
		}
	}

	const FlexUnit size(memory_table.size());
	for (FlexUnit i = 0; i < size; ++i) {
		if (!memory_table[i]->is_free || memory_table[i]->size < p_size)
			continue;

		if (memory_table[i]->size != p_size) {
			// Perform split of current chunk
			insert_chunk(i + 1)->set(memory_table[i]->begin_index + p_size, memory_table[i]->end_index, true);
		}
		memory_table[i]->set(memory_table[i]->begin_index, memory_table[i]->begin_index + p_size - 1, false);
		cache.occupied_memory += memory_table[i]->size;
		find_biggest_chunk_size();
		return memory_table[i];
	}

	ERR_PRINT("No space available in this memory!");
	return NULL;
}

void FlexMemoryAllocator::deallocate_chunk(MemoryChunk *&r_chunk) {

	if (!r_chunk || r_chunk == (&zero_memory_chunk))
		return;

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(r_chunk->owner_memory_allocator != this);
#endif

	r_chunk->is_free = true;
	cache.occupied_memory -= r_chunk->size;

	// update cache
	if (cache.biggest_free_chunk_size < r_chunk->size) {
		cache.biggest_free_chunk_size = r_chunk->size;
	}

	r_chunk = NULL;

	sanitize(false, false); // Merge only, no cache update, no trim
}

void FlexMemoryAllocator::resize_chunk(MemoryChunk *&r_chunk, FlexUnit p_size) {

	ERR_FAIL_COND(p_size < 0);

	if (r_chunk->size == p_size)
		return;

#ifdef DEBUG_ENABLED
	if (r_chunk != (&zero_memory_chunk))
		ERR_FAIL_COND(r_chunk->owner_memory_allocator != this);
#endif

	if (0 >= p_size) {

		deallocate_chunk(r_chunk);
		r_chunk = &zero_memory_chunk;

	} else if (r_chunk->size > p_size) {

		/// Redux memory, don't need reallocation
		// 1. Split chunk
		FlexUnit chunk_index = memory_table.find(r_chunk);
		ERR_FAIL_COND(chunk_index == -1);

		FlexUnit new_chunk_size = r_chunk->size - p_size;
		MemoryChunk *new_chunk = insert_chunk(chunk_index + 1);
		new_chunk->set(r_chunk->end_index - new_chunk_size + 1, r_chunk->end_index, true);

		// 2. Redux
		r_chunk->set(r_chunk->begin_index, new_chunk->begin_index - 1, false);

	} else {

		// Re allocate all chunk
		MemoryChunk *new_chunk = allocate_chunk(p_size);
		copy_chunk(new_chunk, r_chunk);
		deallocate_chunk(r_chunk);
		r_chunk = new_chunk;
	}
}

void FlexMemoryAllocator::copy_chunk(MemoryChunk *p_to, MemoryChunk *p_from) {

#ifdef DEBUG_ENABLED
	if (p_to != (&zero_memory_chunk))
		ERR_FAIL_COND(p_to->owner_memory_allocator != this);

	if (p_from != (&zero_memory_chunk))
		ERR_FAIL_COND(p_from->owner_memory_allocator != this);
#endif

	const FlexUnit copy_size = MIN(p_from->size, p_to->size); // Avoid to copy more then the destination size
	memory->copy(p_to->begin_index, copy_size, p_from->begin_index);
}

FlexUnit FlexMemoryAllocator::get_last_used_index() {
	for (int i(memory_table.size() - 1); 0 <= i; --i) {
		if (!memory_table[i]->is_free) {
			return memory_table[i]->end_index;
		}
	}
	return -1;
}

FlexUnit FlexMemoryAllocator::get_chunk_count() const {
	return memory_table.size();
}

MemoryChunk *FlexMemoryAllocator::get_chunk(FlexUnit i) const {
	memory_table.get(i);
}

bool FlexMemoryAllocator::redux_memory(FlexUnit p_size) {
	if (memory_table.size() > 0) {
		const FlexUnit last_chunk_index = memory_table.size() - 1;
		MemoryChunk *last_chunk = memory_table[last_chunk_index];
		if (last_chunk->is_free && last_chunk->begin_index >= p_size) {

			// The last free chunk fits in the new size
			if (last_chunk->begin_index == p_size) {
				// When the last chunk begin is equal to the new size, no more space available
				delete_chunk(last_chunk_index);
			} else {
				// When the last chunk begin is less then new size, space is available
				last_chunk->set(last_chunk->begin_index, p_size - 1, true);
			}
			memory_size = p_size;
			memory->resize_memory(p_size);
			return true;
		}
		return false;
	} else {
		return false;
	}
}

void FlexMemoryAllocator::find_biggest_chunk_size() {

	// Get biggest chunk size
	cache.biggest_free_chunk_size = 0;
	for (FlexUnit i = memory_table.size() - 1; i >= 0; --i) {
		if (memory_table[i]->is_free && cache.biggest_free_chunk_size < memory_table[i]->size) {
			cache.biggest_free_chunk_size = memory_table[i]->size;
		}
	}
}

MemoryChunk *FlexMemoryAllocator::insert_chunk(FlexBufferIndex p_index) {
	MemoryChunk *chunk = new MemoryChunk;
#ifdef DEBUG_ENABLED
	chunk->owner_memory = memory;
	chunk->owner_memory_allocator = this;
#endif
	memory_table.insert(p_index, chunk);
	return chunk;
}

MemoryChunk *FlexMemoryAllocator::create_chunk() {
	MemoryChunk *chunk = new MemoryChunk;
#ifdef DEBUG_ENABLED
	chunk->owner_memory = memory;
	chunk->owner_memory_allocator = this;
#endif
	memory_table.push_back(chunk);
	return chunk;
}

void FlexMemoryAllocator::delete_chunk(FlexBufferIndex p_index) {
	delete memory_table[p_index];
	memory_table.remove(p_index);
}
