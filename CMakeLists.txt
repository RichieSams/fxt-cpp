cmake_minimum_required(VERSION 3.22)

project(
    fxt
    LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
        "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
    )
endif()

# ---- Options ----

option(FXT_BUILD_TESTS "Build test programs" ON)

# ---- Add source files ----
add_subdirectory(src)

if(FXT_BUILD_TESTS)
    add_subdirectory(tests)
endif(FXT_BUILD_TESTS)
