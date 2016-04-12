#ifndef __DYNAMIC_CONNECTIVITY_HPP
#define __DYNAMIC_CONNECTIVITY_HPP

#if __cplusplus < 201103L
#   error "This header requires C++11"
#endif

#include <vector>

struct dummy_checker {
    void create_vertex();
    void link(int v1, int v2);
    void cut(int v1, int v2);
    bool test_connectivity(int v1, int v2);
    void unroll();
    void flush();
};

class link_cut_tree {
    struct vertex_t {
        int  left, right, parent;
        bool revert;
    };
    struct undo_record_t {
        int  v1, v2;
        bool undo_is_link;
    };

    std::vector<vertex_t>      vertices;
    std::vector<undo_record_t> undo;

    void link(int v1, int v2, bool fill_undo);
    void cut(int v1, int v2, bool fill_undo);

    void push(int index);
    bool is_root(int index) const;
    void connect(int child, int parent, int child_to_parent_cmp);
    void rotate(int index);
    void splay(int index);
    int  expose(int index);
    void make_root(int index);

public:
    link_cut_tree();
    link_cut_tree(link_cut_tree const &src);
    link_cut_tree &operator = (link_cut_tree const &src);
    link_cut_tree &operator = (link_cut_tree &&src);

    void create_vertex();
    void link(int v1, int v2);
    void cut(int v1, int v2);
    bool test_connectivity(int v1, int v2);
    bool test_link(int v1, int v2);
    void unroll();
    void flush();
};

#endif
