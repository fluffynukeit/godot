/*************************************************************************/
/*  player_protocol.h                                                    */
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

#include "scene/main/node.h"

#include "bit_array.h"

#ifndef INPUT_BUFFER_H
#define INPUT_BUFFER_H

class InputsBuffer {
public:
	enum DataType {
		DATA_TYPE_BOOL,
		DATA_TYPE_INT,
		DATA_TYPE_UNIT_REAL,
		DATA_TYPE_NORMALIZED_VECTOR2
	};

	/// Compression level for the stored input data.
	///
	/// Depending on the data type and the compression level used the amount of
	/// bits used and loss change.
	///
	/// ## Bool
	/// Always use 1 bit
	///
	/// ## Int
	/// COMPRESSION_LEVEL_0: 64 bytes are used - Stores integers -9223372036854775808 / 9223372036854775807
	/// COMPRESSION_LEVEL_1: 32 bytes are used - Stores integers -2147483648 / 2147483647
	/// COMPRESSION_LEVEL_2: 16 bytes are used - Stores integers -32768 / 32767
	/// COMPRESSION_LEVEL_3: 8 bytes are used - Stores integers -128 / 127
	///
	///
	/// ## Unit real
	/// COMPRESSION_LEVEL_0: 10 bytes are used - Max loss 0.09%
	/// COMPRESSION_LEVEL_1: 8 bytes are used - Max loss 0.3%
	/// COMPRESSION_LEVEL_2: 6 bytes are used - Max loss 3.2%
	/// COMPRESSION_LEVEL_3: 4 bytes are used - Max loss 6%
	///
	///
	/// ## Vector2
	/// COMPRESSION_LEVEL_0: 11 bytes are used - Max loss 0.17°
	/// COMPRESSION_LEVEL_1: 10 bytes are used - Max loss 0.35°
	/// COMPRESSION_LEVEL_2: 9 bytes are used - Max loss 0.7°
	/// COMPRESSION_LEVEL_3: 8 bytes are used - Max loss 1.1°
	enum CompressionLevel {
		COMPRESSION_LEVEL_0,
		COMPRESSION_LEVEL_1,
		COMPRESSION_LEVEL_2,
		COMPRESSION_LEVEL_3
	};

	struct InputDataMeta {
		DataType type;
		CompressionLevel compression;
		int bit_offset;
	};

private:
	bool init_phase;
	Vector<InputDataMeta> buffer_info;
	BitArray buffer;

public:
	InputsBuffer();

	int add_data_type(DataType p_type, CompressionLevel p_compression);
	void init_buffer();

	const BitArray &get_buffer() const {
		return buffer;
	}

	BitArray &get_buffer_mut() {
		return buffer;
	}

	// Returns the buffer size in bytes
	int get_buffer_size() const;

	/// Set bool
	/// Returns the same data.
	bool set_bool(int p_index, bool p_input);

	/// Get boolean value
	bool get_bool(int p_index) const;

	/// Set integer
	///
	/// Returns the stored values, you can store up to the max value for the
	/// compression.
	int64_t set_int(int p_index, int64_t p_input);

	/// Get integer
	int64_t get_int(int p_index) const;

	/// Set the unit real.
	///
	/// **Note:** Not unitary values lead to unexpected behaviour.
	///
	/// Returns the compressed value so both the client and the peers can use
	/// the same data.
	real_t set_unit_real(int p_index, real_t p_input);

	/// Returns the unit real
	real_t get_unit_real(int p_index) const;

	/// Set a normalized vector.
	/// Note: The compression algorithm rely on the fact that this is a
	/// normalized vector. The behaviour is unexpected for not normalized vectors.
	///
	/// Returns the decompressed vector so both the client and the peers can use
	/// the same data.
	Vector2 set_normalized_vector(int p_index, Vector2 p_input);

	/// Get the normalized vector from the input buffer.
	Vector2 get_normalized_vector(int p_index) const;

	// Puts all the bytes to 0.
	void zero();

private:
	static uint64_t compress_unit_float(double p_value, double p_scale_factor);
	static double decompress_unit_float(uint64_t p_value, double p_scale_factor);

	int get_bit_taken(int p_input_data_index) const;
};

#endif
