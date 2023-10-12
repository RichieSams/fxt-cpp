cmake_minimum_required(VERSION 3.14...3.22)

project(
  fxt
  VERSION 1.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add source files ----

set(SRC_FILES
	${PROJECT_SOURCE_DIR}/src/fxt.h
    ${PROJECT_SOURCE_DIR}/src/fxt.cpp
)

# ---- Create library ----

add_library(${PROJECT_NAME} ${SRC_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)

target_include_directories(
  ${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src
)
