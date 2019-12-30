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

void GdNavigationServer::add_command(SetCommand *command) {
    // TODO Mutex here please
    commands.push_back(command);
}

RID GdNavigationServer::map_create() {
    // TODO Mutex here please
    NavMap *space = memnew(NavMap);
    RID rid = map_owner.make_rid(space);
    space->set_self(rid);
    return rid;
}

#define COMMAND_2(F_NAME, T_0, D_0, T_1, D_1)                   \
    struct __CONCAT(F_NAME, _command) : public SetCommand {     \
        T_0 d_0;                                                \
        T_1 d_1;                                                \
        __CONCAT(F_NAME, _command)                              \
        (                                                       \
                T_0 p_d_0,                                      \
                T_1 p_d_1) :                                    \
                d_0(p_d_0),                                     \
                d_1(p_d_1) {}                                   \
        virtual void exec(GdNavigationServer *server) {         \
            server->__CONCAT(_cmd_, F_NAME)(d_0, d_1);          \
        }                                                       \
    };                                                          \
                                                                \
    void GdNavigationServer::F_NAME(T_0 D_0, T_1 D_1) const {   \
        /* TODO mutex here */                                   \
        auto cmd = memnew(__CONCAT(F_NAME, _command)(           \
                D_0,                                            \
                D_1));                                          \
                                                                \
        auto mut_this = const_cast<GdNavigationServer *>(this); \
        mut_this->add_command(cmd);                             \
    }                                                           \
    void GdNavigationServer::__CONCAT(_cmd_, F_NAME)(T_0 D_0, T_1 D_1)

#define COMMAND_4(F_NAME, T_0, D_0, T_1, D_1, T_2, D_2, T_3, D_3)               \
    struct __CONCAT(F_NAME, _command) : public SetCommand {                     \
        T_0 d_0;                                                                \
        T_1 d_1;                                                                \
        T_2 d_2;                                                                \
        T_3 d_3;                                                                \
        __CONCAT(F_NAME, _command)                                              \
        (                                                                       \
                T_0 p_d_0,                                                      \
                T_1 p_d_1,                                                      \
                T_2 p_d_2,                                                      \
                T_3 p_d_3) :                                                    \
                d_0(p_d_0),                                                     \
                d_1(p_d_1),                                                     \
                d_2(p_d_2),                                                     \
                d_3(p_d_3) {}                                                   \
        virtual void exec(GdNavigationServer *server) {                         \
            server->__CONCAT(_cmd_, F_NAME)(d_0, d_1, d_2, d_3);                \
        }                                                                       \
    };                                                                          \
                                                                                \
    void GdNavigationServer::F_NAME(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3) const { \
        /* TODO mutex here */                                                   \
        auto cmd = memnew(__CONCAT(F_NAME, _command)(                           \
                D_0,                                                            \
                D_1,                                                            \
                D_2,                                                            \
                D_3));                                                          \
                                                                                \
        auto mut_this = const_cast<GdNavigationServer *>(this);                 \
        mut_this->add_command(cmd);                                             \
    }                                                                           \
    void GdNavigationServer::__CONCAT(_cmd_, F_NAME)(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3)

COMMAND_2(map_set_active, RID, p_map, bool, p_active) {
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

COMMAND_2(map_set_up, RID, p_map, Vector3, p_up) {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND(map == NULL);

    map->set_up(p_up);
}

Vector3 GdNavigationServer::map_get_up(RID p_map) const {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND_V(map == NULL, Vector3());

    return map->get_up();
}

COMMAND_2(map_set_cell_size, RID, p_map, real_t, p_cell_size) {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND(map == NULL);

    map->set_cell_size(p_cell_size);
}

real_t GdNavigationServer::map_get_cell_size(RID p_map) const {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND_V(map == NULL, 0);

    return map->get_cell_size();
}

COMMAND_2(map_set_edge_connection_margin, RID, p_map, real_t, p_connection_margin) {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND(map == NULL);

    map->set_edge_connection_margin(p_connection_margin);
}

real_t GdNavigationServer::map_get_edge_connection_margin(RID p_map) const {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND_V(map == NULL, 0);

    return map->get_edge_connection_margin();
}

Vector<Vector3> GdNavigationServer::map_get_path(RID p_map, Vector3 p_origin, Vector3 p_destination, bool p_optimize) const {
    NavMap *map = map_owner.get(p_map);
    ERR_FAIL_COND_V(map == NULL, Vector<Vector3>());

    return map->get_path(p_origin, p_destination, p_optimize);
}

RID GdNavigationServer::region_create() {
    // TODO Mutex here please
    NavRegion *reg = memnew(NavRegion);
    RID rid = region_owner.make_rid(reg);
    reg->set_self(rid);
    return rid;
}

COMMAND_2(region_set_map, RID, p_region, RID, p_map) {
    NavRegion *region = region_owner.get(p_region);
    ERR_FAIL_COND(region == NULL);

    if (region->get_map() != NULL) {

        if (region->get_map()->get_self() == p_map)
            return; // Pointless

        region->get_map()->remove_region(region);
        region->set_map(NULL);
    }

    if (p_map.is_valid()) {
        NavMap *map = map_owner.get(p_map);
        ERR_FAIL_COND(map == NULL);

        map->add_region(region);
        region->set_map(map);
    }
}

COMMAND_2(region_set_transform, RID, p_region, Transform, p_transform) {
    NavRegion *region = region_owner.get(p_region);
    ERR_FAIL_COND(region == NULL);

    region->set_transform(p_transform);
}

COMMAND_2(region_set_navmesh, RID, p_region, Ref<NavigationMesh>, p_nav_mesh) {
    NavRegion *region = region_owner.get(p_region);
    ERR_FAIL_COND(region == NULL);

    region->set_mesh(p_nav_mesh);
}

RID GdNavigationServer::agent_create() {
    // TODO Mutex here please
    RvoAgent *agent = memnew(RvoAgent());
    RID rid = agent_owner.make_rid(agent);
    agent->set_self(rid);

    return rid;
}

COMMAND_2(agent_set_map, RID, p_agent, RID, p_map) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    if (agent->get_map()) {
        if (agent->get_map()->get_self() == p_map)
            return; // Pointless

        agent->get_map()->remove_agent(agent);
    }

    agent->set_map(NULL);

    if (p_map.is_valid()) {
        NavMap *map = map_owner.get(p_map);
        ERR_FAIL_COND(map == NULL);

        agent->set_map(map);
        map->add_agent(agent);

        if (agent->has_callback()) {
            map->set_agent_as_controlled(agent);
        }
    }
}

COMMAND_2(agent_set_neighbor_dist, RID, p_agent, real_t, p_dist) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->neighborDist_ = p_dist;
}

COMMAND_2(agent_set_max_neighbors, RID, p_agent, int, p_count) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->maxNeighbors_ = p_count;
}

COMMAND_2(agent_set_time_horizon, RID, p_agent, real_t, p_time) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->timeHorizon_ = p_time;
}

COMMAND_2(agent_set_radius, RID, p_agent, real_t, p_radius) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->radius_ = p_radius;
}

COMMAND_2(agent_set_max_speed, RID, p_agent, real_t, p_max_speed) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->maxSpeed_ = p_max_speed;
}

COMMAND_2(agent_set_velocity, RID, p_agent, Vector3, p_velocity) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->velocity_ = RVO::Vector2(p_velocity.x, p_velocity.z);
}

COMMAND_2(agent_set_target_velocity, RID, p_agent, Vector3, p_velocity) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->prefVelocity_ = RVO::Vector2(p_velocity.x, p_velocity.z);
}

COMMAND_2(agent_set_position, RID, p_agent, Vector3, p_position) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->get_agent()->position_ = RVO::Vector2(p_position.x, p_position.z);
}

COMMAND_4(agent_set_callback, RID, p_agent, Object *, p_receiver, StringName, p_method, Variant, p_udata) {
    RvoAgent *agent = agent_owner.get(p_agent);
    ERR_FAIL_COND(agent == NULL);

    agent->set_callback(p_receiver == NULL ? 0 : p_receiver->get_instance_id(), p_method, p_udata);

    if (agent->get_map())
        if (p_receiver == NULL) {
            agent->get_map()->remove_agent_as_controlled(agent);
        } else {
            agent->get_map()->set_agent_as_controlled(agent);
        }
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
        // TODO nothing more to remove??
        region_owner.free(p_object);
        memdelete(nav);
    } else if (agent_owner.owns(p_object)) {
        RvoAgent *obj = agent_owner.get(p_object);
        if (obj->get_map() != NULL) {
            obj->get_map()->remove_agent(obj);
        }
        agent_owner.free(p_object);
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

    for (uint i(0); i < commands.size(); i++) {
        commands[i]->exec(this);
        memdelete(commands[i]);
    }
    commands.clear();

    for (int i(0); i < active_maps.size(); i++) {
        active_maps[i]->sync();
        active_maps[i]->step(p_delta_time);
        active_maps[i]->dispatch_callbacks();
    }
}
