#include "dynamic_connectivity.hpp"

#include <stdexcept>

void dummy_checker::create_vertex() {}
void dummy_checker::link(int, int) {}
void dummy_checker::cut(int, int) {}
bool dummy_checker::test_connectivity(int, int) { return false; } // enough for being a dummy
void dummy_checker::unroll() {}
void dummy_checker::flush() {}

bool link_cut_tree::is_root(int index) const {
    vertex_t const &curr = vertices.at(index);
    if (curr.parent == -1) {
        return true;
    } else {
        vertex_t const &p = vertices.at(curr.parent);
        return p.left != index && p.right != index;
    }
}

void link_cut_tree::push(int index) {
    vertex_t &curr = vertices.at(index);
    if (curr.revert) {
        curr.revert = false;
        std::swap(curr.left, curr.right);
        if (curr.left != -1) {
            vertices.at(curr.left).revert ^= true;
        }
        if (curr.right != -1) {
            vertices.at(curr.right).revert ^= true;
        }
    }
}

void link_cut_tree::connect(int child, int parent, int child_to_parent_cmp) {
    if (child != -1) {
        vertices.at(child).parent = parent;
    }
    if (child_to_parent_cmp < 0) {
        vertices.at(parent).left = child;
    } else if (child_to_parent_cmp > 0) {
        vertices.at(parent).right = child;
    }
}

void link_cut_tree::rotate(int index) {
    vertex_t &x = vertices.at(index);
    int parent = x.parent;
    int g = vertices.at(parent).parent;
    bool is_left_child = vertices.at(parent).left == index;
    bool is_root_p = is_root(parent);

    connect(is_left_child ? x.right : x.left, parent, is_left_child ? -1 : 1);
    connect(parent, index, is_left_child ? 1 : -1);
    connect(index, g, is_root_p ? 0 : (parent == vertices.at(g).left ? -1 : 1));
}

void link_cut_tree::splay(int index) {
    while (!is_root(index)) {
        int parent = vertices.at(index).parent;
        int gp = vertices.at(parent).parent;
        if (!is_root(parent)) {
            push(gp);
        }
        push(parent);
        push(index);
        if (!is_root(parent)) {
            rotate((index == vertices.at(parent).left) == (parent == vertices.at(gp).left) ? parent : index);
        }
        rotate(index);
    }
    push(index);
}

int link_cut_tree::expose(int index) {
    int last = -1;
    for (int y = index; y != -1; y = vertices.at(y).parent) {
        splay(y);
        vertices.at(y).left = last;
        last = y;
    }
    splay(index);
    return last;
}

void link_cut_tree::make_root(int index) {
    expose(index);
    vertices.at(index).revert ^= true;
}

link_cut_tree::link_cut_tree() : vertices(), undo() {
}

link_cut_tree::link_cut_tree(link_cut_tree const &src) : vertices(src.vertices), undo(src.undo) {
}

link_cut_tree &link_cut_tree::operator = (link_cut_tree const &src) {
    vertices = src.vertices;
    undo = src.undo;
    return *this;
}

link_cut_tree &link_cut_tree::operator = (link_cut_tree &&src) {
    vertices = src.vertices;
    undo = src.undo;
    return *this;
}

void link_cut_tree::create_vertex() {
    vertices.push_back(vertex_t());
    vertex_t &back = vertices.back();
    back.left = -1;
    back.right = -1;
    back.parent = -1;
    back.revert = false;
}

void link_cut_tree::link(int v1, int v2) {
    link(v1, v2, true);
}

void link_cut_tree::cut(int v1, int v2) {
    cut(v1, v2, true);
}

void link_cut_tree::link(int v1, int v2, bool fill_undo) {
    if (test_connectivity(v1, v2)) {
        throw std::logic_error("[link_cut_tree::link] Vertices are already connected!");
    }
    make_root(v1);
    vertices.at(v1).parent = v2;
    if (fill_undo) {
        undo.push_back(undo_record_t());
        undo_record_t &r = undo.back();
        r.v1 = v1;
        r.v2 = v2;
        r.undo_is_link = false;
    }
}

void link_cut_tree::cut(int v1, int v2, bool fill_undo) {
    make_root(v1);
    expose(v2);
    if (vertices.at(v2).right != v1 || vertices.at(v1).left != -1 || vertices.at(v1).right != -1) {
        throw std::logic_error("[link_cut_tree::cut] No edge between vertices!");
    }
    vertices.at(vertices.at(v2).right).parent = -1;
    vertices.at(v2).right = -1;
    if (fill_undo) {
        undo.push_back(undo_record_t());
        undo_record_t &r = undo.back();
        r.v1 = v1;
        r.v2 = v2;
        r.undo_is_link = true;
    }
}

bool link_cut_tree::test_link(int v1, int v2) {
    make_root(v1);
    expose(v2);
    return vertices.at(v2).right == v1 && vertices.at(v1).left == -1 && vertices.at(v1).right == -1;
}

bool link_cut_tree::test_connectivity(int v1, int v2) {
    if (v1 == v2) {
        return true;
    }
    expose(v1);
    expose(v2);
    return vertices.at(v1).parent != -1;
}

void link_cut_tree::unroll() {
    while (undo.size() > 0) {
        undo_record_t &r = undo.back();
        if (r.undo_is_link) {
            link(r.v1, r.v2, false);
        } else {
            cut(r.v1, r.v2, false);
        }
        undo.pop_back();
    }
}

void link_cut_tree::flush() {
    undo.clear();
}
