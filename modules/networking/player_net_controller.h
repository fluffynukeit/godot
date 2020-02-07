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

#include "core/math/transform.h"
#include "core/node_path.h"
#include "input_buffer.h"
#include "net_utilities.h"
#include <deque>

#ifndef PLAYERPNETCONTROLLER_H
#define PLAYERPNETCONTROLLER_H

struct Controller;
class Spatial;

class PlayerInputsReference : public Object {
	GDCLASS(PlayerInputsReference, Object);

public:
	InputsBuffer inputs_buffer;

	static void _bind_methods();

	PlayerInputsReference() {}
	PlayerInputsReference(const InputsBuffer &p_ib) :
			inputs_buffer(p_ib) {}

	bool get_bool(int p_index) const;
	int64_t get_int(int p_index) const;
	real_t get_unit_real(int p_index) const;
	Vector2 get_normalized_vector(int p_index) const;

	void set_inputs_buffer(const BitArray &p_new_buffer);
};

class PlayerNetController : public Node {
	GDCLASS(PlayerNetController, Node);

public:
	enum InputDataType {
		INPUT_DATA_TYPE_BOOL = InputsBuffer::DATA_TYPE_BOOL,
		INPUT_DATA_TYPE_INT = InputsBuffer::DATA_TYPE_INT,
		INPUT_DATA_TYPE_UNIT_REAL = InputsBuffer::DATA_TYPE_UNIT_REAL,
		INPUT_DATA_TYPE_NORMALIZED_VECTOR2 = InputsBuffer::DATA_TYPE_NORMALIZED_VECTOR2
	};

	enum InputCompressionLevel {
		INPUT_COMPRESSION_LEVEL_0 = InputsBuffer::COMPRESSION_LEVEL_0,
		INPUT_COMPRESSION_LEVEL_1 = InputsBuffer::COMPRESSION_LEVEL_1,
		INPUT_COMPRESSION_LEVEL_2 = InputsBuffer::COMPRESSION_LEVEL_2,
		INPUT_COMPRESSION_LEVEL_3 = InputsBuffer::COMPRESSION_LEVEL_3
	};

private:
	/// The controlled player node path
	NodePath player_node_path;

	/// Amount of time an inputs is re-sent to each node.
	/// Resend inputs is necessary because the packets may be lost since they
	/// are sent in an unreliable way.
	int max_redundant_inputs;

	/// When `true` the server sends only the global space position to check the
	/// client status; otherwise sends the full `Transform`.
	bool check_state_position_only;

	/// Discrepancy recover velocity
	real_t discrepancy_recover_velocity;

	Controller *controller;
	InputsBuffer inputs_buffer;

	Spatial *cached_player;

	Vector<int> active_puppets;
	// Disabled peers is used to stop informatino propagation to a particular pear.
	Vector<int> disabled_puppets;

public:
	static void _bind_methods();

public:
	PlayerNetController();

	void set_player_node_path(NodePath p_path);
	NodePath get_player_node_path() const;

	// Returns the valid pointer of the player.
	Spatial *get_player() const;

	void set_max_redundant_inputs(int p_max);
	int get_max_redundant_inputs() const;

	void set_check_state_position_only(bool p_check_position_only);
	bool get_check_state_position_only() const;

	void set_discrepancy_recover_velocity(real_t p_velocity);
	real_t get_discrepancy_recover_velocity() const;

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

	const InputsBuffer &get_inputs_buffer() const {
		return inputs_buffer;
	}

	InputsBuffer &get_inputs_buffer_mut() {
		return inputs_buffer;
	}

	void set_puppet_active(int p_peer_id, bool p_active);
	const Vector<int> &get_active_puppets() const;

	void on_peer_connection_change(int p_peer_id);
	void update_active_puppets();

public:
	void set_inputs_buffer(const BitArray &p_new_buffer);

	/* On server rpc functions. */
	void rpc_server_send_frames_snapshot(PoolVector<uint8_t> p_data);

	/* On master rpc functions. */
	void rpc_master_send_tick_additional_speed(int p_additional_tick_speed);

	/* On puppet rpc functions. */
	void rpc_puppet_send_frames_snapshot(PoolVector<uint8_t> p_data);

	/* On all peers rpc functions. */
	void rpc_send_player_state(uint64_t p_snapshot_id, Variant p_data);

private:
	virtual void _notification(int p_what);
};

VARIANT_ENUM_CAST(PlayerNetController::InputDataType)
VARIANT_ENUM_CAST(PlayerNetController::InputCompressionLevel)

struct FrameSnapshotSkinny {
	uint64_t id;
	BitArray inputs_buffer;
};

struct FrameSnapshot {
	uint64_t id;
	BitArray inputs_buffer;
	Transform character_transform;
	uint64_t similarity;
};

struct Controller {
	PlayerNetController *node;

	virtual ~Controller() {}

	virtual void physics_process(real_t p_delta) = 0;
	virtual void receive_snapshots(PoolVector<uint8_t> p_data) = 0;

	/// The server call this function on all peers with on server state.
	/// The peers can check if the state is the same or not and in this case
	/// recover its player state.
	virtual void player_state_check(uint64_t p_id, Variant p_data) = 0;
};

struct ServerController : public Controller {
	uint64_t current_packet_id;
	uint32_t ghost_input_count;
	NetworkTracer network_tracer;
	std::deque<FrameSnapshotSkinny> snapshots;
	real_t optimal_snapshots_size;
	real_t client_tick_additional_speed;
	// It goes from -100 to 100
	int client_tick_additional_speed_compressed;
	real_t peers_state_checker_time;

	ServerController();

	virtual void physics_process(real_t p_delta);
	virtual void receive_snapshots(PoolVector<uint8_t> p_data);
	virtual void player_state_check(uint64_t p_snapshot_id, Variant p_data);

	/// Fetch the next inputs, returns true if the input is new.
	bool fetch_next_input();

	/// This function updates the `tick_additional_speed` so that the `frames_inputs`
	/// size is enough to reduce the missing packets to 0.
	///
	/// When the internet connection is bad, the packets need more time to arrive.
	/// To heal this problem, the server tells the client to speed up a little bit
	/// so it send the inputs a bit earlier than the usual.
	///
	/// If the `frames_inputs` size is too big the input lag between the client and
	/// the server is artificial and no more dependent on the internet. For this
	/// reason the server tells the client to slowdown so to keep the `frames_inputs`
	/// size moderate to the needs.
	void adjust_master_tick_rate(real_t p_delta);

	/// This function is executed on server, and call a client function that
	/// checks if the player state is consistent between client and server.
	void check_peers_player_state(real_t p_delta);
};

struct MasterController : public Controller {
	real_t time_bank;
	real_t tick_additional_speed;
	uint64_t snapshot_counter;
	std::deque<FrameSnapshot> frames_snapshot;
	PoolVector<uint8_t> cached_packet_data;
	uint64_t recover_snapshot_id;
	uint64_t recovered_snapshot_id;
	Variant recover_state_data;
	Transform delta_discrepancy;

	MasterController();

	virtual void physics_process(real_t p_delta);
	virtual void receive_snapshots(PoolVector<uint8_t> p_data);
	virtual void player_state_check(uint64_t p_snapshot_id, Variant p_data);

	real_t get_pretended_delta() const;

	void create_snapshot(uint64_t p_id);

	/// Sends an unreliable packet to the server, containing a packed array of
	/// frame snapshots.
	void send_frame_snapshots_to_server();

	/// Computes the motion to recover the discrepancy between client and server.
	/// Removes all the checked snapshots.
	void compute_server_discrepancy();

	/// Resets the client motion so to conbenzate the server discrepancy.
	void recover_server_discrepancy(real_t p_delta);

	void receive_tick_additional_speed(int p_speed);
};

/// The puppets is kind of special controller, indeed you can see that it used
/// a `ServerController` + `MastertController`.
/// The `PuppetController` receive inputs from the client as the server does,
/// and fetch them exactly like the server.
/// After the execution of the inputs, the puppet start to act like the master,
/// because it wait the player status from the server to correct its motion.
struct PuppetController : public Controller {
	/// Used to perform server like operations
	ServerController server_controller;
	/// Used to perform master like operations
	MasterController master_controller;

	PuppetController();

	virtual void physics_process(real_t p_delta);
	virtual void receive_snapshots(PoolVector<uint8_t> p_data);
	virtual void player_state_check(uint64_t p_snapshot_id, Variant p_data);
};
#endif
