#ifndef __THREAD_LOCAL_RANDOM_HPP
#define __THREAD_LOCAL_RANDOM_HPP

#if __cplusplus < 201103L
#   error "This header requires C++11"
#endif

#include <random>

extern thread_local std::mt19937 global_rng;

#endif
