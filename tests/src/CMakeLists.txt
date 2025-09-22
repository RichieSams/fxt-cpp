# ---- Add source files ----

set(SRC_FILES
	${PROJECT_SOURCE_DIR}/tests/src/main.cpp
	${PROJECT_SOURCE_DIR}/tests/src/write.cpp
	${PROJECT_SOURCE_DIR}/tests/src/writer_test.h
)

# ---- Create test binary ----

add_executable(${PROJECT_NAME}-test ${SRC_FILES})
set_target_properties(${PROJECT_NAME}-test PROPERTIES CXX_STANDARD 17)

target_include_directories(
  ${PROJECT_NAME}-test PRIVATE ${PROJECT_SOURCE_DIR}/tests/src
  ${PROJECT_NAME}-test PRIVATE ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME}-test fxt catch2)
