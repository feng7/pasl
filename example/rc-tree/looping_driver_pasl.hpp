#ifndef __LOOPING_DRIVER_PASL_HPP
#define __LOOPING_DRIVER_PASL_HPP

#include <functional>
#include "native.hpp"

struct looping_driver_pasl {
    template<typename loop_body>
    void loop_for(int from, int until, loop_body const &function) {
        pasl::sched::native::parallel_for(from, until, function);
    }
    template<typename value_fun, typename result_fun>
    void compute_prefix_sum(int from, int until, std::function<int&(int)> value, std::function<int&(int)> result) {
        int jump = 1;
        while (jump < until - from) {
            pasl::sched::native::parallel_for(0, (until - from + jump - 1) / (2 * jump), [from, value, jump] (int j) -> void {
                int i = j * 2 * jump + from;
                value(i) += value(i + jump);
            });
            jump += jump;
        }
        result(from) = value(from);
        while (jump > 1) {
            jump /= 2;
            pasl::sched::native::parallel_for(0, (until - from + jump - 1) / (2 * jump), [from, value, result, jump] (int j) -> void {
                int i = j * 2 * jump + from;
                int z = value(i + jump);
                value(i) -= z;
                result(i + jump) = result(i);
                result(i) -= z;
            });
        }
    }
};

#endif
