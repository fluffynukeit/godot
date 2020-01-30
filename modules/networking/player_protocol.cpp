/*************************************************************************/
/*  player_protocol.cpp                                                  */
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

#include "player_protocol.h"

#include "core/io/marshalls.h"
#include <stdint.h>

void PlayerProtocol::_bind_methods() {

	BIND_CONSTANT(INPUT_DATA_TYPE_BOOL);
	BIND_CONSTANT(INPUT_DATA_TYPE_INT);
	BIND_CONSTANT(INPUT_DATA_TYPE_UNIT_REAL);
	BIND_CONSTANT(INPUT_DATA_TYPE_NORMALIZED_VECTOR2);

	BIND_CONSTANT(COMPRESSION_LEVEL_0);
	BIND_CONSTANT(COMPRESSION_LEVEL_1);
	BIND_CONSTANT(COMPRESSION_LEVEL_2);
	BIND_CONSTANT(COMPRESSION_LEVEL_3);

	ClassDB::bind_method(D_METHOD("input_buffer_add_data_type", "type", "compression"), &PlayerProtocol::input_buffer_add_data_type, DEFVAL(COMPRESSION_LEVEL_2));
	ClassDB::bind_method(D_METHOD("input_buffer_ready"), &PlayerProtocol::input_buffer_ready);

	ClassDB::bind_method(D_METHOD("input_buffer_set_bool", "index", "bool"), &PlayerProtocol::input_buffer_set_bool);
	ClassDB::bind_method(D_METHOD("input_buffer_get_bool", "index"), &PlayerProtocol::input_buffer_get_bool);

	ClassDB::bind_method(D_METHOD("input_buffer_set_int", "index", "int"), &PlayerProtocol::input_buffer_set_int);
	ClassDB::bind_method(D_METHOD("input_buffer_get_int", "index"), &PlayerProtocol::input_buffer_get_int);

	ClassDB::bind_method(D_METHOD("input_buffer_set_unit_real", "index", "unit_real"), &PlayerProtocol::input_buffer_set_unit_real);
	ClassDB::bind_method(D_METHOD("input_buffer_get_unit_real", "index"), &PlayerProtocol::input_buffer_get_unit_real);

	ClassDB::bind_method(D_METHOD("input_buffer_set_normalized_vector", "index", "vector"), &PlayerProtocol::input_buffer_set_normalized_vector);
	ClassDB::bind_method(D_METHOD("input_buffer_get_normalized_vector", "index"), &PlayerProtocol::input_buffer_get_normalized_vector);
}

PlayerProtocol::PlayerProtocol() :
		init_phase(true) {}

int PlayerProtocol::input_buffer_add_data_type(InputDataType p_type, CompressionLevel p_compression) {
	ERR_FAIL_COND_V(init_phase == false, -1);

	const int index = input_buffer_info.size();

	InputDataMeta m;
	m.type = p_type;
	m.compression = p_compression;
	input_buffer_info.push_back(m);

	return index;
}

void PlayerProtocol::input_buffer_ready() {
	init_input_buffer();
}

bool PlayerProtocol::input_buffer_set_bool(int p_index, bool p_input) {
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), false);
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_BOOL, false);

	init_input_buffer();

	input_buffer.store_bits(input_buffer_info[p_index].bit_offset, p_input, 1);

	return p_input;
}

bool PlayerProtocol::input_buffer_get_bool(int p_index) const {
	ERR_FAIL_COND_V(init_phase != false, false);
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), false);
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_BOOL, false);

	return input_buffer.read_bits(input_buffer_info[p_index].bit_offset, 1);
}

int64_t PlayerProtocol::input_buffer_set_int(int p_index, int64_t p_input) {
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), 0);
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_INT, 0);

	init_input_buffer();

	const int bits = get_bit_taken(p_index);
	int64_t value = p_input;

	if (bits == 8) {
		value = MAX(MIN(value, INT8_MAX), INT8_MIN) & UINT8_MAX;
	} else if (bits == 16) {
		value = MAX(MIN(value, INT16_MAX), INT16_MIN) & UINT16_MAX;
	} else if (bits == 32) {
		value = MAX(MIN(value, INT32_MAX), INT32_MIN) & UINT32_MAX;
	} else {
		// Nothing to do here
	}

	input_buffer.store_bits(input_buffer_info[p_index].bit_offset, value, bits);

	if (bits == 8) {
		return static_cast<int8_t>(value);
	} else if (bits == 16) {
		return static_cast<int16_t>(value);
	} else if (bits == 32) {
		return static_cast<int32_t>(value);
	} else {
		return value;
	}
}

int64_t PlayerProtocol::input_buffer_get_int(int p_index) const {
	ERR_FAIL_COND_V(init_phase != false, 0);
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), 0);
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_INT, 0);

	const int bits = get_bit_taken(p_index);
	const uint64_t value = input_buffer.read_bits(input_buffer_info[p_index].bit_offset, bits);

	if (bits == 8) {
		return static_cast<int8_t>(value);
	} else if (bits == 16) {
		return static_cast<int16_t>(value);
	} else if (bits == 32) {
		return static_cast<int32_t>(value);
	} else {
		return static_cast<int64_t>(value);
	}
}

real_t PlayerProtocol::input_buffer_set_unit_real(int p_index, real_t p_input) {
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), 0);
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_UNIT_REAL, 0);

	init_input_buffer();

	const int bits = get_bit_taken(p_index);
	const double max_value = ~(0xFFFFFFFF << bits);

	const uint64_t compressed_val = compress_unit_float(p_input, max_value);
	input_buffer.store_bits(input_buffer_info[p_index].bit_offset, compressed_val, bits);

	return decompress_unit_float(compressed_val, max_value);
}

real_t PlayerProtocol::input_buffer_get_unit_real(int p_index) const {
	ERR_FAIL_COND_V(init_phase != false, 0);
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), 0);
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_UNIT_REAL, 0);

	const int bits = get_bit_taken(p_index);
	const double max_value = ~(0xFFFFFFFF << bits);

	const uint64_t compressed_val = input_buffer.read_bits(input_buffer_info[p_index].bit_offset, bits);

	return decompress_unit_float(compressed_val, max_value);
}

Vector2 PlayerProtocol::input_buffer_set_normalized_vector(int p_index, Vector2 p_input) {
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), Vector2());
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_NORMALIZED_VECTOR2, Vector2());

	init_input_buffer();

	const double angle = p_input.angle();

	const int bits = get_bit_taken(p_index);
	const double max_value = ~(0xFFFFFFFF << bits);

	const uint64_t compressed_angle = compress_unit_float((angle + Math_PI) / Math_TAU, max_value);

	input_buffer.store_bits(input_buffer_info[p_index].bit_offset, compressed_angle, bits);

	const real_t decompressed_angle = (decompress_unit_float(compressed_angle, max_value) * Math_TAU) - Math_PI;
	const real_t x = Math::cos(decompressed_angle);
	const real_t y = Math::sin(decompressed_angle);

	return Vector2(x, y);
}

Vector2 PlayerProtocol::input_buffer_get_normalized_vector(int p_index) const {
	ERR_FAIL_COND_V(init_phase != false, Vector2());
	ERR_FAIL_INDEX_V(p_index, input_buffer_info.size(), Vector2());
	ERR_FAIL_COND_V(input_buffer_info[p_index].type != INPUT_DATA_TYPE_NORMALIZED_VECTOR2, Vector2());

	const int bits = get_bit_taken(p_index);
	const double max_value = ~(0xFFFFFFFF << bits);

	const uint64_t compressed_angle = input_buffer.read_bits(input_buffer_info[p_index].bit_offset, bits);

	const real_t decompressed_angle = (decompress_unit_float(compressed_angle, max_value) * Math_TAU) - Math_PI;
	const real_t x = Math::cos(decompressed_angle);
	const real_t y = Math::sin(decompressed_angle);

	return Vector2(x, y);
}

uint64_t PlayerProtocol::compress_unit_float(double p_value, double p_scale_factor) {
	return MIN(p_value * p_scale_factor, p_scale_factor);
}

double PlayerProtocol::decompress_unit_float(uint64_t p_value, double p_scale_factor) {
	return static_cast<double>(p_value) / p_scale_factor;
}

int PlayerProtocol::get_bit_taken(int p_input_data_index) const {
	switch (input_buffer_info[p_input_data_index].type) {
		case INPUT_DATA_TYPE_BOOL:
			// No matter what, 1 bit.
			return 1;
		case INPUT_DATA_TYPE_INT: {
			switch (input_buffer_info[p_input_data_index].compression) {
				case COMPRESSION_LEVEL_0:
					return 64;
				case COMPRESSION_LEVEL_1:
					return 32;
				case COMPRESSION_LEVEL_2:
					return 16;
				case COMPRESSION_LEVEL_3:
					return 8;
				default:
					// Unreachable
					CRASH_NOW_MSG("Compression level not supported!");
			}
		} break;
		case INPUT_DATA_TYPE_UNIT_REAL: {
			switch (input_buffer_info[p_input_data_index].compression) {
				case COMPRESSION_LEVEL_0:
					// Max loss ~0.09%
					return 10;
				case COMPRESSION_LEVEL_1:
					// Max loss ~0.3%
					return 8;
				case COMPRESSION_LEVEL_2:
					// Max loss ~3.2%
					return 6;
				case COMPRESSION_LEVEL_3:
					// Max loss ~6%
					return 4;
				default:
					// Unreachable
					CRASH_NOW_MSG("Compression level not supported!");
			}
		} break;
		case INPUT_DATA_TYPE_NORMALIZED_VECTOR2: {
			switch (input_buffer_info[p_input_data_index].compression) {
				case CompressionLevel::COMPRESSION_LEVEL_0:
					// Max loss 0.17°
					return 11;
				case CompressionLevel::COMPRESSION_LEVEL_1:
					// Max loss 0.35°
					return 10;
				case CompressionLevel::COMPRESSION_LEVEL_2:
					// Max loss 0.7°
					return 9;
				case CompressionLevel::COMPRESSION_LEVEL_3:
					// Max loss 1.1°
					return 8;
			}
		} break;
		default:
			// Unreachable
			CRASH_NOW_MSG("Input type not supported!");
	}

	// Unreachable
	CRASH_NOW_MSG("Was not possible to obtain the bit taken by this input data.");
}

void PlayerProtocol::init_input_buffer() {
	if (!init_phase)
		return;
	init_phase = false;

	int bits = 0;
	for (int i = 0; i < input_buffer_info.size(); i += 1) {
		input_buffer_info.write[i].bit_offset = bits;
		bits += get_bit_taken(i);
	}

	input_buffer.resize(bits);
}
