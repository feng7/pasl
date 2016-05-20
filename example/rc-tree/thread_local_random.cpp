#include "thread_local_random.hpp"

#include <chrono>

thread_local std::mt19937 global_rng(
    std::mt19937::result_type(std::chrono::system_clock::now().time_since_epoch().count())
);

