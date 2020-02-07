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

// TODO can we put all these as node parameters???

#define MAX_STORED_FRAMES 300

// Take into consideration the last 20sec to decide the `optimal_buffer_size`.
#define PACKETS_TO_TRACK 1200

// Don't go below 2 so to take into account internet latency
#define MIN_SNAPSHOTS_SIZE 2

// This parameter is used to amortise packet loss.
// This parameter is already really high and tests showed that 10 is already
// enough to heal 5% of packet loss (Connections with Packet loss above 1% are
// considered broken).
#define MAX_SNAPSHOTS_SIZE 30

#define SNAPSHOTS_SIZE_ACCELERATION 2.5

#define MISSING_SNAPSHOTS_MAX_TOLLERANCE 4

#define TICK_ACCELERATION 2.0

#define MAX_ADDITIONAL_TICK_SPEED 2.0

// 2%
#define TICK_SPEED_CHANGE_NOTIF_THRESHOLD 4

#define PEERS_STATE_CHECK_INTERVAL 10.0 / 60.0

void PlayerInputsReference::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_bool", "index"), &PlayerInputsReference::get_bool);
	ClassDB::bind_method(D_METHOD("get_int", "index"), &PlayerInputsReference::get_int);
	ClassDB::bind_method(D_METHOD("get_unit_real", "index"), &PlayerInputsReference::get_unit_real);
	ClassDB::bind_method(D_METHOD("get_normalized_vector", "index"), &PlayerInputsReference::get_normalized_vector);
}

bool PlayerInputsReference::get_bool(int p_index) const {
	return inputs_buffer.get_bool(p_index);
}

int64_t PlayerInputsReference::get_int(int p_index) const {
	return inputs_buffer.get_int(p_index);
}

real_t PlayerInputsReference::get_unit_real(int p_index) const {

	return inputs_buffer.get_unit_real(p_index);
}

Vector2 PlayerInputsReference::get_normalized_vector(int p_index) const {
	return inputs_buffer.get_normalized_vector(p_index);
}

void PlayerInputsReference::set_inputs_buffer(const BitArray &p_new_buffer) {
	inputs_buffer.get_buffer_mut().get_bytes_mut() = p_new_buffer.get_bytes();
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
	ClassDB::bind_method(D_METHOD("get_player"), &PlayerNetController::get_player);

	ClassDB::bind_method(D_METHOD("set_max_redundant_inputs", "max_redundand_inputs"), &PlayerNetController::set_max_redundant_inputs);
	ClassDB::bind_method(D_METHOD("get_max_redundant_inputs"), &PlayerNetController::get_max_redundant_inputs);

	ClassDB::bind_method(D_METHOD("set_check_state_position_only", "check_type"), &PlayerNetController::set_check_state_position_only);
	ClassDB::bind_method(D_METHOD("get_check_state_position_only"), &PlayerNetController::get_check_state_position_only);

	ClassDB::bind_method(D_METHOD("set_discrepancy_recover_velocity", "velocity"), &PlayerNetController::set_discrepancy_recover_velocity);
	ClassDB::bind_method(D_METHOD("get_discrepancy_recover_velocity"), &PlayerNetController::get_discrepancy_recover_velocity);

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

	ClassDB::bind_method(D_METHOD("set_puppet_active", "peer_id", "active"), &PlayerNetController::set_puppet_active);
	ClassDB::bind_method(D_METHOD("_on_peer_connection_change", "peer_id"), &PlayerNetController::on_peer_connection_change);

	BIND_VMETHOD(MethodInfo("collect_inputs"));
	BIND_VMETHOD(MethodInfo("step_player", PropertyInfo(Variant::REAL, "delta")));
	BIND_VMETHOD(MethodInfo(Variant::BOOL, "are_inputs_different", PropertyInfo(Variant::OBJECT, "inputs_A", PROPERTY_HINT_TYPE_STRING, "PlayerInputsReference"), PropertyInfo(Variant::OBJECT, "inputs_B", PROPERTY_HINT_TYPE_STRING, "PlayerInputsReference")));

	// Rpc to server
	ClassDB::bind_method(D_METHOD("rpc_server_send_frames_snapshot", "data"), &PlayerNetController::rpc_server_send_frames_snapshot);

	// Rpc to master
	ClassDB::bind_method(D_METHOD("rpc_master_send_tick_additional_speed", "tick_speed"), &PlayerNetController::rpc_master_send_tick_additional_speed);

	// Rpc to puppets
	ClassDB::bind_method(D_METHOD("rpc_puppet_send_frames_snapshot", "data"), &PlayerNetController::rpc_puppet_send_frames_snapshot);

	// Rpc to master and puppets
	ClassDB::bind_method(D_METHOD("rpc_send_player_state", "snapshot_id", "data"), &PlayerNetController::rpc_send_player_state);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player_node_path"), "set_player_node_path", "get_player_node_path");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_redundant_inputs", PROPERTY_HINT_RANGE, "0,254,1"), "set_max_redundant_inputs", "get_max_redundant_inputs");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "check_state_position_only"), "set_check_state_position_only", "get_check_state_position_only");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "discrepancy_recover_velocity", PROPERTY_HINT_RANGE, "0.0,1000.0,0.1"), "set_discrepancy_recover_velocity", "get_discrepancy_recover_velocity");
}

PlayerNetController::PlayerNetController() :
		player_node_path(NodePath("../")),
		max_redundant_inputs(50),
		check_state_position_only(true),
		discrepancy_recover_velocity(50.0),
		controller(NULL),
		cached_player(NULL) {

	rpc_config("rpc_server_send_frames_snapshot", MultiplayerAPI::RPC_MODE_REMOTE);

	rpc_config("rpc_master_send_tick_additional_speed", MultiplayerAPI::RPC_MODE_MASTER);

	rpc_config("rpc_puppet_send_frames_snapshot", MultiplayerAPI::RPC_MODE_PUPPET);

	rpc_config("rpc_send_player_state", MultiplayerAPI::RPC_MODE_REMOTE);
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

void PlayerNetController::set_check_state_position_only(bool p_check_position_only) {
	check_state_position_only = p_check_position_only;
}

bool PlayerNetController::get_check_state_position_only() const {
	return check_state_position_only;
}

void PlayerNetController::set_discrepancy_recover_velocity(real_t p_velocity) {
	discrepancy_recover_velocity = p_velocity;
}

real_t PlayerNetController::get_discrepancy_recover_velocity() const {
	return discrepancy_recover_velocity;
}

int PlayerNetController::input_buffer_add_data_type(InputDataType p_type, InputCompressionLevel p_compression) {
	return inputs_buffer.add_data_type((InputsBuffer::DataType)p_type, (InputsBuffer::CompressionLevel)p_compression);
}

void PlayerNetController::input_buffer_ready() {
	inputs_buffer.init_buffer();
}

bool PlayerNetController::input_buffer_set_bool(int p_index, bool p_input) {
	return inputs_buffer.set_bool(p_index, p_input);
}

bool PlayerNetController::input_buffer_get_bool(int p_index) const {
	return inputs_buffer.get_bool(p_index);
}

int64_t PlayerNetController::input_buffer_set_int(int p_index, int64_t p_input) {
	return inputs_buffer.set_int(p_index, p_input);
}

int64_t PlayerNetController::input_buffer_get_int(int p_index) const {
	return inputs_buffer.get_int(p_index);
}

real_t PlayerNetController::input_buffer_set_unit_real(int p_index, real_t p_input) {
	return inputs_buffer.set_unit_real(p_index, p_input);
}

real_t PlayerNetController::input_buffer_get_unit_real(int p_index) const {
	return inputs_buffer.get_unit_real(p_index);
}

Vector2 PlayerNetController::input_buffer_set_normalized_vector(int p_index, Vector2 p_input) {
	return inputs_buffer.set_normalized_vector(p_index, p_input);
}

Vector2 PlayerNetController::input_buffer_get_normalized_vector(int p_index) const {
	return inputs_buffer.get_normalized_vector(p_index);
}

void PlayerNetController::set_puppet_active(int p_peer_id, bool p_active) {
	ERR_FAIL_COND_MSG(get_tree()->is_network_server() == false, "You can set puppet activation only on server");
	ERR_FAIL_COND_MSG(p_peer_id == get_network_master(), "This `peer_id` is equals to the Master `peer_id`, so it's not allowed.");

	const int index = disabled_puppets.find(p_peer_id);
	if (p_active) {
		if (index >= 0) {
			disabled_puppets.remove(p_peer_id);
			update_active_puppets();
		}
	} else {
		if (index == -1) {
			disabled_puppets.push_back(p_peer_id);
			update_active_puppets();
		}
	}
}

const Vector<int> &PlayerNetController::get_active_puppets() const {
	return active_puppets;
}

void PlayerNetController::on_peer_connection_change(int p_peer_id) {
	update_active_puppets();
}

void PlayerNetController::update_active_puppets() {
	// Unreachable
	CRASH_COND(get_tree()->is_network_server() == false);
	active_puppets.clear();
	const Vector<int> peers = get_tree()->get_network_connected_peers();
	for (int i = 0; i < peers.size(); i += 1) {
		const int peer_id = peers[i];
		if (peer_id != get_network_master() && disabled_puppets.find(peer_id) == -1) {
			active_puppets.push_back(peer_id);
		}
	}
}

void PlayerNetController::set_inputs_buffer(const BitArray &p_new_buffer) {
	inputs_buffer.get_buffer_mut().get_bytes_mut() = p_new_buffer.get_bytes();
}

void PlayerNetController::rpc_server_send_frames_snapshot(PoolVector<uint8_t> p_data) {
	ERR_FAIL_COND(get_tree()->is_network_server() == false);

	const Vector<int> &peers = get_active_puppets();
	for (int i = 0; i < peers.size(); i += 1) {
		// This is an active puppet, Let's send the data.
		const int peer_id = peers[i];
		const bool unreliable = true;
		get_multiplayer()->send_bytes_to(this, peer_id, unreliable, "rpc_puppet_send_frames_snapshot", p_data);
	}

	controller->receive_snapshots(p_data);
}

void PlayerNetController::rpc_master_send_tick_additional_speed(int p_additional_tick_speed) {
	ERR_FAIL_COND(is_network_master() == false);

	static_cast<MasterController *>(controller)->receive_tick_additional_speed(p_additional_tick_speed);
}

void PlayerNetController::rpc_puppet_send_frames_snapshot(PoolVector<uint8_t> p_data) {
	ERR_FAIL_COND(get_tree()->is_network_server() == true);
	ERR_FAIL_COND(is_network_master() == true);

	controller->receive_snapshots(p_data);
}

void PlayerNetController::rpc_send_player_state(uint64_t p_snapshot_id, Variant p_data) {
	ERR_FAIL_COND(get_tree()->is_network_server() == true);

	controller->player_state_check(p_snapshot_id, p_data);
}

void PlayerNetController::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			ERR_FAIL_NULL_MSG(get_script_instance(), "You Must have a script to use correctly the `PlayerNetController`.");
			ERR_FAIL_COND_MSG(get_script_instance()->has_method("collect_inputs") == false, "In your script you must inherit the virtual method `collect_inputs` to correctly use the `PlayerNetController`.");
			ERR_FAIL_COND_MSG(get_script_instance()->has_method("step_player") == false, "In your script you must inherit the virtual method `step_player` to correctly use the `PlayerNetController`.");
			ERR_FAIL_COND_MSG(get_script_instance()->has_method("are_inputs_different") == false, "In your script you must inherit the virtual method `are_inputs_different` to correctly use the `PlayerNetController`.");
			ERR_FAIL_NULL_MSG(get_player(), "The `player_node_path` must point to a valid `Spatial` node.");

			inputs_buffer.init_buffer();
			controller->physics_process(get_physics_process_delta_time());

		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (Engine::get_singleton()->is_editor_hint())
				return;

			// Unreachable.
			CRASH_COND(get_tree() == NULL);

			if (get_tree()->is_network_server()) {
				controller = memnew(ServerController);
				get_multiplayer()->connect("network_peer_connected", this, "_on_peer_connection_change");
				get_multiplayer()->connect("network_peer_disconnected", this, "_on_peer_connection_change");
				update_active_puppets();
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

			if (get_tree()->is_network_server()) {
				get_multiplayer()->disconnect("network_peer_connected", this, "_on_peer_connection_change");
				get_multiplayer()->disconnect("network_peer_disconnected", this, "_on_peer_connection_change");
			}
		} break;
	}
}

ServerController::ServerController() :
		current_packet_id(UINT64_MAX),
		ghost_input_count(0),
		network_tracer(PACKETS_TO_TRACK),
		optimal_snapshots_size(0.0),
		client_tick_additional_speed(0.0),
		client_tick_additional_speed_compressed(0),
		peers_state_checker_time(0.0) {
}

void ServerController::physics_process(real_t p_delta) {
	fetch_next_input();
	node->get_script_instance()->call("step_player", p_delta);
	adjust_master_tick_rate(p_delta);
	check_peers_player_state(p_delta);
}

bool is_remote_frame_A_older(const FrameSnapshotSkinny &p_snap_a, const FrameSnapshotSkinny &p_snap_b) {
	return p_snap_a.id < p_snap_b.id;
}

void ServerController::receive_snapshots(PoolVector<uint8_t> p_data) {

	// The packet is composed as follow:
	// - First byte the quantity of sent snapshots.
	// - The following four bytes for the first snapshot ID.
	// - Array of snapshots:
	// |-- First byte the amount of times this snapshot is duplicated in the packet.
	// |-- snapshot inputs buffer.
	//
	// Let's decode it!

	const int data_len = p_data.size();

	if (data_len < 1)
		// Just discard, the packet is corrupted.
		// TODO Measure internet connection status here?
		return;

	const int buffer_size = node->get_inputs_buffer().get_buffer_size();

	PoolVector<uint8_t>::Read r = p_data.read();
	int ofs = 0;

	const int snapshots_count = r[ofs];
	ofs += 1;

	const int packet_size = 0
							// First byte is to store the size of the shapshots_count
							+ sizeof(uint8_t)
							// Then, the first snapshot id in the packet.
							+ sizeof(uint32_t)
							// Array of snapshots.
							+ (sizeof(uint8_t) + buffer_size) * snapshots_count;

	if (packet_size != data_len)
		// Just discard, the packet is corrupted.
		// TODO Measure internet connection status here?
		return;

	// Received data seems fine, let's decode.

	const uint32_t first_snapshot_id = decode_uint32(r.ptr() + ofs);
	ofs += 4;

	uint64_t inserted_snapshot_count = 0;

	for (int i = 0; i < snapshots_count; i += 1) {

		// First byte is used for the duplication count.
		const uint8_t duplication = r[ofs];
		ofs += 1;

		for (int sub = 0; sub <= duplication; sub += 1) {

			const uint64_t snapshot_id = first_snapshot_id + inserted_snapshot_count;
			inserted_snapshot_count += 1;

			if (current_packet_id != UINT64_MAX && current_packet_id >= snapshot_id)
				continue;

			FrameSnapshotSkinny rfs;
			rfs.id = snapshot_id;

			const bool found = std::binary_search(snapshots.begin(), snapshots.end(), rfs, is_remote_frame_A_older);

			if (!found) {
				rfs.inputs_buffer.get_bytes_mut().resize(buffer_size);
				copymem(rfs.inputs_buffer.get_bytes_mut().ptrw(), r.ptr() + ofs, buffer_size);

				snapshots.push_back(rfs);

				// Sort the new inserted snapshot.
				std::sort(snapshots.begin(), snapshots.end(), is_remote_frame_A_older);
			}
		}

		// We can now advance the offset.
		ofs += buffer_size;
	}
}

void ServerController::player_state_check(uint64_t p_id, Variant p_data) {
	ERR_PRINTS("The method `player_state_check` must not be called on server. Be sure why it happened.");
}

bool ServerController::fetch_next_input() {
	bool is_new_input = true;

	// Unreachable
	CRASH_COND(node->get_script_instance() == NULL);

	if (unlikely(current_packet_id == UINT64_MAX)) {
		// As initial packet, anything is good.
		if (snapshots.empty() == false) {
			node->set_inputs_buffer(snapshots.front().inputs_buffer);
			current_packet_id = snapshots.front().id;
			snapshots.pop_front();
			network_tracer.notify_packet_arrived();
		} else {
			is_new_input = false;
			network_tracer.notify_missing_packet();
		}
	} else {
		// Search the next packet, the cycle is used to make sure to not stop
		// with older packets arrived too late.

		const uint64_t next_packet_id = current_packet_id + 1;

		if (unlikely(snapshots.empty() == true)) {
			// The input buffer is empty!
			is_new_input = false;
			network_tracer.notify_missing_packet();
			ghost_input_count += 1;
			//print_line("Input buffer is void, i'm using the previous one!"); // TODO Replace with?

		} else {
			// The input buffer is not empty, search the new input.
			if (next_packet_id == snapshots.front().id) {
				// Wow, the next input is perfect!

				node->set_inputs_buffer(snapshots.front().inputs_buffer);
				current_packet_id = snapshots.front().id;
				snapshots.pop_front();

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

				const int size = MIN(ghost_input_count, snapshots.size());
				const uint64_t ghost_packet_id = next_packet_id + ghost_input_count;

				bool recovered = false;
				FrameSnapshotSkinny pi;

				const PlayerInputsReference pir_A(node->get_inputs_buffer());
				// Copy from the node inputs so to copy the data info
				PlayerInputsReference pir_B(node->get_inputs_buffer());

				for (int i = 0; i < size; i += 1) {
					if (ghost_packet_id < snapshots.front().id) {
						break;
					} else {
						pi = snapshots.front();
						snapshots.pop_front();
						recovered = true;

						// If this input has some important changes compared to the last
						// good input, let's recover to this point otherwise skip it
						// until the last one.
						// Useful to avoid that the server stay too much behind the
						// client.

						pir_B.set_inputs_buffer(pi.inputs_buffer);

						const bool is_meaningful = node->get_script_instance()->call("are_inputs_different", &pir_A, &pir_B);

						if (is_meaningful) {
							break;
						}
					}
				}

				if (recovered) {
					node->set_inputs_buffer(pi.inputs_buffer);
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

void ServerController::adjust_master_tick_rate(real_t p_delta) {
	const int miss_packets = network_tracer.get_missing_packets();

	{
		// The first step to establish the client speed up amount is to define the
		// optimal `frames_inputs` size.
		// This size is increased and decreased using an acceleration, so any speed
		// change is spread across a long period rather a little one.
		// Keep in mind that internet may be really fluctuating.
		const real_t acceleration_level = CLAMP(
				(static_cast<real_t>(miss_packets) - static_cast<real_t>(snapshots.size())) /
						static_cast<real_t>(MISSING_SNAPSHOTS_MAX_TOLLERANCE),
				-2.0,
				2.0);
		optimal_snapshots_size += acceleration_level * SNAPSHOTS_SIZE_ACCELERATION * p_delta;
		optimal_snapshots_size = CLAMP(optimal_snapshots_size, MIN_SNAPSHOTS_SIZE, MAX_SNAPSHOTS_SIZE);
	}

	{
		// The client speed is determined using an acceleration so to have much more
		// control over it and avoid nervous changes.
		const real_t acceleration_level = CLAMP((optimal_snapshots_size - snapshots.size()) / MAX_SNAPSHOTS_SIZE, -1.0, 1.0);
		const real_t acc = acceleration_level * TICK_ACCELERATION * p_delta;
		const real_t damp = client_tick_additional_speed * -0.9;

		// The damping is fully applyied only if it points in the opposite `acc`
		// direction.
		// I want to cut down the oscilations when the target is the same for a while,
		// but I need to move fast toward new targets when they appear.
		client_tick_additional_speed += acc + damp * ((SGN(acc) * SGN(damp) + 1) / 2.0);
		client_tick_additional_speed = CLAMP(client_tick_additional_speed, -MAX_ADDITIONAL_TICK_SPEED, MAX_ADDITIONAL_TICK_SPEED);

		int new_speed = 100 * (client_tick_additional_speed / MAX_ADDITIONAL_TICK_SPEED);

		if (ABS(client_tick_additional_speed_compressed - new_speed) >= TICK_SPEED_CHANGE_NOTIF_THRESHOLD) {
			client_tick_additional_speed_compressed = new_speed;

			node->rpc_id(
					node->get_network_master(),
					"rpc_master_send_tick_additional_speed",
					client_tick_additional_speed_compressed);
		}
	}
}

void ServerController::check_peers_player_state(real_t p_delta) {
	if (current_packet_id == UINT64_MAX) {
		// Skip this until the first input arrive.
		return;
	}

	peers_state_checker_time += p_delta;
	if (peers_state_checker_time < PEERS_STATE_CHECK_INTERVAL) {
		// Not yet the time to check
		return;
	}

	peers_state_checker_time = 0.0;

	Variant data;
	if (node->get_check_state_position_only()) {
		data = node->get_player()->get_global_transform().origin;
	} else {
		data = node->get_player()->get_global_transform();
	}

	// Notify the active puppets.
	const Vector<int> &peers = node->get_active_puppets();
	for (int i = 0; i < peers.size(); i += 1) {

		// This is an active peer, Let's send the data.
		const int peer_id = peers[i];

		// TODO please don't use variant and encode everything inside a more tiny packet.
		// TODO Please make sure to notify the puppets only when they require it!
		// Notify the puppets.
		node->rpc_id(
				peer_id,
				"rpc_send_player_state",
				current_packet_id,
				data);
	}

	// TODO please don't use variant and encode everything inside a more tiny packet.
	// TODO Please make sure to notify the puppets only when they require it!
	// Notify the master
	node->rpc_id(
			node->get_network_master(),
			"rpc_send_player_state",
			current_packet_id,
			data);
}

MasterController::MasterController() :
		time_bank(0.0),
		tick_additional_speed(0.0),
		snapshot_counter(0),
		recover_snapshot_id(0),
		recovered_snapshot_id(0) {
}

void MasterController::physics_process(real_t p_delta) {

	// On `Master` side we may want to speed up input_packet
	// generation, for this reason here I'm performing a sub tick.
	// Also keep in mind that we are just pretending that the time
	// is advancing faster, for this reason we are still using
	// delta to move the player.

	// Unreachable
	CRASH_COND(node->get_script_instance() == NULL);

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
		const bool accept_new_inputs = frames_snapshot.size() < MAX_STORED_FRAMES;

		if (accept_new_inputs) {
			node->get_script_instance()->call("collect_inputs");
		} else {
			node->get_inputs_buffer_mut().zero();
		}

		// The physics process is always emitted, because we still need to simulate
		// the character motion even if we don't store the player inputs.
		node->get_script_instance()->call("step_player", p_delta);

		if (accept_new_inputs) {

			create_snapshot(snapshot_counter);
			snapshot_counter += 1;

			// This must happens here because in case of bad internet connection
			// the client accelerates the execution producing much more packets
			// per second.
			send_frame_snapshots_to_server();
		}
	}

	compute_server_discrepancy();
	recover_server_discrepancy(p_delta);
}

void MasterController::receive_snapshots(PoolVector<uint8_t> p_data) {
	ERR_PRINTS("The master is not supposed to receive snapshots. Check why this happened.");
}

void MasterController::player_state_check(uint64_t p_snapshot_id, Variant p_data) {
	if (p_snapshot_id > recover_snapshot_id && p_snapshot_id > recovered_snapshot_id) {
		recover_snapshot_id = p_snapshot_id;
		recover_state_data = p_data;
	}
}

real_t MasterController::get_pretended_delta() const {
	return 1.0 / (static_cast<real_t>(Engine::get_singleton()->get_iterations_per_second()) + tick_additional_speed);
}

void MasterController::create_snapshot(uint64_t p_id) {
	FrameSnapshot inputs;
	inputs.id = p_id;
	inputs.inputs_buffer = node->get_inputs_buffer().get_buffer();
	inputs.character_transform = node->get_player()->get_global_transform();
	inputs.similarity = UINT64_MAX;
	frames_snapshot.push_back(inputs);
}

void MasterController::send_frame_snapshots_to_server() {

	// The packet is composed as follow:
	// - First byte the quantity of sent snapshots.
	// - The following four bytes for the first snapshot ID.
	// - Array of snapshots:
	// |-- First byte the amount of times this snapshot is duplicated in the packet.
	// |-- snapshot inputs buffer.

	// Unreachable
	CRASH_COND(node->get_script_instance() == NULL);

	const size_t snapshots_count = MIN(frames_snapshot.size(), static_cast<size_t>(node->get_max_redundant_inputs() + 1));
	ERR_FAIL_COND_MSG(snapshots_count >= UINT8_MAX, "Is not allow to send more than 254 redundant packets.");
	CRASH_COND(snapshots_count < 1); // Unreachable

	const int buffer_size = node->get_inputs_buffer().get_buffer_size();

	const int packet_size = 0
							// First byte is to store the size of the shapshots_count
							+ sizeof(uint8_t)
							// Then, the first snapshot id in the packet.
							+ sizeof(uint32_t)
							// Array of snapshots.
							+ (sizeof(uint8_t) + buffer_size) * snapshots_count;

	int final_packet_size = 0;

	if (cached_packet_data.size() < packet_size)
		cached_packet_data.resize(packet_size);

	{
		PoolVector<uint8_t>::Write w = cached_packet_data.write();

		int ofs = 0;

		// The Array of snapshot size is written at the end.
		ofs += 1;

		// Let's store the ID of the first snapshot.
		const uint64_t first_snapshot_id = frames_snapshot[frames_snapshot.size() - snapshots_count].id;
		ofs += encode_uint32(first_snapshot_id, w.ptr() + ofs);

		uint8_t in_packet_snapshots = 0;
		uint64_t previous_snapshot_id = UINT64_MAX;
		uint64_t previous_snapshot_similarity = UINT64_MAX;
		uint8_t duplication_count = 0;

		PlayerInputsReference pir_A(node->get_inputs_buffer());
		// Copy from the node inputs so to copy the data info.
		PlayerInputsReference pir_B(node->get_inputs_buffer());

		// Compose the packets
		for (size_t i = frames_snapshot.size() - snapshots_count; i < frames_snapshot.size(); i += 1) {
			// Unreachable.
			CRASH_COND(frames_snapshot[i].inputs_buffer.get_bytes().size() != buffer_size);

			bool is_similar = false;

			if (previous_snapshot_id == UINT64_MAX) {
				// This happens for the first snapshot of the packet.
				// Just write it.
				is_similar = false;
			} else {
				if (frames_snapshot[i].similarity != previous_snapshot_id) {
					if (frames_snapshot[i].similarity == UINT64_MAX) {
						// This snapshot was never compared, let's do it now.
						pir_B.set_inputs_buffer(frames_snapshot[i].inputs_buffer);
						const bool are_different = node->get_script_instance()->call("are_inputs_different", &pir_A, &pir_B);
						is_similar = are_different == false;

					} else if (frames_snapshot[i].similarity == previous_snapshot_similarity) {
						// This snapshot is similar to the previous one, the thing is
						// that the similarity check was done on an older snapshot.
						// Fortunatelly we are able to compare the similarity id
						// and detect its similarity correctly.
						is_similar = true;

					} else {
						// This snapshot is simply different from the previous one.
						is_similar = false;
					}
				} else {
					// These are the same, let's save some space.
					is_similar = true;
				}
			}

			if (is_similar) {
				duplication_count += 1;
				// In this way, the frame we don't need to compare this again.
				frames_snapshot[i].similarity = previous_snapshot_id;

			} else {
				if (previous_snapshot_id == UINT64_MAX) {
					// The first snapshot is special
					// Nothing to do
				} else {
					// We can finally write the duplicated snapshots count.
					w[ofs - buffer_size - 1] = duplication_count;
				}

				// Resets the duplication count.
				duplication_count = 0;

				// Writes the duplication_count, now is simply 0.
				w[ofs] = 0;
				ofs += 1;

				// Write the inputs
				copymem(w.ptr() + ofs, frames_snapshot[i].inputs_buffer.get_bytes().ptr(), buffer_size);
				ofs += buffer_size;

				in_packet_snapshots += 1;

				// Let's see if we can duplicate this snapshot.
				previous_snapshot_id = frames_snapshot[i].id;
				previous_snapshot_similarity = frames_snapshot[i].similarity;
				pir_A.set_inputs_buffer(frames_snapshot[i].inputs_buffer);
			}
		}

		// Write the last duplication count
		w[ofs - buffer_size - 1] = duplication_count;

		// Write the snapshot array size.
		w[0] = in_packet_snapshots;

		// Size after the compression.
		final_packet_size = 0
							// First byte is to store the size of the shapshots_count
							+ sizeof(uint8_t)
							// Then, the first snapshot id in the packet.
							+ sizeof(uint32_t)
							// Array of snapshots.
							+ (sizeof(uint8_t) + buffer_size) * in_packet_snapshots;

		// Unreachable.
		CRASH_COND(ofs != final_packet_size);
	}

	const int server_peer_id = 1;
	const bool unreliable = true;
	node->get_multiplayer()->send_bytes_to(node, server_peer_id, unreliable, "rpc_server_send_frames_snapshot", cached_packet_data, final_packet_size);
}

// TODO I'm pretty sure this is not good because any game may want to send
// custom data and recovere in a different way.
void MasterController::compute_server_discrepancy() {
	if (recover_snapshot_id <= recovered_snapshot_id) {
		// Nothing to do.
		return;
	}

	FrameSnapshot fs;
	fs.id = 0;

	// Takes the snapshot that we have to recover and remove all the old snapshots.
	while (frames_snapshot.empty() == false && frames_snapshot.front().id <= recover_snapshot_id) {
		fs = frames_snapshot.front();
		frames_snapshot.pop_front();
	}

	if (fs.id != recover_snapshot_id) {
		// `recover_snapshot_id` is already checked
		// or not yet received if this is the pupped, so just pospone this.
		return;
	}

	Transform server_transform = node->get_player()->get_global_transform();
	if (node->get_check_state_position_only()) {
		ERR_FAIL_COND(recover_state_data.get_type() != Variant::VECTOR3);
		server_transform.origin = recover_state_data;
	} else {
		ERR_FAIL_COND(recover_state_data.get_type() != Variant::TRANSFORM);
		server_transform = recover_state_data;
	}

	const real_t delta = 1.0 / Engine::get_singleton()->get_iterations_per_second();
	const Transform unrecovered_transform = node->get_player()->get_global_transform();
	const Transform delta_transform = fs.character_transform.inverse() * server_transform;

	if (delta_transform.origin.length_squared() > CMP_EPSILON || delta_transform.basis.get_euler().length_squared() > CMP_EPSILON) {
		// Calculates the discrepancy motion by rewinding all inputs.
		node->get_player()->set_global_transform(server_transform);

		for (size_t i = 0; i < frames_snapshot.size(); i += 1) {

			// Set snapshot inputs.
			node->set_inputs_buffer(frames_snapshot[i].inputs_buffer);

			node->get_script_instance()->call("step_player", delta);

			// Update snapshot transform
			frames_snapshot[i].character_transform = node->get_player()->get_global_transform();
		}

		const Transform recovered_transform = node->get_player()->get_global_transform();
		delta_discrepancy = unrecovered_transform.inverse() * recovered_transform;

		// Sets the unrecovered transform so we can interpolate the discrepancy
		// and make this transition a bit more soft.
		node->get_player()->set_global_transform(unrecovered_transform);
	}

	recovered_snapshot_id = recover_snapshot_id;
}

void MasterController::recover_server_discrepancy(real_t p_delta) {
	Transform recovered_transform = node->get_player()->get_global_transform();

	{
		const real_t rlen = delta_discrepancy.origin.length();
		const Vector3 frame_recover =
				delta_discrepancy.origin.normalized() *
				MIN(rlen * p_delta * node->get_discrepancy_recover_velocity(), rlen);

		delta_discrepancy.origin -= frame_recover;
		recovered_transform.origin += frame_recover;
	}

	if (node->get_check_state_position_only() == false) {
		// TODO please recover rotation.
		recovered_transform.basis *= delta_discrepancy.basis;
		delta_discrepancy.basis = Basis();
	}

	node->get_player()->set_global_transform(recovered_transform);
}

void MasterController::receive_tick_additional_speed(int p_speed) {
	tick_additional_speed = (p_speed / 100) * MAX_ADDITIONAL_TICK_SPEED;
	tick_additional_speed = CLAMP(tick_additional_speed, -MAX_ADDITIONAL_TICK_SPEED, MAX_ADDITIONAL_TICK_SPEED);
}

PuppetController::PuppetController() {
}

void PuppetController::physics_process(real_t p_delta) {
	server_controller.node = node;
	master_controller.node = node;

	const bool is_new_input = server_controller.fetch_next_input();
	node->get_script_instance()->call("step_player", p_delta);
	if (is_new_input) {
		master_controller.create_snapshot(server_controller.current_packet_id);
	}
	master_controller.compute_server_discrepancy();
	master_controller.recover_server_discrepancy(p_delta);
}

void PuppetController::receive_snapshots(PoolVector<uint8_t> p_data) {
	server_controller.node = node;
	server_controller.receive_snapshots(p_data);
}

void PuppetController::player_state_check(uint64_t p_snapshot_id, Variant p_data) {
	master_controller.node = node;
	master_controller.player_state_check(p_snapshot_id, p_data);
}
