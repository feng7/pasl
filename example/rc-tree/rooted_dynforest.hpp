#ifndef __ROOTED_DYNFOREST_HPP
#define __ROOTED_DYNFOREST_HPP

#if __cplusplus < 201103L
#   error "This header requires C++11"
#endif

// A default monoid implementation
// using the "operator +" for semigroup operation
// and default-constructed value as neutral.
template<typename number_t> struct monoid_plus {
    constexpr number_t neutral() const { return number_t(); }
    number_t sum(number_t const &lhs, number_t const &rhs) const { return lhs + rhs; }
};

// A base class for all rooted dynamic tree implementations.
template<
    typename e_info_t,
    typename v_info_t,
    typename e_monoid_trait = monoid_plus<e_info_t>, // may be non-commutative
    typename v_monoid_trait = monoid_plus<v_info_t>  // should be commutative
> class rooted_dynforest {
    // Access methods
    public:
        // Returns the number of vertices.
        virtual int n_vertices() const = 0;

        // Returns the number of edges.
        virtual int n_edges() const = 0;

        // Returns the number of roots.
        virtual int n_roots() const = 0;

        // Returns the number of children of the given vertex.
        virtual int n_children(int vertex) const = 0;

        // Returns the parent of the given vertex.
        virtual int get_parent(int vertex) const = 0;

        // Returns whether the given vertex is root.
        virtual bool is_root(int vertex) const = 0;

        // Returns the vertex info for the given vertex.
        virtual v_info_t get_vertex_info(int vertex) const = 0;

        // Returns the upwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_upwards(int vertex) const = 0;

        // Returns the downwards edge info for the edge
        // going from the given vertex to its parent.
        virtual e_info_t get_edge_info_downwards(int vertex) const = 0;

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
        virtual int create_vertex(v_info_t const &vertex_info) = 0;

    // Scheduled modification methods.
    public:
        // Tests if a vertex has changed.
        virtual bool scheduled_is_changed(int vertex) const = 0;

        // Returns the parent of the given vertex after all scheduled changes are applied.
        virtual int scheduled_get_parent(int vertex) const = 0;

        // Checks if the vertex is a root after all scheduled changes are applied.
        virtual bool scheduled_is_root(int vertex) const = 0;

        // Returns the number of edges after all scheduled changes are applied.
        virtual int scheduled_n_edges() const = 0;

        // Returns the number of roots after all scheduled changes are applied.
        virtual int scheduled_n_roots() const = 0;

        // Returns the number of children of the given vertex after all scheduled changes are applied.
        virtual int scheduled_n_children(int vertex) const = 0;

        // Checks if there are pending changes.
        virtual bool scheduled_has_changes() const = 0;

        // Schedules (adds to the end of the current changelist)
        // the change of vertex information to the given vertex.
        virtual void scheduled_set_vertex_info(int vertex, v_info_t const &vertex_info) = 0;

        // Schedules (adds to the end of the current changelist)
        // the change of edge information to the edge
        // going from the given vertex to its parent.
        virtual void scheduled_set_edge_info(int vertex, e_info_t const &edge_upwards, e_info_t const &edge_downwards) = 0;

        // Schedules (adds to the end of the current changelist)
        // the given vertex to be detached from its parent.
        virtual void scheduled_detach(int vertex) = 0;

        // Schedules (adds to the end of the current changelist)
        // the given child vertex to be attached to the given parent vertex.
        virtual void scheduled_attach(int v_parent, int v_child, e_info_t const &edge_upwards, e_info_t const &edge_downwards) = 0;

        // Applies all pending changes.
        virtual void scheduled_apply() = 0;

        // Cancels all pending changes.
        virtual void scheduled_cancel() = 0;

    // A necessary virtual destructor
    public:
        virtual ~rooted_dynforest() {}
};

#endif
