set(EVALUATION_ALGORITHMS_TRENDS_SOURCES
    evaluation_algorithms_trends_lib.cpp
    evaluation_algorithms_trends_api.cpp
)

add_library(evaluation_algorithms_trends_shared SHARED ${EVALUATION_ALGORITHMS_TRENDS_SOURCES})
add_library(evaluation_algorithms_trends_static STATIC ${EVALUATION_ALGORITHMS_TRENDS_SOURCES})

# Keep the same logical library name for static/shared outputs.
set_target_properties(evaluation_algorithms_trends_shared PROPERTIES OUTPUT_NAME evaluation_algorithms_trends)
set_target_properties(evaluation_algorithms_trends_static PROPERTIES OUTPUT_NAME evaluation_algorithms_trends)

target_include_directories(evaluation_algorithms_trends_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(evaluation_algorithms_trends_static
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(evaluation_algorithms_trends_shared
    PUBLIC
        Threads::Threads
)

target_link_libraries(evaluation_algorithms_trends_static
    PUBLIC
        Threads::Threads
)

if(EXTRA_INC_DIRS)
    target_include_directories(evaluation_algorithms_trends_shared SYSTEM PRIVATE ${EXTRA_INC_DIRS})
    target_include_directories(evaluation_algorithms_trends_static SYSTEM PRIVATE ${EXTRA_INC_DIRS})
endif()

if(EXTRA_LIB_DIRS)
    target_link_directories(evaluation_algorithms_trends_shared PRIVATE ${EXTRA_LIB_DIRS})
    target_link_directories(evaluation_algorithms_trends_static PRIVATE ${EXTRA_LIB_DIRS})
endif()

# Copy public headers into <build>/include (next to <build>/lib)
file(GLOB ESTIMATOR_PUBLIC_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/include")

add_custom_target(export_public_headers ALL
    DEPENDS ${ESTIMATOR_PUBLIC_HEADERS}
    COMMAND ${CMAKE_COMMAND} -E copy ${ESTIMATOR_PUBLIC_HEADERS} ${CMAKE_BINARY_DIR}/include
    COMMENT "Copying public headers to ${CMAKE_BINARY_DIR}/include"
)
