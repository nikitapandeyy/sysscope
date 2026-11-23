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

# Core library target containing the monitor's modules
add_library(minimon_core STATIC
    src/process.cpp
    src/proc_reader.cpp
    src/cpu_tracker.cpp
    src/filter.cpp
    src/display.cpp
)
target_include_directories(minimon_core PUBLIC src)

# Main CLI executable
add_executable(minimon
    src/main.cpp
)
target_link_libraries(minimon PRIVATE minimon_core)

# Milestone 1 experiment executable
add_executable(read_stat_exp
    experiments/read_stat_exp.cpp
)

# Automated testing with GoogleTest
option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING)
    enable_testing()

    include(FetchContent)
    cmake_policy(SET CMP0135 NEW)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(minimon_tests
        tests/test_trivial.cpp
        tests/test_proc_reader.cpp
        tests/test_cpu_tracker.cpp
        tests/test_filter.cpp
        tests/test_display.cpp
    )
    target_link_libraries(minimon_tests PRIVATE
        minimon_core
        GTest::gtest_main
    )
    target_include_directories(minimon_tests PRIVATE src)
    target_compile_definitions(minimon_tests PRIVATE
        TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )

    include(GoogleTest)
    gtest_discover_tests(minimon_tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DISCOVERY_MODE PRE_TEST
        DISCOVERY_TIMEOUT 30
    )
endif()
