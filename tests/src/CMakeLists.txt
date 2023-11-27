cmake_minimum_required(VERSION 3.14...3.22)

project(
  fxt-test
  LANGUAGES CXX
)

# ---- Add source files ----

set(SRC_FILES
	${PROJECT_SOURCE_DIR}/main.cpp
	${PROJECT_SOURCE_DIR}/write.cpp
	${PROJECT_SOURCE_DIR}/writer_test.h
)

# ---- Create test binary ----

add_executable(${PROJECT_NAME} ${SRC_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

target_include_directories(
  ${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME} fxt catch2)
