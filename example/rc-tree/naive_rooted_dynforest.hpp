#ifndef __NAIVE_ROOTED_DYNFOREST_HPP
#define __NAIVE_ROOTED_DYNFOREST_HPP

#if __cplusplus < 201103L
#   error "This header requires C++11"
#endif

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "rooted_dynforest.hpp"

// A naive implementation of the rooted dynamic tree
template<
    typename e_info_t,
    typename v_info_t,
    typename e_monoid_trait = monoid_plus<e_info_t>, // may be non-commutative
    typename v_monoid_trait = monoid_plus<v_info_t>  // should be commutative
> class naive_rooted_dynforest : public rooted_dynforest<e_info_t, v_info_t, e_monoid_trait, v_monoid_trait> {
    // Members
    private:
        struct vertex_t {
            int              parent;
            std::vector<int> children;
            v_info_t         v_info;
            e_info_t         e_info_up;
            e_info_t         e_info_down;

            int              scheduled_parent;
            std::vector<int> scheduled_children;
            v_info_t         scheduled_v_info;
            e_info_t         scheduled_e_info_up;
            e_info_t         scheduled_e_info_down;

            bool             changed;
        };

        std::vector<vertex_t> vertices;

        int  edge_count;
        int  scheduled_edge_count;
        bool has_scheduled;

        e_monoid_trait  e_ops;
        v_monoid_trait  v_ops;

    // Constructors
    public:
        naive_rooted_dynforest(
            e_monoid_trait const &e_ops = e_monoid_trait(),
            v_monoid_trait const &v_ops = v_monoid_trait()
        ) : vertices()
          , edge_count(0)
          , scheduled_edge_count(0)
          , has_scheduled(false)
          , e_ops(e_ops)
          , v_ops(v_ops)
        {}

        naive_rooted_dynforest(
            naive_rooted_dynforest const &src
        ) : vertices(src.vertices)
          , edge_count(src.edge_count)
          , scheduled_edge_count(src.scheduled_edge_count)
          , has_scheduled(false)
          , e_ops(src.e_ops)
          , v_ops(src.v_ops)
        {}

        naive_rooted_dynforest &operator = (
            naive_rooted_dynforest const &src
        ) {
            vertices             = src.vertices;
            edge_count           = src.edge_count;
            scheduled_edge_count = src.scheduled_edge_count;
            has_scheduled        = src.has_scheduled;
            e_ops                = src.e_ops;
            v_ops                = src.v_ops;
            return *this;
        }

    // Access methods
    public:
        // Returns the number of vertices.
        virtual int n_vertices() const {
            return (int) (vertices.size());
        }

        // Returns the number of edges.
        virtual int n_edges() const {
            return edge_count;
        }

        // Returns the number of roots.
        virtual int n_roots() const {
            return n_vertices() - n_edges();
        }

        // Returns the number of children of the given vertex.
        virtual int n_children(int vertex) const {
            return (int) vertices.at(vertex).children.size();
        }

        // Returns the parent of the given vertex.
        virtual int get_parent(int vertex) const {
            return vertices.at(vertex).parent;
        }

        // Returns whether the given vertex is root.
        virtual bool is_root(int vertex) const {
            return vertices.at(vertex).parent == vertex;
        }

        // Returns the vertex info for the given vertex.
        virtual v_info_t get_vertex_info(int vertex) const {
            return vertices.at(vertex).v_info;
        }

        // Returns the upwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_upwards(int vertex) const {
            if (is_root(vertex)) {
                throw std::invalid_argument("[naive_rooted_dynforest::get_edge_info_upwards]: The vertex is a root!");
            }
            return vertices.at(vertex).e_info_up;
        }

        // Returns the downwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_downwards(int vertex) const {
            if (is_root(vertex)) {
                throw std::invalid_argument("[naive_rooted_dynforest::get_edge_info_downwards]: The vertex is a root!");
            }
            return vertices.at(vertex).e_info_down;
        }

    // Query methods
    public:
        // Returns the root of the tree the given vertex belongs to.
        virtual int get_root(int vertex) const {
            while (!is_root(vertex)) {
                vertex = get_parent(vertex);
            }
            return vertex;
        }

        // Returns the monoid sum for the path from the first vertex to the last one.
        virtual e_info_t get_path(int v_first, int v_last) const {
            if (get_root(v_first) != get_root(v_last)) {
                throw std::logic_error("[naive_rooted_dynforest::get_path] Vertices are not connected!");
            }

            e_info_t downwards_part = e_ops.neutral();
            e_info_t upwards_part = e_ops.neutral();

            // r_first should be the distance from v_first to its parent
            int r_first = 0;
            for (int v_copy = v_first; !is_root(v_copy); ++r_first, v_copy = get_parent(v_copy));

            // r_last should be the distance from v_last to its parent
            int r_last = 0;
            for (int v_copy = v_last;  !is_root(v_copy); ++r_last,  v_copy = get_parent(v_copy));

            // First, we make distances from v_first and from v_last to their parent equal
            // Not r_first > 0 because we don't necessary go all the way to the root
            while (r_first > r_last) {
                upwards_part = e_ops.sum(upwards_part, get_edge_info_upwards(v_first));
                v_first = get_parent(v_first);
                --r_first;
            }
            // Not r_last > 0 because we don't necessary go all the way to the root
            while (r_last > r_first) {
                downwards_part = e_ops.sum(get_edge_info_downwards(v_last), downwards_part);
                v_last = get_parent(v_last);
                --r_last;
            }
            // Then we go upwards until the vertices meet
            while (v_first != v_last) {
                upwards_part = e_ops.sum(upwards_part, get_edge_info_upwards(v_first));
                v_first = get_parent(v_first);
                downwards_part = e_ops.sum(get_edge_info_downwards(v_last), downwards_part);
                v_last = get_parent(v_last);
            }

            return e_ops.sum(upwards_part, downwards_part);
        }

        // Returns the monoid sum for the vertices in the subtree of the given vertex.
        virtual v_info_t get_subtree(int vertex) const {
            v_info_t rv = get_vertex_info(vertex);
            for (int i = 0, i_max = n_children(vertex); i < i_max; ++i) {
                rv = v_ops.sum(rv, get_subtree(vertices.at(vertex).children.at(i)));
            }
            return rv;
        }

    // Non-scheduled modification methods
    public:
        // Creates a new vertex and returns its index (equal to n_vertices() before the call).
        virtual int create_vertex(v_info_t const &vertex_info) {
            int index = (int) vertices.size();
            vertex_t back;

            back.parent = index;
            back.v_info = vertex_info;
            back.e_info_up = e_ops.neutral();
            back.e_info_down = e_ops.neutral();
            back.scheduled_parent = index;
            back.scheduled_v_info = vertex_info;
            back.scheduled_e_info_up = e_ops.neutral();
            back.scheduled_e_info_down = e_ops.neutral();
            back.changed = false;

            vertices.push_back(back);
            return index;
        }

    // Scheduled modification helpers.
    private:
        void ensure_has_scheduled() {
            if (!has_scheduled) {
                has_scheduled = true;
            }
        }

        void ensure_vertex_is_changed(int vertex) {
            vertex_t &vx = vertices.at(vertex);
            if (!vx.changed) {
                vx.changed = true;
                vx.scheduled_parent      = vx.parent;
                vx.scheduled_children    = vx.children;
                vx.scheduled_v_info      = vx.v_info;
                vx.scheduled_e_info_up   = vx.e_info_up;
                vx.scheduled_e_info_down = vx.e_info_down;
            }
        }

    // Scheduled modification methods.
    public:
        // Tests if a vertex has changed.
        virtual bool scheduled_is_changed(int vertex) const {
            return vertices.at(vertex).changed;
        }

        // Returns the parent of the given vertex after all scheduled changes are applied.
        virtual int scheduled_get_parent(int vertex) const {
            return scheduled_is_changed(vertex)
                ? vertices.at(vertex).scheduled_parent
                : get_parent(vertex);
        }

        // Checks if the vertex is a root after all scheduled changes are applied.
        virtual bool scheduled_is_root(int vertex) const {
            return scheduled_is_changed(vertex)
                ? vertices.at(vertex).scheduled_parent == vertex
                : is_root(vertex);
        }

        // Returns the number of edges after all scheduled changes are applied.
        virtual int scheduled_n_edges() const {
            return scheduled_has_changes() ? scheduled_edge_count : edge_count;
        }

        // Returns the number of roots after all scheduled changes are applied.
        virtual int scheduled_n_roots() const {
            return n_vertices() - scheduled_n_edges();
        }

        // Returns the number of children of the given vertex after all scheduled changes are applied.
        virtual int scheduled_n_children(int vertex) const {
            return scheduled_is_changed(vertex)
                ? (int) vertices.at(vertex).scheduled_children.size()
                : n_children(vertex);
        }

        // Checks if there are pending changes.
        virtual bool scheduled_has_changes() const {
            return has_scheduled;
        }

        // Schedules (adds to the end of the current changelist)
        // the change of vertex information to the given vertex.
        virtual void scheduled_set_vertex_info(int vertex, v_info_t const &vertex_info) {
            ensure_has_scheduled();
            ensure_vertex_is_changed(vertex);
            vertices.at(vertex).scheduled_v_info = vertex_info;
        }

        // Schedules (adds to the end of the current changelist)
        // the change of edge information to the edge
        // going from the given vertex to its parent.
        virtual void scheduled_set_edge_info(int vertex, e_info_t const &edge_upwards, e_info_t const &edge_downwards) {
            if (scheduled_is_root(vertex)) {
                throw std::invalid_argument("[naive_rooted_dynforest::scheduled_set_edge_info] The vertex is a root!");
            }
            ensure_has_scheduled();
            ensure_vertex_is_changed(vertex);
            vertices.at(vertex).scheduled_e_info_up   = edge_upwards;
            vertices.at(vertex).scheduled_e_info_down = edge_downwards;
        }

        // Schedules (adds to the end of the current changelist)
        // the given vertex to be detached from its parent.
        virtual void scheduled_detach(int vertex) {
            if (scheduled_is_root(vertex)) {
                throw std::invalid_argument("[naive_rooted_dynforest::scheduled_detach] The vertex is already a root!");
            }
            ensure_has_scheduled();
            ensure_vertex_is_changed(vertex);
            int parent = scheduled_get_parent(vertex);
            ensure_vertex_is_changed(parent);

            std::vector<int> &child_list = vertices.at(parent).scheduled_children;
            child_list.erase(std::find(child_list.begin(), child_list.end(), vertex));
            vertices.at(vertex).scheduled_parent = vertex;

            --scheduled_edge_count;
        }

        // Schedules (adds to the end of the current changelist)
        // the given child vertex to be attached to the given parent vertex.
        virtual void scheduled_attach(int v_parent, int v_child, e_info_t const &edge_upwards, e_info_t const &edge_downwards) {
            if (!scheduled_is_root(v_child)) {
                throw std::invalid_argument("[naive_rooted_dynforest::scheduled_attach] The child vertex is not a root!");
            }
            for (int vp = v_parent; ; vp = scheduled_get_parent(vp)) {
                if (vp == v_child) {
                    throw std::invalid_argument("[naive_rooted_dynforest::scheduled_attach] The connection will make a loop!");
                }
                if (scheduled_is_root(vp)) {
                    break;
                }
            }
            ensure_has_scheduled();
            ensure_vertex_is_changed(v_parent);
            ensure_vertex_is_changed(v_child);
            vertex_t &chv = vertices.at(v_child);
            chv.scheduled_parent = v_parent;
            chv.scheduled_e_info_up = edge_upwards;
            chv.scheduled_e_info_down = edge_downwards;
            vertices.at(v_parent).scheduled_children.push_back(v_child);

            ++scheduled_edge_count;
        }

        // Applies all pending changes.
        virtual void scheduled_apply() {
            for (int i = 0, i_max = n_vertices(); i < i_max; ++i) {
                vertex_t &vx = vertices.at(i);
                if (vx.changed) {
                    vx.parent      = vx.scheduled_parent;
                    vx.children    = vx.scheduled_children;
                    vx.v_info      = vx.scheduled_v_info;
                    vx.e_info_up   = vx.scheduled_e_info_up;
                    vx.e_info_down = vx.scheduled_e_info_down;
                    vx.changed     = false;
                }
            }
            edge_count = scheduled_edge_count;
            has_scheduled = false;
        }

        // Cancels all pending changes.
        virtual void scheduled_cancel() {
            for (int i = 0, i_max = n_vertices(); i < i_max; ++i) {
                vertices.at(i).changed = false;
            }
            scheduled_edge_count = edge_count;
            has_scheduled = false;
        }

    // A necessary virtual destructor
    public:
        virtual ~naive_rooted_dynforest() {}
};

#endif
