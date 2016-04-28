#!/bin/bash

if [[ "$1" == "test" ]]; then

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

elif [[ "$1" == "perf" ]]; then

    g++ -std=c++11 -O3 -Wall -Wextra -o main \
        dc/dynamic_connectivity.cpp sequential_timing.cpp

    ./main
    rm main

elif [[ "$1" == "prof" ]]; then

    g++ -std=c++11 -pg -O3 -Wall -Wextra -o main \
        dc/dynamic_connectivity.cpp sequential_timing.cpp

    ./main
    gprof main gmon.out > gmon.txt
    rm main gmon.out

else
    echo "Expected one of the options as the first argument: 'test', 'perf', 'prof', found '$1'"
fi
