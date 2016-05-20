#ifndef __LOOPING_DRIVER_SEQ_HPP
#define __LOOPING_DRIVER_SEQ_HPP

#include <functional>

struct looping_driver_seq {
    void loop_for(int from, int until, std::function<void(int)> function);
    void compute_prefix_sum(int from, int until, std::function<int&(int)> value, std::function<int&(int)> result);
};

#endif
