set(SRC_FILES
    ${PROJECT_SOURCE_DIR}/include/fxt/internal/constants.h
	${PROJECT_SOURCE_DIR}/include/fxt/internal/defines.h
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