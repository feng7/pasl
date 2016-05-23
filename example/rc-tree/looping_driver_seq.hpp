#ifndef __LOOPING_DRIVER_SEQ_HPP
#define __LOOPING_DRIVER_SEQ_HPP

#include <functional>

struct looping_driver_seq {
    template<typename loop_body>
    void loop_for(int from, int until, loop_body const &function) {
        for (int i = from; i < until; ++i) {
            function(i);
        }
    }
    template<typename value_fun, typename result_fun>
    void compute_prefix_sum(int from, int until, value_fun const &value, result_fun const &result) {
        result(from) = value(from);
        for (int i = from + 1; i < until; ++i) {
            result(i) = result(i - 1) + value(i);
        }
    }
};

#endif
