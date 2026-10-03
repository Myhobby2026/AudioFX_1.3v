#!/bin/sh
# run_tests.sh - build & run the JUCE-free DSP validation suite
cd "$(dirname "$0")"
g++ -std=c++17 -O2 -Wall -Wextra -I.. dsp_tests.cpp -o dsp_tests_bin && ./dsp_tests_bin
