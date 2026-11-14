cmake_minimum_required (VERSION 3.5)
project (TestInputs CXX)

# The input generator of the test suite; run_tests.sh builds it.
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
find_package (OpenCV REQUIRED)
add_executable (make-inputs make_inputs.cpp)
target_link_libraries (make-inputs ${OpenCV_LIBS})
