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

#include "scene/3d/physics_body.h"
#include "servers/navigation_server.h"

void NavigationAgent::_bind_methods() {

    ClassDB::bind_method(D_METHOD("set_neighbor_dist", "neighbor_dist"), &NavigationAgent::set_neighbor_dist);
    ClassDB::bind_method(D_METHOD("get_neighbor_dist"), &NavigationAgent::get_neighbor_dist);

    ClassDB::bind_method(D_METHOD("set_max_neighbors", "max_neighbors"), &NavigationAgent::set_max_neighbors);
    ClassDB::bind_method(D_METHOD("get_max_neighbors"), &NavigationAgent::get_max_neighbors);

    ClassDB::bind_method(D_METHOD("set_time_horizon", "time_horizon"), &NavigationAgent::set_time_horizon);
    ClassDB::bind_method(D_METHOD("get_time_horizon"), &NavigationAgent::get_time_horizon);

    ClassDB::bind_method(D_METHOD("set_time_horizon_obs", "time_horizon_obs"), &NavigationAgent::set_time_horizon_obs);
    ClassDB::bind_method(D_METHOD("get_time_horizon_obs"), &NavigationAgent::get_time_horizon_obs);

    ClassDB::bind_method(D_METHOD("set_radius", "radius"), &NavigationAgent::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &NavigationAgent::get_radius);

    ClassDB::bind_method(D_METHOD("set_max_speed", "max_speed"), &NavigationAgent::set_max_speed);
    ClassDB::bind_method(D_METHOD("get_max_speed"), &NavigationAgent::get_max_speed);

    ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &NavigationAgent::set_velocity);

    ClassDB::bind_method(D_METHOD("_avoidance_done", "new_velocity"), &NavigationAgent::_avoidance_done);

    ADD_PROPERTY(PropertyInfo(Variant::REAL, "neighbor_dist", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_neighbor_dist", "get_neighbor_dist");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_neighbors", PROPERTY_HINT_RANGE, "1,10000,1"), "set_max_neighbors", "get_max_neighbors");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "time_horizon", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_time_horizon", "get_time_horizon");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "time_horizon_obs", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_time_horizon_obs", "get_time_horizon_obs");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "radius", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_radius", "get_radius");
    ADD_PROPERTY(PropertyInfo(Variant::REAL, "max_speed", PROPERTY_HINT_RANGE, "0.1,10000,0.01"), "set_max_speed", "get_max_speed");

    ADD_SIGNAL(MethodInfo("velocity_computed", PropertyInfo(Variant::VECTOR3, "safe_velocity")));
}

void NavigationAgent::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_READY: {
            ERR_FAIL_COND(agent.is_valid());
            PhysicsBody *parent = Object::cast_to<PhysicsBody>(get_parent());
            if (parent) {
                //agent = NavigationServer::get_singleton()->agent_add(parent->get_world()->get_collision_avoidance_space());

                NavigationServer::get_singleton()->agent_set_neighbor_dist(agent, neighbor_dist);
                NavigationServer::get_singleton()->agent_set_max_neighbors(agent, max_neighbors);
                NavigationServer::get_singleton()->agent_set_time_horizon(agent, time_horizon);
                NavigationServer::get_singleton()->agent_set_time_horizon_obs(agent, time_horizon_obs);
                NavigationServer::get_singleton()->agent_set_radius(agent, radius);
                NavigationServer::get_singleton()->agent_set_max_speed(agent, max_speed);
                NavigationServer::get_singleton()->agent_set_callback(agent, this, "_avoidance_done");
                set_physics_process_internal(true);
            }
        } break;
        case NOTIFICATION_EXIT_TREE: {
            if (agent.is_valid()) {
                NavigationServer::get_singleton()->free(agent);
                agent = RID();
                set_physics_process_internal(false);
            }
        } break;
        case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
            if (agent.is_valid()) {
                PhysicsBody *parent = Object::cast_to<PhysicsBody>(get_parent());
                const Vector2 o(parent->get_global_transform().origin.x, parent->get_global_transform().origin.z);
                NavigationServer::get_singleton()->agent_set_position(agent, o);
            }
        } break;
    }
}

NavigationAgent::NavigationAgent() :
        agent(RID()),
        neighbor_dist(30.0),
        max_neighbors(10),
        time_horizon(20.0),
        time_horizon_obs(20.0),
        radius(1.0),
        max_speed(20.0),
        velocity_submitted(false) {
}

void NavigationAgent::set_neighbor_dist(real_t p_dist) {
    neighbor_dist = p_dist;
    if (agent.is_valid()) {
        NavigationServer::get_singleton()->agent_set_neighbor_dist(agent, neighbor_dist);
    }
}

void NavigationAgent::set_max_neighbors(int p_count) {
    max_neighbors = p_count;
    if (agent.is_valid()) {
        NavigationServer::get_singleton()->agent_set_max_neighbors(agent, max_neighbors);
    }
}

void NavigationAgent::set_time_horizon(real_t p_time) {
    time_horizon = p_time;
    if (agent.is_valid()) {
        NavigationServer::get_singleton()->agent_set_time_horizon(agent, time_horizon);
    }
}

void NavigationAgent::set_time_horizon_obs(real_t p_time) {
    time_horizon_obs = p_time;
    if (agent.is_valid()) {
        NavigationServer::get_singleton()->agent_set_time_horizon_obs(agent, time_horizon_obs);
    }
}

void NavigationAgent::set_radius(real_t p_radius) {
    radius = p_radius;
    if (agent.is_valid()) {
        NavigationServer::get_singleton()->agent_set_radius(agent, radius);
    }
}

void NavigationAgent::set_max_speed(real_t p_max_speed) {
    max_speed = p_max_speed;
    if (agent.is_valid()) {
        NavigationServer::get_singleton()->agent_set_max_speed(agent, max_speed);
    }
}

void NavigationAgent::set_velocity(Vector3 p_velocity) {
    if (agent.is_valid()) {
        target_velocity = p_velocity;
        Vector2 v(target_velocity.x, target_velocity.z);
        NavigationServer::get_singleton()->agent_set_target_velocity(agent, v);
        NavigationServer::get_singleton()->agent_set_velocity(agent, prev_safe_velocity);
        velocity_submitted = true;
    }
}

void NavigationAgent::_avoidance_done(Vector2 p_new_velocity) {
    prev_safe_velocity = p_new_velocity;

    if (!velocity_submitted) {
        target_velocity = Vector3();
        return;
    }
    velocity_submitted = false;

    Vector3 vel(p_new_velocity.x, target_velocity.y, p_new_velocity.y);
    emit_signal("velocity_computed", vel);
}

String NavigationAgent::get_configuration_warning() const {
    if (!Object::cast_to<PhysicsBody>(get_parent())) {
        return TTR("CollisionAvoidanceController only serves to provide collision avoidance to a physics object. Please only use it as a child of RigidBody, KinematicBody.");
    }

    return String();
}
