/*************************************************************************/
/*  temporal_id_generator.cpp                                            */
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

#include "temporal_id_generator.h"

#include "core/typedefs.h"
#include <stdint.h>

#define THRESHOLD 1500
#define START_ID 0

TemporalIdGenerator::TemporalIdGenerator() :
		id(START_ID) {
}

GeneratedData TemporalIdGenerator::next() {
	GeneratedData gd;
	gd.id = id;
	gd.compressed_id = static_cast<CompressedId>(id % UINT16_MAX) + 1;
	id += 1;
	return gd;
}

TemporalIdDecoder::TemporalIdDecoder() :
		local_id_head(START_ID),
		generation_count(0),
		highest_received_id(START_ID) {
}

DecompressionResult TemporalIdDecoder::receive(CompressedId p_local_id) {

	// This is how the ids are considered
	// 0                                                    65535
	// --------->-------->----------->--------->-------->------->
	//
	//
	// These are the thresholds:
	//
	//        lf_threshold                               rg_threshold
	// |------|*****|....................................|------|
	//              lf_threshold_margin

	const uint16_t threshold = THRESHOLD;
	const uint16_t left_threshold = threshold;
	const uint16_t left_threshold_margin = threshold + threshold;
	const uint16_t right_threshold = UINT16_MAX - threshold;

	bool want_to_advance_generation = false;
	uint64_t generation = generation_count;

	if (local_id_head > right_threshold && p_local_id < left_threshold_margin) {
		// If the `local_id_head` is after the `right_threshold` and the received
		// `p_local_id` is inside the `left_threshold_margin` we can consider these
		// as ids of the next generation.
		// We have to wait a bit before change the generation because we can still receive
		// old ids.

		// Assume we are in the next generation;
		generation += 1;

		if (p_local_id > left_threshold) {
			// Advance generation
			want_to_advance_generation = true;
		}
	}

	const uint64_t id = static_cast<uint64_t>(p_local_id) + (static_cast<uint64_t>(UINT16_MAX) * generation) - 1;

	const int64_t delta = id - highest_received_id;
	if (delta < (-threshold) || delta > threshold) {
		// Rejects this id since is not supposed to receive the ids with such big gap.

		DecompressionResult s;
		s.success = false;
		return s;

	} else if (want_to_advance_generation) {
		generation_count += 1;
		local_id_head = p_local_id;
	}
	local_id_head = MAX(local_id_head, p_local_id);
	highest_received_id = MAX(highest_received_id, id);

	DecompressionResult s;
	s.success = true;
	s.id = id;
	return s;
}
