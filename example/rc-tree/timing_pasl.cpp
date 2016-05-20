#include "timing.hpp"
#include "looping_driver_pasl.hpp"
#include "benchmark.hpp"

using if_pasl = rooted_rcforest<int, int, looping_driver_pasl>;

int main(int argc, char *argv[]) {
    auto empty = [] {};
    auto pasl_run = [&] (bool sequential) {
        timing("long chain (PASL)",             [](int size) -> void { long_chain<if_pasl>(size); });
        timing("large degree (PASL)",           [](int size) -> void { large_degree<if_pasl>(size); });
        timing("two large degrees (PASL)",      [](int size) -> void { two_large_degrees<if_pasl>(size); });
        timing("incremental long chain (PASL)", [](int size) -> void { incremental_long_chain<if_pasl>(size); });
    };
    pasl::sched::launch(argc, argv, empty, pasl_run, empty, empty);

    return 0;
}
