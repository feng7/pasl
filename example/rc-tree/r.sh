#!/bin/bash

if [[ "$1" == "test" ]]; then

    g++ -std=c++11 -O3 -Wall -Wextra -o test-dc \
        dynamic_connectivity_tests.cpp dynamic_connectivity.cpp

    ./test-dc
    EXITCODE=$?
    rm test-dc

    if [[ $EXITCODE != 0 ]]; then
        exit $EXITCODE
    fi

    g++ -std=c++11 -O3 -Wall -Wextra -fopenmp -o main \
        rooted_dynforest_test.cpp \
        dynamic_connectivity.cpp \
        thread_local_random.cpp

    ./main
    EXITCODE=$?
    rm main

    if [[ $EXITCODE != 0 ]]; then
        exit $EXITCODE
    fi

    g++ -std=c++11 -O3 -Wall -Wextra -fopenmp -o main \
        stress_test.cpp \
        dynamic_connectivity.cpp \
        thread_local_random.cpp

    ./main
    EXITCODE=$?
    rm main

    if [[ $EXITCODE != 0 ]]; then
        exit $EXITCODE
    fi

elif [[ "$1" == "perf" ]]; then

    g++ -std=c++11 -O3 -Wall -Wextra -fopenmp -ltcmalloc -o main \
        dynamic_connectivity.cpp \
        thread_local_random.cpp \
        timing_$2.cpp

    if [[ "$3" == "" ]]; then
        ./main
    else
        OMP_NUM_THREADS=$3 ./main
    fi
    rm main

else
    echo "Expected one of the options as the first argument: 'test', 'perf' [seq|openmp|pasl], found '$1'"
fi
