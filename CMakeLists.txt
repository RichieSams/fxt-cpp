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

# ---- Workarounds ----

# Workaround for OSX currently having a broken linker
# https://github.com/Homebrew/homebrew-core/issues/145991
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-ld_classic")
endif()

# ---- Add source files ----
add_subdirectory(src)

if(FXT_BUILD_TESTS)
    add_subdirectory(tests)
endif(FXT_BUILD_TESTS)
