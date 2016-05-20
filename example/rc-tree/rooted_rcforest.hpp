#ifndef __ROOTED_RCFOREST_HPP
#define __ROOTED_RCFOREST_HPP

#if __cplusplus < 201103L
#   error "This header requires C++11"
#endif

#include <atomic>
#include <algorithm>
#include <vector>

#include "rooted_dynforest.hpp"

#include "dynamic_connectivity.hpp"
#include "thread_local_random.hpp"

// An implementation of the rooted dynamic forest based on RC-trees.
template<
    typename e_info_t,
    typename v_info_t,
    typename e_monoid_trait = monoid_plus<e_info_t>, // may be non-commutative
    typename v_monoid_trait = monoid_plus<v_info_t>, // should be commutative
    typename connectivity_t = dummy_checker,         // use "link_cut_tree" to turn on loop control
    bool     has_debug_contraction = false           // use "true" to check whether non-affected vertices are not affected
> class rooted_rcforest
    : public rooted_dynforest<e_info_t, v_info_t, e_monoid_trait, v_monoid_trait>
{
    // Members
    private:
        enum class contract_t : int { root, rake, compress };

        static constexpr int bits_in_unsigned = 8 * sizeof(unsigned);

        struct vertex_t {
            int         parent;
            int         children_count;
            int         children[3];
            v_info_t    v_info;
            e_info_t    e_info_up;
            e_info_t    e_info_down;

            vertex_t(
                v_info_t const &v_info,
                e_info_t const &e_info_up,
                e_info_t const &e_info_down
            ) : parent(-1)
              , children_count(0)
              , v_info(v_info)
              , e_info_up(e_info_up)
              , e_info_down(e_info_down)
            {
                children[0] = -1;
                children[1] = -1;
                children[2] = -1;
            }

            bool operator == (vertex_t const &that) const {
                return parent == that.parent
                    && children_count == that.children_count
                    && children[0] == that.children[0]
                    && children[1] == that.children[1]
                    && children[2] == that.children[2]
                    && v_info == that.v_info
                    && e_info_up == that.e_info_up
                    && e_info_down == that.e_info_down;
            }

            bool operator != (vertex_t const &that) const {
                return !(*this == that);
            }

            void insert_child(int child) {
                if (children_count == 3) {
                    throw std::logic_error("[vertex_t::insert_child] All children slots are busy!");
                }
                int cp = children_count++;
                children[cp] = child;
                while (cp > 0 && children[cp - 1] >= children[cp]) {
                    if (children[cp - 1] == children[cp]) {
                        throw std::logic_error("[vertex_t::insert_child] Repeated insertion of the child!");
                    }
                    std::swap(children[cp - 1], children[cp]);
                    --cp;
                }
            }

            void remove_child(int child) {
                for (int i = 0; i < children_count; ++i) {
                    if (children[i] == child) {
                        // gcc 4.9.3 has false? positive without   vvvvvvvvv
                        for (int j = i; j + 1 < children_count && j + 1 < 3; ++j) {
                        // ... for this line (array subscript out of bounds)
                            children[j] = children[j + 1];
                        }
                        children[--children_count] = -1;
                        return;
                    }
                }
                throw std::logic_error("[vertex_t::insert_child] No such child!");
            }
        };

        // The private struct - don't need to test the public methods!
        struct vertex_col_t {
        private:
            // Main RC data for a vertex's contraction history
            std::vector<vertex_t>   odd_levels;  // odd-even separation as a premature optimization
            std::vector<vertex_t>   even_levels; // for the parallel version
        public:
            // The information about when and how this vertex disappears
            int last_live_level;
            contract_t contraction;

            // Children counts in the represented tree
            int children_count;
            int scheduled_children_count;

            // The machinery for Cartesian trees
            // which store multiple children from the represented tree
            int my_index;
            int left_index;
            int right_index;
            int scheduled_left_index;
            int scheduled_right_index;
            int heap_key;

            // Random bits for determining whether to compress
            std::vector<unsigned> random_bits;

            // A place for storing vertices affected by this vertex's change
            // Maximum possible: me + parent + parent's parent + 3 children = 6
            int next_affected[6];
            int next_affected_count;
            int next_affected_check_parent;
            int next_affected_prefix_sum;

            bool is_changed;

            bool get_random_bit(int level) {
                int v_index = level / bits_in_unsigned;
                int b_index = level % bits_in_unsigned;
                while ((int) random_bits.size() <= v_index) {
                    random_bits.push_back((unsigned) global_rng());
                }
                return ((random_bits[v_index] >> b_index) & 1) == 1;
            }

            void push_level(vertex_t const &vertex) {
                int level = ++last_live_level;
                std::vector<vertex_t> &pool = (level & 1) == 1 ? odd_levels : even_levels;
                int real_level = level / 2;
                if ((int) pool.size() > real_level) {
                    pool[real_level] = vertex;
                } else if ((int) pool.size() == real_level) {
                    pool.push_back(vertex);
                } else {
                    throw std::logic_error("[vertex_col_t::push_level] some live levels don't exist physically");
                }
            }

            vertex_t &at_level(int level) {
                if (has_debug_contraction) {
                    if (level > last_live_level) {
                        throw std::logic_error("[vertex_col_t::at_level]: nonexistent (logically) level asked");
                    }
                    std::vector<vertex_t> &pool = (level & 1) == 1 ? odd_levels : even_levels;
                    int real_level = level / 2;
                    if (real_level >= (int) pool.size()) {
                        throw std::logic_error("[vertex_col_t::at_level]: nonexistent (physically) level asked");
                    }
                    return pool[real_level];
                } else {
                    return ((level & 1) == 1 ? odd_levels : even_levels)[level / 2];
                }
            }

            vertex_t const &at_level(int level) const {
                if (has_debug_contraction) {
                    if (level > last_live_level) {
                        throw std::logic_error("[vertex_col_t::at_level const]: nonexistent (logically) level asked");
                    }
                    std::vector<vertex_t> const &pool = (level & 1) == 1 ? odd_levels : even_levels;
                    int real_level = level / 2;
                    if (real_level >= (int) pool.size()) {
                        throw std::logic_error("[vertex_col_t::at_level const]: nonexistent (physically) level asked");
                    }
                    return pool[real_level];
                } else {
                    return ((level & 1) == 1 ? odd_levels : even_levels)[level / 2];
                }
            }
        };

        struct cache_line_int_t {
            int data;
        private:
            int padding;
        };

        class atomic_flag_vector {
            std::atomic_flag *data;
            unsigned count, capacity;

            void reset() {
                for (unsigned i = 0; i < capacity; ++i) {
                    data[i].clear();
                }
            }
        public:
            atomic_flag_vector()
                : data(new std::atomic_flag[10])
                , count(0)
                , capacity(10)
            {
                reset();
            }

            atomic_flag_vector(atomic_flag_vector const &that)
                : data(new std::atomic_flag[that.capacity])
                , count(that.count)
                , capacity(that.capacity)
            {
                reset();
            }

            atomic_flag_vector &operator = (atomic_flag_vector const &that) {
                if (capacity < that.capacity) {
                    delete[] data;
                    capacity = that.capacity;
                    data = new std::atomic_flag[capacity];
                    reset();
                }
                count = that.count;
                return *this;
            }

            std::atomic_flag &operator[] (unsigned index) {
                return data[index];
            }

            unsigned size() const {
                return count;
            }

            void insert_element() {
                if (count == capacity) {
                    delete[] data;
                    capacity *= 2;
                    data = new std::atomic_flag[capacity];
                    reset();
                }
                ++count;
            }

            ~atomic_flag_vector() {
                delete[] data;
            }
        };

        e_monoid_trait  e_ops;
        v_monoid_trait  v_ops;

        int  edge_count;
        int  scheduled_edge_count;
        bool has_scheduled;

        std::vector<vertex_col_t> vertices;

        connectivity_t conn_checker;

        atomic_flag_vector atomic_flags;
        std::vector<cache_line_int_t> curr_modified;
        std::vector<cache_line_int_t> next_modified;
        unsigned n_modified;

    // Constructors
    public:
        rooted_rcforest(
            e_monoid_trait const &e_ops = e_monoid_trait(),
            v_monoid_trait const &v_ops = v_monoid_trait()
        ) : e_ops(e_ops)
          , v_ops(v_ops)
          , edge_count(0)
          , scheduled_edge_count(0)
          , has_scheduled(false)
          , vertices()
          , conn_checker()
          , atomic_flags()
          , curr_modified()
          , next_modified()
          , n_modified(0)
        {}

        rooted_rcforest(
            rooted_rcforest const &src
        ) : e_ops(src.e_ops)
          , v_ops(src.v_ops)
          , edge_count(src.edge_count)
          , scheduled_edge_count(src.scheduled_edge_count)
          , has_scheduled(src.has_scheduled)
          , vertices(src.vertices)
          , conn_checker(src.conn_checker)
          , atomic_flags(src.atomic_flags)
          , curr_modified(src.curr_modified)
          , next_modified(src.next_modified)
          , n_modified(src.n_modified)
        {}

        rooted_rcforest &operator = (
            rooted_rcforest const &src
        ) {
            e_ops                = src.e_ops;
            v_ops                = src.v_ops;
            edge_count           = src.edge_count;
            scheduled_edge_count = src.scheduled_edge_count;
            has_scheduled        = src.has_scheduled;
            vertices             = src.vertices;
            conn_checker         = src.conn_checker;
            atomic_flags         = src.atomic_flags;
            curr_modified        = src.curr_modified;
            next_modified        = src.next_modified;
            n_modified           = src.n_modified;
            return *this;
        }

    // Access methods
    public:
        // Returns the number of vertices.
        virtual int n_vertices() const {
            return vertices.size() / 2;
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
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::n_children] Invalid argument");
            }
            return vertices[2 * vertex].children_count;
        }

        // Returns the parent of the given vertex.
        virtual int get_parent(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::get_parent] Invalid argument");
            }
            int vx = 2 * vertex + 1;
            while (vx != -1 && (vx & 1) == 1) {
                vx = vertices[vx].at_level(1).parent;
            }
            if (vx == -1) {
                // A root. For outer interface, return itself
                return vertex;
            } else {
                return vx / 2;
            }
        }

        // Returns whether the given vertex is root.
        virtual bool is_root(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::is_root] Invalid argument");
            }
            return get_parent(vertex) == vertex;
        }

        // Returns the vertex info for the given vertex.
        virtual v_info_t get_vertex_info(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::get_vertex_info] Invalid argument");
            }
            return vertices[2 * vertex].at_level(1).v_info;
        }

        // Returns the upwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_upwards(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::get_edge_info_upwards] Invalid argument");
            }
            if (is_root(vertex)) {
                throw std::invalid_argument("[rooted_rcforest::get_edge_info_upwards]: The vertex is a root!");
            }
            return vertices[2 * vertex].at_level(1).e_info_up;
        }

        // Returns the downwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_downwards(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::get_edge_info_downwards] Invalid argument");
            }
            if (is_root(vertex)) {
                throw std::invalid_argument("[rooted_rcforest::get_edge_info_downwards]: The vertex is a root!");
            }
            return vertices[2 * vertex].at_level(1).e_info_down;
        }

    // A helper for get_path
    private:
        // The private struct - don't need to test the public methods!
        struct get_path_helper {
            int  vertex;
            bool is_first_part;
            bool compress_up;
            e_monoid_trait const &e_ops;
            e_info_t sum;

            get_path_helper(int vertex, bool is_first_part, bool compress_up, e_monoid_trait const &e_ops)
                : vertex(vertex)
                , is_first_part(is_first_part)
                , compress_up(compress_up)
                , e_ops(e_ops)
                , sum(e_ops.neutral())
            {}

            int get_level(std::vector<vertex_col_t> const &vertices) const {
                return vertices[vertex].last_live_level;
            }

            void relax(std::vector<vertex_col_t> const &vertices) {
                vertex_col_t const &col = vertices[vertex];
                int level = col.last_live_level;
                if (col.contraction == contract_t::rake || (col.contraction == contract_t::compress && compress_up)) {
                    vertex = col.at_level(level).parent;
                    if (is_first_part) {
                        sum = e_ops.sum(sum, col.at_level(level).e_info_up);
                    } else {
                        sum = e_ops.sum(col.at_level(level).e_info_down, sum);
                    }
                } else if (col.contraction == contract_t::compress) {
                    vertex = col.at_level(level).children[0];
                    if (is_first_part) {
                        sum = e_ops.sum(sum, vertices[vertex].at_level(level).e_info_down);
                    } else {
                        sum = e_ops.sum(vertices[vertex].at_level(level).e_info_up, sum);
                    }
                }
            }
        };

    // Query methods
    public:
        // Returns the root of the tree the given vertex belongs to.
        virtual int get_root(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::get_root] Invalid argument");
            }
            vertex *= 2;
            while (vertices[vertex].contraction != contract_t::root) {
                vertex_col_t const &col = vertices[vertex];
                // both rake and compress work like this
                vertex = col.at_level(col.last_live_level).parent;
            }
            return vertex / 2;
        }

        // Returns the monoid sum for the path from the first vertex to the last one.
        virtual e_info_t get_path(int v_first, int v_last) const {
            if (v_first < 0 || v_first >= n_vertices() || v_last < 0 || v_last >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::get_path] Invalid argument(s)");
            }
            if (get_root(v_first) != get_root(v_last)) {
                throw std::invalid_argument("[rooted_rcforest::get_path]: There is no path between the vertices!");
            }

            // You never know which way to go when a vertex is compressed.
            // But until the first and last vertices meet, the difference is always one edge,
            // so, for either vertex, we can track only the two endpoints: compress-up and compress-down.

            get_path_helper first_u(2 * v_first, true,  true,  e_ops),
                            first_d(2 * v_first, true,  false, e_ops),
                             last_u(2 * v_last,  false, true,  e_ops),
                             last_d(2 * v_last,  false, false, e_ops);

            get_path_helper *helpers[] = { &first_u, &first_d, &last_u, &last_d };

            while (true) {
                if (first_d.vertex == first_u.vertex) {
                    first_d.sum = first_u.sum;
                }
                if (last_d.vertex == last_u.vertex) {
                    last_d.sum = last_u.sum;
                }
                for (int i = 0; i < 2; ++i) {
                    for (int j = 2; j < 4; ++j) {
                        if (helpers[i]->vertex == helpers[j]->vertex) {
                            return e_ops.sum(helpers[i]->sum, helpers[j]->sum);
                        }
                    }
                }
                int level = helpers[0]->get_level(vertices);
                int relax = 0;
                for (int i = 1; i < 4; ++i) {
                    int new_level = helpers[i]->get_level(vertices);
                    if (new_level < level) {
                        level = new_level;
                        relax = i;
                    }
                }
                helpers[relax]->relax(vertices);
            }
        }

        // Returns the monoid sum for the vertices in the subtree of the given vertex.
        virtual v_info_t get_subtree(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::get_subtree] Invalid argument");
            }
            vertex *= 2;
            v_info_t rv = v_ops.neutral();
            while (true) {
                vertex_col_t const &col = vertices[vertex];
                vertex_t const &v = col.at_level(col.last_live_level);
                rv = v_ops.sum(rv, v.v_info);
                if (col.contraction == contract_t::root || col.contraction == contract_t::rake) {
                    return rv;
                } else {
                    // compress; the only child will continue counting its subtree
                    vertex = v.children[0];
                }
            }
        }

    // Non-scheduled modification methods
    public:
        // Creates a new vertex and returns its index (equal to n_vertices() before the call).
        virtual int create_vertex(v_info_t const &vertex_info) {
            int data_index = (int) (vertices.size());
            int link_index = data_index + 1;

            vertex_t data_vertex(vertex_info, e_ops.neutral(), e_ops.neutral()),
                     link_vertex(v_ops.neutral(), e_ops.neutral(), e_ops.neutral()),
                     rake_vertex(vertex_info, e_ops.neutral(), e_ops.neutral());

            // We create two actual-tree vertices: the data vertex and the link vertex.
            // They will be raked, it is O(1), so we just do it now.

            vertices.push_back(vertex_col_t());
            vertices.push_back(vertex_col_t());

            vertex_col_t &data_col = vertices[data_index];
            vertex_col_t &link_col = vertices[link_index];

            data_col.last_live_level = -1;
            data_col.push_level(data_vertex);
            data_col.push_level(data_vertex);
            data_col.contraction = contract_t::rake;
            data_col.children_count = 0;
            data_col.scheduled_children_count = 0;
            data_col.my_index = data_index;
            data_col.left_index = -1;  // these indices are from the tree
            data_col.right_index = -1; // of current vertex's children
            data_col.scheduled_left_index = -1;
            data_col.scheduled_right_index = -1;
            data_col.heap_key = -1; // heap key of data colums should be the minimum possible
            data_col.is_changed = false;

            link_col.last_live_level = -1;
            link_col.push_level(link_vertex);
            link_col.push_level(link_vertex);
            link_col.push_level(rake_vertex);
            link_col.contraction = contract_t::root;
            link_col.children_count = 0;
            link_col.scheduled_children_count = 0;
            link_col.my_index = link_index;
            link_col.left_index = -1;  // these indices are from the tree
            link_col.right_index = -1; // of current vertex's siblings
            link_col.scheduled_left_index = -1;
            link_col.scheduled_right_index = -1;
            link_col.heap_key = (int) ((unsigned) (global_rng()) >> 1); // a definitely positive random heap key
            link_col.is_changed = false;

            link_col.at_level(0).insert_child(data_index);
            link_col.at_level(1).insert_child(data_index);
            data_col.at_level(0).parent = link_index;
            data_col.at_level(1).parent = link_index;

            conn_checker.create_vertex();

            atomic_flags.insert_element();
            atomic_flags.insert_element();

            curr_modified.push_back(cache_line_int_t());
            curr_modified.push_back(cache_line_int_t());
            next_modified.push_back(cache_line_int_t());
            next_modified.push_back(cache_line_int_t());

            return data_index / 2;
        }

    // Scheduled modification helpers
    private:
        void ensure_has_scheduled() {
            if (!has_scheduled) {
                has_scheduled = true;
                scheduled_edge_count = edge_count;
                n_modified = 0;
            }
        }

        void ensure_internal_vertex_is_changed(int vertex) {
            if (vertex == -1) {
                throw std::logic_error("[rooted_rcforest::ensure_internal_vertex_is_changed] vertex is -1");
            }
            ensure_has_scheduled();
            vertex_col_t &vx = vertices[vertex];
            if (!vx.is_changed) {
                curr_modified[n_modified++].data = vertex;
                vx.is_changed = true;
                vx.at_level(0) = vx.at_level(1);
                vx.scheduled_left_index = vx.left_index;
                vx.scheduled_right_index = vx.right_index;
                vx.scheduled_children_count = vx.children_count;
            }
        }

        void internal_attach(int parent, int child) {
            if (vertices[child].at_level(0).parent != -1) {
                throw std::logic_error("[rooted_rcforest::internal_attach] Child is not a root!");
            }

            ensure_internal_vertex_is_changed(child);
            ensure_internal_vertex_is_changed(parent);

            vertex_t &vch = vertices[child].at_level(0);
            vertex_t &vp = vertices[parent].at_level(0);

            if (vp.children_count == 1) {
                ensure_internal_vertex_is_changed(vp.children[0]);
            }
            if (vp.parent != -1) {
                ensure_internal_vertex_is_changed(vp.parent);
                vertex_t &vgp = vertices[vp.parent].at_level(0);
                if (vgp.parent != -1 && vgp.children_count == 1) {
                    ensure_internal_vertex_is_changed(vgp.parent);
                }
            }
            if (vch.children_count == 1) {
                ensure_internal_vertex_is_changed(vch.children[0]);
            }

            vch.parent = parent;
            vp.insert_child(child);
        }

        void internal_detach(int child) {
            ensure_internal_vertex_is_changed(child);
            int parent = vertices[child].at_level(0).parent;
            ensure_internal_vertex_is_changed(parent);

            vertex_t &vch = vertices[child].at_level(0);
            vertex_t &vp = vertices[parent].at_level(0);

            vch.parent = -1;
            vp.remove_child(child);

            if (vp.parent != -1) {
                ensure_internal_vertex_is_changed(vp.parent);
                vertex_t &vgp = vertices[vp.parent].at_level(0);
                if (vgp.parent != -1 && vgp.children_count == 1) {
                    ensure_internal_vertex_is_changed(vgp.parent);
                }
            }
            if (vch.children_count == 1) {
                ensure_internal_vertex_is_changed(vch.children[0]);
            }
            if (vp.children_count == 1) {
                ensure_internal_vertex_is_changed(vp.children[0]);
            }
        }

        void cartesian_detach(int vertex) {
            if (vertex != -1) {
                ensure_internal_vertex_is_changed(vertex);
                vertex_col_t &vx = vertices[vertex];
                int parent = vx.at_level(0).parent;
                if (parent != -1) {
                    internal_detach(vertex);
                    ensure_internal_vertex_is_changed(parent);
                    vertex_col_t &vp = vertices[parent];
                    if (vp.scheduled_left_index == vertex) {
                        vp.scheduled_left_index = -1;
                    } else if (vp.scheduled_right_index == vertex) {
                        vp.scheduled_right_index = -1;
                    } else {
                        throw std::logic_error("[cartesian_detach] Vertex is not a child of its parent");
                    }
                } else {
                    throw std::logic_error("[cartesian_detach] Detaching a vertex with no parent");
                }
            }
        }

        void cartesian_attach_left(int parent, int child) {
            ensure_internal_vertex_is_changed(parent);
            vertex_col_t &vp = vertices[parent];
            if (vp.scheduled_left_index != -1) {
                throw std::logic_error("[cartesian_attach_left] Attaching to the parent onto an existing child");
            }

            if (child != -1) {
                internal_attach(parent, child);
                vp.scheduled_left_index = child;
            }
        }

        void cartesian_attach_right(int parent, int child) {
            ensure_internal_vertex_is_changed(parent);
            vertex_col_t &vp = vertices[parent];
            if (vp.scheduled_right_index != -1) {
                throw std::logic_error("[cartesian_attach_right] Attaching to the parent onto an existing child");
            }

            if (child != -1) {
                internal_attach(parent, child);
                vp.scheduled_right_index = child;
            }
        }

        void internal_set_einfo(int vertex, e_info_t const &e_info_up, e_info_t const &e_info_down) {
            ensure_internal_vertex_is_changed(vertex);
            vertex_t &vx = vertices[vertex].at_level(0);
            vx.e_info_up = e_info_up;
            vx.e_info_down = e_info_down;

            if (vx.parent != -1 && vx.children_count == 1) {
                ensure_internal_vertex_is_changed(vx.children[0]);
                ensure_internal_vertex_is_changed(vx.parent);
            }
        }

        void internal_set_vinfo(int vertex, v_info_t const &v_info) {
            ensure_internal_vertex_is_changed(vertex);
            vertex_t &vx = vertices[vertex].at_level(0);
            vx.v_info = v_info;

            if (vx.children_count <= 1 && vx.parent != -1) {
                ensure_internal_vertex_is_changed(vx.parent);
            }
        }

        int cartesian_merge(int left, int right) {
            if (left == -1 || right == -1) {
                return left + right + 1;
            } else {
                ensure_internal_vertex_is_changed(left);
                ensure_internal_vertex_is_changed(right);

                vertex_col_t &l = vertices[left];
                vertex_col_t &r = vertices[right];
                if (l.heap_key < r.heap_key) {
                    int lr = l.scheduled_right_index;
                    cartesian_detach(lr);
                    cartesian_attach_right(left, cartesian_merge(lr, right));
                    return left;
                } else {
                    int rl = r.scheduled_left_index;
                    cartesian_detach(rl);
                    cartesian_attach_left(right, cartesian_merge(left, rl));
                    return right;
                }
            }
        }

        std::pair<int, int> cartesian_split(int tree, int index) {
            if (tree == -1) {
                return std::make_pair(-1, -1);
            }

            ensure_internal_vertex_is_changed(tree);
            vertex_col_t &col_tree = vertices[tree];

            if (tree == index) {
                int l = col_tree.scheduled_left_index;
                int r = col_tree.scheduled_right_index;
                cartesian_detach(l);
                cartesian_detach(r);
                return std::make_pair(l, r);
            } else if (tree < index) {
                int r = col_tree.scheduled_right_index;
                cartesian_detach(r);
                std::pair<int, int> rec = cartesian_split(r, index);
                cartesian_attach_right(tree, rec.first);
                rec.first = tree;
                return rec;
            } else {
                int l = col_tree.scheduled_left_index;
                cartesian_detach(l);
                std::pair<int, int> rec = cartesian_split(l, index);
                cartesian_attach_left(tree, rec.second);
                rec.second = tree;
                return rec;
            }
        }

        int cartesian_insert(int tree, int vertex) {
            if (tree == -1) {
                return vertex;
            }

            ensure_internal_vertex_is_changed(tree);
            ensure_internal_vertex_is_changed(vertex);

            vertex_col_t &col_tree = vertices[tree];
            vertex_col_t &col_vertex = vertices[vertex];

            if (col_vertex.heap_key < col_tree.heap_key) {
                std::pair<int, int> split = cartesian_split(tree, vertex);
                cartesian_attach_left(vertex, split.first);
                cartesian_attach_right(vertex, split.second);
                return vertex;
            } else {
                if (vertex < tree) {
                    int l = col_tree.scheduled_left_index;
                    cartesian_detach(l);
                    cartesian_attach_left(tree, cartesian_insert(l, vertex));
                } else {
                    int r = col_tree.scheduled_right_index;
                    cartesian_detach(r);
                    cartesian_attach_right(tree, cartesian_insert(r, vertex));
                }
                return tree;
            }
        }

        int cartesian_delete(int tree, int vertex) {
            if (tree == -1) {
                throw std::logic_error("[cartesian_delete] tree == -1");
            }

            ensure_internal_vertex_is_changed(tree);
            ensure_internal_vertex_is_changed(vertex);

            vertex_col_t &col_vertex = vertices[vertex];
            if (tree == vertex) {
                int l = col_vertex.scheduled_left_index;
                int r = col_vertex.scheduled_right_index;
                cartesian_detach(l);
                cartesian_detach(r);
                return cartesian_merge(l, r);
            } else {
                vertex_col_t &col_tree = vertices[tree];
                if (vertex < tree) {
                    int l = col_tree.scheduled_left_index;
                    cartesian_detach(l);
                    cartesian_attach_left(tree, cartesian_delete(l, vertex));
                } else {
                    int r = col_tree.scheduled_right_index;
                    cartesian_detach(r);
                    cartesian_attach_right(tree, cartesian_delete(r, vertex));
                }
                return tree;
            }
        }

    // Raking and compressing
    private:
        // These routines test if the vertex is going to be affected by contraction
        bool will_become_root(int level, int vertex) {
            vertex_t const &v = vertices[vertex].at_level(level);
            return v.children_count == 0 && v.parent == -1;
        }
        bool will_rake(int level, int vertex) {
            vertex_t const &v = vertices[vertex].at_level(level);
            return v.children_count == 0 && v.parent != -1;
        }
        bool will_compress(int level, int vertex) {
            vertex_t const &v = vertices[vertex].at_level(level);
            return v.children_count == 1
                && v.parent != -1
                && !vertices[vertex].get_random_bit(level)
                && vertices[v.parent].get_random_bit(level)
                && vertices[v.children[0]].get_random_bit(level)
                && !will_rake(level, v.children[0]);
        }
        bool will_accept_change(int level, int vertex) {
            vertex_t const &v = vertices[vertex].at_level(level);
            for (int i = v.children_count - 1; i >= 0; --i) {
                if (will_rake(level, v.children[i])) {
                    return true;
                }
                if (will_compress(level, v.children[i])) {
                    return true;
                }
            }
            return v.parent != -1 && will_compress(level, v.parent);
        }
        // These routines apply contraction to vertices. They return true if something is changed. Apply with care!
        bool do_become_root(int level, int vertex) {
            vertex_col_t &v_col = vertices[vertex];
            bool rv = v_col.last_live_level != level || v_col.contraction != contract_t::root;
            v_col.last_live_level = level;
            v_col.contraction = contract_t::root;
            return rv;
        }
        bool do_rake(int level, int vertex) {
            vertex_col_t &v_col = vertices[vertex];
            bool rv = v_col.last_live_level != level || v_col.contraction != contract_t::rake;
            v_col.last_live_level = level;
            v_col.contraction = contract_t::rake;
            return rv;
        }
        bool do_compress(int level, int vertex) {
            vertex_col_t &v_col = vertices[vertex];
            bool rv = v_col.last_live_level != level || v_col.contraction != contract_t::compress;
            v_col.last_live_level = level;
            v_col.contraction = contract_t::compress;
            return rv;
        }
        bool do_accept_change(int level, int vertex) {
            vertex_col_t &v_col = vertices[vertex];
            vertex_t const &prev_vertex = v_col.at_level(level);
            vertex_t new_vertex = prev_vertex;
            if (prev_vertex.parent != -1 && will_compress(level, prev_vertex.parent)) {
                vertex_t const &parent = vertices[prev_vertex.parent].at_level(level);
                new_vertex.e_info_up = e_ops.sum(new_vertex.e_info_up, parent.e_info_up);
                new_vertex.e_info_down = e_ops.sum(parent.e_info_down, new_vertex.e_info_down);
                new_vertex.parent = parent.parent;
            }
            for (int i = prev_vertex.children_count - 1; i >= 0; --i) {
                int child_idx = prev_vertex.children[i];
                vertex_t const &child_v = vertices[child_idx].at_level(level);
                if (will_rake(level, child_idx)) {
                    new_vertex.remove_child(child_idx);
                    new_vertex.v_info = v_ops.sum(new_vertex.v_info, child_v.v_info);
                }
                if (will_compress(level, child_idx)) {
                    new_vertex.remove_child(child_idx);
                    new_vertex.v_info = v_ops.sum(new_vertex.v_info, child_v.v_info);
                    new_vertex.insert_child(child_v.children[0]);
                }
            }
            if (v_col.last_live_level == level) {
                v_col.push_level(new_vertex);
                return true;
            } else if (new_vertex != v_col.at_level(level + 1)) {
                v_col.at_level(level + 1) = new_vertex;
                return true;
            } else {
                return false;
            }
        }
        bool do_copy_paste(int level, int vertex) {
            vertex_col_t &v_col = vertices[vertex];
            vertex_t const &prev_vertex = v_col.at_level(level);
            if (v_col.last_live_level == level) {
                v_col.push_level(prev_vertex);
                return true;
            } else if (prev_vertex != v_col.at_level(level + 1)) {
                v_col.at_level(level + 1) = prev_vertex;
                return true;
            } else {
                return false;
            }
        }

        void process_changed_vertex(int level, int vertex) {
            // The current vertex V is ready and readable by this thread.
            // No other vertex from this level is readable nor ready.
            // Everything from "level - 1" is accessible.
            vertex_col_t &vx = vertices[vertex];
            vertex_t &new_vx = vertices[vertex].at_level(level);
            vx.next_affected[vx.next_affected_count++] = vertex;

            if (new_vx.parent != -1) {
                // The parent of parent, if exists, can definitely be accepted.
                // We don't know who it is yet, so we mark this like this.
                vx.next_affected_check_parent = vx.next_affected_count;
                // The parent can definitely be affected.
                vx.next_affected[vx.next_affected_count++] = new_vx.parent;
            }

            for (int i = 0; i < new_vx.children_count; ++i) {
                // Every child can technically be affected (e.g. all but one children are fresh new ones,
                // and V used to compress with the remaining one)
                vx.next_affected[vx.next_affected_count++] = new_vx.children[i];
            }
        }

        bool process_vertex(int level, int vertex) {
            vertex_col_t &vx = vertices[vertex];
            vx.next_affected_count = 0;
            vx.next_affected_prefix_sum = 0;
            vx.next_affected_check_parent = -1;
            if (will_become_root(level, vertex)) {
                if (do_become_root(level, vertex)) {
                    return true;
                }
            } else if (will_rake(level, vertex)) {
                if (do_rake(level, vertex)) {
                    vx.next_affected[vx.next_affected_count++] = vx.at_level(level).parent;
                    return true;
                }
            } else if (will_compress(level, vertex)) {
                if (do_compress(level, vertex)) {
                    vx.next_affected[vx.next_affected_count++] = vx.at_level(level).parent;
                    vx.next_affected[vx.next_affected_count++] = vx.at_level(level).children[0];
                    return true;
                }
            } else if (will_accept_change(level, vertex)) {
                if (do_accept_change(level, vertex)) {
                    process_changed_vertex(level + 1, vertex);
                    return true;
                }
            } else {
                if (do_copy_paste(level, vertex)) {
                    process_changed_vertex(level + 1, vertex);
                    return true;
                }
            }
            return false;
        }

        void fetch_parent_uniquify_vertices(int level, int vertex) {
            vertex_col_t &vx = vertices[vertex];
            if (vx.next_affected_check_parent != -1) {
                int child = vx.next_affected[vx.next_affected_check_parent];
                int parent = vertices[child].at_level(level).parent;
                if (parent != -1) {
                    vx.next_affected[vx.next_affected_count++] = parent;
                }
            }
            int affected_count = vx.next_affected_count;
            vx.next_affected_count = 0;
            for (int i = 0; i < affected_count; ++i) {
                if (!atomic_flags[vx.next_affected[i]].test_and_set()) {
                    // This vertex was not claimed by anyone else
                    vx.next_affected[vx.next_affected_count++] = vx.next_affected[i];
                }
            }
        }

    // Scheduled modification methods.
    public:
        // Tests if a vertex has changed.
        virtual bool scheduled_is_changed(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_is_changed] Invalid argument");
            }
            return vertices[2 * vertex].is_changed;
        }

        // Returns the parent of the given vertex after all scheduled changes are applied.
        virtual int scheduled_get_parent(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_get_parent] Invalid argument");
            }
            int vx = 2 * vertex + 1;
            while (vx != -1 && (vx & 1) == 1) {
                if (vertices[vx].is_changed) {
                    vx = vertices[vx].at_level(0).parent;
                } else {
                    vx = vertices[vx].at_level(1).parent;
                }
            }
            return vx == -1 ? vertex : vx / 2;
        }

        // Checks if the vertex is a root after all scheduled changes are applied.
        virtual bool scheduled_is_root(int vertex) const {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_is_root] Invalid argument");
            }
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
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_n_children] Invalid argument");
            }
            return scheduled_is_changed(vertex)
                ? vertices[2 * vertex].scheduled_children_count
                : n_children(vertex);
        }

        // Checks if there are pending changes.
        virtual bool scheduled_has_changes() const {
            return has_scheduled;
        }

        // Schedules (adds to the end of the current changelist)
        // the change of vertex information to the given vertex.
        virtual void scheduled_set_vertex_info(int vertex, v_info_t const &vertex_info) {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_set_vertex_info] Invalid argument");
            }
            internal_set_vinfo(2 * vertex, vertex_info);
        }

        // Schedules (adds to the end of the current changelist)
        // the change of edge information to the edge
        // going from the given vertex to its parent.
        virtual void scheduled_set_edge_info(int vertex, e_info_t const &edge_upwards, e_info_t const &edge_downwards) {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_set_edge_info] Invalid argument");
            }
            if (scheduled_is_root(vertex)) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_set_edge_info] The vertex is a root!");
            }
            internal_set_einfo(2 * vertex, edge_upwards, edge_downwards);
        }

        // Schedules (adds to the end of the current changelist)
        // the given vertex to be detached from its parent.
        virtual void scheduled_detach(int vertex) {
            if (vertex < 0 || vertex >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_detach] Invalid argument");
            }
            if (scheduled_is_root(vertex)) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_set_edge_info] The vertex is already a root!");
            }

            int parent = scheduled_get_parent(vertex);
            cartesian_delete(2 * parent, 2 * vertex + 1);
            conn_checker.cut(parent, vertex);

            --vertices[2 * parent].scheduled_children_count;
            --scheduled_edge_count;
        }

        // Schedules (adds to the end of the current changelist)
        // the given child vertex to be attached to the given parent vertex.
        virtual void scheduled_attach(int v_parent, int v_child, e_info_t const &edge_upwards, e_info_t const &edge_downwards) {
            if (v_parent < 0 || v_parent >= n_vertices() || v_child < 0 || v_child >= n_vertices()) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_attach] Invalid argument(s)");
            }
            if (!scheduled_is_root(v_child)) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_attach] The child vertex is not a root!");
            }

            if (conn_checker.test_connectivity(v_parent, v_child)) {
                throw std::invalid_argument("[rooted_rcforest::scheduled_attach] The parent and the child are already connected!");
            }

            internal_set_einfo(2 * v_child, edge_upwards, edge_downwards);
            cartesian_insert(2 * v_parent, 2 * v_child + 1);
            conn_checker.link(v_parent, v_child);

            ++vertices[2 * v_parent].scheduled_children_count;
            ++scheduled_edge_count;
        }

        // Applies all pending changes.
        virtual void scheduled_apply() {
            if (n_modified > 0) {
                if (has_debug_contraction) {
                    for (unsigned i = 0; i < vertices.size(); ++i) {
                        vertex_col_t &col = vertices[i];
                        col.is_changed = false;
                        col.at_level(1) = col.at_level(0);
                        col.left_index = col.scheduled_left_index;
                        col.right_index = col.scheduled_right_index;
                        col.children_count = col.scheduled_children_count;
                    }
                } else {
                    // TODO: parallel loop
                    for (unsigned i = 0; i < n_modified; ++i) {
                        vertex_col_t &col = vertices[curr_modified[i].data];
                        col.is_changed = false;
                        col.at_level(1) = col.at_level(0);
                        col.left_index = col.scheduled_left_index;
                        col.right_index = col.scheduled_right_index;
                        col.children_count = col.scheduled_children_count;
                    }
                }
            }

            for (int level = 1; n_modified > 0; ++level, std::swap(curr_modified, next_modified)) {
                if (has_debug_contraction) {
                    std::vector<unsigned> curr_affected;
                    for (unsigned i = 0; i < n_modified; ++i) {
                        curr_affected.push_back(curr_modified[i].data);
                    }
                    std::sort(curr_affected.begin(), curr_affected.end());
                    for (unsigned i = 0; i < vertices.size(); ++i) {
                        if (vertices[i].last_live_level < level) {
                            continue;
                        }
                        if (process_vertex(level, i)) {
                            if (!binary_search(curr_affected.begin(), curr_affected.end(), i)) {
                                throw std::logic_error("[rooted_rcforest::scheduled_apply] A non-affected vertex changed!");
                            }
                        }
                    }
                } else {
                    // TODO: parallel loop
                    for (unsigned i = 0; i < n_modified; ++i) {
                        process_vertex(level, curr_modified[i].data);
                    }
                }
                // TODO: parallel loop
                for (unsigned i = 0; i < n_modified; ++i) {
                    fetch_parent_uniquify_vertices(level + 1, curr_modified[i].data);
                }
                // TODO: make it an (inclusive) parallel prefix sum
                for (unsigned i = 0; i < n_modified; ++i) {
                    vertex_col_t &curr_v = vertices[curr_modified[i].data];
                    if (i == 0) {
                        curr_v.next_affected_prefix_sum = curr_v.next_affected_count;
                    } else {
                        vertex_col_t &prev_v = vertices[curr_modified[i - 1].data];
                        curr_v.next_affected_prefix_sum = prev_v.next_affected_prefix_sum + curr_v.next_affected_count;
                    }
                }
                unsigned new_n_modified = vertices[curr_modified[n_modified - 1].data].next_affected_prefix_sum;
                // TODO: parallel loop
                for (unsigned i = 0; i < n_modified; ++i) {
                    vertex_col_t &vx = vertices[curr_modified[i].data];
                    int offset = vx.next_affected_prefix_sum - vx.next_affected_count;
                    for (int j = 0; j < vx.next_affected_count; ++j) {
                        next_modified[j + offset].data = vx.next_affected[j];
                        atomic_flags[vx.next_affected[j]].clear();
                    }
                }
                n_modified = new_n_modified;
            }
            edge_count = scheduled_edge_count;
            conn_checker.flush();
            has_scheduled = false;
        }

        // Cancels all pending changes.
        virtual void scheduled_cancel() {
            scheduled_edge_count = edge_count;
            conn_checker.unroll();
            has_scheduled = false;
            // TODO: maybe also a parallel loop?
            for (unsigned i = 0; i < n_modified; ++i) {
                vertex_col_t &col = vertices[curr_modified[i].data];
                col.is_changed = false;
                col.at_level(0) = col.at_level(1);
                col.scheduled_left_index = col.left_index;
                col.scheduled_right_index = col.right_index;
                col.scheduled_children_count = col.children_count;
            }
            n_modified = 0;
        }

    // A necessary virtual destructor
    public:
        virtual ~rooted_rcforest() {}
};

#endif
