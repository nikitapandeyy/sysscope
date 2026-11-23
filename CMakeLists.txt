cmake_minimum_required(VERSION 3.16)
project(minimon
    VERSION 0.1.0
    DESCRIPTION "Mini Process Monitor - Linux /proc Inspector in C++17"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable compiler warnings for clean code hygiene
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Core executable
add_executable(minimon
    src/main.cpp
)

target_include_directories(minimon PRIVATE src)

# Milestone 1 experiment executable
add_executable(read_stat_exp
    experiments/read_stat_exp.cpp
)

# Optional testing support (configured in Phase 2)
option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING)
    enable_testing()
endif()
