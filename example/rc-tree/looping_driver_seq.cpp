#include "looping_driver_seq.hpp"

#include <functional>

void looping_driver_seq::loop_for(int from, int until, std::function<void(int)> function) {
    for (int i = from; i < until; ++i) {
        function(i);
    }
}

void looping_driver_seq::compute_prefix_sum(int from, int until, std::function<int&(int)> value, std::function<int&(int)> result) {
    result(from) = value(from);
    for (int i = from + 1; i < until; ++i) {
        result(i) = result(i - 1) + value(i);
    }
}
