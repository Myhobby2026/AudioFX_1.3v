# DSP core tests
#
#   Linux/macOS/MSYS:  sh tests/run_tests.sh
#   Windows (MSVC):    cl /std:c++17 /O2 /EHsc tests\dsp_tests.cpp /Fe:build\dsp_tests.exe
#   Windows (MinGW):   g++ -std=c++17 -O2 tests\dsp_tests.cpp -o build\dsp_tests.exe
#
# The suite is intentionally JUCE-free so the maths can be validated on any
# machine - including CI - before the GUI shell is ever compiled.
