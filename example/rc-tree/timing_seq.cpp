#include "timing.hpp"
#include "looping_driver_seq.hpp"

using if_seq = rooted_rcforest<int, int, looping_driver_seq>;

int main() {
    timing("long chain (seq)",                [](int size) -> void { long_chain<if_seq>(size); });
    timing("large degree (seq)",              [](int size) -> void { large_degree<if_seq>(size); });
    timing("two large degrees (seq)",         [](int size) -> void { two_large_degrees<if_seq>(size); });
    timing("incremental long chain (seq)",    [](int size) -> void { incremental_long_chain<if_seq>(size); });

    return 0;
}
