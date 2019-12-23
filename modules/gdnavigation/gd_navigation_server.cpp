/*************************************************************************/
/*  gd_navigation_server.cpp                                             */
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

#include "gd_navigation_server.h"

GdNavigationServer::GdNavigationServer() :
        NavigationServer(),
        active(true) {
}

GdNavigationServer::~GdNavigationServer() {}

RID GdNavigationServer::map_create() {
    NavMap *space = memnew(NavMap);
    RID rid = map_owner.make_rid(space);
    space->set_self(rid);
    return rid;
}

void GdNavigationServer::map_set_active(RID p_map, bool p_active) {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND(map == NULL);

    if (p_active) {
        if (!map_is_active(p_map)) {
            active_maps.push_back(map);
        }
    } else {
        active_maps.erase(map);
    }
}

bool GdNavigationServer::map_is_active(RID p_map) const {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND_V(map == NULL, false);

    return active_maps.find(map) >= 0;
}

RID GdNavigationServer::region_create() {
    NavRegion *reg = memnew(NavRegion);
    RID rid = region_owner.make_rid(reg);
    reg->set_self(rid);
    return rid;
}

void GdNavigationServer::region_set_map(RID p_region, RID p_map) {
    NavRegion *region = region_owner.get(p_region);
    ERR_FAIL_COND(region == NULL);

    NavMap *map = map_owner.get(p_map);

    if (region->get_map() == map)
        // Pointless
        return;

    if (region->get_map() != NULL) {
        region->get_map()->remove_region(region);
        region->set_map(NULL);
    }

    if (map != NULL) {

        map->add_region(region);
        region->set_map(map);
    }
}

void GdNavigationServer::region_set_transform(RID p_region, Transform p_transform) {
    NavRegion *region = region_owner.get(p_region);
    ERR_FAIL_COND(region == NULL);

    region->set_transform(p_transform);
}

void GdNavigationServer::region_set_navmesh(RID p_region, Ref<NavigationMesh> p_nav_mesh) {
    NavRegion *region = region_owner.get(p_region);
    ERR_FAIL_COND(region == NULL);

    region->set_mesh(p_nav_mesh);
}

RID GdNavigationServer::agent_add(RID p_space) {
    NavMap *space = map_owner.get(p_space);
    ERR_FAIL_COND_V(space == NULL, RID());

    RvoAgent *agent = memnew(RvoAgent(space));
    RID rid = agent_owner.make_rid(agent);
    agent->set_self(rid);

    space->add_agent(agent);

    return rid;
}

void GdNavigationServer::agent_set_neighbor_dist(RID p_agent, real_t p_dist) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->neighborDist_ = p_dist;
}

void GdNavigationServer::agent_set_max_neighbors(RID p_agent, int p_count) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->maxNeighbors_ = p_count;
}

void GdNavigationServer::agent_set_time_horizon(RID p_agent, real_t p_time) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->timeHorizon_ = p_time;
}

void GdNavigationServer::agent_set_time_horizon_obs(RID p_agent, real_t p_time) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->timeHorizonObst_ = p_time;
}

void GdNavigationServer::agent_set_radius(RID p_agent, real_t p_radius) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->radius_ = p_radius;
}

void GdNavigationServer::agent_set_max_speed(RID p_agent, real_t p_max_speed) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->maxSpeed_ = p_max_speed;
}

void GdNavigationServer::agent_set_velocity(RID p_agent, Vector2 p_velocity) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->velocity_ = RVO::Vector2(p_velocity.x, p_velocity.y);
}

void GdNavigationServer::agent_set_target_velocity(RID p_agent, Vector2 p_velocity) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->prefVelocity_ = RVO::Vector2(p_velocity.x, p_velocity.y);
}

void GdNavigationServer::agent_set_position(RID p_agent, Vector2 p_position) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->position_ = RVO::Vector2(p_position.x, p_position.y);
}

void GdNavigationServer::agent_set_callback(RID p_agent, Object *p_receiver, const StringName &p_method, const Variant &p_udata) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->set_callback(p_receiver->get_instance_id(), p_method, p_udata);
}

RID GdNavigationServer::obstacle_add(RID p_space) {
    RvoObstacle *obstacle = memnew(RvoObstacle);
    RID rid = obstacle_owner.make_rid(obstacle);
    obstacle->set_self(rid);
    return rid;
}

void GdNavigationServer::free(RID p_object) {
    if (map_owner.owns(p_object)) {
        NavMap *obj = map_owner.get(p_object);

        // Destroy all the agents of this server
        if (obj->get_agents().size() != 0) {
            print_error("The collision avoidance server destroyed contains Agents, please destroy these.");
        }

        for (int i(0); i < obj->get_agents().size(); i++) {
            agent_owner.free(obj->get_agents()[i]->get_self());
            memdelete(obj->get_agents()[i]);
            obj->get_agents()[i] = NULL;
        }

        // TODO please destroy Obstacles

        map_set_active(p_object, false);
        map_owner.free(p_object);

        memdelete(obj);
    } else if (region_owner.owns(p_object)) {
        NavRegion *nav = region_owner.get(p_object);
        region_owner.free(p_object);
        memdelete(nav);
    } else if (agent_owner.owns(p_object)) {
        RvoAgent *obj = agent_owner.get(p_object);
        obj->get_space()->remove_agent(obj);
        agent_owner.free(p_object);
        memdelete(obj);
    } else if (obstacle_owner.owns(p_object)) {
        RvoObstacle *obj = obstacle_owner.get(p_object);
        // TODO please remove from the space
        obstacle_owner.free(p_object);
        memdelete(obj);
    } else {
        ERR_FAIL_COND("Invalid ID.");
    }
}

void GdNavigationServer::set_active(bool p_active) {
    active = p_active;
}

void GdNavigationServer::step(real_t p_delta_time) {
    if (!active) {
        return;
    }

    for (int i(0); i < active_maps.size(); i++) {
        active_maps[i]->sync();
        active_maps[i]->step(p_delta_time);
        active_maps[i]->dispatch_callbacks();
    }
}
