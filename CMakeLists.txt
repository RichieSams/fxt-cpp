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

set(SRC_FILES
    ${PROJECT_SOURCE_DIR}/include/fxt/internal/constants.h
	${PROJECT_SOURCE_DIR}/include/fxt/defines.h
	${PROJECT_SOURCE_DIR}/include/fxt/err.h
	${PROJECT_SOURCE_DIR}/include/fxt/event_args.h
    ${PROJECT_SOURCE_DIR}/include/fxt/writer.h
    ${PROJECT_SOURCE_DIR}/src/event_args.cpp
    ${PROJECT_SOURCE_DIR}/src/writer.cpp
    ${PROJECT_SOURCE_DIR}/src/xxhash.h
)

# ---- Create library ----

add_library(${PROJECT_NAME} ${SRC_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

target_include_directories(
    ${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src
)

if(FXT_BUILD_TESTS)
    add_subdirectory(tests)
endif(FXT_BUILD_TESTS)
