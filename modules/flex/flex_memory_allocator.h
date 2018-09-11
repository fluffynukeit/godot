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

#include "flex_utility.h"
#include "vector.h"

class FlexMemoryAllocator;

class FlexMemory {
	friend class FlexMemoryAllocator;

protected:
	virtual void resize_memory(FlexUnit p_size) = 0;
	virtual void copy_unit(FlexUnit p_to, FlexUnit p_from) = 0;

public:
	void copy(FlexBufferIndex p_to_begin_index, FlexUnit p_size, FlexBufferIndex p_from_begin_index);
};

struct MemoryChunk {
	friend class FlexMemoryAllocator;

#ifdef DEBUG_ENABLED
	FlexMemory *owner_memory;
	FlexMemoryAllocator *owner_memory_allocator;
#endif

private:
	FlexBufferIndex begin_index;
	FlexBufferIndex end_index;
	bool is_free;
	FlexUnit size;

	MemoryChunk() :
			begin_index(0),
			end_index(0),
			is_free(true),
			size(0) {}

	MemoryChunk(FlexBufferIndex p_begin_index, FlexBufferIndex p_end_index, bool p_is_free) :
			begin_index(p_begin_index),
			end_index(p_end_index),
			is_free(p_is_free),
			size(1 + p_end_index - p_begin_index) {
	}

	MemoryChunk(const MemoryChunk &other) :
			begin_index(other.begin_index),
			end_index(other.end_index),
			is_free(other.is_free),
			size(other.size) {}

	void set(FlexBufferIndex p_begin_index, FlexBufferIndex p_end_index, bool p_is_free) {
		begin_index = p_begin_index;
		end_index = p_end_index;
		is_free = p_is_free;
		size = 1 + p_end_index - p_begin_index;
	}

public:
	_FORCE_INLINE_ FlexBufferIndex get_begin_index() const { return begin_index; }
	_FORCE_INLINE_ FlexBufferIndex get_end_index() const { return end_index; }
	_FORCE_INLINE_ FlexUnit get_size() const { return size; }
	_FORCE_INLINE_ bool get_is_free() const { return is_free; }
	/// Get buffer index (relative to the memory)
	_FORCE_INLINE_ FlexBufferIndex get_buffer_index(FlexChunkIndex p_chunk_index) const { return begin_index + p_chunk_index.value; }
	/// Get chunk index (relative to this chunk)
	_FORCE_INLINE_ FlexChunkIndex get_chunk_index(FlexBufferIndex p_buffer_index) const { return p_buffer_index.value - begin_index; }
};

/// This class is responsible for memory management.
///
/// The memory has a fixed size, this size can be increased or decreased.
/// It's possible to allocate and deallocate memory chunk, and this class will take care where to put the datas.
///
/// Since this class was created with the idea to be a general purpose memory manager it doesn't
/// handle memory per byte but instead use a FlexUnit.
/// A FlexUnit can be represented by any kind of information (without any size or type restriction) depending on the implementation of FlexMemory.
///
/// FlexMemory is an interface that must be implemented in order to define memory structure and then store information.
/// As you can see neither FlexMemoryAllocator nor FlexMemory define how to set data in the memory,
/// infact this must be implemented by extending FlexMemory.
///
/// So FlexMemoryAllocator is just a class that manage memory chunk allocation.
///
///This is the structure of memory
///
///  Memory
///     |-Chunk <----------------------------------------------------- [Resource 1] (Size of resource = 1)
///     |   L--- ITEM 1 (one of more info for each item)
///     |
///     |-Chunk <----------------------------------------------------- [Resource 2] (Size of resource = 3)
///     |   |--- ITEM 1 (one of more info for each item)
///     |   |--- ITEM 2 (one of more info for each item)
///     |   L--- ITEM 3 (one of more info for each item)
///
class FlexMemoryAllocator {

	static MemoryChunk zero_memory_chunk; // Special MemoryChunk that is used when the chunk has 0 size

	Vector<MemoryChunk *> memory_table;
	FlexUnit memory_size;
	FlexUnit reallocation_extra_size;
	FlexUnit max_memory_size;

	struct {
		FlexUnit occupied_memory;
		FlexUnit biggest_free_chunk_size;
	} cache;

	FlexMemory *memory;

public:
	FlexMemoryAllocator(FlexMemory *p_memory, FlexUnit p_size, FlexUnit p_reallocation_extra_size = -1, FlexUnit p_max_memory_size = -1);
	~FlexMemoryAllocator();
	bool resize_memory(FlexUnit p_size);
	void sanitize(bool p_want_update_cache = true, bool p_trim = true);

	FlexUnit get_memory_size() const { return memory_size; }
	FlexMemory *get_memory() { return memory; }

	/// Allocate memory, return null if no more space available
	MemoryChunk *allocate_chunk(FlexUnit p_size);
	void deallocate_chunk(MemoryChunk *&r_chunk);
	void resize_chunk(MemoryChunk *&r_chunk, FlexUnit p_size, bool p_keep_data = true);

	/// If the chunks have different sizes the copy will be performed only by the size of smaller chunk
	void copy_chunk(MemoryChunk *p_to, MemoryChunk *p_from);

	FlexUnit get_last_used_index();

	FlexUnit get_chunk_count() const;
	MemoryChunk *get_chunk(FlexUnit i) const;

private:
	bool redux_memory(FlexUnit p_size);
	void find_biggest_chunk_size();

	MemoryChunk *insert_chunk(FlexBufferIndex p_index);
	MemoryChunk *create_chunk();
	void delete_chunk(FlexBufferIndex p_index);
};

class FlexMemoryModificator {
public:
	virtual void exec() = 0;
};

#endif // FLEX_MEMORY_ALLOCATOR_H
