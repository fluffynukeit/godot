/*************************************************************************/
/*  bit_array.cpp                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "bit_array.h"

#include "core/math/math_funcs.h"

void BitArray::resize(int p_bits) {
	int min_size = Math::ceil((static_cast<float>(p_bits)) / 8);
	bytes.resize(min_size);
}

int BitArray::size_in_bytes() const {
	return bytes.size();
}

int BitArray::size_in_bits() const {
	return bytes.size() * 8;
}

void BitArray::store_bits(int p_bit_offset, uint64_t p_value, int p_bits) {

	int bits = p_bits;
	int bit_offset = p_bit_offset;
	uint64_t val = p_value;

	while (bits > 0) {
		const int bits_to_write = MIN(bits, 8 - bit_offset % 8);
		const int bits_to_jump = bit_offset % 8;
		const int bits_to_skip = 8 - (bits_to_write + bits_to_jump);
		const int byte_offset = Math::floor(static_cast<float>(bit_offset) / 8.0);

		// Clear the bits that we have to write
		//const uint8_t byte_clear = ~(((0xFF >> bits_to_jump) << (bits_to_jump + bits_to_skip)) >> bits_to_skip);
		uint8_t byte_clear = 0xFF >> bits_to_jump;
		byte_clear = byte_clear << (bits_to_jump + bits_to_skip);
		byte_clear = ~(byte_clear >> bits_to_skip);
		bytes.write[byte_offset] &= byte_clear;

		// Now we can continue to write bits
		bytes.write[byte_offset] |= (val & 0xFF) << bits_to_jump;

		bits -= bits_to_write;
		bit_offset += bits_to_write;

		val >>= bits_to_write;
	}

	CRASH_COND_MSG(val != 0, "This is a bug, please open an issue");
}

uint64_t BitArray::read_bits(int p_bit_offset, int p_bits) const {
	int bits = p_bits;
	int bit_offset = p_bit_offset;
	uint64_t val = 0;

	int val_bits_to_jump = 0;
	while (bits > 0) {
		const int bits_to_read = MIN(bits, 8 - bit_offset % 8);
		const int bits_to_jump = bit_offset % 8;
		const int bits_to_skip = 8 - (bits_to_read + bits_to_jump);
		const int byte_offset = Math::floor(static_cast<float>(bit_offset) / 8.0);

		uint8_t byte_mask = 0xFF >> bits_to_jump;
		byte_mask = byte_mask << (bits_to_skip + bits_to_jump);
		byte_mask = byte_mask >> bits_to_skip;
		const uint64_t byte_val = static_cast<uint64_t>((bytes[byte_offset] & byte_mask) >> bits_to_jump);
		val |= byte_val << val_bits_to_jump;

		bits -= bits_to_read;
		bit_offset += bits_to_read;
		val_bits_to_jump += bits_to_read;
	}

	return val;
}
