#include "timing.hpp"
#include "looping_driver_openmp.hpp"

using if_omp = rooted_rcforest<int, int, looping_driver_openmp>;

int main() {
    timing("long chain (OpenMP)",                [](int size) -> void { long_chain<if_omp>(size); });
    timing("large degree (OpenMP)",              [](int size) -> void { large_degree<if_omp>(size); });
    timing("two large degrees (OpenMP)",         [](int size) -> void { two_large_degrees<if_omp>(size); });
    timing("incremental long chain (OpenMP)",    [](int size) -> void { incremental_long_chain<if_omp>(size); });

    return 0;
}
