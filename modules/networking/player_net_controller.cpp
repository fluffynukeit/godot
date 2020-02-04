/*************************************************************************/
/*  player_net_controller.cpp                                            */
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

#include "player_net_controller.h"

#include "core/engine.h"
#include "core/io/marshalls.h"
#include "unsync_id_generator.h"
#include <stdint.h>

// TODO make sure 200 is correct
#define MAX_INPUT_BUFFER_SIZE 200

void PlayerNetController::_bind_methods() {

	BIND_CONSTANT(INPUT_DATA_TYPE_BOOL);
	BIND_CONSTANT(INPUT_DATA_TYPE_INT);
	BIND_CONSTANT(INPUT_DATA_TYPE_UNIT_REAL);
	BIND_CONSTANT(INPUT_DATA_TYPE_NORMALIZED_VECTOR2);

	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_0);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_1);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_2);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_3);

	ClassDB::bind_method(D_METHOD("input_buffer_add_data_type", "type", "compression"), &PlayerNetController::input_buffer_add_data_type, DEFVAL(InputBuffer::COMPRESSION_LEVEL_2));
	ClassDB::bind_method(D_METHOD("input_buffer_ready"), &PlayerNetController::input_buffer_ready);

	ClassDB::bind_method(D_METHOD("input_buffer_set_bool", "index", "bool"), &PlayerNetController::input_buffer_set_bool);
	ClassDB::bind_method(D_METHOD("input_buffer_get_bool", "index"), &PlayerNetController::input_buffer_get_bool);

	ClassDB::bind_method(D_METHOD("input_buffer_set_int", "index", "int"), &PlayerNetController::input_buffer_set_int);
	ClassDB::bind_method(D_METHOD("input_buffer_get_int", "index"), &PlayerNetController::input_buffer_get_int);

	ClassDB::bind_method(D_METHOD("input_buffer_set_unit_real", "index", "unit_real"), &PlayerNetController::input_buffer_set_unit_real);
	ClassDB::bind_method(D_METHOD("input_buffer_get_unit_real", "index"), &PlayerNetController::input_buffer_get_unit_real);

	ClassDB::bind_method(D_METHOD("input_buffer_set_normalized_vector", "index", "vector"), &PlayerNetController::input_buffer_set_normalized_vector);
	ClassDB::bind_method(D_METHOD("input_buffer_get_normalized_vector", "index"), &PlayerNetController::input_buffer_get_normalized_vector);

	ADD_SIGNAL(MethodInfo("server_physics_process", PropertyInfo(Variant::REAL, "delta")));
	ADD_SIGNAL(MethodInfo("master_physics_process", PropertyInfo(Variant::REAL, "delta"), PropertyInfo(Variant::BOOL, "input_buffer_free")));
	ADD_SIGNAL(MethodInfo("puppet_physics_process", PropertyInfo(Variant::REAL, "delta")));
}

PlayerNetController::PlayerNetController() :
		input_counter(0),
		time_bank(0.0),
		tick_additional_speed(0.0) {

	//inputs_buffer = ringbuf_new(10);
}

int PlayerNetController::input_buffer_add_data_type(InputDataType p_type, InputCompressionLevel p_compression) {
	return input_buffer.add_data_type((InputBuffer::DataType)p_type, (InputBuffer::CompressionLevel)p_compression);
}

void PlayerNetController::input_buffer_ready() {
	input_buffer.init_buffer();
}

bool PlayerNetController::input_buffer_set_bool(int p_index, bool p_input) {
	return input_buffer.set_bool(p_index, p_input);
}

bool PlayerNetController::input_buffer_get_bool(int p_index) const {
	return input_buffer.get_bool(p_index);
}

int64_t PlayerNetController::input_buffer_set_int(int p_index, int64_t p_input) {
	return input_buffer.set_int(p_index, p_input);
}

int64_t PlayerNetController::input_buffer_get_int(int p_index) const {
	return input_buffer.get_int(p_index);
}

real_t PlayerNetController::input_buffer_set_unit_real(int p_index, real_t p_input) {
	return input_buffer.set_unit_real(p_index, p_input);
}

real_t PlayerNetController::input_buffer_get_unit_real(int p_index) const {
	return input_buffer.get_unit_real(p_index);
}

Vector2 PlayerNetController::input_buffer_set_normalized_vector(int p_index, Vector2 p_input) {
	return input_buffer.set_normalized_vector(p_index, p_input);
}

Vector2 PlayerNetController::input_buffer_get_normalized_vector(int p_index) const {
	return input_buffer.get_normalized_vector(p_index);
}

void PlayerNetController::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {

			const real_t delta = get_process_delta_time();

			if (get_tree() && get_tree()->is_network_server()) {
				emit_signal("server_physics_process", delta);

			} else if (is_network_master()) {
				// On `Master` side we may want to speed up input_packet
				// generation, for this reason here I'm performing a sub tick.
				// Also keep in mind that we are just pretending that the time
				// is advancing faster, for this reason we are still using
				// delta to move the player.

				const real_t pretended_delta = get_pretended_delta();

				time_bank += delta;
				uint32_t sub_ticks = static_cast<uint32_t>(time_bank / pretended_delta);
				time_bank -= static_cast<real_t>(sub_ticks) * pretended_delta;

				while (sub_ticks > 0) {
					sub_ticks -= 1;
					// When the input buffer is full the server connection is
					// not good.
					// Is better to not accept any other input, otherwise we
					// difer from the server too much.
					const bool input_buffer_free = true; // TODO set this please
					emit_signal("master_physics_process", delta, input_buffer_free);
					if (input_buffer_free) {
						// TODO here we can send the inputs to the server
						//const uint64_t input_index = input_counter;
						input_counter += 1;
						// TODO I'm arrived here: This is an algorithm to avoid using
						// 32bit int. This algorithm let me use a ring ID on client
						// that on remote is able to keep counting.
					}
				}

				// TODO compute server discrepancy here?
				// TODO recover server discrepancy here?

			} else {
				emit_signal("puppet_physics_process", delta);
			}
		} break;
		case NOTIFICATION_ENTER_TREE:
			// TODO just a test for the unsync ids.
			LocalIdGenerator lig;
			RemoteIdReceptor rir;

			for (int i = 0; i < 140000; i += 1) {
				GeneratedData gd = lig.next();

				bool skip_receive = Math::random(0.0, 1.0) > 0.5;
				if (!skip_receive) {
					DecompressionResult s = rir.receive(gd.compressed_id);
					if (s.success) {
						print_line("Success: " + itos(gd.id) + " => " + itos(gd.compressed_id) + " --> " + itos(s.id));
					} else {
						print_line("Failed.");
					}
					//s = rir.receive((gd.compressed_id + 100) % 65535);
					//if (s.success) {
					//	print_line("Success: " + itos(gd.id) + " => " + itos((gd.compressed_id + 100) % 65535) + " --> " + itos(s.id));
					//} else {
					//	print_line("Failed.");
					//}
				}
			}

			if (Engine::get_singleton()->is_editor_hint())
				return;

			set_physics_process_internal(true);
			break;
	}
}

real_t PlayerNetController::get_pretended_delta() const {
	return 1.0 / (Engine::get_singleton()->get_iterations_per_second() + tick_additional_speed);
}
