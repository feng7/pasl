#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "rooted_rcforest.hpp"
#include "naive_rooted_rcforest.hpp"

using std::cout;
using std::endl;
using std::function;
using std::ostringstream;
using std::shared_ptr;
using std::string;

using int_forest     = rooted_rcforest<int, int>;
using int_forest_ptr = shared_ptr<int_forest>;
using int_forest_gen = function<int_forest_ptr()>;

template<typename T>
void assert_equal_impl(int line, T const &expected, T const &found) {
    if (expected != found) {
        ostringstream oss;
        oss << "Assertion failed at line " << line << ": expected " << expected << " found " << found;
        throw std::logic_error(oss.str());
    }
}

#define ASSERT_EQUAL(a, b) try {                                        \
                               assert_equal_impl(__LINE__, a, b);       \
                           } catch (std::exception &ex) {               \
                               ostringstream oss;                       \
                               oss << "[Exception in line " << __LINE__ \
                                   << "]: " << ex.what();               \
                               throw std::runtime_error(oss.str());     \
                           }

// just a couple of quotes for stupid Nano highlighting: ""

#define ASSERT_THROWS(exc, expr) try {                                    \
                                     expr;                                \
                                     ostringstream oss;                   \
                                     oss << "[Exception " << #exc         \
                                         << " was not thrown in line "    \
                                         << __LINE__ << "]" << endl;      \
                                     throw std::runtime_error(oss.str()); \
                                 } catch (exc &) {}                       \

// just a couple of quotes for stupid Nano highlighting: ""

void example_test(int_forest_gen new_forest) {
    cout << "    example_test... ";
    int_forest_ptr forest = new_forest();

    // Create two vertices
    int v0 = forest->create_vertex(20);
    ASSERT_EQUAL(0, v0);
    int v1 = forest->create_vertex(52);
    ASSERT_EQUAL(1, v1);
    int v2 = forest->create_vertex(46);
    ASSERT_EQUAL(2, v2);

    // Single-vertex subtree queries
    ASSERT_EQUAL(20, forest->get_subtree(v0));
    ASSERT_EQUAL(52, forest->get_subtree(v1));

    // root query for a tree
    ASSERT_EQUAL(0, forest->get_root(v0));
    ASSERT_EQUAL(1, forest->get_root(v1));
    ASSERT_EQUAL(2, forest->get_root(v2));

    // Check if both are roots
    ASSERT_EQUAL(true, forest->is_root(v0));
    ASSERT_EQUAL(true, forest->is_root(v1));
    ASSERT_EQUAL(v0, forest->get_parent(v0));
    ASSERT_EQUAL(v1, forest->get_parent(v1));

    // Check the children
    ASSERT_EQUAL(0, forest->n_children(v0));
    ASSERT_EQUAL(0, forest->n_children(v1));

    // Check the roots,edges,vertices in the forest
    ASSERT_EQUAL(3, forest->n_roots());
    ASSERT_EQUAL(0, forest->n_edges());
    ASSERT_EQUAL(3, forest->n_vertices());

    // Check vertex info
    ASSERT_EQUAL(20, forest->get_vertex_info(v0));
    ASSERT_EQUAL(52, forest->get_vertex_info(v1));

    // Check edge info connected with the vertex
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v1));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v1));

    // Check if we don't have changes
    ASSERT_EQUAL(false, forest->scheduled_has_changes());

    // Apply a change: v0 is now a parent of v1
    forest->scheduled_attach(v0, v1, 7, 4);
    //forest->scheduled_detach(v0));
    forest->scheduled_detach(v1);

    // if the vertex is attached by itself, the edge info setting has no sense
    ASSERT_THROWS(std::invalid_argument, forest->scheduled_attach(v1, v1, 7, 4));
    forest->scheduled_attach(v0, v1, 7, 4);
    ASSERT_THROWS(std::invalid_argument, forest->scheduled_attach(v1, v0, 7, 4));

    // Check if we have changes
    ASSERT_EQUAL(true, forest->scheduled_has_changes());

    // Cancel changes
    forest->scheduled_cancel();

    // Check if we don't have changes
    ASSERT_EQUAL(false, forest->scheduled_has_changes());

    // Apply a change: v0 is now a parent of v1
    forest->scheduled_attach(v0, v1, 7, 4);

    // Check if we have changes
    ASSERT_EQUAL(true, forest->scheduled_has_changes());

    // Check if both are STILL roots - as the changes were not applied
    ASSERT_EQUAL(true, forest->is_root(v0));
    ASSERT_EQUAL(true, forest->is_root(v1));
    ASSERT_EQUAL(v0, forest->get_parent(v0));
    ASSERT_EQUAL(v1, forest->get_parent(v1));

    // Check their parent if the changes are applied
    ASSERT_EQUAL(true, forest->scheduled_is_root(v0));
    ASSERT_EQUAL(false, forest->scheduled_is_root(v1));
    ASSERT_EQUAL(v0, forest->scheduled_get_parent(v0));
    ASSERT_EQUAL(v0, forest->scheduled_get_parent(v1));

    // Check the children - should STILL be nothing
    ASSERT_EQUAL(0, forest->n_children(v0));
    ASSERT_EQUAL(0, forest->n_children(v1));

    // Check the scheduled children if the changes are applied
    ASSERT_EQUAL(1, forest->scheduled_n_children(v0));
    ASSERT_EQUAL(0, forest->scheduled_n_children(v1));

    // Check the roots,edges in the forest after changes before applied
    ASSERT_EQUAL(2, forest->scheduled_n_roots());
    ASSERT_EQUAL(1, forest->scheduled_n_edges());

    // Before applied, the vertex is marked as changed
    ASSERT_EQUAL(true, forest->scheduled_is_changed(v0));
    ASSERT_EQUAL(true, forest->scheduled_is_changed(v1));

    // Set vertex info
    forest->scheduled_set_vertex_info(v0,50);
    forest->scheduled_set_vertex_info(v1,61);
    ASSERT_EQUAL(20, forest->get_vertex_info(v0));
    ASSERT_EQUAL(52, forest->get_vertex_info(v1));

    // Set edges info
    ASSERT_THROWS(std::invalid_argument, forest->scheduled_set_edge_info(v0, 10, 2));
    forest->scheduled_set_edge_info(v1, 16, 11);

    // Check edge info connected with the vertex befoe changes applied
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v1));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v1));

    // Apply changes
    forest->scheduled_apply();

    // Check the set is applied
    ASSERT_EQUAL(50, forest->get_vertex_info(v0));
    ASSERT_EQUAL(61, forest->get_vertex_info(v1));

    // After applied, the vertex is marked as not changed
    ASSERT_EQUAL(false, forest->scheduled_is_changed(v0));
    ASSERT_EQUAL(false, forest->scheduled_is_changed(v1));

    // Check edge info connected with the vertex after applied
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v0));
    ASSERT_EQUAL(16, forest->get_edge_info_upwards(v1));
    ASSERT_EQUAL(11, forest->get_edge_info_downwards(v1));

    // Check if we don't have changes
    ASSERT_EQUAL(false, forest->scheduled_has_changes());

    // Check the new root disposition
    ASSERT_EQUAL(true, forest->is_root(v0));
    ASSERT_EQUAL(false, forest->is_root(v1));
    ASSERT_EQUAL(v0, forest->get_parent(v0));
    ASSERT_EQUAL(v0, forest->get_parent(v1));

    // Check the roots,edges,vertices in the forest
    ASSERT_EQUAL(2, forest->n_roots());
    ASSERT_EQUAL(1, forest->n_edges());

    // Single-vertex subtree queries
    ASSERT_EQUAL(111, forest->get_subtree(v0));// Subtree = 50+61
    ASSERT_EQUAL(61, forest->get_subtree(v1));
    ASSERT_EQUAL(46, forest->get_subtree(v2));

    // root query for a tree
    ASSERT_EQUAL(0, forest->get_root(v0));
    ASSERT_EQUAL(0, forest->get_root(v1));
    ASSERT_EQUAL(2, forest->get_root(v2));

    // Check the new children disposition
    ASSERT_EQUAL(1, forest->n_children(v0));
    ASSERT_EQUAL(0, forest->n_children(v1));

    // Check if the path is there and correct
    ASSERT_EQUAL(16, forest->get_path(v1, v0));
    ASSERT_EQUAL(11, forest->get_path(v0, v1));

    cout << "OK!" << endl;
}

struct matrix {
  int aa, ab, ba, bb;

  matrix() : aa(1), ab(0), ba(0), bb(1) {}   // constructs an identity matrix

  matrix(int aa, int ab, int ba, int bb) : aa(aa), ab(ab), ba(ba), bb(bb) {}  // constructs a given matrix

  matrix operator + (matrix right) const {
     return matrix(
        aa * right.aa + ab * right.ba,
        aa * right.ab + ab * right.bb,
        ba * right.aa + bb * right.ba,
        ba * right.ab + bb * right.bb
     );
  }
  bool operator ==(matrix &right) const {
     if(
        aa == right.aa && ab == right.ba&&
        ba == right.ba && bb == right.bb)return true;
        else return false;
  }
  bool operator != (matrix right) const {
     if(aa != right.aa || ab != right.ab ||
        ba != right.ba || bb != right.bb)return true;
        else return false;
  }
};

std::ostream &operator << (std::ostream &out, matrix const &m) {
    return out << "[" << m.aa << ", " << m.ab << ", " << m.ba << ", " << m.bb << "]";
}

using matrix_forest     = rooted_rcforest<matrix,int>;
using matrix_forest_ptr = shared_ptr<matrix_forest>;
using matrix_forest_gen = function<matrix_forest_ptr()>;

void matrix_test(matrix_forest_gen new_forest) {
    cout << "    matrix_test... ";
    matrix_forest_ptr forest = new_forest();

    // Create two vertices
    int v0 = forest->create_vertex(0);
    int v1 = forest->create_vertex(1);
    int v2 = forest->create_vertex(2);
    int v3 = forest->create_vertex(0);
    int v4 = forest->create_vertex(1);
    int v5 = forest->create_vertex(2);
    int v6 = forest->create_vertex(3);
    int v7 = forest->create_vertex(4);

    matrix eup(1,2,3,4),edo(5,6,7,8);
    // Build simple trees
    //       v0
    //      /  |
    //    v1   v2
    forest->scheduled_attach(v0, v1, eup, edo);
    forest->scheduled_attach(v0, v2, eup, edo);

    //        v3
    //       /  |
    //     v4   v5
    //    /  |
    //   v6  v7
    forest->scheduled_attach(v3, v4, eup, edo);
    forest->scheduled_attach(v3, v5, eup, edo);
    forest->scheduled_attach(v4, v6, eup, edo);
    forest->scheduled_attach(v4, v7, eup, edo);

    // Apply the changes
    forest->scheduled_apply();

    ASSERT_EQUAL(edo, forest->get_path(v0, v1));
    ASSERT_EQUAL(eup, forest->get_path(v1, v0));
    ASSERT_EQUAL(edo+edo, forest->get_path(v3, v7));

    // Connect two trees
    //             v0
    //            /  |
    //          v1   v2
    //          /
    //        v3
    //       /  |
    //     v4   v5
    //    /  |
    //   v6  v7
    matrix up_info(11,22,33,44),down_info(55,66,77,88);
    forest->scheduled_attach(v1, v3, up_info, down_info);
    // Apply the changes
    forest->scheduled_apply();
    ASSERT_EQUAL(eup+edo+down_info+edo, forest->get_path(v2, v5));
    ASSERT_EQUAL(eup+up_info+eup+edo, forest->get_path(v5, v2));

    // Connect two trees
    //             v0
    //            /  |
    //          v1   v2
    //          /
    //        v3
    //          |
    //          v5
    //     v4
    //    /  |
    //   v6  v7
    forest->scheduled_detach(v4);
    // Apply the changes
    forest->scheduled_apply();
    ASSERT_THROWS(std::logic_error, forest->get_path(v5, v6));

    cout << "OK!" << endl;
}

void test_matrix(string const &name, matrix_forest_gen new_forest) {
    cout << "Testing " << name << "..." << endl;
    matrix_test(new_forest);
}

void test_everything(string const &name, int_forest_gen new_forest) {
    cout << "Testing " << name << "..." << endl;
    example_test(new_forest);
}

int main() {
    test_everything("naive forest", []() -> shared_ptr<int_forest> { return shared_ptr<int_forest>(new naive_rooted_rcforest<int, int>()); });
    test_matrix("naive forest with matrix info", []() -> shared_ptr<matrix_forest> { return shared_ptr<matrix_forest>(new naive_rooted_rcforest<matrix, int>()); });
    return 0;
}
