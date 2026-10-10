cmake_minimum_required(VERSION 3.20)
project(PTXJITStudio VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_library(ptxjit_lib src/compiler.cpp)
target_include_directories(ptxjit_lib PUBLIC include)
target_compile_options(ptxjit_lib PRIVATE -Wall -Wextra -Wpedantic)
add_executable(ptxjit src/main.cpp)
target_link_libraries(ptxjit PRIVATE ptxjit_lib)
add_executable(ptxjit_tests tests/compiler_tests.cpp)
target_link_libraries(ptxjit_tests PRIVATE ptxjit_lib)
enable_testing()
add_test(NAME compiler_tests COMMAND ptxjit_tests)

find_path(CUDA_DRIVER_INCLUDE_DIR cuda.h
    PATHS /usr/include /usr/local/cuda/include
    REQUIRED
)

find_library(CUDA_DRIVER_LIBRARY cuda
    PATHS /usr/lib/x86_64-linux-gnu /lib/x86_64-linux-gnu
    REQUIRED
)

add_executable(gpu_runner src/gpu_runner.cpp)
target_include_directories(gpu_runner PRIVATE ${CUDA_DRIVER_INCLUDE_DIR})
target_link_libraries(gpu_runner PRIVATE ptxjit_lib ${CUDA_DRIVER_LIBRARY})

add_executable(ptxjit_benchmark benchmarks/compiler_benchmark.cpp)
target_link_libraries(ptxjit_benchmark PRIVATE ptxjit_lib)
