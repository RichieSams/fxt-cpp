# ---- Add source files ----

set(SRC_FILES
	${PROJECT_SOURCE_DIR}/tests/src/main.cpp
	${PROJECT_SOURCE_DIR}/tests/src/write.cpp
	${PROJECT_SOURCE_DIR}/tests/src/writer_test.h
	${PROJECT_SOURCE_DIR}/tests/src/writer_validation.h
)

# ---- Create test binary ----

add_executable(${PROJECT_NAME}-test ${SRC_FILES})
set_target_properties(${PROJECT_NAME}-test PROPERTIES CXX_STANDARD 17)

target_include_directories(
  ${PROJECT_NAME}-test PRIVATE ${PROJECT_SOURCE_DIR}/tests/src
  ${PROJECT_NAME}-test PRIVATE ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(${PROJECT_NAME}-test fxt catch2)

# ---- Enable warnings ----

target_compile_options(${PROJECT_NAME}-test PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX>
)
