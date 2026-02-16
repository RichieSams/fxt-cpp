set(SRC_FILES
	${PROJECT_SOURCE_DIR}/include/fxt/err.h
    ${PROJECT_SOURCE_DIR}/include/fxt/writer.h
	${PROJECT_SOURCE_DIR}/include/fxt/record_args.h
    ${PROJECT_SOURCE_DIR}/src/constants.h
    ${PROJECT_SOURCE_DIR}/src/fields.h
    ${PROJECT_SOURCE_DIR}/src/record_args.h
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

# ---- Enable warnings ----

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX>
)
