/*************************************************************************/
/*  player_net_controller.h                                              */
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

#include "input_buffer.h"
#include <deque>

#ifndef PLAYERPNETCONTROLLER_H
#define PLAYERPNETCONTROLLER_H

class PlayerNetController : public Node {
	GDCLASS(PlayerNetController, Node);

public:
	enum InputDataType {
		INPUT_DATA_TYPE_BOOL = InputBuffer::DATA_TYPE_BOOL,
		INPUT_DATA_TYPE_INT = InputBuffer::DATA_TYPE_INT,
		INPUT_DATA_TYPE_UNIT_REAL = InputBuffer::DATA_TYPE_UNIT_REAL,
		INPUT_DATA_TYPE_NORMALIZED_VECTOR2 = InputBuffer::DATA_TYPE_NORMALIZED_VECTOR2
	};

	enum InputCompressionLevel {
		INPUT_COMPRESSION_LEVEL_0 = InputBuffer::COMPRESSION_LEVEL_0,
		INPUT_COMPRESSION_LEVEL_1 = InputBuffer::COMPRESSION_LEVEL_1,
		INPUT_COMPRESSION_LEVEL_2 = InputBuffer::COMPRESSION_LEVEL_2,
		INPUT_COMPRESSION_LEVEL_3 = InputBuffer::COMPRESSION_LEVEL_3
	};

	struct InputData {
		uint64_t index;
		Vector<uint8_t> input_buffer;
	};

private:
	uint64_t input_counter;
	InputBuffer input_buffer;
	std::deque<InputData> a;

	// Used by the `Master` to control the tick speed.
	real_t time_bank;
	// Used by the `Master` to control the tick speed.
	real_t tick_additional_speed;

public:
	static void _bind_methods();

public:
	PlayerNetController();

	int input_buffer_add_data_type(InputDataType p_type, InputCompressionLevel p_compression = INPUT_COMPRESSION_LEVEL_2);
	void input_buffer_ready();

	/// Set bool
	/// Returns the same data.
	bool input_buffer_set_bool(int p_index, bool p_input);

	/// Get boolean value
	bool input_buffer_get_bool(int p_index) const;

	/// Set integer
	///
	/// Returns the stored values, you can store up to the max value for the
	/// compression.
	int64_t input_buffer_set_int(int p_index, int64_t p_input);

	/// Get integer
	int64_t input_buffer_get_int(int p_index) const;

	/// Set the unit real.
	///
	/// **Note:** Not unitary values lead to unexpected behaviour.
	///
	/// Returns the compressed value so both the client and the peers can use
	/// the same data.
	real_t input_buffer_set_unit_real(int p_index, real_t p_input);

	/// Returns the unit real
	real_t input_buffer_get_unit_real(int p_index) const;

	/// Set a normalized vector.
	/// Note: The compression algorithm rely on the fact that this is a
	/// normalized vector. The behaviour is unexpected for not normalized vectors.
	///
	/// Returns the decompressed vector so both the client and the peers can use
	/// the same data.
	Vector2 input_buffer_set_normalized_vector(int p_index, Vector2 p_input);

	/// Get the normalized vector from the input buffer.
	Vector2 input_buffer_get_normalized_vector(int p_index) const;

private:
	virtual void _notification(int p_what);

	real_t get_pretended_delta() const;
};

VARIANT_ENUM_CAST(PlayerNetController::InputDataType)
VARIANT_ENUM_CAST(PlayerNetController::InputCompressionLevel)

#endif
