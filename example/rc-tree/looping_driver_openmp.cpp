#include "looping_driver_openmp.hpp"

#include <functional>

void looping_driver_openmp::loop_for(int from, int until, std::function<void(int)> function) {
    #pragma omp parallel for schedule(guided, 100)
    for (int i = from; i < until; ++i) {
        function(i);
    }
}

void looping_driver_openmp::compute_prefix_sum(int from, int until, std::function<int&(int)> value, std::function<int&(int)> result) {
    int jump = 1;
    while (jump < until - from) {
        #pragma omp parallel for schedule(guided, 100)
        for (int i = from; i < until - jump; i += 2 * jump) {
            value(i) += value(i + jump);
        }
        jump += jump;
    }
    result(from) = value(from);
    while (jump > 1) {
        jump /= 2;
        #pragma omp parallel for schedule(guided, 100)
        for (int i = from; i < until - jump; i += 2 * jump) {
            int z = value(i + jump);
            value(i) -= z;
            result(i + jump) = result(i);
            result(i) -= z;
        }
    }
}
