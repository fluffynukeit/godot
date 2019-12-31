/*************************************************************************/
/*  navigation_agent.cpp                                                 */
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

#include "navigation_agent.h"

#include "scene/3d/navigation.h"
#include "scene/3d/physics_body.h"
#include "servers/navigation_server.h"

void NavigationAgent::_bind_methods() {

    ClassDB::bind_method(D_METHOD("set_radius", "radius"), &NavigationAgent::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &NavigationAgent::get_radius);

    ClassDB::bind_method(D_METHOD("set_half_height", "navigation"), &NavigationAgent::set_half_height);
    ClassDB::bind_method(D_METHOD("get_half_height"), &NavigationAgent::get_half_height);

    ClassDB::bind_method(D_METHOD("set_navigation", "navigation"), &NavigationAgent::set_navigation_node);
    ClassDB::bind_method(D_METHOD("get_navigation"), &NavigationAgent::get_navigation_node);

    ClassDB::bind_method(D_METHOD("set_neighbor_dist", "neighbor_dist"), &NavigationAgent::set_neighbor_dist);
    ClassDB::bind_method(D_METHOD("get_neighbor_dist"), &NavigationAgent::get_neighbor_dist);

    ClassDB::bind_method(D_METHOD("set_max_neighbors", "max_neighbors"), &NavigationAgent::set_max_neighbors);
    ClassDB::bind_method(D_METHOD("get_max_neighbors"), &NavigationAgent::get_max_neighbors);

    ClassDB::bind_method(D_METHOD("set_time_horizon", "time_horizon"), &NavigationAgent::set_time_horizon);
    ClassDB::bind_method(D_METHOD("get_time_horizon"), &NavigationAgent::get_time_horizon);

    ClassDB::bind_method(D_METHOD("set_max_speed", "max_speed"), &NavigationAgent::set_max_speed);
    ClassDB::bind_method(D_METHOD("get_max_speed"), &NavigationAgent::get_max_speed);

    ClassDB::bind_method(D_METHOD("set_path_max_distance", "max_speed"), &NavigationAgent::set_path_max_distance);
    ClassDB::bind_method(D_METHOD("get_path_max_distance"), &NavigationAgent::get_path_max_distance);

    ClassDB::bind_method(D_METHOD("set_target_location", "location"), &NavigationAgent::set_target_location);
    ClassDB::bind_method(D_METHOD("get_target_location"), &NavigationAgent::get_target_location);
    ClassDB::bind_method(D_METHOD("get_next_location"), &NavigationAgent::get_next_location);
    ClassDB::bind_method(D_METHOD("distance_to_target"), &NavigationAgent::distance_to_target);
    ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &NavigationAgent::set_velocity);
    ClassDB::bind_method(D_METHOD("get_nav_path"), &NavigationAgent::get_nav_path);
    ClassDB::bind_method(D_METHOD("get_nav_path_index"), &NavigationAgent::get_nav_path_index);

    ClassDB::bind_method(D_METHOD("_avoidance_done", "new_velocity"), &NavigationAgent::_avoidance_done);

    ADD_PROPERTY(PropertyInfo(Variant::REAL, "radius", PROPERTY_HINT_RANGE, "0.1,100,0.01"), "set_radius", "get_radius");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "half_height", PROPERTY_HINT_RANGE, "0.1,100,0.01"), "set_half_height", "get_half_height");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "neighbor_dist", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_neighbor_dist", "get_neighbor_dist");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_neighbors", PROPERTY_HINT_RANGE, "1,10000,1"), "set_max_neighbors", "get_max_neighbors");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "time_horizon", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_time_horizon", "get_time_horizon");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "max_speed", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_max_speed", "get_max_speed");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "path_max_distance", PROPERTY_HINT_RANGE, "1,10,0.1"), "set_path_max_distance", "get_path_max_distance");

    ADD_SIGNAL(MethodInfo("path_changed"));
    ADD_SIGNAL(MethodInfo("velocity_computed", PropertyInfo(Variant::VECTOR3, "safe_velocity")));
}

void NavigationAgent::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {

            agent_node = Object::cast_to<Spatial>(get_parent());

            NavigationServer::get_singleton()->agent_set_callback(agent, this, "_avoidance_done");

            // Search the navigation node and set it
            {
                Navigation *nav = NULL;
                Node *p = get_parent();
                while (p != NULL) {
                    nav = Object::cast_to<Navigation>(p);
                    if (nav != NULL)
                        p = NULL;
                    else
                        p = p->get_parent();
                }

                set_navigation(nav);
            }

            set_physics_process_internal(true);
        } break;
        case NOTIFICATION_EXIT_TREE: {
            agent_node = NULL;
            set_navigation(NULL);
            set_physics_process_internal(false);
        } break;
        case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
            PhysicsBody *parent = Object::cast_to<PhysicsBody>(get_parent());
            NavigationServer::get_singleton()->agent_set_position(agent, parent->get_global_transform().origin);
        } break;
    }
}

NavigationAgent::NavigationAgent() :
        half_height(1.5),
        agent_node(NULL),
        navigation(NULL),
        agent(RID()),
        velocity_submitted(false),
        path_max_distance(3.0) {
    agent = NavigationServer::get_singleton()->agent_create();
    set_neighbor_dist(30.0);
    set_max_neighbors(10);
    set_time_horizon(20.0);
    set_radius(1.0);
    set_max_speed(20.0);
}

NavigationAgent::~NavigationAgent() {
    NavigationServer::get_singleton()->free(agent);
    agent = RID(); // Pointless
}

void NavigationAgent::set_navigation(Navigation *p_nav) {
    if (navigation == p_nav)
        return; // Pointless

    navigation = p_nav;
    NavigationServer::get_singleton()->agent_set_map(agent, navigation == NULL ? RID() : navigation->get_rid());
}

void NavigationAgent::set_navigation_node(Node *p_nav) {
    Navigation *nav = Object::cast_to<Navigation>(p_nav);
    ERR_FAIL_COND(nav == NULL);
    set_navigation(nav);
}

Node *NavigationAgent::get_navigation_node() const {
    return Object::cast_to<Node>(navigation);
}

void NavigationAgent::set_radius(real_t p_radius) {
    radius = p_radius;
    NavigationServer::get_singleton()->agent_set_radius(agent, radius);
}

void NavigationAgent::set_half_height(real_t p_hh) {
    half_height = p_hh;
}

void NavigationAgent::set_neighbor_dist(real_t p_dist) {
    neighbor_dist = p_dist;
    NavigationServer::get_singleton()->agent_set_neighbor_dist(agent, neighbor_dist);
}

void NavigationAgent::set_max_neighbors(int p_count) {
    max_neighbors = p_count;
    NavigationServer::get_singleton()->agent_set_max_neighbors(agent, max_neighbors);
}

void NavigationAgent::set_time_horizon(real_t p_time) {
    time_horizon = p_time;
    NavigationServer::get_singleton()->agent_set_time_horizon(agent, time_horizon);
}

void NavigationAgent::set_max_speed(real_t p_max_speed) {
    max_speed = p_max_speed;
    NavigationServer::get_singleton()->agent_set_max_speed(agent, max_speed);
}

void NavigationAgent::set_path_max_distance(real_t p_pmd) {
    path_max_distance = p_pmd;
}

real_t NavigationAgent::get_path_max_distance() {
    return path_max_distance;
}

void NavigationAgent::set_target_location(Vector3 p_location) {
    target_location = p_location;
    navigation_path.clear();
}

Vector3 NavigationAgent::get_target_location() const {
    return target_location;
}

Vector3 NavigationAgent::get_next_location() {

    Vector3 o = agent_node->get_global_transform().origin;

    ERR_FAIL_COND_V(agent_node == NULL, o);

    bool reload_path = false;

    if (NavigationServer::get_singleton()->agent_is_map_changed(agent)) {
        reload_path = true;
    } else if (navigation_path.size() == 0) {
        reload_path = true;
    } else {
        // Check if too far from the navigation path
        if (nav_path_index > 0) {
            Vector3 segment[2];
            segment[0] = navigation_path[nav_path_index - 1];
            segment[1] = navigation_path[nav_path_index];
            segment[0].y += half_height;
            segment[1].y += half_height;
            Vector3 p = Geometry::get_closest_point_to_segment(o, segment);
            if (o.distance_to(p) >= path_max_distance) {
                // To faraway, reload path
                reload_path = true;
            }
        }
    }

    if (reload_path) {
        navigation_path = NavigationServer::get_singleton()->map_get_path(navigation->get_rid(), o, target_location, true);
        nav_path_index = 0;
        emit_signal("path_changed");
    } else {
        // Check if we can advance the navigation path
        if (nav_path_index + 1 < navigation_path.size()) {
            if (o.distance_to(navigation_path[nav_path_index] + Vector3(0, half_height, 0)) < radius) {
                nav_path_index += 1;
            }
        }
    }

    if (navigation_path.size() == 0)
        return o;

    return navigation_path[nav_path_index] + Vector3(0, half_height, 0);
}

real_t NavigationAgent::distance_to_target() const {
    ERR_FAIL_COND_V(agent_node == NULL, 0.0);
    return agent_node->get_global_transform().origin.distance_to(target_location);
}

void NavigationAgent::set_velocity(Vector3 p_velocity) {
    target_velocity = p_velocity;
    NavigationServer::get_singleton()->agent_set_target_velocity(agent, target_velocity);
    NavigationServer::get_singleton()->agent_set_velocity(agent, prev_safe_velocity);
    velocity_submitted = true;
}

void NavigationAgent::_avoidance_done(Vector3 p_new_velocity) {
    prev_safe_velocity = p_new_velocity;

    if (!velocity_submitted) {
        target_velocity = Vector3();
        return;
    }
    velocity_submitted = false;

    // TODO Skip Y velocity?
    //Vector3 vel(p_new_velocity.x, target_velocity.y, p_new_velocity.y);
    emit_signal("velocity_computed", p_new_velocity);
}

String NavigationAgent::get_configuration_warning() const {
    if (!Object::cast_to<PhysicsBody>(get_parent())) {
        return TTR("The NavigationAgent can be used only under a spatial node");
    }

    return String();
}
