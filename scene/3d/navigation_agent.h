/*************************************************************************/
/*  navigation_agent.h                                                   */
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

#ifndef NAVIGATION_AGENT_H
#define NAVIGATION_AGENT_H

#include "core/vector.h"
#include "scene/main/node.h"

class Spatial;
class Navigation;

class NavigationAgent : public Node {
    GDCLASS(NavigationAgent, Node);

    Spatial *agent_node;
    Navigation *navigation;

    RID agent;

    real_t half_height;
    real_t radius;
    real_t neighbor_dist;
    int max_neighbors;
    real_t time_horizon;
    real_t max_speed;

    real_t path_max_distance;

    Vector3 target_location;
    Vector<Vector3> navigation_path;
    uint nav_path_index;
    bool velocity_submitted;
    Vector3 prev_safe_velocity;
    /// The submitted target velocity
    Vector3 target_velocity;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    NavigationAgent();
    virtual ~NavigationAgent();

    void set_navigation(Navigation *p_nav);
    const Navigation *get_navigation() const {
        return navigation;
    }

    void set_navigation_node(Node *p_nav);
    Node *get_navigation_node() const;

    RID get_rid() const {
        return agent;
    }

    void set_radius(real_t p_radius);
    real_t get_radius() const {
        return radius;
    }

    void set_half_height(real_t p_hh);
    real_t get_half_height() const {
        return half_height;
    }

    void set_neighbor_dist(real_t p_dist);
    real_t get_neighbor_dist() const {
        return neighbor_dist;
    }

    void set_max_neighbors(int p_count);
    int get_max_neighbors() const {
        return max_neighbors;
    }

    void set_time_horizon(real_t p_time);
    real_t get_time_horizon() const {
        return time_horizon;
    }

    void set_max_speed(real_t p_max_speed);
    real_t get_max_speed() const {
        return max_speed;
    }

    void set_path_max_distance(real_t p_pmd);
    real_t get_path_max_distance();

    void set_target_location(Vector3 p_location);
    Vector3 get_target_location() const;

    Vector3 get_next_location();

    Vector<Vector3> get_nav_path() const {
        return navigation_path;
    }

    uint get_nav_path_index() const {
        return nav_path_index;
    }

    real_t distance_to_target() const;

    void set_velocity(Vector3 p_velocity);
    void _avoidance_done(Vector3 p_new_velocity);

    virtual String get_configuration_warning() const;
};

#endif
