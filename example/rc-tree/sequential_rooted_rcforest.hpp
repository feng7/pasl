#ifndef __SEQUENTIAL_ROOTED_RCFOREST_HPP
#define __SEQUENTIAL_ROOTED_RCFOREST_HPP

#if __cplusplus < 201103L
#   error "This header requires C++11"
#endif

#include <chrono>
#include <random>
#include <unordered_set>
#include <vector>

#include "rooted_rcforest.hpp"
#include "dc/dynamic_connectivity.hpp"

// A sequential implementation of the rooted RC-forest.
template<
    typename e_info_t,
    typename v_info_t,
    typename connectivity_t = link_cut_tree,         // use "dummy_checker" to turn off control and make it faster
    typename e_monoid_trait = monoid_plus<e_info_t>, // may be non-commutative
    typename v_monoid_trait = monoid_plus<v_info_t>, // should be commutative
    typename random_t       = std::default_random_engine
> class sequential_rooted_rcforest
    : public rooted_rcforest<e_info_t, v_info_t, e_monoid_trait, v_monoid_trait>
{
    // Members
    private:
        enum class contract_t : int { root, rake, compress };

        struct vertex_t {
            int         parent;
            int         children_count;
            int         children_sum;
            v_info_t    v_info;
            e_info_t    e_info_up;
            e_info_t    e_info_down;
        };

        struct vertex_col_t {
            // Main RC data for a vertex's contraction history
            vertex_t                changes;
            std::vector<vertex_t>   vertices;
            int                     children_count;
            int                     scheduled_children_count;
            contract_t              contraction;

            // The machinery for ternarization and multiple children handling
            int my_index;
            int left_index;
            int right_index;
            int scheduled_left_index;
            int scheduled_right_index;
            int heap_key;

            void prepare_for_changes() {
                changes = vertices[0];
                scheduled_children_count = children_count;
                scheduled_left_index = left_index;
                scheduled_right_index = right_index;
            }

            vertex_t const &first() const {
                return vertices[0];
            }
        };

        e_monoid_trait  e_ops;
        v_monoid_trait  v_ops;

        int  edge_count;
        int  scheduled_edge_count;
        bool has_scheduled;

        std::vector<vertex_col_t> vertices;
        std::unordered_set<int>   changed_vertices;

        random_t rng;

        connectivity_t conn_checker;

    // Constructors
    public:
        sequential_rooted_rcforest(
            unsigned seed = (unsigned) (std::chrono::system_clock::now().time_since_epoch().count()),
            e_monoid_trait const &e_ops = e_monoid_trait(),
            v_monoid_trait const &v_ops = v_monoid_trait()
        ) : e_ops(e_ops)
          , v_ops(v_ops)
          , edge_count(0)
          , scheduled_edge_count(0)
          , has_scheduled(false)
          , vertices()
          , changed_vertices()
          , rng(seed)
          , conn_checker()
        {}

        sequential_rooted_rcforest(
            sequential_rooted_rcforest const &src
        ) : e_ops(src.e_ops)
          , v_ops(src.v_ops)
          , edge_count(src.edge_count)
          , scheduled_edge_count(src.scheduled_edge_count)
          , has_scheduled(src.has_scheduled)
          , vertices(src.vertices)
          , changed_vertices(src.changed_vertices)
          , rng(src.rng)
          , conn_checker(src.conn_checker)
        {}

        sequential_rooted_rcforest &operator = (
            sequential_rooted_rcforest const &src
        ) {
            e_ops                = src.e_ops;
            v_ops                = src.v_ops;
            edge_count           = src.edge_count;
            scheduled_edge_count = src.scheduled_edge_count;
            has_scheduled        = src.has_scheduled;
            vertices             = src.vertices;
            changed_vertices     = src.changed_vertices;
            rng                  = src.rng;
            conn_checker         = src.conn_checker;
            return *this;
        }

        sequential_rooted_rcforest &operator = (
            sequential_rooted_rcforest &&src
        ) {
            e_ops                = src.e_ops;
            v_ops                = src.v_ops;
            edge_count           = src.edge_count;
            scheduled_edge_count = src.scheduled_edge_count;
            has_scheduled        = src.has_scheduled;
            vertices             = src.vertices;
            changed_vertices     = src.changed_vertices;
            rng                  = src.rng;
            conn_checker         = src.conn_checker;
            return *this;
        }

    // Access methods
    public:
        // Returns the number of vertices.
        virtual int n_vertices() const {
            return vertices.size();
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
            return vertices.at(2 * vertex).children_count;
        }

        // Returns the parent of the given vertex.
        virtual int get_parent(int vertex) const {
            return vertices.at(2 * vertex + 1).first().parent / 2;
        }

        // Returns whether the given vertex is root.
        virtual bool is_root(int vertex) const {
            return get_parent(vertex) == vertex;
        }

        // Returns the vertex info for the given vertex.
        virtual v_info_t get_vertex_info(int vertex) const {
            return vertices.at(2 * vertex).first().v_info;
        }

        // Returns the upwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_upwards(int vertex) const {
            if (is_root(vertex)) {
                throw std::invalid_argument("[sequential_rooted_rcforest::get_edge_info_upwards]: The vertex is a root!");
            }
            return vertices.at(2 * vertex).first().e_info_up;
        }

        // Returns the downwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_downwards(int vertex) const {
            if (is_root(vertex)) {
                throw std::invalid_argument("[sequential_rooted_rcforest::get_edge_info_downwards]: The vertex is a root!");
            }
            return vertices.at(2 * vertex).first().e_info_down;
        }

    // Query methods
    public:
        // Returns the root of the tree the given vertex belongs to.
        virtual int get_root(int vertex) const = 0;

        // Returns the monoid sum for the path from the first vertex to the last one.
        virtual e_info_t get_path(int v_first, int v_last) const = 0;

        // Returns the monoid sum for the vertices in the subtree of the given vertex.
        virtual v_info_t get_subtree(int vertex) const = 0;

    // Non-scheduled modification methods
    public:
        // Creates a new vertex and returns its index (equal to n_vertices() before the call).
        virtual int create_vertex(v_info_t const &vertex_info) {
            int data_index = (int) (vertices.size());
            int link_index = data_index + 1;
            vertex_t data_vertex, link_vertex, rake_vertex;

            // We create two actual-tree vertices: the data vertex and the link vertex.
            // They will be raked, it is O(1), so we just do it now.

            data_vertex.parent = link_index;
            data_vertex.children_count = 0;
            data_vertex.children_sum = 0;
            data_vertex.v_info = vertex_info;
            data_vertex.e_info_up = e_ops.neutral();
            data_vertex.e_info_down = e_ops.neutral();

            link_vertex.parent = link_index;
            link_vertex.children_count = 1;
            link_vertex.children_sum = data_index;
            link_vertex.v_info = v_ops.neutral();
            link_vertex.e_info_up = e_ops.neutral();
            link_vertex.e_info_down = e_ops.neutral();

            rake_vertex.parent = link_index;
            rake_vertex.children_count = 0;
            rake_vertex.children_sum = 0;
            rake_vertex.v_info = vertex_info;
            rake_vertex.e_info_up = e_ops.neutral();
            rake_vertex.e_info_down = e_ops.neutral();

            vertices.push_back(vertex_col_t());
            vertices.push_back(vertex_col_t());

            vertex_col_t &data_col = vertices[data_index];
            vertex_col_t &link_col = vertices[link_index];

            data_col.changes = data_vertex;
            data_col.vertices.push_back(data_vertex);
            data_col.children_count = 0;
            data_col.scheduled_children_count = 0;
            data_col.contraction = contract_t::rake;
            data_col.my_index = data_index;
            data_col.left_index = -1;  // these indices are from the tree
            data_col.right_index = -1; // of current vertex's children
            data_col.scheduled_left_index = -1;
            data_col.scheduled_right_index = -1;
            data_col.heap_key = -1; // heap key of data colums should be the minimum possible

            link_col.changes = link_vertex;
            link_col.vertices.push_back(link_vertex);
            link_col.vertices.push_back(rake_vertex);
            link_col.children_count = 0;
            link_col.scheduled_children_count = 0;
            link_col.contraction = contract_t::root;
            link_col.my_index = link_index;
            link_col.left_index = -1;  // these indices are from the tree
            link_col.right_index = -1; // of current vertex's siblings
            link_col.scheduled_left_index = -1;
            link_col.scheduled_right_index = -1;
            link_col.heap_key = (int) (rng() >> 1); // a definitely positive random heap key

            return data_index / 2;
        }

    // Scheduled modification helpers
    private:
        void ensure_has_scheduled() {
            if (!has_scheduled) {
                has_scheduled = true;
                scheduled_edge_count = edge_count;
                scheduled_conn_checker = conn_checker;
            }
        }

        void ensure_internal_vertex_is_changed(int vertex) {
            if (changed_vertices.count(vertex) == 0) {
                changed_vertices.insert(vertex);
                vertices[vertex].prepare_for_changes();
            }
        }

        void ensure_vertex_is_changed(int vertex) {
            ensure_internal_vertex_is_changed(2 * vertex);
            ensure_internal_vertex_is_changed(2 * vertex + 1);
        }

        int cartesian_insert(int tree, int vertex) {
            if (tree == -1) {
                return vertex;
            }
            vertex_col_t &col_tree = vertices[tree];
            vertex_col_t &col_vertex = vertices[vertex];
            if (col_vertex.heap_key < col_tree.heap_key) {
                if (col_vertex.my_index < col_tree.my_index) {
                    col_vertex.scheduled_right = tree;
                } else {
                    col_vertex.scheduled_left = tree;
                }
                col_tree.changes.parent = vertex;
                ensure_internal_vertex_is_changed(tree);
                ensure_internal_vertex_is_changed(vertex);
                return vertex;
            } else {
                if (col_vertex.my_index < col_tree.my_index) {
                    col_tree.scheduled_left = cartesian_insert(col_tree.scheduled_left, vertex);
                    vertices[col_tree.scheduled_left].changes.parent = tree;
                    // No need to mark it changed: if it is changed, it has already been marked.
                } else {
                    col_tree.scheduled_right = cartesian_insert(col_tree.scheduled_right, vertex);
                    vertices[col_tree.scheduled_right].changes.parent = tree;
                    // No need to mark it changed: if it is changed, it has already been marked.
                }
                return tree;
            }
        }

        int cartesian_merge(int left, int right) {
            if (left == -1 || right == -1) {
                return left + right + 1;
            } else {
                vertex_col_t &l = vertices[left];
                vertex_col_t &r = vertices[right];
                if (l.heap_key < r.heap_key) {
                    l.scheduled_right = cartesian_merge(l.scheduled_right, r);
                    vertices[l.scheduled_right].changes.parent = r.my_index;
                    ensure_internal_vertex_is_changed(right);
                    ensure_internal_vertex_is_changed(l.scheduled_right);
                    return left;
                } else {
                    r.scheduled_left = cartesian_merge(l, r.scheduled_left);
                    vertices[r.scheduled_left].changes.parent = l.my_index;
                    ensure_internal_vertex_is_changed(left);
                    ensure_internal_vertex_is_changed(r.scheduled_left);
                    return right;
                }
            }
        }

        int cartesian_delete(int tree, int vertex) {
            if (tree == -1) {
                throw std::logic_error("[cartesian_delete] tree == -1");
            }
            vertex_col_t &col_vertex = vertices[vertex];
            if (tree == vertex) {
                int rv = cartesian_merge(col_vertex.scheduled_left, col_vertex.scheduled_right);
                col_vertex.scheduled_left = -1;
                col_vertex.scheduled_right = -1;
                col_vertex.changes.parent = -1;
                ensure_internal_vertex_is_changed(vertex);
                return rv;
            } else {
                vertex_col_t &col_tree = vertices[tree];
                if (col_vertex.my_index < col_tree.my_index) {
                    col_tree.scheduled_left = cartesian_delete(col_tree.scheduled_left, vertex);
                    vertices[col_tree.scheduled_left].changes.parent = tree;
                    // No need to mark it changed: if it is changed, it has already been marked.
                } else {
                    col_tree.scheduled_right = cartesian_delete(col_tree.scheduled_right, vertex);
                    vertices[col_tree.scheduled_right].changes.parent = tree;
                    // No need to mark it changed: if it is changed, it has already been marked.
                }
                return tree;
            }
        }

    // Scheduled modification methods.
    public:
        // Tests if a vertex has changed.
        virtual bool scheduled_is_changed(int vertex) const {
            return changed_vertices.count(2 * vertex) != 0;
        }

        // Returns the parent of the given vertex after all scheduled changes are applied.
        virtual int scheduled_get_parent(int vertex) const {
            return scheduled_is_changed(vertex)
                ? vertices.at(2 * vertex + 1).changes.parent
                : get_parent(vertex);
        }

        // Checks if the vertex is a root after all scheduled changes are applied.
        virtual bool scheduled_is_root(int vertex) const {
            return scheduled_get_parent(vertex) == vertex;
        }

        // Returns the number of edges after all scheduled changes are applied.
        virtual int scheduled_n_edges() const {
            return scheduled_edge_count;
        }

        // Returns the number of roots after all scheduled changes are applied.
        virtual int scheduled_n_roots() const {
            return n_vertices() - scheduled_n_edges();
        }

        // Returns the number of children of the given vertex after all scheduled changes are applied.
        virtual int scheduled_n_children(int vertex) const {
            return scheduled_is_changed(vertex)
                ? vertices.at(2 * vertex).scheduled_children_count
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
            vertices[2 * vertex].changes.v_info = vertex_info;
        }

        // Schedules (adds to the end of the current changelist)
        // the change of edge information to the edge
        // going from the given vertex to its parent.
        virtual void scheduled_set_edge_info(int vertex, e_info_t const &edge_upwards, e_info_t const &edge_downwards) {
            if (scheduled_is_root(vertex)) {
                throw std::invalid_argument("[sequential_rooted_rcforest::scheduled_set_edge_info] The vertex is a root!");
            }
            ensure_has_scheduled();
            ensure_vertex_is_changed(vertex);
            vertices[2 * vertex].changes.e_info_up = edge_upwards;
            vertices[2 * vertex + 1].changes.e_info_down = edge_downwards;
        }

        // Schedules (adds to the end of the current changelist)
        // the given vertex to be detached from its parent.
        virtual void scheduled_detach(int vertex) {
            if (scheduled_is_root(vertex)) {
                throw std::invalid_argument("[sequential_rooted_rcforest::scheduled_set_edge_info] The vertex is already a root!");
            }
            ensure_has_scheduled();
            ensure_vertex_is_changed(vertex);
            int parent = scheduled_get_parent(vertex);
            ensure_vertex_is_changed(parent);

            cartesian_delete(2 * parent, 2 * vertex + 1);
            scheduled_conn_checker.cut(parent, vertex);

            --vertices[2 * parent].scheduled_children_count;
            --scheduled_edge_count;
        }

        // Schedules (adds to the end of the current changelist)
        // the given child vertex to be attached to the given parent vertex.
        virtual void scheduled_attach(int v_parent, int v_child, e_info_t const &edge_upwards, e_info_t const &edge_downwards) {
            if (!scheduled_is_root(v_child)) {
                throw std::invalid_argument("[sequential_rooted_rcforest::scheduled_attach] The child vertex is not a root!");
            }

            if (scheduled_conn_checker.test_connectivity(v_parent, v_child)) {
                throw std::invalid_argument("[sequential_rooted_rcforest::scheduled_attach] The parent and the child are already connected!");
            }

            ensure_has_scheduled();
            ensure_vertex_is_changed(v_parent);
            ensure_vertex_is_changed(v_child);

            vertex_col_t col_child_data = vertices[2 * v_child];
            vertex_col_t col_child_link = vertices[2 * v_child + 1];

            col_child_data.changes.e_info_up = edge_upwards;
            col_child_data.changes.e_info_down = edge_downwards;

            cartesian_insert(2 * v_parent, 2 * v_child + 1);
            scheduled_conn_checker.link(v_parent, v_child);

            ++vertices[2 * v_parent].scheduled_children_count;
            ++scheduled_edge_count;
        }

        // Applies all pending changes.
        virtual void scheduled_apply() {
            // TODO: the main stuff
            edge_count = scheduled_edge_count;
            conn_checker.flush();
            has_scheduled = false;
        }

        // Cancels all pending changes.
        virtual void scheduled_cancel() {
            scheduled_edge_count = edge_count;
            conn_checker.unroll();
            has_scheduled = false;
            changed_vertices.clear();
        }

    // A necessary virtual destructor
    public:
        virtual ~sequential_rooted_rcforest() {}
};

#endif
