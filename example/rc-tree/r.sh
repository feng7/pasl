#!/bin/bash

g++ -std=c++11 -O3 -Wall -Wextra -o test-dc \
    dc/dynamic_connectivity_tests.cpp dc/dynamic_connectivity.cpp

./test-dc
EXITCODE=$?
rm test-dc

if [[ $EXITCODE != 0 ]]; then
    exit $EXITCODE
fi

g++ -std=c++11 -O3 -Wall -Wextra -o main \
   stress_test.cpp dc/dynamic_connectivity.cpp

./main
EXITCODE=$?
rm main

if [[ $EXITCODE != 0 ]]; then
    exit $EXITCODE
fi

g++ -std=c++11 -O3 -Wall -Wextra -o main \
   rooted_rcforest_test.cpp dc/dynamic_connectivity.cpp

./main
EXITCODE=$?
rm main

if [[ $EXITCODE != 0 ]]; then
    exit $EXITCODE
fi
