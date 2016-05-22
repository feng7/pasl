#include "looping_driver_openmp.hpp"
#include <omp.h>
#include <functional>

void looping_driver_openmp::loop_for(int from, int until, std::function<void(int)> function) {
    #pragma omp parallel for schedule(static)
    for (int i = from; i < until; ++i) {
        function(i);
    }
}

void looping_driver_openmp::compute_prefix_sum(int from, int until, std::function<int&(int)> value, std::function<int&(int)> result) {
    #pragma omp parallel if (until - from >= 100 * omp_get_max_threads())
    {
        int thread_my = omp_get_thread_num();
        int thread_count = omp_get_num_threads();
        int my_begin = from + (int) ((long long) (until - from) * (thread_my + 0) / thread_count);
        int my_end   = from + (int) ((long long) (until - from) * (thread_my + 1) / thread_count);
        if (my_begin < my_end) {
            result(my_begin) = value(my_begin);
            for (int i = my_begin + 1; i < my_end; ++i) {
                result(i) = result(i - 1) + value(i);
            }
        }

        #pragma omp barrier
        #pragma omp single
        {
            int prev_end = from + (until - from) / thread_count;
            for (int i = 1; i < thread_count; ++i) {
                int curr_end = from + (int) ((long long) (until - from) * (i + 1) / thread_count);
                if (curr_end != prev_end) {
                    result(curr_end - 1) += prev_end > 0 ? result(prev_end - 1) : 0;
                    prev_end = curr_end;
                }
            }
        }
        #pragma omp barrier

        if (my_begin + 1 < my_end) {
            int diff = result(my_end - 1) - (result(my_end - 2) + value(my_end - 1));
            for (int i = my_begin; i + 1 < my_end; ++i) {
                result(i) += diff;
            }
        }
    }
}

