#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "rooted_rcforest.hpp"
#include "naive_rooted_rcforest.hpp"
#include "sequential_rooted_rcforest.hpp"

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

#define ASSERT_NOTHROW(expr) try {                                        \
                                 expr;                                    \
                             } catch (std::exception &ex) {               \
                                 ostringstream oss;                       \
                                 oss << "[Exception in line " << __LINE__ \
                                     << "]: " << ex.what();               \
                                 throw std::runtime_error(oss.str());     \
                             }

// just a couple of quotes for stupid Nano highlighting: ""

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
    ASSERT_EQUAL(true, forest->is_root(v2));
    ASSERT_EQUAL(v0, forest->get_parent(v0));
    ASSERT_EQUAL(v1, forest->get_parent(v1));
    ASSERT_EQUAL(v2, forest->get_parent(v2));

    // Check the children
    ASSERT_EQUAL(0, forest->n_children(v0));
    ASSERT_EQUAL(0, forest->n_children(v1));
    ASSERT_EQUAL(0, forest->n_children(v2));

    // Check the roots,edges,vertices in the forest
    ASSERT_EQUAL(3, forest->n_roots());
    ASSERT_EQUAL(0, forest->n_edges());
    ASSERT_EQUAL(3, forest->n_vertices());

    // Check vertex info
    ASSERT_EQUAL(20, forest->get_vertex_info(v0));
    ASSERT_EQUAL(52, forest->get_vertex_info(v1));
    ASSERT_EQUAL(46, forest->get_vertex_info(v2));

    // Check edge info connected with the vertex
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v1));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v2));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v1));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v2));

    // Check if we don't have changes
    ASSERT_EQUAL(false, forest->scheduled_has_changes());

    // Apply a change: v0 is now a parent of v1
    ASSERT_NOTHROW(forest->scheduled_attach(v0, v1, 7, 4));
    //forest->scheduled_detach(v0));
    ASSERT_NOTHROW(forest->scheduled_detach(v1));

    // if the vertex is attached by itself, the edge info setting has no sense
    ASSERT_THROWS(std::invalid_argument, forest->scheduled_attach(v1, v1, 7, 4));
    ASSERT_NOTHROW(forest->scheduled_attach(v0, v1, 7, 4));
    ASSERT_THROWS(std::invalid_argument, forest->scheduled_attach(v1, v0, 7, 4));

    // Check if we have changes
    ASSERT_EQUAL(true, forest->scheduled_has_changes());

    // Cancel changes
    ASSERT_NOTHROW(forest->scheduled_cancel());

    // Check if we don't have changes
    ASSERT_EQUAL(false, forest->scheduled_has_changes());

    // Apply a change: v0 is now a parent of v1
    ASSERT_NOTHROW(forest->scheduled_attach(v0, v1, 7, 4));

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
    ASSERT_NOTHROW(forest->scheduled_set_vertex_info(v0, 50));
    ASSERT_NOTHROW(forest->scheduled_set_vertex_info(v1, 61));
    ASSERT_EQUAL(20, forest->get_vertex_info(v0));
    ASSERT_EQUAL(52, forest->get_vertex_info(v1));

    // Set edges info
    ASSERT_THROWS(std::invalid_argument, forest->scheduled_set_edge_info(v0, 10, 2));
    ASSERT_NOTHROW(forest->scheduled_set_edge_info(v1, 16, 11));

    // Check edge info connected with the vertex befoe changes applied
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_upwards(v1));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v0));
    ASSERT_THROWS(std::invalid_argument, forest->get_edge_info_downwards(v1));

    // Apply changes
    ASSERT_NOTHROW(forest->scheduled_apply());

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

  matrix operator + (matrix const &right) const {
     return matrix(
        aa * right.aa + ab * right.ba,
        aa * right.ab + ab * right.bb,
        ba * right.aa + bb * right.ba,
        ba * right.ab + bb * right.bb
     );
  }
  bool operator ==(matrix const &right) const {
     return aa == right.aa && ab == right.ab && ba == right.ba && bb == right.bb;
  }

  bool operator != (matrix const &right) const {
     return aa != right.aa || ab != right.ab || ba != right.ba || bb != right.bb;
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

    // Create vertices of two trees
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
    ASSERT_NOTHROW(forest->scheduled_attach(v0, v1, eup, edo));
    ASSERT_NOTHROW(forest->scheduled_attach(v0, v2, eup, edo));

    //        v3
    //       /  |
    //     v4   v5
    //    /  |
    //   v6  v7
    ASSERT_NOTHROW(forest->scheduled_attach(v3, v4, eup, edo));
    ASSERT_NOTHROW(forest->scheduled_attach(v3, v5, eup, edo));
    ASSERT_NOTHROW(forest->scheduled_attach(v4, v6, eup, edo));
    ASSERT_NOTHROW(forest->scheduled_attach(v4, v7, eup, edo));

    // Apply the changes
    ASSERT_NOTHROW(forest->scheduled_apply());

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
    ASSERT_NOTHROW(forest->scheduled_attach(v1, v3, up_info, down_info));
    // Apply the changes
    ASSERT_NOTHROW(forest->scheduled_apply());
    ASSERT_EQUAL(eup+edo+down_info+edo, forest->get_path(v2, v5));
    ASSERT_EQUAL(eup+up_info+eup+edo, forest->get_path(v5, v2));

    // Split two trees
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
    ASSERT_NOTHROW(forest->scheduled_detach(v4));
    // Apply the changes
    ASSERT_NOTHROW(forest->scheduled_apply());
    ASSERT_THROWS(std::logic_error, forest->get_path(v5, v6));

    // Create vertices of two big trees
    // Vertices of first big tree
    int v_t1[10000];
    for(int i=0;i<10000;i++)
    {
      v_t1[i] = forest->create_vertex(i);
    }
    // Vertices of second big tree
    int v_t2[100000];
    for(int i=0;i<100000;i++)
    {
      v_t2[i] = forest->create_vertex(i);
    }
    // Build simple trees
    for(int i=1;i<10000;i++)
    {
      ASSERT_NOTHROW(forest->scheduled_attach(v_t1[i-1], v_t1[i], eup, edo));
    }
    //         v_t1[0]
    //        /      |
    //   v_t1[1]   v_t1[4999]
    //     |          |
    //     Δ          Δ     subtrees
    ASSERT_THROWS(std::logic_error, forest->scheduled_detach(v_t1[14999]));
    ASSERT_NOTHROW(forest->scheduled_detach(v_t1[4999]));
    ASSERT_NOTHROW(forest->scheduled_attach(v_t1[0], v_t1[4999], eup, edo));

    for(int i=1;i<100000;i++)
    {
      ASSERT_NOTHROW(forest->scheduled_attach(v_t2[i-1], v_t2[i], eup, edo));
    }
    // Change the structure of second tree
    //   v_t2[0]-v_t2[1000]- ...... -v_t2[1999]
    //     |
    //   v_t2[1]-v_t2[2000]- ...... -v_t2[2999]
    //     |
    //   ......................................
    //     |
    //   v_t2[98]-v_t2[99000]- ...... -v_t2[99999]
    //     |
    //    ...
    //     |
    //   v_t2[999]
    for(int j=1;j<100;j++)
    {
      ASSERT_NOTHROW(forest->scheduled_detach(v_t2[1000*j]));
      ASSERT_NOTHROW(forest->scheduled_attach(v_t2[j-1], v_t2[1000*j], eup, edo));
    }

    // Apply the changes
    ASSERT_NOTHROW(forest->scheduled_apply());

    /*
    // Check the edge info
    matrix edo3000(0,0,0,0),eup1002(0,0,0,0),eup3000_edo1002(0,0,0,0);
    for(int i=0;i<3000;++i) edo3000 = edo3000+edo;
    for(int i=0;i<1002;++i) eup1002 = eup1002+eup;
    for(int i=0;i<3000;++i) eup3000_edo1002 = eup3000_edo1002+eup;
    for(int i=0;i<1002;++i) eup3000_edo1002 = eup3000_edo1002+edo;
    ASSERT_EQUAL(edo3000, forest->get_path(v_t1[0], v_t1[3000]));
    ASSERT_EQUAL(eup1002, forest->get_path(v_t1[6000], v_t1[0]));
    ASSERT_EQUAL(eup3000_edo1002, forest->get_path(v_t1[3000], v_t1[6000]));
    */

    // Connect two trees
    ASSERT_NOTHROW(forest->scheduled_attach(v_t1[0], v_t2[0], up_info, down_info));
    // Apply the changes
    ASSERT_NOTHROW(forest->scheduled_apply());
    //ASSERT_EQUAL(eup10_downinfo_edo502, forest->get_path(v_t1[10], v_t1[2500]));

    ASSERT_NOTHROW(forest->scheduled_detach(v_t2[2600]));
    // Apply the changes
    ASSERT_NOTHROW(forest->scheduled_apply());
    ASSERT_THROWS(std::logic_error, forest->get_path(v_t1[50], v_t2[2800]));

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
    test_everything("naive forest", []() -> shared_ptr<int_forest> {
        return shared_ptr<int_forest>(new naive_rooted_rcforest<int, int>());
    });
    test_everything("sequential forest", []() -> shared_ptr<int_forest> {
        return shared_ptr<int_forest>(new sequential_rooted_rcforest<int, int, monoid_plus<int>, monoid_plus<int>, link_cut_tree, true>());
    });

    test_matrix("naive forest with matrix info", []() -> shared_ptr<matrix_forest> {
        return shared_ptr<matrix_forest>(new naive_rooted_rcforest<matrix, int>());
    });
    test_matrix("sequential forest with matrix info", []() -> shared_ptr<matrix_forest> {
        return shared_ptr<matrix_forest>(new sequential_rooted_rcforest<matrix, int, monoid_plus<matrix>, monoid_plus<int>, link_cut_tree, true>());
    });
    return 0;
}
