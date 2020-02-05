/*************************************************************************/
/*  .cpp                                            */
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

#include "vec_test.h"

#include "core/vector.h"
#include <vector>

void VecTest::_bind_methods() {
}

#define BIG_NUMBER 100000000

void test_writing_std_vector() {
	std::vector<uint64_t> v;
	v.resize(BIG_NUMBER);
	for (uint64_t i = 0; i < BIG_NUMBER; i += 1) {
		v[i] = i;
	}
	int r = 0;
	for (size_t i = 0; i < v.size(); i += 1) {
		r += v[i];
	}
	print_line(itos(r));
}

void test_writing_godot_vector() {
	Vector<uint64_t> v;
	v.resize(BIG_NUMBER);
	for (uint64_t i = 0; i < BIG_NUMBER; i += 1) {
		v.write[i] = i;
	}
	int r = 0;
	for (int i = 0; i < v.size(); i += 1) {
		r += v[i];
	}
	print_line(itos(r));
}

void test_reading_std_vector() {
	std::vector<uint64_t> v;
	for (uint64_t i = 0; i < BIG_NUMBER; i += 1) {
		v.push_back(i);
	}
	int r = 0;
	for (size_t i = 0; i < v.size(); i += 1) {
		r += v[i];
	}
	print_line(itos(r));
}

void test_reading_godot_vector() {
	Vector<uint64_t> v;
	for (uint64_t i = 0; i < BIG_NUMBER; i += 1) {
		v.push_back(i);
	}
	int r = 0;
	for (int i = 0; i < v.size(); i += 1) {
		r += v[i];
	}
	print_line(itos(r));
}

VecTest::VecTest() {
	test_writing_std_vector();
	test_writing_godot_vector();
	//test_reading_std_vector();
	//test_reading_godot_vector();
}
