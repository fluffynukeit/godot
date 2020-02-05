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
#include <algorithm>

// TODO can we put this on the node?
#define MAX_STORED_FRAMES 300

// TODO can we put this on the node?
// Take into consideration the last 20sec to decide the `optimal_buffer_size`.
#define PACKETS_TO_TRACK 1200

void PlayerInputsReference::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_bool", "index"), &PlayerInputsReference::get_bool);
	ClassDB::bind_method(D_METHOD("get_int", "index"), &PlayerInputsReference::get_int);
	ClassDB::bind_method(D_METHOD("get_unit_real", "index"), &PlayerInputsReference::get_unit_real);
	ClassDB::bind_method(D_METHOD("get_normalized_vector", "index"), &PlayerInputsReference::get_normalized_vector);
}

bool PlayerInputsReference::get_bool(int p_index) const {
	return input_buffer.get_bool(p_index);
}

int64_t PlayerInputsReference::get_int(int p_index) const {
	return input_buffer.get_int(p_index);
}

real_t PlayerInputsReference::get_unit_real(int p_index) const {

	return input_buffer.get_unit_real(p_index);
}

Vector2 PlayerInputsReference::get_normalized_vector(int p_index) const {
	return input_buffer.get_normalized_vector(p_index);
}

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

	ClassDB::bind_method(D_METHOD("set_max_redundant_inputs", "max_redundand_inputs"), &PlayerNetController::set_max_redundant_inputs);
	ClassDB::bind_method(D_METHOD("get_max_redundant_inputs"), &PlayerNetController::get_max_redundant_inputs);

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

	BIND_VMETHOD(MethodInfo(Variant::BOOL, "are_inputs_different", PropertyInfo(Variant::OBJECT, "inputs_A", PROPERTY_HINT_TYPE_STRING, "PlayerInputsReference"), PropertyInfo(Variant::OBJECT, "inputs_B", PROPERTY_HINT_TYPE_STRING, "PlayerInputsReference")));

	// Rpc to server
	ClassDB::bind_method(D_METHOD("rpc_server_send_frames_snapshot", "data"), &PlayerNetController::rpc_server_send_frames_snapshot);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player_node_path", PROPERTY_HINT_RANGE, "0,254,1"), "set_player_node_path", "get_player_node_path");

	ADD_SIGNAL(MethodInfo("server_physics_process", PropertyInfo(Variant::REAL, "delta")));
	ADD_SIGNAL(MethodInfo("master_physics_process", PropertyInfo(Variant::REAL, "delta"), PropertyInfo(Variant::BOOL, "accept_new_inputs")));
	ADD_SIGNAL(MethodInfo("puppet_physics_process", PropertyInfo(Variant::REAL, "delta")));
}

PlayerNetController::PlayerNetController() :
		player_node_path(NodePath("../")),
		max_redundant_inputs(9),
		controller(NULL),
		cached_player(NULL) {

	rpc_config("rpc_server_send_frames_snapshot", MultiplayerAPI::RPC_MODE_REMOTE);
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

void PlayerNetController::set_max_redundant_inputs(int p_max) {
	max_redundant_inputs = p_max;
}

int PlayerNetController::get_max_redundant_inputs() const {
	return max_redundant_inputs;
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

void PlayerNetController::rpc_server_send_frames_snapshot(PoolVector<uint8_t> p_data) {
	ERR_FAIL_COND(get_tree()->is_network_server() != true);

	// TODO Propagates to the other puppets?
	// Please add a mechanism to start / stop puppets propagation.

	controller->receive_snapshots(p_data);
}

void PlayerNetController::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			ERR_FAIL_NULL_MSG(get_player(), "The `player_node_path` must point to a valid `Spatial` node.");
			input_buffer.init_buffer();
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

ServerController::ServerController() :
		current_packet_id(UINT64_MAX),
		ghost_input_count(0),
		network_tracer(PACKETS_TO_TRACK) {
}

void ServerController::physics_process(real_t p_delta) {
	fetch_next_input();
	node->emit_signal("server_physics_process", p_delta);
}

bool is_remote_frame_A_older(const PlayerInputs &p_snap_a, const PlayerInputs &p_snap_b) {
	return p_snap_a.id < p_snap_b.id;
}

void ServerController::receive_snapshots(PoolVector<uint8_t> p_data) {
	const int data_len = p_data.size();

	if (p_data.size() < data_len)
		// Just discard, the packet is corrupted.
		// TODO Measure internet connection status here?
		return;

	PoolVector<uint8_t>::Read r = p_data.read();
	int ofs = 0;

	const int buffer_size = node->get_inputs_buffer().get_buffer_size();

	const int snapshots_count = r[ofs];
	ofs += 1;

	const int packet_size = 0
							// First byte is to store the size of the shapshots_count
							+ sizeof(uint8_t)
							// uint16_t is used to store the compressed packet ID.
							+ snapshots_count * (sizeof(uint16_t) + buffer_size);

	if (packet_size != data_len)
		// Just discard, the packet is corrupted.
		// TODO Measure internet connection status here?
		return;

	// Received data seems fine.

	for (int i = 0; i < snapshots_count; i += 1) {

		// First available byte used for the compressed input
		uint16_t compressed_packet_id = decode_uint16(r.ptr() + ofs);
		ofs += 2;

		DecompressionResult dec_res = input_id_decoder.receive(compressed_packet_id);
		// Is unlikelly that this happens in case of really bad internet connect.
		// - If this happens on production the reason is another.
		// - If this happens during test phase open an issue please.
		ERR_FAIL_COND_MSG(dec_res.success == false, "Was not possible to decode the id of the received packet.")

		if (current_packet_id >= dec_res.id)
			continue;

		PlayerInputs rfs;
		rfs.id = dec_res.id;

		const bool found = std::binary_search(player_inputs.begin(), player_inputs.end(), rfs, is_remote_frame_A_older);

		if (!found) {
			rfs.inputs_buffer.get_bytes_mut().resize(buffer_size);
			copymem(rfs.inputs_buffer.get_bytes_mut().ptrw(), r.ptr() + ofs, buffer_size);
			ofs += buffer_size;

			player_inputs.push_back(rfs);

			// Sort the new inserted snapshot.
			std::sort(player_inputs.begin(), player_inputs.end(), is_remote_frame_A_older);
		}
	}
}

bool ServerController::fetch_next_input() {
	bool is_new_input = true;

	if (unlikely(current_packet_id == UINT64_MAX)) {
		// As initial packet, anything is good.
		if (player_inputs.empty() == false) {
			node->get_inputs_buffer_mut().get_buffer_mut().get_bytes_mut() = player_inputs.front().inputs_buffer.get_bytes();
			current_packet_id = player_inputs.front().id;
			player_inputs.pop_front();
			network_tracer.notify_packet_arrived();
		} else {
			is_new_input = false;
			network_tracer.notify_missing_packet();
		}
	} else {
		// Search the next packet, the cycle is used to make sure to not stop
		// with older packets arrived too late.

		const uint64_t next_packet_id = current_packet_id + 1;

		if (unlikely(player_inputs.empty() == true)) {
			// The input buffer is empty!
			is_new_input = false;
			network_tracer.notify_missing_packet();
			ghost_input_count += 1;
			//print_line("Input buffer is void, i'm using the previous one!"); // TODO Replace with?

		} else {
			// The input buffer is not empty, search the new input.
			if (next_packet_id == player_inputs.front().id) {
				// Wow, the next input is perfect!

				node->get_inputs_buffer_mut().get_buffer_mut().get_bytes_mut() = player_inputs.front().inputs_buffer.get_bytes();
				current_packet_id = player_inputs.front().id;
				player_inputs.pop_front();

				ghost_input_count = 0;
				network_tracer.notify_packet_arrived();
			} else {
				// The next packet is not here. This can happen when:
				// - The packet is lost or not yet arrived.
				// - The client for any reason desync with the server.
				//
				// In this cases, the server has the hard task to re-sync.
				//
				// # What it does, then?
				// Initially it see that only 1 packet is missing so it just use
				// the previous one and increase `ghost_inputs_count` to 1.
				//
				// The next iteration, if the packet is not yet arrived the
				// server trys to take the next packet with the `id` less or
				// equal to `next_packet_id + ghost_packet_id`.
				//
				// As you can see the server doesn't lose immediately the hope
				// to find the missing packets, but at the same time deals with
				// it so increases its search pool per each iteration.
				//
				// # Wise input search.
				// Let's consider the case when a set of inputs arrive at the
				// same time, while the server is struggling for the missing packets.
				//
				// In the meanwhile that the packets were chilling on the net,
				// the server were simulating by guessing on their data; this
				// mean that they don't have any longer room to be simulated
				// when they arrive, and the right thing would be just forget
				// about these.
				//
				// The thing is that these can still contain meaningful data, so
				// instead to jump directly to the newest we restart the inputs
				// from the next important packet.
				//
				// For this reason we keep track the amount of missing packets
				// using `ghost_input_count`.

				network_tracer.notify_missing_packet();
				ghost_input_count += 1;

				const int size = MIN(ghost_input_count, player_inputs.size());
				const uint64_t ghost_packet_id = next_packet_id + ghost_input_count;

				bool recovered = false;
				PlayerInputs pi;

				const PlayerInputsReference pir_A(node->get_inputs_buffer());
				// Copy from the node inputs so to copy the data info
				PlayerInputsReference pir_B(node->get_inputs_buffer());

				for (int i = 0; i < size; i += 1) {
					if (ghost_packet_id < player_inputs.front().id) {
						break;
					} else {
						pi = player_inputs.front();
						player_inputs.pop_front();
						recovered = true;

						// If this input has some important changes compared to the last
						// good input, let's recover to this point otherwise skip it
						// until the last one.
						// Useful to avoid that the server stay too much behind the
						// client.

						pir_B.input_buffer.get_buffer_mut().get_bytes_mut() = pi.inputs_buffer.get_bytes();

						bool is_meaningful = false;
						if (node->get_script_instance() && node->get_script_instance()->has_method("are_inputs_different"))
							is_meaningful = node->get_script_instance()->call("are_inputs_different", &pir_A, &pir_B);
						if (is_meaningful) {
							break;
						}
					}
				}

				if (recovered) {
					node->get_inputs_buffer_mut().get_buffer_mut().get_bytes_mut() = pi.inputs_buffer.get_bytes();
					current_packet_id = pi.id;
					ghost_input_count = 0;
					// print_line("Packet recovered"); // TODO how?
				} else {
					is_new_input = false;
					// print_line("Packet still missing"); // TODO how?
				}
			}
		}
	}

	return is_new_input;
}

MasterController::MasterController() :
		time_bank(0.0),
		tick_additional_speed(0.0) {
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
			GeneratedData id = input_id_generator.next();

			FrameSnapshot inputs;
			inputs.id = id.id;
			inputs.compressed_id = id.compressed_id;
			inputs.inputs_buffer = node->get_inputs_buffer().get_buffer();
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

void MasterController::receive_snapshots(PoolVector<uint8_t> p_data) {
	ERR_PRINTS("The master is not supposed to receive snapshots");
}

real_t MasterController::get_pretended_delta() const {
	return 1.0 / (Engine::get_singleton()->get_iterations_per_second() + tick_additional_speed);
}

void MasterController::send_frame_snapshots_to_server() {
	const int snapshots_count = MIN(processed_frames.size(), node->get_max_redundant_inputs() + 1);
	ERR_FAIL_COND_MSG(snapshots_count >= UINT8_MAX, "Is not allow to send more than 254 redundant packets.");

	const int buffer_size = node->get_inputs_buffer().get_buffer_size();

	const int packet_size = 0
							// First byte is to store the size of the shapshots_count
							+ sizeof(uint8_t)
							// uint16_t is used to store the compressed packet ID.
							+ snapshots_count * (sizeof(uint16_t) + buffer_size);

	cached_packet_data.resize(packet_size);

	{
		PoolVector<uint8_t>::Write w = cached_packet_data.write();

		int ofs = 0;
		w[ofs] = snapshots_count;
		ofs += 1;

		// Compose the packets
		for (int i = processed_frames.size() - snapshots_count; i < processed_frames.size(); i += 1) {
			// Unreachable.
			CRASH_COND(processed_frames[i].inputs_buffer.get_bytes().size() != buffer_size);
			// First available byte used for the compressed input
			encode_uint16(processed_frames[i].compressed_id, w.ptr() + ofs);
			ofs += 2;
			copymem(w.ptr() + ofs, processed_frames[i].inputs_buffer.get_bytes().ptr(), buffer_size);
			ofs += buffer_size;
		}

		// Unreachable.
		CRASH_COND(ofs != packet_size);
	}

	const int server_peer_id = 0;
	const bool unreliable = true;
	node->get_multiplayer()->send_bytes_to(node, server_peer_id, unreliable, "rpc_server_send_frames_snapshot", cached_packet_data);
}

void PuppetController::physics_process(real_t p_delta) {
	node->emit_signal("puppet_physics_process", p_delta);
}

void PuppetController::receive_snapshots(PoolVector<uint8_t> p_data) {
}
