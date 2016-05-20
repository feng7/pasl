#include "looping_driver_pasl.hpp"

#include <functional>
#include "native.hpp"

void looping_driver_pasl::loop_for(int from, int until, std::function<void(int)> function) {
    pasl::sched::native::parallel_for(int(from), until, function);
}

void looping_driver_pasl::compute_prefix_sum(int from, int until, std::function<int&(int)> value, std::function<int&(int)> result) {
    /*int jump = 1;
    while (jump < until - from) {
        int end = (until - from + 2 * jump - 1) / (2 * jump);
        pasl::sched::native::parallel_for(int(0), end, [&] (int j) {
            int i = j * 2 * jump + from;
            value(i) += value(i + jump);
        });
        jump += jump;
    }
    result(from) = value(from);
    while (jump > 1) {
        jump /= 2;
        int end = (until - from + 2 * jump - 1) / (2 * jump);
        pasl::sched::native::parallel_for(int(0), end, [&] (int j) {
            int i = j * 2 * jump + from;
            int z = value(i + jump);
            value(i) -= z;
            result(i + jump) = result(i);
            result(i) -= z;
        });
    }*/
    int jump = 1;
    while (jump < until - from) {
        pasl::sched::native::parallel_for (int(from), until - jump, [&] (int i) {
            if((i-from)%(2*jump)!=0){
              ;
            }
            else{
              value(i) += value(i + jump);
            }
        });
        jump += jump;
    }
    result(from) = value(from);
    while (jump > 1) {
        jump /= 2;
        pasl::sched::native::parallel_for (int(from), until - jump, [&] (int i) {
            if((i-from)%(2*jump)!=0){
              ;
            }
            else{
              int z = value(i + jump);
              value(i) -= z;
              result(i + jump) = result(i);
              result(i) -= z;
            }
        }
    }
}
