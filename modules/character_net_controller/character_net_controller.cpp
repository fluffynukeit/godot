/*************************************************************************/
/*  character_net_controller.cpp                                         */
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

#include "character_net_controller.h"

#include "core/engine.h"
#include "core/io/marshalls.h"
#include "scene/3d/spatial.h"
#include <stdint.h>
#include <algorithm>

// Don't go below 2 so to take into account internet latency
#define MIN_SNAPSHOTS_SIZE 2

#define MAX_ADDITIONAL_TICK_SPEED 2.0

// 2%
#define TICK_SPEED_CHANGE_NOTIF_THRESHOLD 4

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

void CharacterNetController::_bind_methods() {

	BIND_CONSTANT(INPUT_DATA_TYPE_BOOL);
	BIND_CONSTANT(INPUT_DATA_TYPE_INT);
	BIND_CONSTANT(INPUT_DATA_TYPE_UNIT_REAL);
	BIND_CONSTANT(INPUT_DATA_TYPE_NORMALIZED_VECTOR2);

	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_0);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_1);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_2);
	BIND_CONSTANT(INPUT_COMPRESSION_LEVEL_3);

	ClassDB::bind_method(D_METHOD("set_master_snapshot_storage_size", "size"), &CharacterNetController::set_master_snapshot_storage_size);
	ClassDB::bind_method(D_METHOD("get_master_snapshot_storage_size"), &CharacterNetController::get_master_snapshot_storage_size);

	ClassDB::bind_method(D_METHOD("set_network_traced_frames", "size"), &CharacterNetController::set_network_traced_frames);
	ClassDB::bind_method(D_METHOD("get_network_traced_frames"), &CharacterNetController::get_network_traced_frames);

	ClassDB::bind_method(D_METHOD("set_max_redundant_inputs", "max_redundand_inputs"), &CharacterNetController::set_max_redundant_inputs);
	ClassDB::bind_method(D_METHOD("get_max_redundant_inputs"), &CharacterNetController::get_max_redundant_inputs);

	ClassDB::bind_method(D_METHOD("set_server_snapshot_storage_size", "size"), &CharacterNetController::set_server_snapshot_storage_size);
	ClassDB::bind_method(D_METHOD("get_server_snapshot_storage_size"), &CharacterNetController::get_server_snapshot_storage_size);

	ClassDB::bind_method(D_METHOD("set_optimal_size_acceleration", "acceleration"), &CharacterNetController::set_optimal_size_acceleration);
	ClassDB::bind_method(D_METHOD("get_optimal_size_acceleration"), &CharacterNetController::get_optimal_size_acceleration);

	ClassDB::bind_method(D_METHOD("set_missing_snapshots_max_tollerance", "tollerance"), &CharacterNetController::set_missing_snapshots_max_tollerance);
	ClassDB::bind_method(D_METHOD("get_missing_snapshots_max_tollerance"), &CharacterNetController::get_missing_snapshots_max_tollerance);

	ClassDB::bind_method(D_METHOD("set_tick_acceleration", "acceleration"), &CharacterNetController::set_tick_acceleration);
	ClassDB::bind_method(D_METHOD("get_tick_acceleration"), &CharacterNetController::get_tick_acceleration);

	ClassDB::bind_method(D_METHOD("set_state_notify_interval", "interval"), &CharacterNetController::set_state_notify_interval);
	ClassDB::bind_method(D_METHOD("get_state_notify_interval"), &CharacterNetController::get_state_notify_interval);

	ClassDB::bind_method(D_METHOD("input_buffer_add_data_type", "type", "compression"), &CharacterNetController::input_buffer_add_data_type, DEFVAL(InputsBuffer::COMPRESSION_LEVEL_2));
	ClassDB::bind_method(D_METHOD("input_buffer_ready"), &CharacterNetController::input_buffer_ready);

	ClassDB::bind_method(D_METHOD("input_buffer_set_bool", "index", "bool"), &CharacterNetController::input_buffer_set_bool);
	ClassDB::bind_method(D_METHOD("input_buffer_get_bool", "index"), &CharacterNetController::input_buffer_get_bool);

	ClassDB::bind_method(D_METHOD("input_buffer_set_int", "index", "int"), &CharacterNetController::input_buffer_set_int);
	ClassDB::bind_method(D_METHOD("input_buffer_get_int", "index"), &CharacterNetController::input_buffer_get_int);

	ClassDB::bind_method(D_METHOD("input_buffer_set_unit_real", "index", "unit_real"), &CharacterNetController::input_buffer_set_unit_real);
	ClassDB::bind_method(D_METHOD("input_buffer_get_unit_real", "index"), &CharacterNetController::input_buffer_get_unit_real);

	ClassDB::bind_method(D_METHOD("input_buffer_set_normalized_vector", "index", "vector"), &CharacterNetController::input_buffer_set_normalized_vector);
	ClassDB::bind_method(D_METHOD("input_buffer_get_normalized_vector", "index"), &CharacterNetController::input_buffer_get_normalized_vector);

	ClassDB::bind_method(D_METHOD("set_puppet_active", "peer_id", "active"), &CharacterNetController::set_puppet_active);
	ClassDB::bind_method(D_METHOD("_on_peer_connection_change", "peer_id"), &CharacterNetController::on_peer_connection_change);

	ClassDB::bind_method(D_METHOD("replay_snapshots"), &CharacterNetController::replay_snapshots);

	BIND_VMETHOD(MethodInfo("collect_inputs"));
	BIND_VMETHOD(MethodInfo("step_player", PropertyInfo(Variant::REAL, "delta")));
	BIND_VMETHOD(MethodInfo(Variant::BOOL, "are_inputs_different", PropertyInfo(Variant::OBJECT, "inputs_A", PROPERTY_HINT_TYPE_STRING, "PlayerInputsReference"), PropertyInfo(Variant::OBJECT, "inputs_B", PROPERTY_HINT_TYPE_STRING, "PlayerInputsReference")));
	BIND_VMETHOD(MethodInfo(Variant::ARRAY, "create_snapshot"));
	BIND_VMETHOD(MethodInfo("process_recovery", PropertyInfo(Variant::INT, "snapshot_id"), PropertyInfo(Variant::ARRAY, "server_snapshot"), PropertyInfo(Variant::ARRAY, "client_snapshot")))

	// Rpc to server
	ClassDB::bind_method(D_METHOD("rpc_server_send_frames_snapshot", "data"), &CharacterNetController::rpc_server_send_frames_snapshot);

	// Rpc to master
	ClassDB::bind_method(D_METHOD("rpc_master_send_tick_additional_speed", "tick_speed"), &CharacterNetController::rpc_master_send_tick_additional_speed);

	// Rpc to puppets
	ClassDB::bind_method(D_METHOD("rpc_puppet_send_frames_snapshot", "data"), &CharacterNetController::rpc_puppet_send_frames_snapshot);
	ClassDB::bind_method(D_METHOD("rpc_puppet_notify_connection_status", "is_open"), &CharacterNetController::rpc_puppet_notify_connection_status);

	// Rpc to master and puppets
	ClassDB::bind_method(D_METHOD("rpc_send_player_state", "snapshot_id", "data"), &CharacterNetController::rpc_send_player_state);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "input_storage_size", PROPERTY_HINT_RANGE, "100,2000,1"), "set_master_snapshot_storage_size", "get_master_snapshot_storage_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "network_traced_frames", PROPERTY_HINT_RANGE, "100,10000,1"), "set_network_traced_frames", "get_network_traced_frames");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_redundant_inputs", PROPERTY_HINT_RANGE, "0,254,1"), "set_max_redundant_inputs", "get_max_redundant_inputs");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "server_snapshot_storage_size", PROPERTY_HINT_RANGE, "10,100,1"), "set_server_snapshot_storage_size", "get_server_snapshot_storage_size");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "optimal_size_acceleration", PROPERTY_HINT_RANGE, "0.1,20.0,0.01"), "set_optimal_size_acceleration", "get_optimal_size_acceleration");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "missing_snapshots_max_tollerance", PROPERTY_HINT_RANGE, "3,50,1"), "set_missing_snapshots_max_tollerance", "get_missing_snapshots_max_tollerance");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "tick_acceleration", PROPERTY_HINT_RANGE, "0.1,20.0,0.01"), "set_tick_acceleration", "get_tick_acceleration");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "state_notify_interval", PROPERTY_HINT_RANGE, "0.0001,10.0,0.0001"), "set_state_notify_interval", "get_state_notify_interval");

	ADD_SIGNAL(MethodInfo("control_process_done"));
	ADD_SIGNAL(MethodInfo("puppet_server_comunication_opened"));
	ADD_SIGNAL(MethodInfo("puppet_server_comunication_closed"));
}

CharacterNetController::CharacterNetController() :
		player_node_path(NodePath("../")),
		master_snapshot_storage_size(300),
		network_traced_frames(1200),
		max_redundant_inputs(50),
		server_snapshot_storage_size(30),
		optimal_size_acceleration(2.5),
		missing_snapshots_max_tollerance(4),
		tick_acceleration(2.0),
		state_notify_interval(1.0),
		controller(NULL) {

	rpc_config("rpc_server_send_frames_snapshot", MultiplayerAPI::RPC_MODE_REMOTE);

	rpc_config("rpc_master_send_tick_additional_speed", MultiplayerAPI::RPC_MODE_MASTER);

	rpc_config("rpc_puppet_send_frames_snapshot", MultiplayerAPI::RPC_MODE_PUPPET);
	rpc_config("rpc_puppet_notify_connection_status", MultiplayerAPI::RPC_MODE_PUPPET);

	rpc_config("rpc_send_player_state", MultiplayerAPI::RPC_MODE_REMOTE);
}

void CharacterNetController::set_master_snapshot_storage_size(int p_size) {
	master_snapshot_storage_size = p_size;
}

int CharacterNetController::get_master_snapshot_storage_size() const {
	return master_snapshot_storage_size;
}

void CharacterNetController::set_network_traced_frames(int p_size) {
	network_traced_frames = p_size;
}

int CharacterNetController::get_network_traced_frames() const {
	return network_traced_frames;
}

void CharacterNetController::set_max_redundant_inputs(int p_max) {
	max_redundant_inputs = p_max;
}

int CharacterNetController::get_max_redundant_inputs() const {
	return max_redundant_inputs;
}

void CharacterNetController::set_server_snapshot_storage_size(int p_size) {
	server_snapshot_storage_size = p_size;
}

int CharacterNetController::get_server_snapshot_storage_size() const {
	return server_snapshot_storage_size;
}

void CharacterNetController::set_optimal_size_acceleration(real_t p_acceleration) {
	optimal_size_acceleration = p_acceleration;
}

real_t CharacterNetController::get_optimal_size_acceleration() const {
	return optimal_size_acceleration;
}

void CharacterNetController::set_missing_snapshots_max_tollerance(int p_tollerance) {
	missing_snapshots_max_tollerance = p_tollerance;
}

int CharacterNetController::get_missing_snapshots_max_tollerance() const {
	return missing_snapshots_max_tollerance;
}

void CharacterNetController::set_tick_acceleration(real_t p_acceleration) {
	tick_acceleration = p_acceleration;
}

real_t CharacterNetController::get_tick_acceleration() const {
	return tick_acceleration;
}

void CharacterNetController::set_state_notify_interval(real_t p_interval) {
	state_notify_interval = p_interval;
}

real_t CharacterNetController::get_state_notify_interval() const {
	return state_notify_interval;
}

int CharacterNetController::input_buffer_add_data_type(InputDataType p_type, InputCompressionLevel p_compression) {
	return inputs_buffer.add_data_type((InputsBuffer::DataType)p_type, (InputsBuffer::CompressionLevel)p_compression);
}

void CharacterNetController::input_buffer_ready() {
	inputs_buffer.init_buffer();
}

bool CharacterNetController::input_buffer_set_bool(int p_index, bool p_input) {
	return inputs_buffer.set_bool(p_index, p_input);
}

bool CharacterNetController::input_buffer_get_bool(int p_index) const {
	return inputs_buffer.get_bool(p_index);
}

int64_t CharacterNetController::input_buffer_set_int(int p_index, int64_t p_input) {
	return inputs_buffer.set_int(p_index, p_input);
}

int64_t CharacterNetController::input_buffer_get_int(int p_index) const {
	return inputs_buffer.get_int(p_index);
}

real_t CharacterNetController::input_buffer_set_unit_real(int p_index, real_t p_input) {
	return inputs_buffer.set_unit_real(p_index, p_input);
}

real_t CharacterNetController::input_buffer_get_unit_real(int p_index) const {
	return inputs_buffer.get_unit_real(p_index);
}

Vector2 CharacterNetController::input_buffer_set_normalized_vector(int p_index, Vector2 p_input) {
	return inputs_buffer.set_normalized_vector(p_index, p_input);
}

Vector2 CharacterNetController::input_buffer_get_normalized_vector(int p_index) const {
	return inputs_buffer.get_normalized_vector(p_index);
}

void CharacterNetController::set_puppet_active(int p_peer_id, bool p_active) {
	ERR_FAIL_COND_MSG(get_tree()->is_network_server() == false, "You can set puppet activation only on server");
	ERR_FAIL_COND_MSG(p_peer_id == get_network_master(), "This `peer_id` is equals to the Master `peer_id`, so it's not allowed.");

	const int index = disabled_puppets.find(p_peer_id);
	if (p_active) {
		if (index >= 0) {
			disabled_puppets.remove(index);
			update_active_puppets();
			rpc_id(p_peer_id, "rpc_puppet_notify_connection_status", true);
		}
	} else {
		if (index == -1) {
			disabled_puppets.push_back(p_peer_id);
			update_active_puppets();
			rpc_id(p_peer_id, "rpc_puppet_notify_connection_status", false);
		}
	}
}

const Vector<int> &CharacterNetController::get_active_puppets() const {
	return active_puppets;
}

void CharacterNetController::on_peer_connection_change(int p_peer_id) {
	update_active_puppets();
}

void CharacterNetController::update_active_puppets() {
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

void CharacterNetController::replay_snapshots() {
	controller->replay_snapshots();
}

void CharacterNetController::set_inputs_buffer(const BitArray &p_new_buffer) {
	inputs_buffer.get_buffer_mut().get_bytes_mut() = p_new_buffer.get_bytes();
}

void CharacterNetController::rpc_server_send_frames_snapshot(PoolVector<uint8_t> p_data) {
	ERR_FAIL_COND(get_tree()->is_network_server() == false);

	const Vector<int> &peers = get_active_puppets();
	for (int i = 0; i < peers.size(); i += 1) {
		// This is an active puppet, Let's send the data.
		const int peer_id = peers[i];
		rpc_unreliable_id(peer_id, "rpc_puppet_send_frames_snapshot", p_data);
	}

	controller->receive_snapshots(p_data);
}

void CharacterNetController::rpc_master_send_tick_additional_speed(int p_additional_tick_speed) {
	ERR_FAIL_COND(is_network_master() == false);

	static_cast<MasterController *>(controller)->receive_tick_additional_speed(p_additional_tick_speed);
}

void CharacterNetController::rpc_puppet_send_frames_snapshot(PoolVector<uint8_t> p_data) {
	ERR_FAIL_COND(get_tree()->is_network_server() == true);
	ERR_FAIL_COND(is_network_master() == true);

	controller->receive_snapshots(p_data);
}
void CharacterNetController::rpc_puppet_notify_connection_status(bool p_open) {
	ERR_FAIL_COND(get_tree()->is_network_server() == true);
	ERR_FAIL_COND(is_network_master() == true);

	if (p_open) {
		static_cast<PuppetController *>(controller)->open_flow();
	} else {
		static_cast<PuppetController *>(controller)->close_flow();
	}
}

void CharacterNetController::rpc_send_player_state(uint64_t p_snapshot_id, Variant p_data) {
	ERR_FAIL_COND(get_tree()->is_network_server() == true);

	controller->player_state_check(p_snapshot_id, p_data);
}

void CharacterNetController::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			inputs_buffer.init_buffer();
			controller->physics_process(get_physics_process_delta_time());
			emit_signal("control_process_done");

		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (Engine::get_singleton()->is_editor_hint())
				return;

			// Unreachable.
			CRASH_COND(get_tree() == NULL);

			if (get_tree()->is_network_server()) {
				controller = memnew(ServerController(this));
				get_multiplayer()->connect("network_peer_connected", this, "_on_peer_connection_change");
				get_multiplayer()->connect("network_peer_disconnected", this, "_on_peer_connection_change");
				update_active_puppets();
			} else if (is_network_master()) {
				controller = memnew(MasterController(this));
			} else {
				controller = memnew(PuppetController(this));
			}

			set_physics_process_internal(true);

			ERR_FAIL_COND_MSG(has_method("collect_inputs") == false, "In your script you must inherit the virtual method `collect_inputs` to correctly use the `PlayerNetController`.");
			ERR_FAIL_COND_MSG(has_method("step_player") == false, "In your script you must inherit the virtual method `step_player` to correctly use the `PlayerNetController`.");
			ERR_FAIL_COND_MSG(has_method("are_inputs_different") == false, "In your script you must inherit the virtual method `are_inputs_different` to correctly use the `PlayerNetController`.");
			ERR_FAIL_COND_MSG(has_method("create_snapshot") == false, "In your script you must inherit the virtual method `create_snapshot` to correctly use the `PlayerNetController`.");
			ERR_FAIL_COND_MSG(has_method("process_recovery") == false, "In your script you must inherit the virtual method `process_recovery` to correctly use the `PlayerNetController`.");
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

ServerController::ServerController(CharacterNetController *p_node) :
		Controller(p_node),
		current_packet_id(UINT64_MAX),
		ghost_input_count(0),
		network_tracer(p_node->get_network_traced_frames()),
		optimal_snapshots_size(0.0),
		client_tick_additional_speed(0.0),
		client_tick_additional_speed_compressed(0),
		peers_state_checker_time(0.0) {
}

void ServerController::physics_process(real_t p_delta) {
	const bool is_new_input = fetch_next_input();

	if (unlikely(current_packet_id == UINT64_MAX)) {
		// Skip this until the first input arrive.
		return;
	}

	node->call("step_player", p_delta);
	adjust_master_tick_rate(p_delta);
	check_peers_player_state(p_delta, is_new_input);
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

void ServerController::replay_snapshots() {
	ERR_PRINTS("The method `replay_snapshots` must not be called on server. Be sure why it happened.");
}

bool ServerController::fetch_next_input() {
	bool is_new_input = true;

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

						const bool is_meaningful = node->call("are_inputs_different", &pir_A, &pir_B);

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
						static_cast<real_t>(node->get_missing_snapshots_max_tollerance()),
				-2.0,
				2.0);
		optimal_snapshots_size += acceleration_level * static_cast<real_t>(node->get_optimal_size_acceleration()) * p_delta;
		optimal_snapshots_size = CLAMP(optimal_snapshots_size, MIN_SNAPSHOTS_SIZE, node->get_server_snapshot_storage_size());
	}

	{
		// The client speed is determined using an acceleration so to have much more
		// control over it and avoid nervous changes.
		const real_t acceleration_level = CLAMP((optimal_snapshots_size - snapshots.size()) / node->get_server_snapshot_storage_size(), -1.0, 1.0);
		const real_t acc = acceleration_level * node->get_tick_acceleration() * p_delta;
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

			// TODO Send bytes please.
			// TODO consider to send this unreliably each X sec
			node->rpc_id(
					node->get_network_master(),
					"rpc_master_send_tick_additional_speed",
					client_tick_additional_speed_compressed);
		}
	}
}

void ServerController::check_peers_player_state(real_t p_delta, bool is_new_input) {
	if (current_packet_id == UINT64_MAX) {
		// Skip this until the first input arrive.
		return;
	}

	peers_state_checker_time += p_delta;
	if (peers_state_checker_time < node->get_state_notify_interval() || is_new_input == false) {
		// Not yet the time to check.
		return;
	}

	peers_state_checker_time = 0.0;

	Variant data = node->call("create_snapshot");

	// Notify the active puppets.
	const Vector<int> &peers = node->get_active_puppets();
	for (int i = 0; i < peers.size(); i += 1) {

		// This is an active peer, Let's send the data.
		const int peer_id = peers[i];

		// TODO Try to encode things in a more compact form, or improve variant compression even more
		// Notify the puppets.
		node->rpc_id(
				peer_id,
				"rpc_send_player_state",
				current_packet_id,
				data);
	}

	// TODO Try to encode things in a more compact form, or improve variant compression even more
	// Notify the master
	node->rpc_id(
			node->get_network_master(),
			"rpc_send_player_state",
			current_packet_id,
			data);
}

MasterController::MasterController(CharacterNetController *p_node) :
		Controller(p_node),
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
	// `delta` to move the player.

	const real_t pretended_delta = get_pretended_delta();

	time_bank += p_delta;
	uint32_t sub_ticks = static_cast<uint32_t>(time_bank / pretended_delta);
	time_bank -= static_cast<real_t>(sub_ticks) * pretended_delta;

	while (sub_ticks > 0) {
		sub_ticks -= 1;

		// We need to know if we can accept a new input because in case of bad
		// internet connection we can't keep accumulates inputs up to infinite
		// otherwise the server will difer too much from the client and we
		// introduce virtual lag.
		const bool accept_new_inputs = frames_snapshot.size() < static_cast<size_t>(node->get_master_snapshot_storage_size());

		if (accept_new_inputs) {
			node->call("collect_inputs");
		} else {
			// Zeros all inputs so the `step_player` will run with 0 inputs.
			node->get_inputs_buffer_mut().zero();
		}

		// The physics process is always emitted, because we still need to simulate
		// the character motion even if we don't store the player inputs.
		node->call("step_player", p_delta);

		if (accept_new_inputs) {

			create_snapshot(snapshot_counter);
			snapshot_counter += 1;

			// This must happens here because in case of bad internet connection
			// the client accelerates the execution producing much more packets
			// per second.
			send_frame_snapshots_to_server();
		}
	}

	process_recovery();
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

void MasterController::replay_snapshots() {
	const real_t delta = node->get_physics_process_delta_time();
	for (size_t i = 0; i < frames_snapshot.size(); i += 1) {

		// Set snapshot inputs.
		node->set_inputs_buffer(frames_snapshot[i].inputs_buffer);

		node->call("step_player", delta);

		// Update snapshot transform
		frames_snapshot[i].custom_data = node->call("create_snapshot");
	}
}

real_t MasterController::get_pretended_delta() const {
	return 1.0 / (static_cast<real_t>(Engine::get_singleton()->get_iterations_per_second()) + tick_additional_speed);
}

void MasterController::create_snapshot(uint64_t p_id) {
	FrameSnapshot inputs;
	inputs.id = p_id;
	inputs.inputs_buffer = node->get_inputs_buffer().get_buffer();
	inputs.custom_data = node->call("create_snapshot");
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
						const bool are_different = node->call("are_inputs_different", &pir_A, &pir_B);
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

	// TODO this MUST not resize the vector.
	cached_packet_data.resize(final_packet_size);

	const int server_peer_id = 1;
	node->rpc_unreliable_id(server_peer_id, "rpc_server_send_frames_snapshot", cached_packet_data);
}

void MasterController::process_recovery() {
	if (recover_snapshot_id <= recovered_snapshot_id) {
		// Nothing to do.
		return;
	}

	FrameSnapshot fs;
	fs.id = 0;

	// Pop the snapshots until we arrive to the `recover_snapshot_id`
	while (frames_snapshot.empty() == false && frames_snapshot.front().id <= recover_snapshot_id) {
		fs = frames_snapshot.front();
		frames_snapshot.pop_front();
	}

	if (fs.id != recover_snapshot_id) {
		// `recover_snapshot_id` is already checked
		// or not yet received if this is the pupped, so just pospone this.
		return;
	}

	recovered_snapshot_id = recover_snapshot_id;

	node->call("process_recovery", recover_snapshot_id, recover_state_data, fs.custom_data);
}

void MasterController::receive_tick_additional_speed(int p_speed) {
	tick_additional_speed = (static_cast<real_t>(p_speed) / 100.0) * MAX_ADDITIONAL_TICK_SPEED;
	tick_additional_speed = CLAMP(tick_additional_speed, -MAX_ADDITIONAL_TICK_SPEED, MAX_ADDITIONAL_TICK_SPEED);
}

PuppetController::PuppetController(CharacterNetController *p_node) :
		Controller(p_node),
		server_controller(p_node),
		master_controller(p_node),
		is_server_communication_detected(false),
		is_server_state_update_received(false),
		is_flow_open(true) {
}

void PuppetController::physics_process(real_t p_delta) {

	// Lock mechanism when the server don't update anymore this puppet!
	if (is_flow_open && is_server_state_update_received) {
		if (is_server_communication_detected == false) {
			is_server_communication_detected = true;
			hard_reset_to_server_state();
			node->emit_signal("puppet_server_comunication_opened");
		}
	} else {
		// Locked
		return;
	}

	const bool is_new_input = server_controller.fetch_next_input();
	node->call("step_player", p_delta);
	if (is_new_input) {
		master_controller.create_snapshot(server_controller.current_packet_id);
	}
	master_controller.process_recovery();
}

void PuppetController::receive_snapshots(PoolVector<uint8_t> p_data) {
	if (is_flow_open == false)
		return;
	server_controller.receive_snapshots(p_data);
}

void PuppetController::player_state_check(uint64_t p_snapshot_id, Variant p_data) {
	if (is_flow_open == false)
		return;
	master_controller.player_state_check(p_snapshot_id, p_data);
	is_server_state_update_received = true;
}

void PuppetController::replay_snapshots() {
	master_controller.replay_snapshots();
}

void PuppetController::open_flow() {
	if (is_flow_open == true)
		return;
	is_flow_open = true;
	is_server_communication_detected = false;
	is_server_state_update_received = false;
}

void PuppetController::close_flow() {
	if (is_flow_open == false)
		return;
	is_flow_open = false;
	node->emit_signal("puppet_server_comunication_closed");
}

void PuppetController::hard_reset_to_server_state() {
	// Discart all the old inputs
	server_controller.current_packet_id = master_controller.recover_snapshot_id - 1;
	// Consume all old inputs
	while (server_controller.snapshots.size() > 0 && master_controller.recover_snapshot_id > server_controller.snapshots.front().id)
		server_controller.snapshots.pop_front();
}
