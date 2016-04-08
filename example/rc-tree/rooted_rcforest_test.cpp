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
    ASSERT_EQUAL(0, forest->get_edge_info_upwards(v0));
    ASSERT_EQUAL(0, forest->get_edge_info_upwards(v1));
    ASSERT_EQUAL(0, forest->get_edge_info_downwards(v0));
    ASSERT_EQUAL(0, forest->get_edge_info_downwards(v1));

    // Check if we don't have changes
    ASSERT_EQUAL(false, forest->scheduled_has_changes());

    // Apply a change: v0 is now a parent of v1
    forest->scheduled_attach(v0, v1, 7, 4);
    //forest->scheduled_detach(v0));
    forest->scheduled_detach(v1);
    forest->scheduled_attach(v0, v1, 7, 4);

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
    ASSERT_EQUAL(3, forest->scheduled_n_roots());
    ASSERT_EQUAL(0, forest->scheduled_n_edges());// should be 1 edge?

    // Before applied, the vertex is marked as changed
    ASSERT_EQUAL(true, forest->scheduled_is_changed(v0));
    ASSERT_EQUAL(true, forest->scheduled_is_changed(v1));

    // Set vertex info
    forest->scheduled_set_vertex_info(v0,50);
    forest->scheduled_set_vertex_info(v1,61);
    ASSERT_EQUAL(20, forest->get_vertex_info(v0));
    ASSERT_EQUAL(52, forest->get_vertex_info(v1));

    // Set edges info
    forest->scheduled_set_edge_info(v0,10,2);// Check its edge?
    forest->scheduled_set_edge_info(v1,16,11);

    // Check edge info connected with the vertex befoe changes applied
    ASSERT_EQUAL(0, forest->get_edge_info_upwards(v0));
    ASSERT_EQUAL(0, forest->get_edge_info_upwards(v1));
    ASSERT_EQUAL(0, forest->get_edge_info_downwards(v0));
    ASSERT_EQUAL(0, forest->get_edge_info_downwards(v1));

    // Apply changes
    forest->scheduled_apply();

    // Check the set is applied
    ASSERT_EQUAL(50, forest->get_vertex_info(v0));
    ASSERT_EQUAL(61, forest->get_vertex_info(v1));

    // After applied, the vertex is marked as not changed
    ASSERT_EQUAL(false, forest->scheduled_is_changed(v0));
    ASSERT_EQUAL(false, forest->scheduled_is_changed(v1));

    // Check edge info connected with the vertex after applied
    ASSERT_EQUAL(10, forest->get_edge_info_upwards(v0));
    ASSERT_EQUAL(2, forest->get_edge_info_downwards(v0));
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
    ASSERT_EQUAL(3, forest->n_roots());
    ASSERT_EQUAL(0, forest->n_edges());// should be 1 edge?

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

void test_everything(string const &name, int_forest_gen new_forest) {
    cout << "Testing " << name << "..." << endl;
    example_test(new_forest);
}

int main() {
    test_everything("naive forest", []() -> shared_ptr<int_forest> { return shared_ptr<int_forest>(new naive_rooted_rcforest<int, int>()); });
    return 0;
}
