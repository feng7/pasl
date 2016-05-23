#!/bin/bash

TCMALLOC_TEST_PRELOAD=

function try_compiling_with_tcmalloc() {
    g++ -std=c++11 -O3 -Wall -Wextra -fopenmp -ltcmalloc -o main "$@"
    if [[ "$?" == "0" ]]; then
        echo "Compilation was OK with -ltcmalloc"
        TCMALLOC_TEST_PRELOAD=
    else
        echo "Compilation failed with -ltcmalloc"
        g++ -std=c++11 -O3 -Wall -Wextra -fopenmp -l:libtcmalloc.4.so -o main "$@"
        if [[ "$?" == "0" ]]; then
            echo "Compilation was OK with -l:libtcmalloc.4.so"
            TCMALLOC_TEST_PRELOAD="LD_PRELOAD=/usr/lib/libtcmalloc.so.4"
        else
            echo "Compilation failed with -l:libtcmalloc.4.so"
            exit 1
        fi
    fi
}

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

    try_compiling_with_tcmalloc \
        dynamic_connectivity.cpp \
        thread_local_random.cpp \
        timing_$2.cpp

    if [[ "$3" == "" ]]; then
        $TCMALLOC_TEST_PRELOAD ./main
    else
        OMP_NUM_THREADS=$3 $TCMALLOC_TEST_PRELOAD ./main
    fi
    rm main

elif [[ "$1" == "grind" ]]; then


    try_compiling_with_tcmalloc \
        dynamic_connectivity.cpp \
        thread_local_random.cpp \
        timing_$2.cpp

    if [[ "$3" == "" ]]; then
        $TCMALLOC_TEST_PRELOAD valgrind --tool=callgrind --cache-sim=yes ./main
    else
        OMP_NUM_THREADS=$3 $TCMALLOC_TEST_PRELOAD valgrind --tool=callgrind --cache-sim=yes ./main
    fi
    rm main

else
    echo "Expected one of the options as the first argument: 'test', 'perf' [seq|openmp|pasl], found '$1'"
fi
