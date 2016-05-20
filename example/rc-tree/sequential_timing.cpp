#include <algorithm>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>

#include <sys/time.h>

#include "rooted_dynforest.hpp"
#include "rooted_rcforest.hpp"
#include "looping_driver_seq.hpp"
#include "looping_driver_openmp.hpp"

using std::clock;
using std::clock_t;
using std::cout;
using std::endl;
using std::function;
using std::string;

template<typename int_forest>
void timed_scheduled_apply(int_forest &forest) {
    clock_t start_clock = clock();
    timeval start_time;
    gettimeofday(&start_time, 0);

    forest.scheduled_apply();

    clock_t end_clock = clock();
    timeval end_time;
    gettimeofday(&end_time, 0);

    double wall_time = (end_time.tv_sec + 1e-6 * end_time.tv_usec) - (start_time.tv_sec + 1e-6 * start_time.tv_usec);
    double proc_time = (double) (end_clock - start_clock) / CLOCKS_PER_SEC;
    cout << "    scheduled_apply: all-thread = "
         << proc_time << " sec, wall-clock = "
         << wall_time << " sec, ratio = "
         << proc_time / wall_time << "." << endl;
}

template<typename int_forest>
void long_chain(int size) {
    int_forest forest;
    for (int i = 0; i < size; ++i) {
        forest.create_vertex(1);
    }
    for (int i = 1; i < size; ++i) {
        forest.scheduled_attach(i - 1, i, 1, 1);
    }
    timed_scheduled_apply(forest);
    for (int i = 0; i < size; ++i) {
        int source = ((i * 3214 + 9132) % size + size) % size;
        int target = ((i * 26466 + 913532) % size + size) % size;
        if (forest.get_path(source, target) != std::abs(source - target)) {
            throw std::logic_error("Wrong result (get_path)");
        }
        if (forest.get_subtree(i) != size - i) {
            throw std::logic_error("Wrong result (get_subtree)");
        }
    }
}

template<typename int_forest>
void large_degree(int size) {
    int_forest forest;
    for (int i = 0; i < size; ++i) {
        forest.create_vertex(1);
    }
    for (int i = 1; i < size; ++i) {
        forest.scheduled_attach(0, i, 1, 1);
    }
    timed_scheduled_apply(forest);
    for (int i = 0; i < size; ++i) {
        int source = ((i * 3214 + 9132) % size + size) % size;
        int target = ((i * 26466 + 913532) % size + size) % size;
        int expected_path;
        if (source == target) {
            expected_path = 0;
        } else if (source == 0 || target == 0) {
            expected_path = 1;
        } else {
            expected_path = 2;
        }
        if (forest.get_path(source, target) != expected_path) {
            throw std::logic_error("Wrong result (get_path)");
        }
        if (forest.get_subtree(i) != (i == 0 ? size : 1)) {
            throw std::logic_error("Wrong result (get_subtree)");
        }
    }
}

template<typename int_forest>
void two_large_degrees(int size) {
    size &= ~1;
    int_forest forest;
    for (int i = 0; i < size; ++i) {
        forest.create_vertex(1);
    }
    int half_size = size / 2;
    for (int i = 1; i < half_size; ++i) {
        forest.scheduled_attach(0, i, 1, 1);
        forest.scheduled_attach(half_size, half_size + i, 2, 2);
    }
    forest.scheduled_attach(0, half_size, 3, 3);
    timed_scheduled_apply(forest);

    int expected_path[][4] = {
        { 0, 3, 1, 5 },
        { 3, 0, 4, 2 },
        { 1, 4, 2, 6 },
        { 5, 2, 6, 4 }
    };

    for (int i = 0; i < size; ++i) {
        int source = ((i * 3214 + 9132) % size + size) % size;
        int target = ((i * 26466 + 913532) % size + size) % size;
        int g_source = source == 0 ? 0 : source == half_size ? 1 : source < half_size ? 2 : 3;
        int g_target = target == 0 ? 0 : target == half_size ? 1 : target < half_size ? 2 : 3;
        int expected = source == target ? 0 : expected_path[g_source][g_target];
        int found = forest.get_path(source, target);
        if (found != expected) {
            cout << "source=" << source << " target=" << target << " g_source=" << g_source << " g_target=" << g_target << " expected=" << expected << " found=" << found << endl;
            throw std::logic_error("Wrong result (get_path)");
        }
    }
}

template<typename int_forest>
void incremental_long_chain(int size) {
    int num_rounds = 10;
    size -= size % num_rounds;
    int round_size = size / num_rounds;
    int_forest forest;
    for (int round = 0; round < num_rounds; ++round) {
        int previous_size = forest.n_vertices();
        for (int i = 0; i < round_size; ++i) {
            forest.create_vertex(1);
        }
        for (int i = 1; i < round_size; ++i) {
            forest.scheduled_attach(i - 1 + previous_size, i + previous_size, 1, 1);
        }
        if (round != 0) {
            forest.scheduled_attach(previous_size - 1, previous_size, 1, 1);
        }

        timed_scheduled_apply(forest);

        int actual_size = forest.n_vertices();
        for (int i = 0; i < actual_size; ++i) {
            int source = ((i * 3214 + 9132) % actual_size + actual_size) % actual_size;
            int target = ((i * 26466 + 913532) % actual_size + actual_size) % actual_size;
            if (forest.get_path(source, target) != std::abs(source - target)) {
                throw std::logic_error("Wrong result (get_path)");
            }
            if (forest.get_subtree(i) != actual_size - i) {
                throw std::logic_error("Wrong result (get_subtree)");
            }
        }
    }
}

void timing(string name, function<void(int)> callee) {
    int sizes[] = {100, 1000, 10000, 100000, 1000000};

    for (int i = 0; i < 5; ++i) {
        cout << name << ": " << sizes[i] << " => " << endl;

        clock_t start_time = clock();
        callee(sizes[i]);
        clock_t end_time = clock();

        cout << "    Total time " << ((double) (end_time - start_time) / CLOCKS_PER_SEC) << " sec" << endl;
    }
}

using if_seq = rooted_rcforest<int, int, looping_driver_seq>;
using if_omp = rooted_rcforest<int, int, looping_driver_openmp>;

int main() {
    timing("long chain (seq)",    [](int size) -> void { long_chain<if_seq>(size); });
    timing("long chain (OpenMP)", [](int size) -> void { long_chain<if_omp>(size); });

    timing("large degree (seq)",    [](int size) -> void { large_degree<if_seq>(size); });
    timing("large degree (OpenMP)", [](int size) -> void { large_degree<if_omp>(size); });

    timing("two large degrees (seq)",    [](int size) -> void { two_large_degrees<if_seq>(size); });
    timing("two large degrees (OpenMP)", [](int size) -> void { two_large_degrees<if_omp>(size); });

    timing("incremental long chain (seq)",    [](int size) -> void { incremental_long_chain<if_seq>(size); });
    timing("incremental long chain (OpenMP)", [](int size) -> void { incremental_long_chain<if_omp>(size); });
    return 0;
}
