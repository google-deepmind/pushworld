Setup
-----

1) Install Boost (>= v1.78): https://www.boost.org/

2) Install CMake (>= v3.24): https://cmake.org/install/

3) Build all binaries:

    cmake -B build; cmake --build build

4) Run all tests (must run within the `test` directory):

    cd build/bin/test
    ./run_tests


Running the RGD Planner
-----------------------

Run the following to print available options:

    ./build/bin/run_planner
