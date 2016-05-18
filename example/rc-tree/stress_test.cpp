#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "rooted_dynforest.hpp"
#include "naive_rooted_dynforest.hpp"
#include "rooted_rcforest.hpp"

using std::cout;
using std::endl;
using std::function;
using std::ostringstream;
using std::shared_ptr;
using std::string;

using int_forest     = rooted_dynforest<int, int>;
using int_forest_ptr = shared_ptr<int_forest>;
using int_forest_gen = function<int_forest_ptr()>;

std::mt19937 rng;

void stress_testing(int max_vertices, int max_operations, int_forest_gen gen_ans, int_forest_gen gen_out) {
    int_forest_ptr tree_ans = gen_ans();
    int_forest_ptr tree_out = gen_out();

    #define X_ASSERT_TRUE(value, message) { \
        if (!value) { \
            log << "Line " << __LINE__ << ": " << message << endl; \
            cout << log.str(); \
            exit(1); \
        } \
    }
    // ""

    #define X_ASSERT_EQ(query, message) { \
        std::string xp_what = ""; \
        std::string fn_what = ""; \
        int xp = -1, fn = -1; \
        try { \
            xp = tree_ans->query; \
        } catch(std::exception &ex) { \
            xp_what = ex.what(); \
        } \
        try { \
            fn = tree_out->query; \
        } catch (std::exception &ex) { \
            fn_what = ex.what(); \
        } \
        if ((xp_what == "") != (fn_what == "") || xp != fn) { \
            log << "Line " << __LINE__ << ": " << message << " differ: "; \
            if (xp_what != "") { \
                log << " expected throwing exception (" << xp_what << ")"; \
            } else { \
                log << " expected " << xp; \
            } \
            if (fn_what != "") { \
                log << " found throwing exception (" << fn_what << ")"; \
            } else { \
                log << " found " << fn; \
            } \
            log << endl; \
            cout << log.str(); \
            exit(1); \
        } else { \
            if (xp_what == "") { \
                log << "Line " << __LINE__ << ": " << message << " returned " << xp << endl; \
            } else { \
                log << "Line " << __LINE__ << ": " << message << " thrown exception (" << xp_what << ")" << endl; \
            } \
        } \
    }
    // ""

    #define X_DO_OR_THROW(action, message, out) { \
        std::string xp_what = ""; \
        std::string fn_what = ""; \
        try { \
            tree_ans->action; \
        } catch(std::exception &ex) { \
            xp_what = ex.what(); \
        } \
        try { \
            tree_out->action; \
        } catch (std::exception &ex) { \
            fn_what = ex.what(); \
        } \
        if ((xp_what == "") != (fn_what == "")) { \
            log << "Line " << __LINE__ << ": " << message << " differ: "; \
            if (xp_what != "") { \
                log << " expected throwing exception (" << xp_what << ")"; \
            } else { \
                log << " expected OK "; \
            } \
            if (fn_what != "") { \
                log << " found throwing exception (" << fn_what << ")"; \
            } else { \
                log << " found OK"; \
            } \
            log << endl; \
            cout << log.str(); \
            exit(1); \
        } else { \
            if (xp_what == "") { \
                log << "Line " << __LINE__ << ": " << message << " OK" << endl; \
                out = true; \
            } else { \
                log << "Line " << __LINE__ << ": " << message << " thrown exception (" << xp_what << ")" << endl; \
                out = false; \
            } \
        } \
    }
    // ""

    std::ostringstream log;

    for (int i = 0; i < max_vertices; ++i) {
        int value = rng();
        bool dummy;
        X_DO_OR_THROW(create_vertex(value), "create_vertex(" << value << ")", dummy);
        X_ASSERT_TRUE(dummy, "create_vertex shouldn't throw");
    }

    int done_operations = 0;
    while (done_operations < max_operations) {
        int now_operations = (int) (rng() % (std::min(max_operations - done_operations, max_vertices))) + 1;
        done_operations += now_operations;
        for (int i = 0; i < now_operations; ++i) {
            bool op_completed = false;
            while (!op_completed) {
                switch (rng() % 4) {
                    case 0: { // vertex relabel
                        int v = rng() % max_vertices;
                        int label = rng();
                        bool dummy;
                        X_DO_OR_THROW(scheduled_set_vertex_info(v, label),
                                      "scheduled_set_vertex_info(" << v << ", " << label << ")",
                                      dummy);
                        X_ASSERT_TRUE(dummy, "scheduled_set_vertex_info shouldn't throw");
                        op_completed = true;
                        break;
                    }
                    case 1: {// edge relabel
                        X_ASSERT_EQ(scheduled_n_edges(), "scheduled_n_edges()");
                        if (tree_ans->scheduled_n_edges() > 0) {
                            int v;
                            do {
                                v = rng() % max_vertices;
                                X_ASSERT_EQ(scheduled_is_root(v), "scheduled_is_root(" << v << ")");
                            } while (tree_ans->scheduled_is_root(v));
                            int label_up = rng();
                            int label_down = rng();
                            bool dummy;
                            X_DO_OR_THROW(scheduled_set_edge_info(v, label_up, label_down),
                                          "scheduled_set_edge_info(" << v << ", " << label_up << ", " << label_down << ")",
                                          dummy);
                            X_ASSERT_TRUE(dummy, "scheduled_set_edge_info shouldn't throw");
                            op_completed = true;
                        }
                        break;
                    }
                    case 2: { // attach
                        X_ASSERT_EQ(scheduled_n_roots(), "scheduled_n_roots()");
                        if (tree_ans->scheduled_n_roots() > 1) {
                            int v;
                            do {
                                v = rng() % max_vertices;
                                X_ASSERT_EQ(scheduled_is_root(v), "scheduled_is_root(" << v << ")");
                            } while (!tree_ans->scheduled_is_root(v));
                            int p;
                            int label_up = rng();
                            int label_down = rng();
                            bool result;
                            do {
                                p = rng() % max_vertices;
                                X_DO_OR_THROW(scheduled_attach(p, v, label_up, label_down),
                                              "scheduled_attach(" << p << ", " << v << ", " << label_up << ", " << label_down << ")",
                                              result);
                            } while (!result);
                            op_completed = true;
                        }
                        break;
                    }
                    case 3: { // detach
                        X_ASSERT_EQ(scheduled_n_edges(), "scheduled_n_edges()");
                        if (tree_ans->scheduled_n_edges() > 0) {
                            int v;
                            do {
                                v = rng() % max_vertices;
                                X_ASSERT_EQ(scheduled_is_root(v), "scheduled_is_root(" << v << ")");
                            } while (tree_ans->scheduled_is_root(v));
                            bool dummy;
                            X_DO_OR_THROW(scheduled_detach(v), "scheduled_detach(" << v << ")", dummy);
                            X_ASSERT_TRUE(dummy, "scheduled_detach shouldn't throw");
                            op_completed = true;
                        }
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < max_vertices; ++i) {
            X_ASSERT_EQ(get_parent(i), "get_parent(" << i << ")");
            X_ASSERT_EQ(scheduled_get_parent(i), "scheduled_get_parent(" << i << ")");
            X_ASSERT_EQ(is_root(i), "is_root(" << i << ")");
            X_ASSERT_EQ(scheduled_is_root(i), "scheduled_is_root(" << i << ")");
            X_ASSERT_EQ(get_root(i), "get_root(" << i << ")");
        }

        bool dummy;
        X_DO_OR_THROW(scheduled_apply(), "scheduled_apply()", dummy);
        X_ASSERT_TRUE(dummy, "scheduled_apply shouldn't throw");
        for (int i = 0; i < max_vertices; ++i) {
            X_ASSERT_EQ(get_root(i), "get_root(" << i << ")");
            X_ASSERT_EQ(is_root(i), "is_root(" << i << ")");
            X_ASSERT_EQ(get_parent(i), "get_parent(" << i << ")");
            X_ASSERT_EQ(get_edge_info_upwards(i), "get_edge_info_upwards(" << i << ")");
            X_ASSERT_EQ(get_edge_info_downwards(i), "get_edge_info_downwards(" << i << ")");
        }
        for (int i = 0; i < max_vertices; ++i) {
            X_ASSERT_EQ(get_subtree(i), "get_subtree(" << i << ")");
            for (int j = 0; j < max_vertices; ++j) {
                X_ASSERT_EQ(get_path(i, j), "get_path(" << i << ", " << j << ")");
            }
        }
    }

    #undef X_ASSERT_EQ
    #undef X_DO_OR_THROW
    #undef X_ASSERT_TRUE
}

int main() {
    auto naive = []() -> shared_ptr<int_forest> {
        return shared_ptr<int_forest>(new naive_rooted_dynforest<int, int>());
    };
    auto seq = []() -> shared_ptr<int_forest> {
        return shared_ptr<int_forest>(new rooted_rcforest<int, int, monoid_plus<int>, monoid_plus<int>, link_cut_tree, true>());
    };

    cout << "6 vertices 100 operations" << endl;
    cout << "  Starting naive vs naive..." << endl;
    stress_testing(6, 100, naive, naive);
    cout << "    done!" << endl << "  Starting naive vs sequential..." << endl;;
    stress_testing(6, 100, naive, seq);
    cout << "    done!" << endl;

    cout << "10 vertices 200 operations" << endl;
    cout << "  Starting naive vs naive..." << endl;
    stress_testing(10, 200, naive, naive);
    cout << "    done!" << endl << "  Starting naive vs sequential..." << endl;;
    stress_testing(10, 200, naive, seq);
    cout << "    done!" << endl;

    cout << "10 vertices 10000 operations" << endl;
    cout << "  Starting naive vs naive..." << endl;
    stress_testing(10, 10000, naive, naive);
    cout << "    done!" << endl << "  Starting naive vs sequential..." << endl;;
    stress_testing(10, 10000, naive, seq);
    cout << "    done!" << endl;

    cout << "10 vertices 100000 operations" << endl;
    cout << "  Starting naive vs naive..." << endl;
    stress_testing(10, 100000, naive, naive);
    cout << "    done!" << endl << "  Starting naive vs sequential..." << endl;;
    stress_testing(10, 100000, naive, seq);
    cout << "    done!" << endl;

    cout << "100 vertices 10000 operations" << endl;
    cout << "  Starting naive vs naive..." << endl;
    stress_testing(100, 10000, naive, naive);
    cout << "    done!" << endl << "  Starting naive vs sequential..." << endl;;
    stress_testing(100, 10000, naive, seq);
    cout << "    done!" << endl;
    return 0;
}
