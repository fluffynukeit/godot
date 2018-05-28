/*************************************************************************/
/*  flex_particle_physics_server.cpp                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
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

#include "flex_particle_physics_server.h"

#include "flex/include/NvFlex.h"
#include "flex_maths.h"

#include "string.h"

/**
	@author AndreaCatania
*/

#define DEVICE 0

NvFlexErrorSeverity error_severity; // contain last error severity

void ErrorCallback(NvFlexErrorSeverity severity, const char *msg, const char *file, int line) {

	print_error(String("Flex error: ") + msg + ", FILE: " + file + ", LINE: " + String::num(line, 0));
	error_severity = severity;
}

bool has_error() {
	return error_severity == NvFlexErrorSeverity::eNvFlexLogError;
}

void FlexParticlePhysicsServer::init() {

	// Init library
	ERR_FAIL_COND(flex_lib);

	NvFlexInitDesc desc;
	desc.deviceIndex = DEVICE;
	desc.enableExtensions = true;
	desc.renderDevice = DEVICE;
	desc.renderContext = 0;
	desc.computeContext = 0;
	desc.computeType = eNvFlexCUDA;
	desc.runOnRenderContext = false;

	flex_lib = NvFlexInit(NV_FLEX_VERSION, ErrorCallback, &desc);
	ERR_FAIL_COND(!flex_lib);
	ERR_FAIL_COND(!has_error());

	// Init solvert
	ERR_FAIL_COND(solver);

	NvFlexSolverDesc solver_desc;
	solver_desc.featureMode = eNvFlexFeatureModeDefault; // All modes enabled (Solid|Fluids) // TODO should be customizable
	solver_desc.maxParticles = 1000; // TODO should be customizable
	solver_desc.maxDiffuseParticles = 1000; // TODO should be customizable
	solver_desc.maxNeighborsPerParticle = 32; // TODO should be customizable
	solver_desc.maxContactsPerParticle = 10; // TODO should be customizable

	solver = NvFlexCreateSolver(flex_lib, &solver_desc);
}

void FlexParticlePhysicsServer::terminate() {

	ERR_FAIL_COND(!solver);
	NvFlexDestroySolver(solver);

	ERR_FAIL_COND(!flex_lib);
	NvFlexShutdown(flex_lib);
}

void FlexParticlePhysicsServer::sync() {
}

void FlexParticlePhysicsServer::flush_queries() {
}

void FlexParticlePhysicsServer::step(real_t p_delta_time) {
	print_line(String::num(p_delta_time) + " <- Particle P server DT");
}

FlexParticlePhysicsServer::FlexParticlePhysicsServer() :
		ParticlePhysicsServer(),
		flex_lib(NULL) {
}

FlexParticlePhysicsServer::~FlexParticlePhysicsServer() {
}
