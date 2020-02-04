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
#include "scene/3d/spatial.h"
#include <stdint.h>

// TODO can we put this on the node?
#define MAX_STORED_FRAMES 300

void PlayerNetController::_bind_methods() {

	BIND_CONSTANT(INPUT_DATA_TYPE_BOOL);
	BIND_CONSTANT(INPUT_DATA_TYPE_INT);
	BIND_CONSTANT(INPUT_DATA_TYPE_UNIT_REAL);
	BIND_CONSTANT(INPUT_DATA_TYPE_NORMALIZED_VECTOR2);

	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_0);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_1);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_2);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_3);

	ClassDB::bind_method(D_METHOD("set_player_node_path", "player_node_path"), &PlayerNetController::set_player_node_path);
	ClassDB::bind_method(D_METHOD("get_player_node_path"), &PlayerNetController::get_player_node_path);

	ClassDB::bind_method(D_METHOD("input_buffer_add_data_type", "type", "compression"), &PlayerNetController::input_buffer_add_data_type, DEFVAL(InputsBuffer::COMPRESSION_LEVEL_2));
	ClassDB::bind_method(D_METHOD("input_buffer_ready"), &PlayerNetController::input_buffer_ready);

	ClassDB::bind_method(D_METHOD("input_buffer_set_bool", "index", "bool"), &PlayerNetController::input_buffer_set_bool);
	ClassDB::bind_method(D_METHOD("input_buffer_get_bool", "index"), &PlayerNetController::input_buffer_get_bool);

	ClassDB::bind_method(D_METHOD("input_buffer_set_int", "index", "int"), &PlayerNetController::input_buffer_set_int);
	ClassDB::bind_method(D_METHOD("input_buffer_get_int", "index"), &PlayerNetController::input_buffer_get_int);

	ClassDB::bind_method(D_METHOD("input_buffer_set_unit_real", "index", "unit_real"), &PlayerNetController::input_buffer_set_unit_real);
	ClassDB::bind_method(D_METHOD("input_buffer_get_unit_real", "index"), &PlayerNetController::input_buffer_get_unit_real);

	ClassDB::bind_method(D_METHOD("input_buffer_set_normalized_vector", "index", "vector"), &PlayerNetController::input_buffer_set_normalized_vector);
	ClassDB::bind_method(D_METHOD("input_buffer_get_normalized_vector", "index"), &PlayerNetController::input_buffer_get_normalized_vector);

	// Rpc to server
	ClassDB::bind_method(D_METHOD("rpc_server_test"), &PlayerNetController::rpc_server_test);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player_node_path"), "set_player_node_path", "get_player_node_path");

	ADD_SIGNAL(MethodInfo("server_physics_process", PropertyInfo(Variant::REAL, "delta")));
	ADD_SIGNAL(MethodInfo("master_physics_process", PropertyInfo(Variant::REAL, "delta"), PropertyInfo(Variant::BOOL, "accept_new_inputs")));
	ADD_SIGNAL(MethodInfo("puppet_physics_process", PropertyInfo(Variant::REAL, "delta")));
}

PlayerNetController::PlayerNetController() :
		player_node_path(NodePath("../")),
		controller(NULL),
		cached_player(NULL) {

	rpc_config("rpc_server_test", MultiplayerAPI::RPC_MODE_REMOTE);
}

void PlayerNetController::set_player_node_path(NodePath p_path) {
	player_node_path = p_path;
	cached_player = Object::cast_to<Spatial>(get_node(player_node_path));
}

NodePath PlayerNetController::get_player_node_path() const {
	return player_node_path;
}

Spatial *PlayerNetController::get_player() const {
	return cached_player;
}

int PlayerNetController::input_buffer_add_data_type(InputDataType p_type, InputCompressionLevel p_compression) {
	return input_buffer.add_data_type((InputsBuffer::DataType)p_type, (InputsBuffer::CompressionLevel)p_compression);
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

void PlayerNetController::rpc_server_test() {
	ERR_FAIL_COND(get_tree()->is_network_server() != true);

	// TODO Propagates to the other puppets?
	// Please add a mechanism to start / stop puppets propagation.

	controller->receive_snapshots();
}

void PlayerNetController::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			ERR_FAIL_NULL_MSG(get_player(), "The `player_node_path` must point to a valid `Spatial` node.");
			controller->physics_process(get_process_delta_time());

		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (Engine::get_singleton()->is_editor_hint())
				return;

			// Unreachable.
			CRASH_COND(get_tree() == NULL);

			if (get_tree()->is_network_server()) {
				controller = memnew(ServerController);
			} else if (is_network_master()) {
				controller = memnew(MasterController);
			} else {
				controller = memnew(PuppetController);
			}
			controller->node = this;

			set_physics_process_internal(true);
			cached_player = Object::cast_to<Spatial>(get_node(player_node_path));

		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (Engine::get_singleton()->is_editor_hint())
				return;

			set_physics_process_internal(false);
			memdelete(controller);
			controller = NULL;
		} break;
	}
}

void ServerController::physics_process(real_t p_delta) {
	node->emit_signal("server_physics_process", p_delta);
}

void ServerController::receive_snapshots() {
}

MasterController::MasterController() :
		time_bank(0.0),
		tick_additional_speed(0.0) {

	processed_frames = std::deque<FramesSnapshot>();
}

void MasterController::physics_process(real_t p_delta) {

	// On `Master` side we may want to speed up input_packet
	// generation, for this reason here I'm performing a sub tick.
	// Also keep in mind that we are just pretending that the time
	// is advancing faster, for this reason we are still using
	// delta to move the player.

	const real_t pretended_delta = get_pretended_delta();

	time_bank += p_delta;
	uint32_t sub_ticks = static_cast<uint32_t>(time_bank / pretended_delta);
	time_bank -= static_cast<real_t>(sub_ticks) * pretended_delta;

	while (sub_ticks > 0) {
		sub_ticks -= 1;
		// The frame snapshot array is full only when the server connection is
		// bad.
		// Is better to not accumulate any other input, in this cases so to avoid
		// difer too much from the server.
		const bool accept_new_inputs = processed_frames.size() < MAX_STORED_FRAMES;

		// The physics process is always emitted, because we still need to simulate
		// the character motion even if we don't store the player inputs.
		node->emit_signal("master_physics_process", p_delta, accept_new_inputs);

		if (accept_new_inputs) {
			GeneratedData id = id_generator.next();
			BitArray buffer = node->get_inputs_buffer().get_buffer();

			FramesSnapshot inputs;
			inputs.id = id.id;
			inputs.inputs_buffer = buffer;
			inputs.character_transform = node->get_player()->get_global_transform();
			processed_frames.push_back(inputs);

			// This must happens here because in case of bad internet connection
			// the client accelerates the execution producing much more packets
			// per second.
			send_frame_snapshots_to_server();
		}
	}

	// TODO compute server discrepancy here?
	// TODO recover server discrepancy here?
}

void MasterController::receive_snapshots() {
	ERR_PRINTS("The master is not supposed to receive snapshots");
}

real_t MasterController::get_pretended_delta() const {
	return 1.0 / (Engine::get_singleton()->get_iterations_per_second() + tick_additional_speed);
}

void MasterController::send_frame_snapshots_to_server() const {
	node->rpc_unreliable_id(0, "rpc_server_test");
}

void PuppetController::physics_process(real_t p_delta) {
	node->emit_signal("puppet_physics_process", p_delta);
}

void PuppetController::receive_snapshots() {
}
