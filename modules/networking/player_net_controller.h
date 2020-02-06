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
#include "temporal_id_generator.h"
#include <deque>

#ifndef PLAYERPNETCONTROLLER_H
#define PLAYERPNETCONTROLLER_H

struct Controller;
class Spatial;

class PlayerInputsReference : public Object {
	GDCLASS(PlayerInputsReference, Object);

public:
	InputsBuffer input_buffer;

	static void _bind_methods();

	PlayerInputsReference() {}
	PlayerInputsReference(const InputsBuffer &p_ib) :
			input_buffer(p_ib) {}

	bool get_bool(int p_index) const;
	int64_t get_int(int p_index) const;
	real_t get_unit_real(int p_index) const;
	Vector2 get_normalized_vector(int p_index) const;
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

	Controller *controller;
	InputsBuffer input_buffer;

	Spatial *cached_player;

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
		return input_buffer;
	}

	InputsBuffer &get_inputs_buffer_mut() {
		return input_buffer;
	}

public:
	/* On server rpc functions. */
	void rpc_server_send_frames_snapshot(PoolVector<uint8_t> p_data);

	/* On master rpc functions. */
	void rpc_master_send_tick_additional_speed(int p_additional_tick_speed);

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
	uint16_t compressed_id; // Is not anymore valid after 1500 frames
	BitArray inputs_buffer;
	Transform character_transform;
};

struct Controller {
	PlayerNetController *node;

	virtual ~Controller() {}

	virtual void physics_process(real_t p_delta) = 0;
	virtual void receive_snapshots(PoolVector<uint8_t> p_data) = 0;
};

struct ServerController : public Controller {
	uint64_t current_packet_id;
	uint32_t ghost_input_count;
	TemporalIdDecoder input_id_decoder;
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

private:
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
	TemporalIdGenerator input_id_generator;
	Vector<FrameSnapshot> processed_frames;
	PoolVector<uint8_t> cached_packet_data;

	MasterController();

	virtual void physics_process(real_t p_delta);
	virtual void receive_snapshots(PoolVector<uint8_t> p_data);

	real_t get_pretended_delta() const;

	/// Sends an unreliable packet to the server, containing a packed array of
	/// frame snapshots.
	void send_frame_snapshots_to_server();

	void receive_tick_additional_speed(int p_speed);
};

struct PuppetController : public Controller {
	// TODO Use deque here?

	virtual void physics_process(real_t p_delta);
	virtual void receive_snapshots(PoolVector<uint8_t> p_data);
};
#endif
