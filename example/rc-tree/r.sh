#!/bin/bash

g++ -std=c++11 -O3 -Wall -Wextra -o test-dc \
    dc/dynamic_connectivity_tests.cpp dc/dynamic_connectivity.cpp

./test-dc
rm test-dc

g++ -std=c++11 -O3 -Wall -Wextra -o main \
   rooted_rcforest_test.cpp dc/dynamic_connectivity.cpp

./main
rm main
