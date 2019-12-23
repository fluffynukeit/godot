/*************************************************************************/
/*  rvo_space.cpp                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
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

#include "nav_map.h"

#include "core/os/threaded_array_processor.h"
#include "rvo_agent.h"

NavMap::NavMap() :
        deltatime(0.0) {
}

void NavMap::add_region(NavRegion *p_region) {
}

void NavMap::remove_region(NavRegion *p_region) {
}

bool NavMap::has_agent(RvoAgent *agent) const {
    return std::find(agents.begin(), agents.end(), agent) != agents.end();
}

void NavMap::add_agent(RvoAgent *agent) {
    if (!has_agent(agent)) {
        agents.push_back(agent);
        agents_dirty = true;
    }
}

void NavMap::remove_agent(RvoAgent *agent) {
    remove_agent_as_controlled(agent);
    auto it = std::find(agents.begin(), agents.end(), agent);
    if (it != agents.end()) {
        agents.erase(it);
        agents_dirty = true;
    }
}

void NavMap::set_agent_as_controlled(RvoAgent *agent) {
    const bool exist = std::find(controlled_agents.begin(), controlled_agents.end(), agent) != controlled_agents.end();
    if (!exist) {
        ERR_FAIL_COND(!has_agent(agent));
        controlled_agents.push_back(agent);
    }
}

void NavMap::remove_agent_as_controlled(RvoAgent *agent) {
    auto it = std::find(controlled_agents.begin(), controlled_agents.end(), agent);
    if (it != controlled_agents.end()) {
        controlled_agents.erase(it);
    }
}

void NavMap::sync() {
    if (agents_dirty) {
        std::vector<RVO::Agent *> raw_agents;
        raw_agents.reserve(agents.size());
        for (int i(0); i < agents.size(); i++)
            raw_agents.push_back(agents[i]->get_agent());
        rvo.buildAgentTree(raw_agents);
        agents_dirty = false;
    }
}

void NavMap::compute_single_step(uint32_t _index, RvoAgent **agent) {
    (*agent)->get_agent()->computeNeighbors(&rvo);
    (*agent)->get_agent()->computeNewVelocity(deltatime);
}

void NavMap::step(real_t p_deltatime) {
    deltatime = p_deltatime;
    thread_process_array(
            controlled_agents.size(),
            this,
            &NavMap::compute_single_step,
            controlled_agents.data());
}

void NavMap::dispatch_callbacks() {
    for (int i(0); i < static_cast<int>(controlled_agents.size()); i++) {
        controlled_agents[i]->dispatch_callback();
    }
}
