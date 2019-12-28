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
#include "nav_region.h"
#include "rvo_agent.h"

#define USE_ENTRY_POINT

NavMap::NavMap() :
        up(0, 1, 0),
        cell_size(0.2),
        edge_connection_margin(2.0),
        regenerate_polygons(true),
        regenerate_links(true),
        agents_dirty(false),
        deltatime(0.0) {}

void NavMap::set_up(Vector3 p_up) {
    up = p_up;
    regenerate_polygons = true;
}

void NavMap::set_cell_size(float p_cell_size) {
    cell_size = p_cell_size;
    regenerate_polygons = true;
}

void NavMap::set_edge_connection_margin(float p_edge_connection_margin) {
    edge_connection_margin = p_edge_connection_margin;
    regenerate_links = true;
}

PointKey NavMap::get_point_key(const Vector3 &p_pos) const {
    const int x = int(Math::floor(p_pos.x / cell_size));
    const int y = int(Math::floor(p_pos.y / cell_size));
    const int z = int(Math::floor(p_pos.z / cell_size));

    PointKey p;
    p.key = 0;
    p.x = x;
    p.y = y;
    p.z = z;
    return p;
}

Vector<Vector3> NavMap::get_path(Vector3 p_origin, Vector3 p_destination, bool p_optimize) const {

    // TODO check for write lock?

    const Polygon *begin_poly = NULL;
    const Polygon *end_poly = NULL;
    Vector3 begin_point;
    Vector3 end_point;
    float begin_d = 1e20;
    float end_d = 1e20;

    // Find the initial poly and the end poly on this map.
    for (uint i(0); i < polygons.size(); i++) {
        const Polygon &p = polygons[i];

        // For each point cast a face and check the distance between the origin/destination
        for (uint point_id = 2; point_id < p.points.size(); point_id++) {

            Face3 f(p.points[point_id - 2].pos, p.points[point_id - 1].pos, p.points[point_id].pos);
            Vector3 spoint = f.get_closest_point_to(p_origin);
            float dpoint = spoint.distance_to(p_origin);
            if (dpoint < begin_d) {
                begin_d = dpoint;
                begin_poly = &p;
                begin_point = spoint;
            }

            spoint = f.get_closest_point_to(p_destination);
            dpoint = spoint.distance_to(p_destination);
            if (dpoint < end_d) {
                end_d = dpoint;
                end_poly = &p;
                end_point = spoint;
            }
        }
    }

    if (!begin_poly || !end_poly) {
        // No path
        return Vector<Vector3>();
    }

    if (begin_poly == end_poly) {

        Vector<Vector3> path;
        path.resize(2);
        path.write[0] = begin_point;
        path.write[1] = end_point;
        return path;
    }

    // TODO reserve size?
    std::vector<NavigationPoly> navigation_polys;
    // The elements indices in the `navigation_polys`.
    int least_cost_id(-1);
    List<uint> open_list;
    bool found_route = false;

    navigation_polys.push_back(NavigationPoly(begin_poly));
    {
        least_cost_id = 0;
        NavigationPoly *least_cost_poly = &navigation_polys[least_cost_id];
        least_cost_poly->self_id = least_cost_id;
        least_cost_poly->entry = begin_point;
    }

    // Just some sugar
    open_list.push_back(0);

    while (found_route == false) {

        {
            NavigationPoly *least_cost_poly = &navigation_polys[least_cost_id];
            // Takes the current least_cost_poly neighbors and compute the traveled_distance of each
            for (int i = 0; i < least_cost_poly->poly->edges.size(); i++) {

                const Edge &edge = least_cost_poly->poly->edges[i];
                if (!edge.other_polygon)
                    continue;

#ifdef USE_ENTRY_POINT
                Vector3 edge_line[2] = {
                    least_cost_poly->poly->points[i].pos,
                    least_cost_poly->poly->points[(i + 1) % least_cost_poly->poly->points.size()].pos
                };

                const Vector3 new_entry = Geometry::get_closest_point_to_segment(least_cost_poly->entry, edge_line);
                const float new_distance = least_cost_poly->entry.distance_to(new_entry) + least_cost_poly->traveled_distance;
#else
                const float new_distance = least_cost_poly->poly->center.distance_to(edge.other_polygon->center) + least_cost_poly->traveled_distance;
#endif

                auto it = std::find(
                        navigation_polys.begin(),
                        navigation_polys.end(),
                        NavigationPoly(edge.other_polygon));

                if (it != navigation_polys.end()) {
                    // Oh this was visited already, can we win the cost?
                    if (it->traveled_distance > new_distance) {

                        it->prev_navigation_poly_id = least_cost_id;
                        it->back_navigation_edge = edge.other_edge;
                        it->traveled_distance = new_distance;
#ifdef USE_ENTRY_POINT
                        it->entry = new_entry;
#endif
                    }
                } else {
                    // Add to open neighbours

                    navigation_polys.push_back(NavigationPoly(edge.other_polygon));
                    NavigationPoly *np = &navigation_polys[navigation_polys.size() - 1];

                    np->self_id = navigation_polys.size() - 1;
                    np->prev_navigation_poly_id = least_cost_id;
                    np->back_navigation_edge = edge.other_edge;
                    np->traveled_distance = new_distance;
#ifdef USE_ENTRY_POINT
                    np->entry = new_entry;
#endif
                    open_list.push_back(navigation_polys.size() - 1);
                }

                // At this point the pointer may not be valid anymore because I've touched the `navigation_polys`.
                // So reset the pointer just to be sure.
                least_cost_poly = &navigation_polys[least_cost_id];
            }
        }

        // Removes the least cost polygon from the open list so we can advance.
        open_list.erase(least_cost_id);

        if (open_list.size() == 0) {
            // When the open list is empty at this point the path is not found :(
            break;
        }

        // Now take the new least_cost_poly from the open list.
        least_cost_id = -1;
        float least_cost = 1e30;

        for (auto element = open_list.front(); element != NULL; element = element->next()) {
            NavigationPoly *np = &navigation_polys[element->get()];
            float cost = np->traveled_distance;
#ifdef USE_ENTRY_POINT
            cost += np->entry.distance_to(end_point);
#else
            cost += np->poly->center.distance_to(end_point);
#endif
            if (cost < least_cost) {
                least_cost_id = np->self_id;
                least_cost = cost;
            }
        }

        ERR_BREAK(least_cost_id == -1);

        // Check if we reached the end
        if (navigation_polys[least_cost_id].poly == end_poly) {
            // Yep, done!!
            found_route = true;
            break;
        }
    }

    if (found_route) {

        Vector<Vector3> path;
        if (p_optimize) {

            // String pulling

            NavigationPoly *apex_poly = &navigation_polys[least_cost_id];
            Vector3 apex_point = end_point;
            Vector3 portal_left = apex_point;
            Vector3 portal_right = apex_point;
            NavigationPoly *left_poly = apex_poly;
            NavigationPoly *right_poly = apex_poly;
            NavigationPoly *p = apex_poly;

            path.push_back(end_point);

            while (p) {

                Vector3 left;
                Vector3 right;

#define CLOCK_TANGENT(m_a, m_b, m_c) (((m_a) - (m_c)).cross((m_a) - (m_b)))

                if (p->poly == begin_poly) {
                    left = begin_point;
                    right = begin_point;
                } else {
                    int prev = p->back_navigation_edge;
                    int prev_n = (p->back_navigation_edge + 1) % p->poly->points.size();
                    left = p->poly->points[prev].pos;
                    right = p->poly->points[prev_n].pos;

                    //if (CLOCK_TANGENT(apex_point,left,(left+right)*0.5).dot(up) < 0){
                    if (p->poly->clockwise) {
                        SWAP(left, right);
                    }
                }

                bool skip = false;

                if (CLOCK_TANGENT(apex_point, portal_left, left).dot(up) >= 0) {
                    //process
                    if (portal_left == apex_point || CLOCK_TANGENT(apex_point, left, portal_right).dot(up) > 0) {
                        left_poly = p;
                        portal_left = left;
                    } else {

                        clip_path(navigation_polys, path, apex_poly, portal_right, right_poly);

                        apex_point = portal_right;
                        p = right_poly;
                        left_poly = p;
                        apex_poly = p;
                        portal_left = apex_point;
                        portal_right = apex_point;
                        path.push_back(apex_point);
                        skip = true;
                    }
                }

                if (!skip && CLOCK_TANGENT(apex_point, portal_right, right).dot(up) <= 0) {
                    //process
                    if (portal_right == apex_point || CLOCK_TANGENT(apex_point, right, portal_left).dot(up) < 0) {
                        right_poly = p;
                        portal_right = right;
                    } else {

                        clip_path(navigation_polys, path, apex_poly, portal_left, left_poly);

                        apex_point = portal_left;
                        p = left_poly;
                        right_poly = p;
                        apex_poly = p;
                        portal_right = apex_point;
                        portal_left = apex_point;
                        path.push_back(apex_point);
                    }
                }

                if (p->prev_navigation_poly_id != -1)
                    p = &navigation_polys[p->prev_navigation_poly_id];
                else
                    // The end
                    p = NULL;
            }

            if (path[path.size() - 1] != begin_point)
                path.push_back(begin_point);

            path.invert();

        } else {
            path.push_back(end_point);

            // Add mid points
            int np_id = least_cost_id;
            while (np_id != -1) {

#ifdef USE_ENTRY_POINT
                Vector3 point = navigation_polys[np_id].entry;
#else
                int prev = navigation_polys[np_id].back_navigation_edge;
                int prev_n = (navigation_polys[np_id].back_navigation_edge + 1) % navigation_polys[np_id].poly->points.size();
                Vector3 point = (navigation_polys[np_id].poly->points[prev].pos + navigation_polys[np_id].poly->points[prev_n].pos) * 0.5;
#endif

                path.push_back(point);
                np_id = navigation_polys[np_id].prev_navigation_poly_id;
            }

            path.invert();
        }

        return path;
    }
    return Vector<Vector3>();
}

void NavMap::add_region(NavRegion *p_region) {
    regions.push_back(p_region);
    // TODO reload region links?
}

void NavMap::remove_region(NavRegion *p_region) {
    regions.push_back(p_region);
    // TODO reload region links?
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
    // TODO write lock here?

    if (regenerate_polygons) {
        for (int r(0); r < regions.size(); r++) {
            regions[r]->scratch_polygons();
            regenerate_links = true;
        }
    }

    for (uint r(0); r < regions.size(); r++) {
        if (regions[r]->sync()) {
            regenerate_links = true;
        }
    }

    if (regenerate_links) {
        // Copy all region polygons in the map.
        int count = 0;
        for (uint r(0); r < regions.size(); r++) {
            count += regions[r]->get_polygons().size();
        }

        polygons.resize(count);
        count = 0;

        for (uint r(0); r < regions.size(); r++) {
            std::copy(
                    regions[r]->get_polygons().data(),
                    regions[r]->get_polygons().data() + regions[r]->get_polygons().size(),
                    polygons.begin() + count);

            count += regions[r]->get_polygons().size();
        }

        // Connects the `Edges` of all the `Polygons` of all `Regions` each other.
        Map<EdgeKey, Connection> connections;

        for (uint poly_id(0); poly_id < polygons.size(); poly_id++) {
            Polygon &poly(polygons[poly_id]);

            for (uint p(0); p < poly.points.size(); p++) {
                int next_point = (p + 1) % poly.points.size();
                EdgeKey ek(poly.points[p].key, poly.points[next_point].key);

                Map<EdgeKey, Connection>::Element *connection = connections.find(ek);
                if (!connection) {
                    // Nothing yet
                    Connection c;
                    c.A = &poly;
                    c.A_edge = p;
                    c.B = NULL;
                    c.B_edge = -1;
                    connections[ek] = c;

                } else if (connection->get().B == NULL) {
                    CRASH_COND(connection->get().A == NULL); // Unreachable

                    // Connect the two Polygons by this edge
                    connection->get().B = &poly;
                    connection->get().B_edge = p;

                    connection->get().A->edges[connection->get().A_edge].this_edge = connection->get().A_edge;
                    connection->get().A->edges[connection->get().A_edge].other_polygon = connection->get().B;
                    connection->get().A->edges[connection->get().A_edge].other_edge = connection->get().B_edge;

                    connection->get().B->edges[connection->get().B_edge].this_edge = connection->get().B_edge;
                    connection->get().B->edges[connection->get().B_edge].other_polygon = connection->get().A;
                    connection->get().B->edges[connection->get().B_edge].other_edge = connection->get().A_edge;
                } else {
                    // The edge is already connected with another edge, skip.
                }
            }
        }

        // Connect regions by connecting the free edges
        // TODO
    }

    if (agents_dirty) {
        std::vector<RVO::Agent *> raw_agents;
        raw_agents.reserve(agents.size());
        for (uint i(0); i < agents.size(); i++)
            raw_agents.push_back(agents[i]->get_agent());
        rvo.buildAgentTree(raw_agents);
    }

    regenerate_polygons = false;
    regenerate_links = false;
    agents_dirty = false;
}

void NavMap::compute_single_step(uint32_t _index, RvoAgent **agent) {
    (*agent)->get_agent()->computeNeighbors(&rvo);
    (*agent)->get_agent()->computeNewVelocity(deltatime);
}

void NavMap::clip_path(const std::vector<NavigationPoly> &p_navigation_polys, Vector<Vector3> &path, const NavigationPoly *from_poly, const Vector3 &p_to_point, const NavigationPoly *p_to_poly) const {
    Vector3 from = path[path.size() - 1];

    if (from.distance_to(p_to_point) < CMP_EPSILON)
        return;
    Plane cut_plane;
    cut_plane.normal = (from - p_to_point).cross(up);
    if (cut_plane.normal == Vector3())
        return;
    cut_plane.normal.normalize();
    cut_plane.d = cut_plane.normal.dot(from);

    while (from_poly != p_to_poly) {

        int back_nav_edge = from_poly->back_navigation_edge;
        Vector3 a = from_poly->poly->points[back_nav_edge].pos;
        Vector3 b = from_poly->poly->points[(back_nav_edge + 1) % from_poly->poly->points.size()].pos;

        ERR_FAIL_COND(from_poly->prev_navigation_poly_id == -1);
        from_poly = &p_navigation_polys[from_poly->prev_navigation_poly_id];

        if (a.distance_to(b) > CMP_EPSILON) {

            Vector3 inters;
            if (cut_plane.intersects_segment(a, b, &inters)) {
                if (inters.distance_to(p_to_point) > CMP_EPSILON && inters.distance_to(path[path.size() - 1]) > CMP_EPSILON) {
                    path.push_back(inters);
                }
            }
        }
    }
}

void NavMap::step(real_t p_deltatime) {
    deltatime = p_deltatime;
    // TODO reactive this ?
    //thread_process_array(
    //        controlled_agents.size(),
    //        this,
    //        &NavMap::compute_single_step,
    //        controlled_agents.data());
}

void NavMap::dispatch_callbacks() {
    for (int i(0); i < static_cast<int>(controlled_agents.size()); i++) {
        controlled_agents[i]->dispatch_callback();
    }
}
