#!/bin/bash

g++ -std=c++11 -O3 -Wall -Wextra -o main \
   rooted_rcforest_test.cpp

./main
rm main
